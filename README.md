# CMI8738_PowerPC_Intel
Mac OS X driver for the CMI8738, CMI8330, CMI8768 and compatible sound cards, for both Intel and Power PC Macs.

Based on Dogbert's (dogber1@gmail.com) CMI8738 OS X driver, this is a fork that adds support for big endian cpu architectures (like PPC), adds more supported chips, and some minor tweaks.

# Requirements

As now i couldn't get the driver working on OS X Tiger and older versions (except for OS X Tiger Server), so for now it requires OS X Leopard or OS X Tiger Server.

# Download

A very old and obsolete PPC binary can be downloaded in the [Releases section of this github repo](https://github.com/ITzTravelInTime/CMI8738_PowerPC_Intel/releases), it has been tested on my PowerMac G4, it doesn't support other chips than the CMI8738, support for other chips is still in development.

# Compiling Requirements

This driver can be compiled just with Xcode 2.x and 3.x, the project is configured to be buildable for PPC and Intel (which also has support for both 32 and 64 bits targets, to be buildable for 64 bits you need to use OS X Snow Leopard and the OS X 10.6 SDK).

# Credits

Dogbert (dogber1@gmail.com) for the original CMI8738 driver.
ITzTravelInTime for PPC support, more chips and other mods and tweaks.
