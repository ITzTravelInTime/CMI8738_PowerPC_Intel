#!/bin/sh

if [ -x /usr/bin/sudo -a $UID -ne 0 ]; then
   exec /usr/bin/sudo $0 $*
fi

echo "Installing CMI8738 kernel extension..."

if [ -d /System/Library/Extensions/CMI8738PCIAudioDriver.kext ]; then
	kextunload /System/Library/Extensions/CMI8738PCIAudioDriver.kext > /dev/null
	kextunload /System/Library/Extensions/CMI8738PCIAudioDriver.kext > /dev/null
	rm -R /System/Library/Extensions/CMI8738PCIAudioDriver.kext
fi

cp -R CMI8738PCIAudioDriver.kext /System/Library/Extensions/

find /System/Library/Extensions/CMI8738PCIAudioDriver.kext -type d -exec /bin/chmod 0755 {} \;
find /System/Library/Extensions/CMI8738PCIAudioDriver.kext -type f -exec /bin/chmod 0744 {} \;
chown -R root:wheel /System/Library/Extensions/CMI8738PCIAudioDriver.kext

kextload /System/Library/Extensions/CMI8738PCIAudioDriver.kext

echo "Installation finished - enjoy! dogbert <dogber1@gmail.com>"