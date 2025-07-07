#include <stdio.h>
#include <string.h>
#include "portmidi.h"

int main()
{
    PmError err = Pm_Initialize();
    if (err != pmNoError)
    {
        printf("PortMidi initialization failed: %s\n", Pm_GetErrorText(err));
        return 1;
    }

    int num_devices = Pm_CountDevices();
    if (num_devices <= 0)
    {
        printf("No MIDI devices found.\n");
        Pm_Terminate();
        return 0;
    }

    printf("Found %d MIDI device(s):\n\n", num_devices);
    for (int i = 0; i < num_devices; ++i)
    {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (info == NULL)
            continue;

        const char *direction = info->input ? "Input" : (info->output ? "Output" : "N/A");
        const char *name = info->name;
        const char *interf = info->interf;

        printf("Device ID: %d\n", i);
        printf("  Interface: %s\n", interf);
        printf("  Name:      %s\n", name);
        printf("  Type:      %s\n", direction);
    }

    Pm_Terminate();
    return 0;
}
