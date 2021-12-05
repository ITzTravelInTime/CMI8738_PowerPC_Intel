#!/bin/sh

cd "$( dirname "$0" )"

if [ -x /usr/bin/sudo -a $UID -ne 0 ]; then
   exec /usr/bin/sudo $0 $*
fi

echo "Installing CMI8738 kernel extension..."

if [ -d /System/Library/Extensions/CMI8738AudioDriver_updated.kext ]; then

	echo "This driver is already Installed, it will be unloaded and overwritten!"

	kextunload /System/Library/Extensions/CMI8738AudioDriver_updated.kext > /dev/null
	kextunload /System/Library/Extensions/CMI8738AudioDriver_updated.kext > /dev/null
	
	kextunload /System/Library/Extensions/CMI8738AudioDriver_updated.kext > /dev/null
	kextunload /System/Library/Extensions/CMI8738AudioDriver_updated.kext > /dev/null
	
	rm -R /System/Library/Extensions/CMI8738AudioDriver_updated.kext
fi

echo "Installing a fresh copy of the driver ..."

cp -R ./CMI8738AudioDriver_updated.kext /System/Library/Extensions/

find /System/Library/Extensions/CMI8738AudioDriver_updated.kext -type d -exec /bin/chmod 0755 {} \;
find /System/Library/Extensions/CMI8738AudioDriver_updated.kext -type f -exec /bin/chmod 0744 {} \;
chown -R root:wheel /System/Library/Extensions/CMI8738AudioDriver_updated.kext

kextload -t /System/Library/Extensions/CMI8738AudioDriver_updated.kext

echo "Installation finished - enjoy! dogbert <dogber1@gmail.com>, ITzTravelInTime"