#include <portmidi.h>
#include <portaudio.h>
#include <stdio.h>

void displayMidiMessage(int message)
{
    int status = message & 0xFF;
    message >>= 8;
    int note = message & 0xFF;
    message >>= 8;
    int velocity = message & 0xFF;
    if (status == 144)
        printf("Key pressed : Velocity: %i, note: %i\n", velocity, note);
    if (status == 128)
        printf("Key released: Velocity: %i, note: %i\n", velocity, note);
}

int main()
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
    return 0;
}