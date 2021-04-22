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
    globals.gpioCtrl->writeLed(apuStartControl, apuStart && apuStartFlash < 6);
    globals.gpioCtrl->writeLed(apuBleedControl, apuBleed && airliner);
}

void powerLights::update()
{
    // Check for aircraft change
    bool aircraftChanged = (globals.electrics && loadedAircraft != globals.aircraft);
    if (aircraftChanged) {
        loadedAircraft = globals.aircraft;
        airliner = (loadedAircraft != NO_AIRCRAFT && simVars->cruiseSpeed >= 300 && loadedAircraft != CESSNA_CJ4);
        apuMaster = false;
        apuStart = false;
        apuBleed = false;
        lastApuMasterAdjust = 0;
        lastApuStartAdjust = 0;
        lastApuBleedAdjust = 0;
        lastFlapsPos = -1;
        parkBrakeOn = true;
        prevBeaconToggle = -1;
        prevLandToggle = -1;
        prevTaxiToggle = -1;
        prevNavToggle = -1;
        prevStrobeToggle = -1;
        prevPitotHeatToggle = -1;
        prevFlapsUpToggle = -1;
        prevFlapsDownToggle = -1;
        prevParkBrakeOffToggle = -1;
        prevParkBrakeOnToggle = -1;
    }

    time(&now);
    gpioSwitchesInput();
    gpioButtonsInput();
    gpioFlapsInput();
    gpioParkBrakeInput();

    // Only update local values from sim if they are not currently being adjusted.
    // This stops them from jumping around due to lag of fetch/update cycle.
    if (lastApuMasterAdjust == 0) {
        apuMaster = simVars->apuMasterSw > 0;
    }

    if (lastApuStartAdjust == 0) {
        if (simVars->apuStartAvail > 0) {
            // APU is on and available
            apuStart = true;
            apuStartFlash = 0;
        }
        else if (simVars->apuStart > 0) {
            // APU is starting up
            apuStart = true;
            apuStartFlash++;
        }
        else {
            // APU is shut down
            apuStart = false;
            apuStartFlash = 0;
        }
    }

    if (apuStartFlash > 15) {
        apuStartFlash = 0;
    }

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
    flapsUpControl = globals.gpioCtrl->addSwitch("Flaps Up");
    flapsPosControl = globals.gpioCtrl->addRotaryEncoder("Flaps Pos");
    flapsDownControl = globals.gpioCtrl->addSwitch("Flaps Down");
    parkBrakeOffControl = globals.gpioCtrl->addSwitch("Park Brake Off");
    parkBrakeOnControl = globals.gpioCtrl->addSwitch("Park Brake On");
}

void powerLights::gpioSwitchesInput()
{
    // Battery 1 toggle
    int val = globals.gpioCtrl->readToggle(battery1Control);
    if (val != INT_MIN && val != prevBattery1Toggle) {
        // Switch toggled
        // Only action if APU Bleed is not being pressed and held.
        // This allows a toggle to be switched without causing an
        // action (to fix an inverted toggle).
        if (prevApuBleedPush % 2 == 1) {
            if (globals.aircraft == FBW_A320NEO) {
                globals.simVars->write(KEY_ELEC_BAT1, val);
            }
            else if (airliner) {
                // SDK bug - On not working
                globals.simVars->write(KEY_TOGGLE_MASTER_BATTERY, 1);
            }
            else {
                globals.simVars->write(KEY_TOGGLE_MASTER_ALTERNATOR, 1);
                globals.simVars->write(KEY_TOGGLE_MASTER_ALTERNATOR, 2);
            }
        }
        prevBattery1Toggle = val;
    }

    // Battery 2 toggle
    val = globals.gpioCtrl->readToggle(battery2Control);
    if (val != INT_MIN && val != prevBattery2Toggle) {
        // Switch toggled (ignore if APU Bleed being pressed)
        if (prevApuBleedPush % 2 == 1) {
            if (globals.aircraft == FBW_A320NEO) {
                globals.simVars->write(KEY_ELEC_BAT2, val);
            }
            else if (airliner) {
                // SDK bug - On not working
                globals.simVars->write(KEY_TOGGLE_MASTER_BATTERY, 2);
            }
            else {
                globals.simVars->write(KEY_TOGGLE_MASTER_BATTERY, 1);
            }
        }
        prevBattery2Toggle = val;
    }

    // Fuel Pump toggle
    val = globals.gpioCtrl->readToggle(fuelPumpControl);
    if (val != INT_MIN && val != prevFuelPumpToggle) {
        // Switch toggled (ignore if APU Bleed being pressed)
        if (prevApuBleedPush % 2 == 1) {
            // Toggle fuel pump
            globals.simVars->write(KEY_FUEL_PUMP);
        }
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
        // Switch toggled (ignore if APU Bleed being pressed)
        if (prevApuBleedPush % 2 == 1) {
            if (airliner) {
#ifdef vJoyFallback
                // Toggle external power
                globals.simVars->write(VJOY_BUTTON_14);
#endif
            }
            else {
                globals.simVars->write(KEY_AVIONICS_MASTER_SET, val);
            }
        }
        prevAvionics1Toggle = val;
    }

    // Avionics 2 toggle (Jetway on airliner)
    val = globals.gpioCtrl->readToggle(avionics2Control);
    if (val != INT_MIN && val != prevAvionics2Toggle) {
        // Switch toggled (ignore if APU Bleed being pressed)
        if (prevApuBleedPush % 2 == 1) {
            if (airliner) {
                globals.simVars->write(KEY_TOGGLE_JETWAY);
            }
            else {
                globals.simVars->write(KEY_AVIONICS_MASTER_SET, val);
            }
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
            globals.simVars->write(KEY_APU_OFF_SWITCH, apuMaster);
        }
        prevApuMasterPush = val;
        time(&lastApuMasterAdjust);
    }
    else if (lastApuMasterAdjust != 0) {
        if (now - lastApuMasterAdjust > 1) {
            lastApuMasterAdjust = 0;
        }
    }

    // APU Start push
    val = globals.gpioCtrl->readPush(apuStartControl);
    if (val != INT_MIN) {
        if (prevApuStartPush % 2 == 1) {
            // Button pushed - Can only turn APU Start on, not off
            if (!apuStart) {
                apuStart = !apuStart;
                apuStartFlash = 0;
                globals.gpioCtrl->writeLed(apuStartControl, apuStart);
                globals.simVars->write(KEY_APU_STARTER, apuStart);
            }
        }
        prevApuStartPush = val;
        time(&lastApuStartAdjust);
    }
    else if (lastApuStartAdjust != 0) {
        if (now - lastApuStartAdjust > 1) {
            lastApuStartAdjust = 0;
        }
    }

    // APU Bleed push
    val = globals.gpioCtrl->readPush(apuBleedControl);
    if (val != INT_MIN) {
        if (prevApuBleedPush % 2 == 1) {
            // Button pushed
            apuBleed = !apuBleed;
            globals.gpioCtrl->writeLed(apuBleedControl, apuBleed && airliner);
            // Toggle APU bleed air source
            globals.simVars->write(KEY_BLEED_AIR_SOURCE_CONTROL_SET, apuBleed);
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

void powerLights::gpioFlapsInput()
{
    // Flaps rotate
    int val = globals.gpioCtrl->readRotation(flapsPosControl);
    if (val != INT_MIN) {
        flapsVal = val;
    }

    // Flaps up toggle
    val = globals.gpioCtrl->readToggle(flapsUpControl);
    if (val != INT_MIN && val != prevFlapsUpToggle) {
        // Switch toggled
        prevFlapsUpToggle = val;
        if (val == 1) {
            // Switch pressed
            globals.simVars->write(KEY_FLAPS_UP);
            lastFlapsPos = 0;
            if (flapsVal != INT_MIN) {
                flapsUpVal = flapsVal;  // Re-calibrate flaps values
                int diff = flapsDownVal - flapsUpVal;
                if (diff < 17 || diff > 23) {
                    flapsDownVal = flapsUpVal + 20;
                }
            }
            return;
        }
    }

    // Flaps down toggle
    val = globals.gpioCtrl->readToggle(flapsDownControl);
    if (val != INT_MIN && val != prevFlapsDownToggle) {
        // Switch toggled
        prevFlapsDownToggle = val;
        if (val == 1) {
            // Switch pressed
            globals.simVars->write(KEY_FLAPS_DOWN);
            lastFlapsPos = 4;
            if (flapsVal != INT_MIN) {
                flapsDownVal = flapsVal;    // Re-calibrate flaps values
                int diff = flapsDownVal - flapsUpVal;
                if (diff < 17 || diff > 23) {
                    flapsUpVal = flapsDownVal - 20;
                }
            }
            return;
        }
    }

    if (flapsVal != INT_MIN) {
        // Check for new flaps position
        double onePos = (flapsDownVal - flapsUpVal) / 4.0;
        int flapsPos = (flapsVal + (onePos / 2.0) - flapsUpVal) / onePos;
        if (flapsPos != lastFlapsPos) {
            lastFlapsPos = flapsPos;
            // Set flaps to position 1, 2 or 3
            switch (flapsPos) {
            case 1: globals.simVars->write(KEY_FLAPS_1); break;
            case 2: globals.simVars->write(KEY_FLAPS_2); break;
            case 3: globals.simVars->write(KEY_FLAPS_3); break;
            }
        }
    }
}

void powerLights::gpioParkBrakeInput()
{
    // Park brake off toggle
    int val = globals.gpioCtrl->readToggle(parkBrakeOffControl);
    if (val != INT_MIN && val != prevParkBrakeOffToggle) {
        // Switch toggled
        if (val == 1 && parkBrakeOn) {
            // Switch pressed
            globals.simVars->write(VJOY_BUTTON_15);
            parkBrakeOn = false;
        }
        prevParkBrakeOffToggle = val;
    }

    // Park brake on toggle
    val = globals.gpioCtrl->readToggle(parkBrakeOnControl);
    if (val != INT_MIN && val != prevParkBrakeOnToggle) {
        // Switch toggled
        if (val == 1 && !parkBrakeOn) {
            // Switch pressed
            globals.simVars->write(VJOY_BUTTON_16);
            parkBrakeOn = true;
        }
        prevParkBrakeOnToggle = val;
    }
}
