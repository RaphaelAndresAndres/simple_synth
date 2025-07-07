#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>
#include <pthread.h>
#include <portmidi.h>

#define SAMPLE_RATE 44100
#define AMPLITUDE 0.5
#define FRAMES_PER_BUFFER 256
#define MAX_KEYS 10
#define true 1
#define false 0
#define ATTACK_TIME_SEC 0.01
#define RELEASE_TIME_SEC 0.05

#ifndef M_PI
#define M_PI 3.141592653
#endif

typedef struct
{
    float phase;
    float frequency;
    float g_amplitude;
    float amplitude;
    int status;
    int is_active;
    float attack_duration_samples;  // New
    float release_duration_samples; // New
} WaveData;
/*
    EXPLANATION OF THE PARAMETERS:
    phase:          phase of the sine wave signal, required for boundary conditions to be approximately met
    frequency:      frequency of the sine wave signal
    g_amplitude:    final amplitude that the wave will have after the appropriate fade-in
    amplitude:      current amplitude, scales the sine wave at each point in time
    status:         status of the sine wave: 0 means fade-in and 1 the note is held and 2 means fade-out and 3 means that the note is inactive
    is_active:      0: note is not active, 1: note is active
*/
WaveData *WaveList[MAX_KEYS];

static int WaveCallback(const void *inputBuffer,
                        void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData)
{
    float *out = (float *)outputBuffer;
    for (unsigned long i = 0; i < framesPerBuffer; i++)
    {
        float value = 0.f;
        for (int j = 0; j < MAX_KEYS; j++)
        {
            if (!WaveList[j] || WaveList[j]->is_active == 0)
                continue;
            WaveData *data = WaveList[j];

            if (data->status == 0)
            {
                if (data->attack_duration_samples > 0)
                {
                    data->amplitude += (data->g_amplitude - data->amplitude) / data->attack_duration_samples;
                    data->attack_duration_samples--;
                }
                else
                {
                    data->amplitude = data->g_amplitude;
                }

                if (data->amplitude >= data->g_amplitude * 0.99f || data->attack_duration_samples <= 0)
                {
                    data->amplitude = data->g_amplitude;
                    data->status = 1;
                }
            }
            else if (data->status == 1)
            {
                data->amplitude *= 0.99999f;
            }
            else if (data->status == 2)
            {
                if (data->release_duration_samples > 0)
                {
                    data->amplitude -= data->g_amplitude / data->release_duration_samples;
                    data->release_duration_samples--;
                }
                else
                {
                    data->amplitude = 0;
                }

                if (data->amplitude <= 0)
                {
                    data->amplitude = 0;
                    data->status = 3;
                    data->is_active = 0;
                    data->phase = 0;
                    data->g_amplitude = 0;
                    data->frequency = 0;
                }
            }

            value += data->amplitude * sinf(data->phase);
            data->phase += 2.0f * M_PI * data->frequency / SAMPLE_RATE;
            if (data->phase >= 2.0f * M_PI)
            {
                data->phase -= 2.0f * M_PI;
            }
        }
        *out++ = value;
        *out++ = value;
    }
    return paContinue;
}

void addKey(int message)
{
    int status_msg = message & 0xFF;
    int note = (message >> 8) & 0xFF;
    int velocity = (message >> 16) & 0xFF;

    float frequency = 27.5f * pow(2.f, (float)(note - 21) / 12.);
    float volume = (float)velocity / 500;

    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (WaveList[i]->is_active == 0)
            continue;
        if (WaveList[i]->frequency == frequency)
        {
            WaveList[i]->status = 0;
            WaveList[i]->g_amplitude = volume;
            WaveList[i]->attack_duration_samples = (int)(ATTACK_TIME_SEC * SAMPLE_RATE);
            return;
        }
    }

    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (WaveList[i]->is_active == 0)
        {
            WaveList[i]->status = 0;
            WaveList[i]->amplitude = 0;
            WaveList[i]->g_amplitude = volume;
            WaveList[i]->frequency = frequency;
            WaveList[i]->is_active = 1;
            WaveList[i]->phase = 0.0f;
            WaveList[i]->attack_duration_samples = (int)(ATTACK_TIME_SEC * SAMPLE_RATE);
            return;
        }
    }

    float curmin = 1.;
    int min_index = -1;
    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (WaveList[i]->is_active == true && WaveList[i]->amplitude < curmin)
        {
            curmin = WaveList[i]->amplitude;
            min_index = i;
        }
    }
    if (min_index != -1)
    {

        WaveList[min_index]->status = 0;
        WaveList[min_index]->amplitude = 0;
        WaveList[min_index]->g_amplitude = volume;
        WaveList[min_index]->frequency = frequency;
        WaveList[min_index]->is_active = 1;
        WaveList[min_index]->phase = 0.0f;
        WaveList[min_index]->attack_duration_samples = (int)(ATTACK_TIME_SEC * SAMPLE_RATE);
    }
}

void removeKey(int message)
{
    int note = (message >> 8) & 0xFF;
    float frequency = 27.5f * pow(2.f, (float)(note - 21) / 12.);
    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (WaveList[i]->frequency == frequency && (WaveList[i]->status == 0 || WaveList[i]->status == 1))
        {
            WaveList[i]->status = 2;
            WaveList[i]->release_duration_samples = (int)(RELEASE_TIME_SEC * SAMPLE_RATE);
            break;
        }
    }
}
//
void *setupMidiDevice(void *vargp)
{
    printf("Setting up midi device...\n");
    Pm_Initialize();
    PmStream *midi_stream;
    int device_id = 2;
    const int midiBufferLength = 4;
    PmEvent buffer[midiBufferLength];
    Pm_OpenInput(&midi_stream, device_id, NULL, midiBufferLength, NULL, NULL);
    while (true)
    {
        int num_events =
            Pm_Read(midi_stream, buffer, midiBufferLength);
        if (num_events > 0)
        {
            for (int i = 0; i < num_events; i++)
            {
                if ((buffer[i].message & 0xFF) == 144)
                    addKey(buffer[i].message);
                if ((buffer[i].message & 0xFF) == 128)
                    removeKey(buffer[i].message);
            }
        }
        Pa_Sleep(10);
    }
}

void setupAudioDevice()
{
    printf("Setting up the audio device ...\n");

    for (int i = 0; i < MAX_KEYS; ++i)
    {
        WaveList[i] = (WaveData *)malloc(sizeof(WaveData));
        WaveList[i]->phase = 0.0f;
        WaveList[i]->frequency = 0.0f;
        WaveList[i]->g_amplitude = 0.0f;
        WaveList[i]->amplitude = 0.0f;
        WaveList[i]->status = 0;
        WaveList[i]->is_active = 0;
        WaveList[i]->attack_duration_samples = 0;
        WaveList[i]->release_duration_samples = 0;
    }

    Pa_Initialize();

    PaStream *stream;
    Pa_OpenDefaultStream(&stream,
                         1,
                         2,
                         paFloat32,
                         SAMPLE_RATE,
                         FRAMES_PER_BUFFER,
                         WaveCallback,
                         NULL);

    Pa_StartStream(stream);

    printf("\e[1;1H\e[2J");
    printf("Press Enter to stop...\n");
    getchar();
    for (int i = 0; i < MAX_KEYS; ++i)
    {
        free(WaveList[i]);
    }
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}

int main(void)
{
    pthread_t midi_thread_id;
    pthread_create(&midi_thread_id, NULL, setupMidiDevice, NULL);
    setupAudioDevice();
    return 0;
}