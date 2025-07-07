// gcc simple_synth.c -o simple_synth -lportaudio -lm
#include <portaudio.h>
#include <math.h>
#include <stdio.h>

#define PI 3.141592653
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define OVERTONE_COUNT 10

static volatile float phases[OVERTONE_COUNT];
float volumes[OVERTONE_COUNT];

float frequency = 440.;
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
int main()
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
    PaStream *stream;
    Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, WaveCallback, NULL);
    Pa_StartStream(stream);
    printf("\e[1;1H\e[2J");
    getchar();
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;
}