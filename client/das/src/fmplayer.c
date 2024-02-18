#include "das/fmplayer.h"
#include "das/cbuffer.h"
#include "das/fm.h"
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>

static pthread_t fmThread;
static pthread_t playerThread;

static snd_pcm_t* pcmHandle;

// TODO not this probably
static FmSynthesizer* synth;

static int run = 1;

static void
_synthCBufWrite(int16_t* chunk, size_t bufSize, void* usrdata)
{
    // TODO don't need the user data?
    (void)usrdata;
    Fm_generateSamples(synth, chunk, bufSize);
}

static void*
_synth(void* arg)
{
    (void)arg;
    while (run) {
        Fm_performQueuedUpdates(synth);

        if (CBuffer_writeNextChunk(_synthCBufWrite, NULL) == CBUFFER_EFULL) {
            // fprintf(stderr, "WARN: Synth buffer full\n");
        }
    }
    return NULL;
}

static void
_playerCBufPlay(int16_t* chunk, size_t bufsize, void* _arg)
{
    (void)_arg;
    snd_pcm_sframes_t framesWritten = snd_pcm_writei(pcmHandle, chunk, bufsize);
    if (framesWritten < 0) {
        framesWritten = snd_pcm_recover(pcmHandle, framesWritten, 0);
    }

    if (framesWritten < 0) {
        fprintf(
          stderr, "snd_pcm_writei failed: %s\n", snd_strerror(framesWritten));
    }

    if (framesWritten > 0 && framesWritten < (long)bufsize) {
        fprintf(stderr,
                "Short write (expected %li, wrote %li)\n",
                bufsize,
                framesWritten);
    }
}

static void*
_play(void* arg)
{
    (void)arg;
    while (run) {
        if (CBuffer_readNextChunk(_playerCBufPlay, NULL) == CBUFFER_ENODATA) {
            // fprintf(stderr, "WARN: Synth buffer no data\n");
        }
    }
    return NULL;
}

int
FmPlayer_initialize(FmSynthParams* params)
{

    int sndStatus =
      snd_pcm_open(&pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (sndStatus < 0) {
        return sndStatus;
    }
    sndStatus = snd_pcm_set_params(pcmHandle,
                                   SND_PCM_FORMAT_S16_LE,
                                   SND_PCM_ACCESS_RW_INTERLEAVED,
                                   1, // 1 channel
                                   44100,
                                   1,      // Allow software resampling
                                   50000); // 0.05 seconds per buffer
    if (sndStatus < 0) {
        return sndStatus;
    }
    // Allocate this software's playback buffer to be the same size as the
    // the hardware's playback buffers for efficient data transfers.
    // ..get info on the hardware buffers:
    unsigned long unusedBufferSize = 0;
    unsigned long playbackBufferSize = 0;
    snd_pcm_get_params(pcmHandle, &unusedBufferSize, &playbackBufferSize);
    // ..allocate playback buffer:
    CBuffer_new(playbackBufferSize);

    // init synth
    synth = Fm_createFmSynthesizer(params);
    // start threads
    pthread_create(&fmThread, NULL, _synth, NULL);
    pthread_create(&playerThread, NULL, _play, NULL);

    return 1;
}

FmSynthesizer*
FmPlayer_synth(void)
{
    return synth;
}

void
FmPlayer_close(void)
{
    run = 0;
    pthread_join(playerThread, NULL);
    pthread_join(fmThread, NULL);

    snd_pcm_drain(pcmHandle);
    snd_pcm_close(pcmHandle);

    CBuffer_destroy();
}
