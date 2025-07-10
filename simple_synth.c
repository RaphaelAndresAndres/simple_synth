// gcc simple_synth.c -o simple_synth -lportaudio -lm
#include <portaudio.h>
#include <math.h>
#include <stdio.h>

#define PI 3.141592653
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

static volatile float phase = 0.;
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
        *out++ = sinf(phase);
        phase += frequency * coeff;
        if (phase > 2 * PI)
            phase -= 2 * PI;
    }
    return paContinue;
}
int main()
{
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
