/*
 * Flight Simulator Power/Lights Panel
 * Copyright (c) 2021 Scott Vincent
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include "gpioctrl.h"
#include "globals.h"
#include "settings.h"
#include "simvars.h"
#include "powerLights.h"

const char* powerLightsVersion = "v1.2.7";
const bool Debug = false;

struct globalVars globals;

powerLights* powLights;

/// <summary>
/// Initialise
/// </summary>
void init(const char *settingsFile = NULL)
{
    globals.allSettings = new settings(settingsFile);
    globals.simVars = new simvars();
    globals.gpioCtrl = new gpioctrl(true, false);
}

/// <summary>
/// Fetch latest values of common variables
/// </summary>
void updateCommon()
{
    SimVars* simVars = &globals.simVars->simVars;

    // Electrics check
    globals.electrics = globals.connected && simVars->dcVolts > 0;

    // Avionics check
    globals.avionics = globals.connected && (simVars->com1Status == 0 || simVars->com2Status == 0);
}

/// <summary>
/// Update everything before the next frame
/// </summary>
void doUpdate()
{
    updateCommon();

    powLights->update();
}

///
/// main
///
int main(int argc, char **argv)
{
    printf("power-lights-panel %s\n", powerLightsVersion);
    fflush(stdout);

    if (argc > 1) {
        init(argv[1]);
    }
    else {
        init();
    }

    powLights = new powerLights();

    while (!globals.quit) {
        doUpdate();
        powLights->render();

        // Update 5 times per second
        usleep(200000);
    }

    return 0;
}
