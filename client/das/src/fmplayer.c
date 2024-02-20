#include "das/fmplayer.h"
#include "das/cbuffer.h"
#include "das/fm.h"
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>

typedef struct
{
    _Atomic int running;
    _Atomic int update;

    pthread_t fmThread;
    pthread_t playerThread;

    snd_pcm_t* pcmHandle;
    FmSynthesizer* synth;

    FmSynthParams params;
} _FmPlayer;

static _FmPlayer* _fmPlayer;

// TODO: It occurs to me: why are we even bufferring at all?
//
// I think the original rationale was to ensure that we could be generating
// samples while we're sending them to the speakers. `snd_pcm_writei` blocks,
// and we can use the blocking time to generate the next set of samples.
// However, we have to be careful, as this means we are generating samples ahead
// of time and will miss events and configuration changes. If we've filled a
// buffer full of 8 chunks of 0.05s of samples each and a "note on" event
// happens, that note will not be played for at least 0.4 seconds! That's huge!
//
// There are a few things we could do instead:
//
// - Write PCM data directly from the synthesizer :: This option is nice and
//   simple. Further, alsa supports non-blocking PCM streams, and memory-mapped
//   IO into the audio buffer.
// - Create a frontbuffer/backbuffer :: This module could be adapted for this
//   approach pretty simply, in fact I think we would get this by simply using 2
//   CBuffer chunks.
//
// I'm leaning towards the former here. We could potentially generate frames
// /directly into/ the audio buffer, which would be amazing for reducing
// latency.

static void
_synthCBufWrite(int16_t* chunk, size_t bufSize)
{
    Fm_generateSamples(_fmPlayer->synth, chunk, bufSize);
}

static void*
_synth(void* arg)
{
    (void)arg;
    while (_fmPlayer->running) {
        if (CBuffer_writeNextChunk(_synthCBufWrite) == CBUFFER_EFULL) {
            // fprintf(stderr, "WARN: Synth buffer full\n");
        }
        if (_fmPlayer->update) {
            Fm_updateParams(_fmPlayer->synth, &_fmPlayer->params);
            _fmPlayer->update = 0;
        }
    }
    return NULL;
}

static void
_playerCBufPlay(int16_t* chunk, size_t bufsize)
{
    snd_pcm_sframes_t framesWritten =
      snd_pcm_writei(_fmPlayer->pcmHandle, chunk, bufsize);
    if (framesWritten < 0) {
        framesWritten = snd_pcm_recover(_fmPlayer->pcmHandle, framesWritten, 0);
    }

    if (framesWritten < 0) {
        fprintf(
          stderr, "snd_pcm_writei failed: %s\n", snd_strerror(framesWritten));
    }

    if (framesWritten > 0 && framesWritten < (long)bufsize) {
        fprintf(stderr,
                "Short write (expected %u, wrote %li)\n",
                bufsize,
                framesWritten);
    }
}

static void*
_play(void* arg)
{
    (void)arg;
    while (_fmPlayer->running) {
        if (CBuffer_readNextChunk(_playerCBufPlay) == CBUFFER_ENODATA) {
            // fprintf(stderr, "WARN: Synth buffer no data\n");
        }
    }
    return NULL;
}

// TODO a better interface
void
FmPlayer_noteOn(void)
{
    Fm_noteOn(_fmPlayer->synth);
}

void
FmPlayer_noteOff(void)
{
    Fm_noteOff(_fmPlayer->synth);
}

void
FmPlayer_updateSynthParams(FmSynthParams* params)
{
    memcpy(&_fmPlayer->params, params, sizeof(FmSynthParams));
    _fmPlayer->update = 1;
}

int
FmPlayer_initialize(FmSynthParams* params)
{
    _fmPlayer = malloc(sizeof(_FmPlayer));
    memset(_fmPlayer, 0, sizeof(_FmPlayer));

    int sndStatus = snd_pcm_open(
      &_fmPlayer->pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (sndStatus < 0) {
        free(_fmPlayer);
        return sndStatus;
    }
    // TODO: need to config-pin the i2c pins so we have audio out.

    // TODO: buffer size will have to be tuned to the bbg
    sndStatus = snd_pcm_set_params(_fmPlayer->pcmHandle,
                                   SND_PCM_FORMAT_S16_LE,
                                   SND_PCM_ACCESS_RW_INTERLEAVED,
                                   1, // 1 channel
                                   44100,
                                   1,      // Allow software resampling
                                   50000); // 0.05 seconds per buffer
    if (sndStatus < 0) {
        free(_fmPlayer);
        return sndStatus;
    }

    // Allocate this software's playback buffer to be the same size as the
    // the hardware's playback buffers for efficient data transfers.
    // ..get info on the hardware buffers:
    unsigned long unusedBufferSize = 0;
    unsigned long playbackBufferSize = 0;
    snd_pcm_get_params(
      _fmPlayer->pcmHandle, &unusedBufferSize, &playbackBufferSize);
    // ..allocate playback buffer:

    // Trying for 32 k of data, chunked up
    CBuffer_new(playbackBufferSize, 32);

    // init synth
    memcpy(&_fmPlayer->params, params, sizeof(FmSynthParams));
    _fmPlayer->synth = Fm_createFmSynthesizer(&_fmPlayer->params);

    _fmPlayer->running = 1;

    // start threads
    pthread_create(&_fmPlayer->fmThread, NULL, _synth, NULL);
    pthread_create(&_fmPlayer->playerThread, NULL, _play, NULL);

    return 1;
}

void
FmPlayer_close(void)
{
    _fmPlayer->running = 0;
    pthread_join(_fmPlayer->playerThread, NULL);
    pthread_join(_fmPlayer->fmThread, NULL);

    Fm_destroySynthesizer(_fmPlayer->synth);

    snd_pcm_drain(_fmPlayer->pcmHandle);
    // snd_pcm_drop(_fmPlayer->pcmHandle);
    snd_pcm_close(_fmPlayer->pcmHandle);

    CBuffer_destroy();
}
