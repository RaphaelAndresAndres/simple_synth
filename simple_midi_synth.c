#include <portmidi.h>
#include <portaudio.h>
#include <math.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define OVERTONE_COUNT 10
#define PI 3.141592653

static volatile float frequency = 440.;
static volatile float phases[OVERTONE_COUNT];
float volumes[OVERTONE_COUNT];
PaStream *stream;

static int WaveCallback(const void *inputBuffer,
                        void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData)
{
    (void)inputBuffer;
    (void)framesPerBuffer;
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;
    float *out = (float *)outputBuffer;
    float coeff = 2. * PI / SAMPLE_RATE;
    for (int i = 0; i < FRAMES_PER_BUFFER; ++i)
    {
        float value = 0;
        for (int j = 0; j < OVERTONE_COUNT; ++j)
        {
            value += sinf(phases[j]) * volumes[j];
            phases[j] += (float)j * frequency * coeff;
            if (phases[j] > 2 * PI)
                phases[j] -= 2 * PI;
        }
        *out++ = value / (float)OVERTONE_COUNT;
    }
    return paContinue;
}

void displayMidiMessage(int message)
{
    int status = message & 0xFF;
    message >>= 8;
    int note = message & 0xFF;
    message >>= 8;
    int velocity = message & 0xFF;
    if (status == 144)
    {
        printf("Key pressed : Velocity: %i, note: %i\n", velocity, note);
        frequency = 27.5f * pow(2.f, (float)(note - 21) / 12.);
    }
    if (status == 128)
        printf("Key released: Velocity: %i, note: %i\n", velocity, note);
}

void initAudio()
{
    printf("Overtones: ");
    for (int i = 0; i < OVERTONE_COUNT; ++i)
    {
        phases[i] = 0.;
        volumes[i] = 1 / powf(2., (float)i);
        printf("    volumes[%i]: %f\n", i, volumes[i]);
    }
    printf("\n");
    Pa_Initialize();
    Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, WaveCallback, NULL);
    Pa_StartStream(stream);
    printf("\e[1;1H\e[2J");
}

void initMidi()
{
    Pm_Initialize();
    PmStream *midi_stream;
    int device_id = 2;
    const int midiBufferLength = 16;
    PmEvent buffer[midiBufferLength];
    Pm_OpenInput(&midi_stream, device_id, NULL, midiBufferLength, NULL, NULL);
    while (1)
    {
        int num_events =
            Pm_Read(midi_stream, buffer, midiBufferLength);
        if (num_events > 0)
        {
            for (int i = 0; i < num_events; i++)
            {
                displayMidiMessage(buffer[i].message);
            }
        }
        Pa_Sleep(10);
    }
}
int main()
{
    initAudio();
    initMidi();
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;
}