/**
 * @file fmplayer.c
 * @brief Implementation of the FmPlayer.
 * @author Spencer Leslie 301571329
 */
#include "das/fmplayer.h"
#include "das/fm.h"
#include "das/wavetable.h"
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <asm-generic/errno-base.h>
#include <bits/pthreadtypes.h>
#include <bits/types/struct_sched_param.h>
#include <pthread.h>
#include <stdio.h>

/** This is the main paramter for tuning latency. This will set the length of
 * the hardware buffer in microseconds. In practice, the actual length of the
 * buffer is determined by what is available in the hardware. */
#define ALSA_BUFFERTIME 200000

/** Flags determining whether the synth voice or any operators need updating. */
#define UPDATE_NEEDED_BIT 0x1
#define UPDATE_VOICE_BIT (0x1 << FM_OPERATORS)

/** FmPlayer private data. */
typedef struct
{
    /** Note to play. */
    Note note;
    /** Control operation that needs to be performed. */
    FmPlayer_NoteCtrl ctrl;
    /** Are we running? */
    int running;
    /** Flags indicating which updates need to be made. */
    int updatesNeeded;

    /** Pointer to a sample buffer that receives PCM frames from the synth. */
    int16_t* sampleBuffer;

    /** Handle to the PCM device. */
    snd_pcm_t* pcmHandle;
    /** PCM hardware buffer size. */
    snd_pcm_uframes_t bufferSize;
    /** PCM hardware period size. */
    snd_pcm_uframes_t periodSize;

    /** The synthesizer.*/
    FmSynthesizer* synth;

    /** Worker thread that drives the synth. */
    pthread_t playerThread;

    /**
     * Lock for updates to the synth params.
     * We only lock updates to the params, not the note or note control.
     * Updating the note or note control will be atomic since they are both
     * 32-bit ints and should be aligned. */
    pthread_rwlock_t updateRwLock;

    /** Current synth params. */
    FmSynthParams params;
} _FmPlayer;

/** Pointer to the Fm Player. */
static _FmPlayer* _fmPlayer;

/** Writes samples to the PCM driver. */
static int
_writeToPcmBuffer(snd_pcm_t* pcm, int16_t* buffer, size_t nSamples);
/** Main worker thread function. */
static void*
_play(void* arg);
/** Configures audio hardware parameters. */
static int
_setHwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params);
/** Configures audio driver software parameters. */
static int
_setSwparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams);
/** Configures Alsa for our needs. */
static int
_configureAlsa(snd_pcm_t* handle);

static int
_writeToPcmBuffer(snd_pcm_t* pcm, int16_t* buffer, size_t nSamples)
{
    long written = 0;
    long offset = 0;
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
        pthread_rwlock_rdlock(&_fmPlayer->updateRwLock);
        if ((_fmPlayer->updatesNeeded & UPDATE_VOICE_BIT) == UPDATE_VOICE_BIT) {
            Fm_updateParams(_fmPlayer->synth, &_fmPlayer->params);
        }

        // apply any pending operator updates
        for (int op = 0; op < FM_OPERATORS; op++) {
            if ((_fmPlayer->updatesNeeded & (UPDATE_NEEDED_BIT << op)) ==
                (UPDATE_NEEDED_BIT << op)) {
                Fm_updateOpParams(
                  _fmPlayer->synth, op, &_fmPlayer->params.opParams[op]);
            }
        }
        _fmPlayer->updatesNeeded = 0;
        pthread_rwlock_unlock(&_fmPlayer->updateRwLock);

        // apply pending note update
        if (_fmPlayer->note != NOTE_NONE) {
            Fm_setNote(_fmPlayer->synth, _fmPlayer->note);
            _fmPlayer->note = NOTE_NONE;
        }

        // trigger or gate a note
        if (_fmPlayer->ctrl != NOTE_CTRL_NONE) {
            if (_fmPlayer->ctrl == NOTE_CTRL_NOTE_STOCCATO ||
                _fmPlayer->ctrl == NOTE_CTRL_NOTE_ON) {
                Fm_noteOn(_fmPlayer->synth);
            }

            if (_fmPlayer->ctrl == NOTE_CTRL_NOTE_STOCCATO ||
                _fmPlayer->ctrl == NOTE_CTRL_NOTE_OFF) {
                Fm_noteOff(_fmPlayer->synth);
            }

            _fmPlayer->ctrl = NOTE_CTRL_NONE;
        }

        // wait for a period to become available.
        // This blocks until the next period is ready to write.
        status = snd_pcm_wait(_fmPlayer->pcmHandle, 200);
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
        } else if (status == 0) {
            // time out on wait. Loop again to update params.
            printf("we timed out\n");
            continue;
        }

        // we have a period ready to be written, and the previous one is
        // being sent to the speakers. Now is the time we generate samples,
        Fm_generateSamples(
          _fmPlayer->synth, _fmPlayer->sampleBuffer, _fmPlayer->periodSize);

        // .. and write them out
        if ((status = _writeToPcmBuffer(_fmPlayer->pcmHandle,
                                        _fmPlayer->sampleBuffer,
                                        _fmPlayer->periodSize)) < 0) {
            fprintf(stderr, "recovering from error %s\n", snd_strerror(status));
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
    // Implementation adapted from the one given at
    // https://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html

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

    /* Configure the buffer size. We want the buffer to be 2 periods long.
     */
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
     * reaches the period boundaries like sending data to the ADC (I think?)
     * and waking up applications waiting for buffers to become ready. To
     * keep latency low low low, we wanna use 2 periods. Alsa can then do
     * whatever it's gotta with the data in one period while we're writing
     * to the other. */
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
    // Implementation adapted from the one given at
    // https://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html

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
    /* allow the transfer when at least period_size samples can be processed
     */
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
FmPlayer_controlNote(FmPlayer_NoteCtrl ctrl)
{
    _fmPlayer->ctrl = ctrl;
}

void
FmPlayer_setSynthVoice(const FmSynthParams* newVoice)
{
    pthread_rwlock_wrlock(&_fmPlayer->updateRwLock);
    if (newVoice != NULL) {
        memcpy(&_fmPlayer->params, newVoice, sizeof(FmSynthParams));
    }

    // Set update voice bit and clear operator update bits as those updates
    // are now invalid
    _fmPlayer->updatesNeeded |= UPDATE_VOICE_BIT;

    pthread_rwlock_unlock(&_fmPlayer->updateRwLock);
}

void
FmPlayer_updateOperatorWaveType(FmOperator op, WaveType wave)
{
    pthread_rwlock_wrlock(&_fmPlayer->updateRwLock);

    _fmPlayer->params.opParams[op].waveType = wave;
    _fmPlayer->updatesNeeded |= (UPDATE_NEEDED_BIT << op);

    pthread_rwlock_unlock(&_fmPlayer->updateRwLock);
}

void
FmPlayer_updateOperatorCm(FmOperator op, float cm)
{
    pthread_rwlock_wrlock(&_fmPlayer->updateRwLock);

    _fmPlayer->params.opParams[op].CmRatio = cm;
    _fmPlayer->updatesNeeded |= (UPDATE_NEEDED_BIT << op);

    pthread_rwlock_unlock(&_fmPlayer->updateRwLock);
}

void
FmPlayer_updateOperatorOutputStrength(FmOperator op, float outStrength)
{
    pthread_rwlock_wrlock(&_fmPlayer->updateRwLock);

    _fmPlayer->params.opParams[op].outputStrength = outStrength;
    _fmPlayer->updatesNeeded |= (UPDATE_NEEDED_BIT << op);

    pthread_rwlock_unlock(&_fmPlayer->updateRwLock);
}

void
FmPlayer_fixOperatorToNote(FmOperator op, Note note)
{
    pthread_rwlock_wrlock(&_fmPlayer->updateRwLock);

    _fmPlayer->params.opParams[op].CmRatio = -1;
    _fmPlayer->params.opParams[op].fixToNote = note;
    _fmPlayer->updatesNeeded |= (UPDATE_NEEDED_BIT << op);

    pthread_rwlock_unlock(&_fmPlayer->updateRwLock);
}

void
FmPlayer_updateOperatorAlgorithmConnection(FmOperator op,
                                           FmOperator moddingOp,
                                           float modIndex)
{
    pthread_rwlock_wrlock(&_fmPlayer->updateRwLock);

    _fmPlayer->params.opParams[op].algorithmConnections[moddingOp] = modIndex;
    _fmPlayer->updatesNeeded |= (UPDATE_NEEDED_BIT << op);

    pthread_rwlock_unlock(&_fmPlayer->updateRwLock);
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
    _fmPlayer->sampleBuffer = malloc(_fmPlayer->periodSize * sizeof(int16_t));

    pthread_rwlock_init(&_fmPlayer->updateRwLock, NULL);
    pthread_create(&_fmPlayer->playerThread, NULL, _play, NULL);

    return 1;
}

void
FmPlayer_close(void)
{
    _fmPlayer->running = 0;
    pthread_join(_fmPlayer->playerThread, NULL);
    pthread_rwlock_destroy(&_fmPlayer->updateRwLock);

    Fm_destroySynthesizer(_fmPlayer->synth);

    snd_pcm_drain(_fmPlayer->pcmHandle);
    snd_pcm_close(_fmPlayer->pcmHandle);

    free(_fmPlayer->sampleBuffer);
    free(_fmPlayer);
}
