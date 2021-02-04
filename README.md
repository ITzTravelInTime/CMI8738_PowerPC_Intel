# CMI8738_PowerPC_Intel
Mac OS X driver for the CMI8738 and compatible sound cards, for both Intel and Power PC Macs.

Based on Dogbert's (dogber1@gmail.com) CMI8738 driver, this is a fork that adds support for big endian and little endian architectures, and some minor tweaks.

You can find pre-build binary files in the releases section, which have been tested on my Power Mac G4 (2001 quick silver model).

# Requirements

As now i couldn't get the driver working on OS X Tiger and older versions, so for now it requires OS X Leopard.

# Compiling Requirements

Can be compiled just with Xcode 2.x and 3.x, the project is configured to be buildable for both PPC and Intel and has support for both 32 and 64 bits targets for either architectures.

# Credits

Dogbert (dogber1@gmail.com) for it's very good and simple CMI8738 driver.
ITzTravelInTime for PPC mods and tweaks.