echo Building power-lights-panel
cd power-lights-panel
g++ -lwiringPi -lpthread  \
    -o power-lights-panel \
    -I . \
    settings.cpp \
    simvarDefs.cpp \
    simvars.cpp \
    gpioctrl.cpp \
    powerLights.cpp \
    powerLights-panel.cpp \
    || exit
echo Done
