echo Building power-lights-panel
cd power-lights-panel
g++ -o power-lights-panel -I . \
    settings.cpp \
    simvarDefs.cpp \
    simvars.cpp \
    globals.cpp \
    gpioctrl.cpp \
    powerLights.cpp \
    powerLights-panel.cpp \
    -lwiringPi -lpthread || exit
echo Done
