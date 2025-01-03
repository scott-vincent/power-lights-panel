# MICROSOFT FLIGHT SIMULATOR 2020 - POWER/LIGHTS PANEL

![Screenshot](Screenshot.jpg)
![Screenshot](Screenshot2.jpg)

# Quick Start

Download the following two files.

Link: [Latest release of Power/Lights Panel for Raspberry Pi Zero W](https://github.com/scott-vincent/power-lights-panel/releases/latest/download/power-lights-panel-v1.5.5-raspi.tar.gz)

Link: [Latest release of Instrument Data Link for Windows](https://github.com/scott-vincent/instrument-data-link/releases/latest/download/instrument-data-link-v2.0.5-windows-x64.zip)

Unzip instrument-data-link into its own folder and double-click instrument-data-link.exe to run it.

Untar power-lights-panel on your Raspberry Pi. Edit settings/power-lights-panel.json and in the "Data Link" section change the IP address of the "Host" to the address where FS2020 is running on your local network, e.g. 192.168.0.1 - You can find the correct address of your host by running a command prompt on the host machine and running ipconfig, then scroll back and look for the first "IPv4 Address" line. Now enter ./run.sh to run the program.

# Introduction

A power/lights panel for MS FlightSim 2020. This program is designed to run
on a Raspberry Pi Zero W and requires the following hardware:

Double rocker switch x 2 : https://www.amazon.co.uk/gp/product/B0743BMMGG

On/off switch x 7 : https://www.amazon.co.uk/gp/product/B07MS12WV6    

Push button with LED x 3 : https://www.amazon.co.uk/gp/product/B07KPSZ731 

It requires the companion program from here

  https://github.com/scott-vincent/instrument-data-link

The companion program runs on the same host as MS FS2020 and passes data between
the panel and the flight simulator over your Wifi connection.

# Donate

If you find this project useful, would like to see it developed further or would just like to buy the author a beer, please consider a small donation.

[<img src="donate.svg" width="210" height="40">](https://paypal.me/scottvincent2020)

# Additional Photos

Panel Front
![Panel Front](Panel_Front.jpg)

Panel Back
![Panel Back](Panel_Back.jpg)
