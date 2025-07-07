#include <stdio.h>
#include <math.h>
#include <portaudio.h>
#include <string.h>
#include <fftw3.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

static float input[FRAMES_PER_BUFFER];
static float output[FRAMES_PER_BUFFER];

fftwf_plan PlanForward;
fftwf_plan PlanBackward;
fftwf_complex FFTout[FRAMES_PER_BUFFER / 2 + 1];

static int WaveCallback(const void *inputBuffer,
                        void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData)
{
    float *out = (float *)outputBuffer;
    float *in = (float *)inputBuffer;
    memcpy(input, in, sizeof(float) * FRAMES_PER_BUFFER);
    fftwf_execute(PlanForward);
    fftwf_execute(PlanBackward);
    for (int i = 0; i < FRAMES_PER_BUFFER; ++i)
    {
        output[i] /= FRAMES_PER_BUFFER;
    }
    memcpy(out, output, sizeof(float) * FRAMES_PER_BUFFER);

    return paContinue;
}
void main()
{
    Pa_Initialize();
    PaStream *stream;
    Pa_OpenDefaultStream(&stream,
                         1,
                         1,
                         paFloat32,
                         SAMPLE_RATE,
                         FRAMES_PER_BUFFER,
                         WaveCallback,
                         NULL);
    PlanForward = fftwf_plan_dft_r2c_1d(FRAMES_PER_BUFFER, input, FFTout, FFTW_MEASURE);
    PlanBackward = fftwf_plan_dft_c2r_1d(FRAMES_PER_BUFFER, FFTout, output, FFTW_MEASURE);
    for (int i = 0; i < FRAMES_PER_BUFFER; ++i)
    {
        input[i] = 0.f;
        output[i] = 0.f;
    }

    Pa_StartStream(stream);
    getchar();
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}