#include <stdio.h>
#include <stdlib.h>
#include "gpioctrl.h"
#include "powerLights.h"

// Some SimConnect events don't work with certain aircraft
// so use vJoy to simulate joystick button presses instead.
//
// Comment the following line out if you don't want to use vJoy.
#define vJoyFallback

//#define DEBUG

powerLights::powerLights()
{
    simVars = &globals.simVars->simVars;
    addGpio();

    fflush(stdout);
}

void powerLights::render()
{
#ifdef DEBUG
    lightsTest--;
    if (lightsTest < 0) {
        lightsTest = 20;
    }
    if (lightsTest == 20) {
        globals.gpioCtrl->writeLed(apuMasterControl, true);
        globals.gpioCtrl->writeLed(apuStartControl, true);
        globals.gpioCtrl->writeLed(apuBleedControl, true);
    }
    else if (lightsTest == 5) {
        globals.gpioCtrl->writeLed(apuMasterControl, false);
        globals.gpioCtrl->writeLed(apuStartControl, false);
        globals.gpioCtrl->writeLed(apuBleedControl, false);
    }
    return;
#endif

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
    // Need to know if airliner even when electrics are off
    airliner = (globals.aircraft != NO_AIRCRAFT && simVars->cruiseSpeed >= 300
        && globals.aircraft != CESSNA_CJ4 && globals.aircraft != F15_EAGLE && globals.aircraft != F18_HORNET);

    // Check for aircraft change
    bool aircraftChanged = (globals.electrics && loadedAircraft != globals.aircraft);
    if (aircraftChanged) {
        loadedAircraft = globals.aircraft;
        apuMaster = false;
        apuStart = false;
        apuBleed = false;
        lastApuMasterAdjust = 0;
        lastApuStartAdjust = 0;
        lastApuBleedAdjust = 0;
        lastFlapsPos = -1;
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
        if (simVars->sbParkBrake != -1) {
            prevSbParkBrake = 0;
        }
        if (simVars->altAboveGround < 50) {
            // Start with parking brake on (will only turn on when electrics enabled!)
            if (!simVars->parkingBrakeOn) {
                globals.simVars->write(KEY_PARKING_BRAKE_SET, 1);
            }
            // Start with beacon off
            globals.simVars->write(KEY_BEACON_LIGHTS_SET, 0);
        }
    }

    time(&now);
    gpioSwitchesInput();
    gpioButtonsInput();
    gpioFlapsInput();
    gpioParkBrakeInput();

    // Only update local values from sim if they are not currently being adjusted.
    // This stops them from jumping around due to lag of fetch/update cycle.
    if (lastApuMasterAdjust == 0) {
        if (loadedAircraft == AIRBUS_A310 || loadedAircraft == FBW) {
            apuMaster = simVars->apuMasterSw > 0;
        }
        else if (simVars->apuStartSwitch > 0) {
            apuMaster = true;
        }
    }

    if (lastApuStartAdjust == 0) {
        if (simVars->apuPercentRpm == 100) {
            // APU is on and available
            apuStart = true;
            apuStartFlash = 0;
        }
        else if (simVars->apuStartSwitch > 0) {
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
        // Only relevant to A32NX
        if (loadedAircraft == AIRBUS_A310 || loadedAircraft == FBW) {
            apuBleed = simVars->apuBleed > 0;
        }
        else {
            // APU bleed seems to be automatic on 747
            apuBleed = simVars->apuPercentRpm == 100;
        }
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
            if (globals.aircraft == AIRBUS_A310 || globals.aircraft == FBW) {
                globals.simVars->write(KEY_ELEC_BAT1, val);
            }
            else if (airliner) {
                // SDK bug - Not working
                // globals.simVars->write(KEY_TOGGLE_MASTER_BATTERY, 1);
#ifdef vJoyFallback
                // Toggle master battery using vJoy
                globals.simVars->write(VJOY_BUTTON_15);
#endif
            }
            else {
                globals.simVars->write(KEY_TOGGLE_MASTER_ALTERNATOR, 1);
                //globals.simVars->write(KEY_TOGGLE_MASTER_ALTERNATOR, 2);
            }
        }
        prevBattery1Toggle = val;
    }

    // Battery 2 toggle
    val = globals.gpioCtrl->readToggle(battery2Control);
    if (val != INT_MIN && val != prevBattery2Toggle) {
        // Switch toggled (ignore if APU Bleed being pressed)
        if (prevApuBleedPush % 2 == 1) {
            if (globals.aircraft == AIRBUS_A310 || globals.aircraft == FBW) {
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

    // Fuel Pump toggle (Jetway toggle on airliner)
    val = globals.gpioCtrl->readToggle(fuelPumpControl);
    if (val != INT_MIN && val != prevFuelPumpToggle) {
        // Switch toggled (ignore if APU Bleed being pressed)
        if (prevApuBleedPush % 2 == 1) {
            if (airliner) {
                // Don't re-engage jetway if pushback being requested
                if (simVars->pushbackState > 0) {
                    globals.simVars->write(KEY_TOGGLE_JETWAY);
                }
            }
            else {
                // Toggle fuel pump
                globals.simVars->write(KEY_FUEL_PUMP);
                // Use same switch to toggle dome light on Cessna 152
                globals.simVars->write(KEY_CABIN_LIGHTS_SET, val);
#ifdef vJoyFallback
                // Not working so use vJoy
                //globals.simVars->write(VJOY_BUTTON_11);
#endif
            }
        }
        prevFuelPumpToggle = val;
    }

    // Beacon toggle
    val = globals.gpioCtrl->readToggle(beaconControl);
    if (val != INT_MIN && val != prevBeaconToggle) {
        // Switch toggled
        globals.simVars->write(KEY_BEACON_LIGHTS_SET, val);
        prevBeaconToggle = val;
    }

    // Land toggle
    val = globals.gpioCtrl->readToggle(landControl);
    if (val != INT_MIN && val != prevLandToggle) {
        // Switch toggled
        globals.simVars->write(KEY_LANDING_LIGHTS_SET, val);
        prevLandToggle = val;
    }

    // Taxi toggle
    val = globals.gpioCtrl->readToggle(taxiControl);
    if (val != INT_MIN && val != prevTaxiToggle) {
        // Switch toggled
        globals.simVars->write(KEY_TAXI_LIGHTS_SET, val);
        prevTaxiToggle = val;
    }

    // Nav toggle
    val = globals.gpioCtrl->readToggle(navControl);
    if (val != INT_MIN && val != prevNavToggle) {
        // Switch toggled
        globals.simVars->write(KEY_NAV_LIGHTS_SET, val);
        prevNavToggle = val;
    }

    // Strobe toggle
    val = globals.gpioCtrl->readToggle(strobeControl);
    if (val != INT_MIN && val != prevStrobeToggle) {
        // Switch toggled
        globals.simVars->write(KEY_STROBES_SET, val);
        prevStrobeToggle = val;
    }

    // Pitot Heat toggle
    val = globals.gpioCtrl->readToggle(pitotHeatControl);
    if (val != INT_MIN && val != prevPitotHeatToggle) {
        // Switch toggled
        globals.simVars->write(KEY_PITOT_HEAT_SET, val);
        //globals.simVars->write(KEY_ANTI_ICE_SET, val);
        //globals.simVars->write(KEY_WINDSHIELD_DEICE_SET, val);
        // SDK bug - Not working for A320 so use vJoy
        if (loadedAircraft != CESSNA_152 && loadedAircraft != CESSNA_172 && loadedAircraft != UNDEFINED) {
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
        }
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
            else if (loadedAircraft == CESSNA_152) {
                // Turn Radios on/off
                globals.simVars->write(KEY_COM1_VOLUME_SET, val);
                globals.simVars->write(KEY_COM2_VOLUME_SET, val);
            }
            else {
                globals.simVars->write(KEY_AVIONICS_MASTER_SET, val);
            }
        }
        prevAvionics1Toggle = val;
    }

    // Avionics 2 toggle (not working so disable)
    //val = globals.gpioCtrl->readToggle(avionics2Control);
    //if (val != INT_MIN && val != prevAvionics2Toggle) {
    //    // Switch toggled (ignore if APU Bleed being pressed)
    //    if (prevApuBleedPush % 2 == 1) {
    //        if (airliner) {
    //            // Don't re-engage jetway if pushback being requested
    //            if (simVars->pushbackState > 0) {
    //                globals.simVars->write(KEY_TOGGLE_JETWAY);
    //            }
    //        }
    //        else if (loadedAircraft == CESSNA_152) {
    //            // Turn Radio 2 on/off
    //            globals.simVars->write(KEY_COM2_VOLUME_SET, val);
    //        }
    //        else
    //            globals.simVars->write(KEY_AVIONICS_MASTER_SET, val);
    //        }
    //    }
    //    prevAvionics2Toggle = val;
    //}
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
            if (apuMaster && !apuStart) {
                apuStart = true;
                apuStartFlash = 0;
                globals.gpioCtrl->writeLed(apuStartControl, apuStart);
                globals.simVars->write(KEY_APU_STARTER, apuStart);
            }
            else if (!apuMaster) {
                // If Start button pushed and Master Switch is off call for ramp instead
                globals.simVars->write(KEY_TOGGLE_RAMPTRUCK);
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
    int flapsVal = globals.gpioCtrl->readRotation(flapsPosControl);

    // Flaps up toggle
    int val = globals.gpioCtrl->readToggle(flapsUpControl);
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
            if (simVars->tfFlapsCount == 5) {
                lastFlapsPos = 4;
            }
            else {
                lastFlapsPos = simVars->tfFlapsCount;
            }
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
        if (simVars->tfFlapsCount == 6) {
            // Boeing 747 has 6 flap positions so insert 2 extra
            // flap positions between 1,2 and 3,full
            double halfPos = (flapsDownVal - flapsUpVal) / 8.0;
            int flapsHalfPos = (flapsVal + (halfPos / 2.0) - flapsUpVal) / halfPos;
            if (flapsHalfPos >= 3) {
                flapsPos++;
            }
            if (flapsHalfPos >= 7) {
                flapsPos++;
            }
        }
        else if (simVars->tfFlapsCount == 9) {
            // Boeing 787 has 9 flap positions so insert 2 extra flap
            // positions between 1,2 and 3,full and 3 between 2,3.
            double halfPos = (flapsDownVal - flapsUpVal) / 8.0;
            int flapsHalfPos = (flapsVal + (halfPos / 2.0) - flapsUpVal) / halfPos;
            double thirdPos = (flapsDownVal - flapsUpVal) / 12.0;
            int flapsThirdPos = (flapsVal + (thirdPos / 3.0) - flapsUpVal) / thirdPos;
            if (flapsHalfPos >= 3) {
                flapsPos++;
            }
            if (flapsHalfPos >= 7) {
                flapsPos++;
            }
            if (flapsThirdPos >= 7) {
                flapsPos++;
            }
            if (flapsThirdPos >= 8) {
                flapsPos++;
            }
            if (flapsThirdPos >= 9) {
                flapsPos++;
            }
        }
        if (flapsPos != lastFlapsPos) {
            if (lastFlapsPos != -1) {
                if (simVars->tfFlapsCount == 5) {
                    while (flapsPos > lastFlapsPos) {
                        globals.simVars->write(KEY_FLAPS_INCR);
                        lastFlapsPos++;
                    }
                    while (flapsPos < lastFlapsPos) {
                        globals.simVars->write(KEY_FLAPS_DECR);
                        lastFlapsPos--;
                    }
                }
                else {
                    double flapsSet = 16384.0 * flapsPos / simVars->tfFlapsCount;
                    globals.simVars->write(KEY_FLAPS_SET, flapsSet);
                }
            }
            lastFlapsPos = flapsPos;
        }
    }
}

void powerLights::gpioParkBrakeInput()
{
    // Park brake off toggle
    int val = globals.gpioCtrl->readToggle(parkBrakeOffControl);
    if (val != INT_MIN && val != prevParkBrakeOffToggle) {
        // Switch toggled
        if (val == 1 && simVars->parkingBrakeOn) {
            // Switch pressed and parking brake on so release it
            globals.simVars->write(KEY_PARKING_BRAKE_SET, 0);
        }
        prevParkBrakeOffToggle = val;
        return;
    }

    // Park brake on toggle
    val = globals.gpioCtrl->readToggle(parkBrakeOnControl);
    if (val != INT_MIN && val != prevParkBrakeOnToggle) {
        // Switch toggled
        if (val == 1 && !simVars->parkingBrakeOn) {
            // Switch pressed and parking brake off so apply it
            globals.simVars->write(KEY_PARKING_BRAKE_SET, 1);
        }
        prevParkBrakeOnToggle = val;
        return;
    }

    // Check Switchbox parking brake
    if (sbParkBrakeDelay > 0) {
        sbParkBrakeDelay--;
    }
    else if (simVars->sbParkBrake != prevSbParkBrake) {
        prevSbParkBrake = simVars->sbParkBrake;
        if (simVars->sbParkBrake == 1 && !simVars->parkingBrakeOn) {
            globals.simVars->write(KEY_PARKING_BRAKE_SET, 1);
            sbParkBrakeDelay = 3;
        }
        else if (simVars->sbParkBrake == 0 && simVars->parkingBrakeOn) {
            globals.simVars->write(KEY_PARKING_BRAKE_SET, 0);
            sbParkBrakeDelay = 3;
        }
    }
}
