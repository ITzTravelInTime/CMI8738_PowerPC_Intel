# CMI8738_PowerPC_Intel
Mac OS X driver for the CMI8738 and compatible sound cards, for both Intel and Power PC Macs.

Based on Dogbert's (dogber1@gmail.com) CMI8738 OS X driver, this is a fork that adds support for big endian cpu architectures (like PPC), and some minor tweaks.

# Requirements

As now i couldn't get the driver working on OS X Tiger and older versions, so for now it requires OS X Leopard.

# Download

A PPC 32 binary can be downloaded in the Releases section of this github repo (https://github.com/ITzTravelInTime/CMI8738_PowerPC_Intel/releases), it has been tested on my PowerMac G4 (2001 quick silver model).

# Compiling Requirements

This driver can be compiled just with Xcode 2.x and 3.x, the project is configured to be buildable for both PPC and Intel and has support for both 32 and 64 bits targets 9at least at code level).

# Credits

Dogbert (dogber1@gmail.com) for it's very good and simple CMI8738 driver.
ITzTravelInTime for PPC mods and tweaks.
