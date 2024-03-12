#include "das/fmplayer.h"
#include "das/fm.h"
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <asm-generic/errno-base.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>

/** This is the main paramter for tuning latency. This will set the length of
 * the hardware buffer in microseconds. Setting to 0.05 seconds currently gives
 * a period of about 735 samples, which means we will have very low latency
 * (0.05 seconds is great), but will require more CPU time to keep up. */
#define ALSA_BUFFERTIME 64000

typedef struct
{
    _Atomic int running;
    _Atomic int update;

    pthread_t playerThread;

    int16_t* sampleBuffer;

    snd_pcm_t* pcmHandle;
    snd_pcm_uframes_t bufferSize;
    snd_pcm_uframes_t periodSize;
    FmSynthesizer* synth;

    FmSynthParams params;
    Note note;
} _FmPlayer;

static _FmPlayer* _fmPlayer;

/** Write samples to the PCM driver. */
static int
_writeToPcmBuffer(snd_pcm_t* pcm, int16_t* buffer, size_t nSamples)
{
    long written = 0;
    size_t offset = 0;
    int remain = nSamples;
    while (remain > 0) {
        written = snd_pcm_writei(pcm, buffer + offset, remain);
        // This is non-blocking, so we may get asked to try again
        if (written == -EAGAIN) {
            continue;
        }
        if (written < 0) {
            return written;
        }
        offset += written;
        remain -= written;
    }
    return 0;
}

static void*
_play(void* arg)
{
    (void)arg;
    int status;

    snd_pcm_start(_fmPlayer->pcmHandle);

    while (_fmPlayer->running) {
        // set synth params if we need to
        //
        // TODO: This is bad bad not good.
        //
        // There's a race condition where if we update synth params and then
        // call noteOn/noteOff the noteOn/noteOff will get clobbered by the
        // params update.
        //
        // We can'd do like we do with `setNote` and `updateSynthParams` because
        // we need at least noteOn and noteOff to happen immediately. This also
        // needs to be fast, and I don't know that we can get away with any sort
        // of locking overhead when we have to generate 44100 samples every
        // second.
        //
        // Parameter updating has been the biggest weakpoint in the synth design
        // so far. It needs a complete rethink, but I don't know that we have
        // the time for it now :(
        if (_fmPlayer->update) {
            Fm_updateParams(_fmPlayer->synth, &_fmPlayer->params);
            _fmPlayer->update = 0;
        }

        if (_fmPlayer->note != NOTE_NONE) {
            Fm_setNote(_fmPlayer->synth, _fmPlayer->note);
            _fmPlayer->note = NOTE_NONE;
        }

        // wait for a period to become available.
        // This blocks until the next period is ready to write.
        status = snd_pcm_wait(_fmPlayer->pcmHandle, 1000);
        if (status < 0) {
            // Try to recover the stream!
            if ((status = snd_pcm_recover(_fmPlayer->pcmHandle, status, 0)) <
                0) {
                fprintf(stderr,
                        "Player received unrecoverable error %s\n",
                        snd_strerror(status));
                // TODO: we need a way to shut down from here.
                return NULL;
            }
        }

        // we have a period ready to be written, and the previous one is being
        // sent to the speakers. Now is the time we generate samples,
        Fm_generateSamples(
          _fmPlayer->synth, _fmPlayer->sampleBuffer, _fmPlayer->periodSize);

        // .. and write them out
        if ((status = _writeToPcmBuffer(_fmPlayer->pcmHandle,
                                        _fmPlayer->sampleBuffer,
                                        _fmPlayer->periodSize)) < 0) {
            if ((status = snd_pcm_recover(_fmPlayer->pcmHandle, status, 0)) <
                0) {
                fprintf(stderr,
                        "Player received unrecoverable error %s\n",
                        snd_strerror(status));
                // TODO: we need a way to shut down from here.
                return NULL;
            }
        }
    }
    return NULL;
}

static int
_setHwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params)
{
    unsigned int rrate;
    int err;

    /* choose all parameters */
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0) {
        printf("Broken configuration for playback: no configurations "
               "available: %s\n",
               snd_strerror(err));
        return err;
    }
    /* set hardware resampling */
    err = snd_pcm_hw_params_set_rate_resample(handle, params, 1);
    if (err < 0) {
        printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(
      handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        printf("Access type not available for playback: %s\n",
               snd_strerror(err));
        return err;
    }
    /* set the sample format */
    err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        printf("Sample format not available for playback: %s\n",
               snd_strerror(err));
        return err;
    }
    /* set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, params, 1);
    if (err < 0) {
        printf("Channels count (%u) not available for playbacks: %s\n",
               1,
               snd_strerror(err));
        return err;
    }
    /* set the stream rate */
    unsigned int rate = 44100;
    rrate = rate;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
    if (err < 0) {
        printf("Rate %uHz not available for playback: %s\n",
               rrate,
               snd_strerror(err));
        return err;
    }
    if (rrate != rate) {
        printf("Rate doesn't match (requested %uHz, get %iHz)\n", rate, err);
        return -EINVAL;
    }

    /* Configure the buffer size. We want the buffer to be 2 periods long. */
    unsigned int buftime = ALSA_BUFFERTIME;
    int dir = 0;
    err =
      snd_pcm_hw_params_set_buffer_time_near(handle, params, &buftime, &dir);
    if (err < 0) {
        printf(
          "Unable to set buffer time %u: %s\n", buftime, snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_hw_params_get_buffer_size(params,
                                                 &_fmPlayer->bufferSize)) < 0) {
        printf("err getting buffer size size: %s\n", snd_strerror(err));
        return err;
    } else {
        printf(
          "Buffer size is %lu and dir is %d\n", _fmPlayer->bufferSize, dir);
    }

    /* Alsa divides its buffers into "periods", and does stuff when playback
     * reaches the period boundaries like sending data to the ADC (I think?) and
     * waking up applications waiting for buffers to become ready.
     * To keep latency low low low, we wanna use 2 periods. Alsa can then do
     * whatever it's gotta with the data in one period while we're writing to
     * the other. */
    snd_pcm_uframes_t period = _fmPlayer->bufferSize / 2;
    dir = 0;
    printf("Trying to set period size to %lu\n", period);
    err = snd_pcm_hw_params_set_period_size_near(handle, params, &period, &dir);
    if (err < 0) {
        printf(
          "Unable to set period size %lu: %s\n", period, snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_hw_params_get_period_size(
           params, &_fmPlayer->periodSize, &dir)) < 0) {
        printf("err getting period size: %s\n", snd_strerror(err));
        return err;
    } else {
        printf(
          "Period size is %lu and dir is %d\n", _fmPlayer->periodSize, dir);
    }

    /* write the parameters to device */
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
        return err;
    }
    return 0;
}

static int
_setSwparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams)
{
    int err;

    /* get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        printf("Unable to determine current swparams for playback: %s\n",
               snd_strerror(err));
        return err;
    }
    // TODO: this should be much closer to the buffer size
    if ((err = snd_pcm_sw_params_set_avail_min(
           handle, swparams, _fmPlayer->periodSize)) < 0) {
        printf("Unable to set avail min: %s\n", snd_strerror(err));
        return err;
    }

    /* Threshold to stat playing. Choose one period. */
    err = snd_pcm_sw_params_set_start_threshold(
      handle, swparams, _fmPlayer->periodSize);
    if (err < 0) {
        printf("Unable to set start threshold mode for playback: %s\n",
               snd_strerror(err));
        return err;
    }
    /* allow the transfer when at least period_size samples can be processed */
    /* or disable this mechanism when period event is enabled (aka interrupt
     * like style processing) */
    err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
    if (err < 0) {
        printf("Unable to set period event: %s\n", snd_strerror(err));
        return err;
    }

    /* write the parameters to the playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
        return err;
    }
    snd_pcm_drop(handle);
    return 0;
}

static int
_configureAlsa(snd_pcm_t* handle)
{
    snd_pcm_hw_params_t* hwParams;
    snd_pcm_sw_params_t* swParams;
    int err;
    int16_t* silence;

    snd_pcm_hw_params_alloca(&hwParams);
    snd_pcm_sw_params_alloca(&swParams);

    if ((err = _setHwparams(handle, hwParams)) < 0) {
        return err;
    }

    if ((err = _setSwparams(handle, swParams)) < 0) {
        return err;
    }

    silence = malloc(sizeof(int16_t) * _fmPlayer->bufferSize);

    if ((err = snd_pcm_prepare(handle)) < 0) {
        printf("Prepare error: %s\n", snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_format_set_silence(
           SND_PCM_FORMAT_S16_LE, silence, _fmPlayer->bufferSize)) < 0) {
        printf("Silence error: %s\n", snd_strerror(err));
        return err;
    }

    if ((err = _writeToPcmBuffer(handle, silence, _fmPlayer->bufferSize)) < 0) {
        printf("Write error: %s\n", snd_strerror(err));
        return err;
    }
    // TODO pretty sure this just lets alsa write errors to stdout?
    snd_output_t* output;
    err = snd_output_stdio_attach(&output, stdout, 0);
    if (err < 0) {
        printf("Output failed: %s\n", snd_strerror(err));
        return err;
    }
    return 0;
}

void
FmPlayer_setNote(Note note)
{
    _fmPlayer->note = note;
}

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
FmPlayer_updateSynthParams(const FmSynthParams* params)
{
    memcpy(&_fmPlayer->params, params, sizeof(FmSynthParams));
    _fmPlayer->update = 1;
}

int
FmPlayer_initialize(const FmSynthParams* params)
{
    _fmPlayer = malloc(sizeof(_FmPlayer));
    memset(_fmPlayer, 0, sizeof(_FmPlayer));

    // open PCM in non-blocking mode.
    int sndStatus = snd_pcm_open(&_fmPlayer->pcmHandle,
                                 "default",
                                 SND_PCM_STREAM_PLAYBACK,
                                 SND_PCM_NONBLOCK);
    if (sndStatus < 0) {
        free(_fmPlayer);
        return sndStatus;
    }
    // TODO: need to config-pin the i2c pins so we have audio out.

    // Configure Alsa
    sndStatus = _configureAlsa(_fmPlayer->pcmHandle);
    if (sndStatus < 0) {
        free(_fmPlayer);
        return sndStatus;
    }

    // init synth
    memcpy(&_fmPlayer->params, params, sizeof(FmSynthParams));
    _fmPlayer->synth = Fm_createFmSynthesizer(&_fmPlayer->params);

    _fmPlayer->running = 1;
    // Buffer a single period at a time
    _fmPlayer->sampleBuffer = malloc(_fmPlayer->bufferSize * sizeof(int16_t));

    // start threads
    pthread_create(&_fmPlayer->playerThread, NULL, _play, NULL);

    return 1;
}

void
FmPlayer_close(void)
{
    _fmPlayer->running = 0;
    pthread_join(_fmPlayer->playerThread, NULL);

    Fm_destroySynthesizer(_fmPlayer->synth);

    snd_pcm_drain(_fmPlayer->pcmHandle);
    snd_pcm_close(_fmPlayer->pcmHandle);

    free(_fmPlayer->sampleBuffer);
    free(_fmPlayer);
}
