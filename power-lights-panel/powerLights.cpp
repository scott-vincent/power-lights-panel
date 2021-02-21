#include <stdio.h>
#include <stdlib.h>
#include "gpioctrl.h"
#include "powerLights.h"

// Some SimConnect events don't work with certain aircraft
// so use vJoy to simulate joystick button presses instead.
//
// Comment the following line out if you don't want to use vJoy.
#define vJoyFallback

powerLights::powerLights()
{
    simVars = &globals.simVars->simVars;
    addGpio();

    fflush(stdout);
}

void powerLights::render()
{
    if (!globals.electrics) {
        // Turn off LEDS
        globals.gpioCtrl->writeLed(apuMasterControl, false);
        globals.gpioCtrl->writeLed(apuStartControl, false);
        globals.gpioCtrl->writeLed(apuBleedControl, false);

        // Make sure settings get re-initialised
        loadedAircraft = UNDEFINED;

        return;
    }

    // Write LEDs
    globals.gpioCtrl->writeLed(apuMasterControl, apuMaster);
    globals.gpioCtrl->writeLed(apuStartControl, apuStart && apuStartFlash < 8);
    globals.gpioCtrl->writeLed(apuBleedControl, apuBleed);
}

void powerLights::update()
{
    // Check for aircraft change
    bool aircraftChanged = (loadedAircraft != globals.aircraft);
    if (aircraftChanged) {
        loadedAircraft = globals.aircraft;
        airliner = (loadedAircraft != NO_AIRCRAFT && simVars->cruiseSpeed >= 300);
        apuMaster = false;
        apuStart = false;
        apuBleed = false;
        lastApuAdjust = 0;
        lastApuBleedAdjust = 0;
    }

    time(&now);
    gpioSwitchesInput();
    gpioButtonsInput();

    // Only update local values from sim if they are not currently being adjusted.
    // This stops them from jumping around due to lag of fetch/update cycle.
    if (lastApuAdjust == 0) {
        // APU Start can be flashing (on but not avail yet)
        if (simVars->apuPercentStart == 0 && simVars->apuPercentRpm == 0) {
            // Can't determine state of APU Master so assume no change
            apuStart = false;
            apuStartFlash = 0;
        }
        else if (simVars->apuPercentStart > 0) {
            // APU is starting up
            apuMaster = true;
            apuStart = true;
            apuStartFlash++;
        }
        else if (simVars->apuPercentRpm < 95) {
            if (simVars->apuPercentRpm > prevApuPercentRpm) {
                // APU is starting up
                apuMaster = true;
                apuStart = true;
                apuStartFlash++;
            }
            else if (simVars->apuPercentRpm < prevApuPercentRpm) {
                // APU is shutting down so master must have been turned off
                apuMaster = false;
                apuStart = false;
                apuStartFlash = 0;
            }
        }
        else {
            // APU is on and available
            apuMaster = true;
            apuStart = true;
            apuStartFlash = 0;
        }
    }

    if (apuStartFlash > 15) {
        apuStartFlash = 0;
    }

    prevApuPercentRpm = simVars->apuPercentRpm;

    if (lastApuBleedAdjust == 0) {
        apuBleed = simVars->apuBleed > 0;
    }
}

void powerLights::addGpio()
{
    battery1Control = globals.gpioCtrl->addSwitch("Battery1");
    battery2Control = globals.gpioCtrl->addSwitch("Battery2");
    fuelPumpControl = globals.gpioCtrl->addSwitch("Fuel Pump");
    beaconControl = globals.gpioCtrl->addSwitch("Beacon");
    landControl = globals.gpioCtrl->addSwitch("Land");
    taxiControl = globals.gpioCtrl->addSwitch("Taxi");
    navControl = globals.gpioCtrl->addSwitch("Nav");
    strobeControl = globals.gpioCtrl->addSwitch("Strobe");
    pitotHeatControl = globals.gpioCtrl->addSwitch("Pitot Heat");
    avionics1Control = globals.gpioCtrl->addSwitch("Avionics1");
    avionics2Control = globals.gpioCtrl->addSwitch("Avionics2");
    apuMasterControl = globals.gpioCtrl->addButton("APU Master");
    apuStartControl = globals.gpioCtrl->addButton("APU Start");
    apuBleedControl = globals.gpioCtrl->addButton("APU Bleed");
}

void powerLights::gpioSwitchesInput()
{
    // Battery 1 toggle
    int val = globals.gpioCtrl->readToggle(battery1Control);
    if (val != INT_MIN && val != prevBattery1Toggle) {
        // Switch toggled
        // SDK bug - On not working on A320
        globals.simVars->write(KEY_TOGGLE_MASTER_BATTERY, 1);
        prevBattery1Toggle = val;
    }

    // Battery 2 toggle
    val = globals.gpioCtrl->readToggle(battery2Control);
    if (val != INT_MIN && val != prevBattery2Toggle) {
        // Switch toggled
        // SDK bug - On not working on A320
        globals.simVars->write(KEY_TOGGLE_MASTER_BATTERY, 2);
        prevBattery2Toggle = val;
    }

    // Fuel Pump toggle
    val = globals.gpioCtrl->readToggle(fuelPumpControl);
    if (val != INT_MIN && val != prevFuelPumpToggle) {
        // Switch toggled
        // SDK bug - Not working for A320 so use vJoy
        globals.simVars->write(KEY_FUEL_PUMP);
#ifdef vJoyFallback
        // Toggle fuel pump
        globals.simVars->write(VJOY_BUTTON_1);
#endif
        prevFuelPumpToggle = val;
    }

    // Beacon toggle
    val = globals.gpioCtrl->readToggle(beaconControl);
    if (val != INT_MIN && val != prevBeaconToggle) {
        // Switch toggled
        // SDK bug - Not working for A320 so use vJoy
        globals.simVars->write(KEY_BEACON_LIGHTS_SET, val);
#ifdef vJoyFallback
        if (val == 0) {
            // Beacon off
            globals.simVars->write(VJOY_BUTTON_2);
        }
        else {
            // Beacon on
            globals.simVars->write(VJOY_BUTTON_3);
        }
#endif
        prevBeaconToggle = val;
    }

    // Land toggle
    val = globals.gpioCtrl->readToggle(landControl);
    if (val != INT_MIN && val != prevLandToggle) {
        // Switch toggled
        // SDK bug - Not working for A320 so use vJoy
        globals.simVars->write(KEY_LANDING_LIGHTS_SET, val);
#ifdef vJoyFallback
        if (val == 0) {
            // Landing off
            globals.simVars->write(VJOY_BUTTON_4);
        }
        else {
            // Landing on
            globals.simVars->write(VJOY_BUTTON_5);
        }
#endif
        prevLandToggle = val;
    }

    // Taxi toggle
    val = globals.gpioCtrl->readToggle(taxiControl);
    if (val != INT_MIN && val != prevTaxiToggle) {
        // Switch toggled
        // SDK bug - Not working for A320 so use vJoy
        globals.simVars->write(KEY_TAXI_LIGHTS_SET, val);
#ifdef vJoyFallback
        if (val == 0) {
            // Taxi off
            globals.simVars->write(VJOY_BUTTON_6);
        }
        else {
            // Taxi on
            globals.simVars->write(VJOY_BUTTON_7);
        }
#endif
        prevTaxiToggle = val;
    }

    // Nav toggle
    val = globals.gpioCtrl->readToggle(navControl);
    if (val != INT_MIN && val != prevNavToggle) {
        // Switch toggled
        // SDK bug - Not working for A320 so use vJoy
        globals.simVars->write(KEY_NAV_LIGHTS_SET, val);
#ifdef vJoyFallback
        if (val == 0) {
            // Nav off
            globals.simVars->write(VJOY_BUTTON_8);
        }
        else {
            // Nav on
            globals.simVars->write(VJOY_BUTTON_9);
        }
#endif
        prevNavToggle = val;
    }

    // Strobe toggle
    val = globals.gpioCtrl->readToggle(strobeControl);
    if (val != INT_MIN && val != prevStrobeToggle) {
        // Switch toggled
        // SDK bug - Not working for A320 so use vJoy
        globals.simVars->write(KEY_STROBES_SET, val);
#ifdef vJoyFallback
        if (val == 0) {
            // Strobe off
            globals.simVars->write(VJOY_BUTTON_10);
        }
        else {
            // Strobe on
            globals.simVars->write(VJOY_BUTTON_11);
        }
#endif
        prevStrobeToggle = val;
    }

    // Pitot Heat toggle
    val = globals.gpioCtrl->readToggle(pitotHeatControl);
    if (val != INT_MIN && val != prevPitotHeatToggle) {
        // Switch toggled
        globals.simVars->write(KEY_PITOT_HEAT_SET, val);
        // SDK bug - Not working for A320 so use vJoy
        globals.simVars->write(KEY_ANTI_ICE_SET, val);
#ifdef vJoyFallback
        if (val == 0) {
            // Anti ice off
            globals.simVars->write(VJOY_BUTTON_12);
        }
        else {
            // Anti ice on
            globals.simVars->write(VJOY_BUTTON_13);
        }
#endif
        prevPitotHeatToggle = val;
    }

    // Avionics 1 toggle (external power on airliner)
    val = globals.gpioCtrl->readToggle(avionics1Control);
    if (val != INT_MIN && val != prevAvionics1Toggle) {
        // Switch toggled
        if (airliner) {
#ifdef vJoyFallback
            // Toggle external power
            globals.simVars->write(VJOY_BUTTON_14);
#endif
        }
        else {
            globals.simVars->write(KEY_TOGGLE_MASTER_ALTERNATOR, 1);
        }
        prevAvionics1Toggle = val;
    }

    // Avionics 2 toggle (Jetway on airliner)
    val = globals.gpioCtrl->readToggle(avionics2Control);
    if (val != INT_MIN && val != prevAvionics2Toggle) {
        // Switch toggled
        if (airliner) {
            globals.simVars->write(KEY_TOGGLE_JETWAY);
        }
        else {
            globals.simVars->write(KEY_TOGGLE_MASTER_ALTERNATOR, 2);
        }
        prevAvionics2Toggle = val;
    }
}

void powerLights::gpioButtonsInput()
{
    // APU Master push
    int val = globals.gpioCtrl->readPush(apuMasterControl);
    if (val != INT_MIN) {
        if (prevApuMasterPush % 2 == 1) {
            // Button pushed
            apuMaster = !apuMaster;
            globals.gpioCtrl->writeLed(apuMasterControl, apuMaster);
            // SDK bug - Not working on A320
            globals.simVars->write(KEY_APU_OFF_SWITCH);
        }
        prevApuMasterPush = val;
        time(&lastApuAdjust);
    }
    else if (lastApuAdjust != 0) {
        if (now - lastApuAdjust > 2) {
            lastApuAdjust = 0;
        }
    }

    // APU Start push
    val = globals.gpioCtrl->readPush(apuStartControl);
    if (val != INT_MIN) {
        if (prevApuStartPush % 2 == 1) {
            // Button pushed
            apuStart = !apuStart;
            apuStartFlash = 0;
            globals.gpioCtrl->writeLed(apuStartControl, apuStart);
            // SDK bug - Not working on A320
            globals.simVars->write(KEY_APU_STARTER);
        }
        prevApuStartPush = val;
        time(&lastApuAdjust);
    }
    // lastApuAdjust is reset by apuMaster

    // APU Bleed push
    val = globals.gpioCtrl->readPush(apuBleedControl);
    if (val != INT_MIN) {
        if (prevApuBleedPush % 2 == 1) {
            // Button pushed
            apuBleed = !apuBleed;
            globals.gpioCtrl->writeLed(apuBleedControl, apuBleed);
            // Toggle APU bleed air source
            globals.simVars->write(VJOY_BUTTON_15);
        }
        prevApuBleedPush = val;
        time(&lastApuBleedAdjust);
    }
    else if (lastApuBleedAdjust != 0) {
        if (now - lastApuBleedAdjust > 1) {
            lastApuBleedAdjust = 0;
        }
    }
}
