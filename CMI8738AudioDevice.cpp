/*
Copyright (c) 2005 dogbert <dogber1@gmail.com>, Apple Computer, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ArchDetect.h"

#include "CMI8738AudioDevice.h"

#include "CMI8738AudioEngine.h"

#include <IOKit/audio/IOAudioControl.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>
#include <IOKit/audio/IOAudioDefines.h>

#include <IOKit/IOLib.h>

#include <IOKit/pci/IOPCIDevice.h>

#define super IOAudioDevice

#warning missing mutex implementation 

#define ARRAYSZ(ar) ((sizeof(ar))/(sizeof(*ar)))

OSDefineMetaClassAndStructors(CMI8738AudioDevice, IOAudioDevice)

bool CMI8738AudioDevice::initHardware(IOService *provider)
{
    bool result = false;
    UInt32 deviceID = 0;
	
	/*
	{
		char tmp[16];
		
		if (PE_parse_boot_argn("-cmioff", tmp, 16)){
			DBGPRINT("CMI8738AudioDevice[%p] Driver load disabled by boot arg\n", this);
			return false;
		}
	}
    */
	 
    DBGPRINT("CMI8738AudioDevice[%p]::initHardware(%p)\n", this, provider);
    
    if (!super::initHardware(provider)) {
        goto Done;
    }
	
	cm.pciDevice = NULL;
	cm.deviceMap = NULL;
    
    cm.can96k = false;
    cm.canAC3SW = false;
    cm.canAC3HW = false;
    cm.chipVersion = 0;
    cm.maxChannels = 2;
    cm.canMultiChannel = false;
    cm.hasDualDAC = false;
    cm.supports24Bit = false;
    
    // Get the PCI device provider
    cm.pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!cm.pciDevice) {
        goto Done;
    }
    
    // Config a map for the PCI config base registers
    // We need to keep this map around until we're done accessing the registers
    cm.deviceMap = cm.pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!cm.deviceMap) {
        goto Done;
    }
    
    // Enable the PCI memory access - the kernel will panic if this isn't done before accessing the 
    // mapped registers
    cm.pciDevice->setMemoryEnable(false);
    cm.pciDevice->setIOEnable(true);
    cm.pciDevice->setBusMasterEnable(true);
	
    deviceID = cm.pciDevice->configRead32(kIOPCIConfigDeviceID) >> 16;
    
    DBGPRINT("CMI8738AudioDevice[%p]::initHardware - Detected deviceID: %lx\n", this, deviceID);
    
    if (deviceID == PCI_DEVICE_ID_CMEDIA_CM8338A || deviceID == PCI_DEVICE_ID_CMEDIA_CM8338B){
        //UInt32 detect = readUInt32(CM_REG_INT_HLDCLR) & CM_CHIP_MASK2;
        //DBGPRINT("CMI8738AudioDevice[%p]::initHardware - Detected interrupt register value: %lx\n", this, detect);
        
        setDeviceName("C-Media 8338");
        setDeviceShortName("CMI8338");
		
		//cm.canAC3SW = true;
        
        DBGPRINT("CMI8738AudioDevice[%p]::initHardware - Detected device: CMI83338\n", this);
	}else if (deviceID == PCI_DEVICE_ID_CMEDIA_CM8738 || deviceID == PCI_DEVICE_ID_CMEDIA_CM8738B){
        
        setDeviceName("C-Media 8738");
        setDeviceShortName("CMI8738");
        
        DBGPRINT("CMI8738AudioDevice[%p]::initHardware - Detected device: CMI83738\n", this);
    }else{
        
        setDeviceName("C-Media Chip");
        setDeviceShortName("CMI8xx8");
        
        DBGPRINT("CMI8738AudioDevice[%p]::initHardware - Detected device: Generic/Other CMI chip\n", this);
    }
    
    if (deviceID != PCI_DEVICE_ID_CMEDIA_CM8338A && deviceID != PCI_DEVICE_ID_CMEDIA_CM8338B){
        queryChip();
    }
    
    setManufacturerName("C-Media, mod by ITzTravelInTime");
	setDeviceTransportType(kIOAudioDeviceTransportTypePCI);

	cm.regFUNCTRL0 = CM_CHADC1;
	
	/* initialize codec registers */
	writeUInt32(CM_REG_INT_HLDCLR, 0);	// disable ints
	resetChannel(CM_CH_PLAY);
	resetChannel(CM_CH_CAPT);
	writeUInt32(CM_REG_FUNCTRL0, 0);	/* disable channels */
	writeUInt32(CM_REG_FUNCTRL1, 0);

	writeUInt32(CM_REG_CHFORMAT, 0);
	
    if (cm.hasDualDAC)
        setUInt32Bit(CM_REG_MISC_CTRL, CM_ENDBDAC | CM_N4SPK3D);
    else
        clearUInt32Bit(CM_REG_MISC_CTRL, CM_ENDBDAC | CM_N4SPK3D);
	
    clearUInt32Bit(CM_REG_MISC_CTRL, CM_XCHGDAC); //Sets ch0 as the front output and ch1 as the rear
    
    //from the linux driver
    if (cm.chipVersion) {
        writeUInt8(CM_REG_EXT_MISC, 0x20); /* magic */
        writeUInt8(CM_REG_EXT_MISC + 1, 0x09); /* more magic */
    }

    /* Set Bus Master Request */
	setUInt32Bit(CM_REG_FUNCTRL1, CM_BREQ);
    
    //presumebly distributed dma suport, any intel mac should support this
    #ifndef PPC
    if (deviceID == PCI_DEVICE_ID_CMEDIA_CM8738 || deviceID == PCI_DEVICE_ID_CMEDIA_CM8738B)
        setUInt32Bit(CM_REG_MISC_CTRL, CM_TXVX);
    #endif
	
    if (cm.chipVersion < 68){
        //disable opl3
        clearUInt32Bit(CM_REG_LEGACY_CTRL, CM_FMSEL_MASK);
        clearUInt32Bit(CM_REG_MISC_CTRL, CM_FM_EN);
    }
	
	writeUInt8(CM_REG_MIXER1, 0);
	
    //TODO: eventually add those as toggle controls
    //if (cm.chipVersion)
    //setUInt8Bit(CM_REG_MIXER1, CM_X3DEN); //enable X3D only form 8738 and up, it causes just harm on 8338
	
	/*
	#define CM_REG_MIXER1        0x24
#define CM_FMMUTE        0x80    // mute FM 
#define CM_FMMUTE_SHIFT        7
#define CM_WSMUTE        0x40    // mute PCM 
#define CM_WSMUTE_SHIFT        6
#define CM_REAR2LIN        0x20    // lin-in -> rear line out 
#define CM_REAR2LIN_SHIFT    5
#define CM_REAR2FRONT        0x10    // exchange rear/front
#define CM_REAR2FRONT_SHIFT    4
#define CM_WAVEINL        0x08    // digital wave rec. left chan
#define CM_WAVEINL_SHIFT    3
#define CM_WAVEINR        0x04    // digical wave rec. right 
#define CM_WAVEINR_SHIFT    2
#define CM_X3DEN        0x02    // 3D surround enable 
#define CM_X3DEN_SHIFT        1
#define CM_CDPLAY        0x01    // enable SPDIF/IN PCM -> DAC 
#define CM_CDPLAY_SHIFT        0
	*/
	
	setUInt8Bit(CM_REG_MIXER1, CM_CDPLAY | CM_FMMUTE /*| CM_REAR2LIN*/);
    
    if (!cm.hasDualDAC)
		setUInt8Bit(CM_REG_MIXER1, CM_WAVEINR | CM_WAVEINL); //enable wave recording
    
    //setting second mixer
    
    writeUInt8(CM_REG_MIXER2, 0);
    /*
    writeUInt8(CM_REG_MIXER2, 7 << CM_VADMIC_SHIFT); // sets mic vol?
    
    setUInt8Bit(CM_REG_MIXER2, CM_RAUXLEN | CM_RAUXREN | CM_MICGAINZ); //enable aux in + gain control
    
    clearUInt8Bit(CM_REG_MIXER2, CM_VAUXLM | CM_VAUXRM); //unmute aux in
    
    writeUInt8(CM_REG_AUX_VOL, 0xFF); //maxes out aux vol
	 */
	 
	if (!cm.chipVersion){
		writeUInt8(CM_REG_EXTENT_IND, 0);
        /*
         #define CM_VSPKM        0x08    // Speaker mute control, default high
         #define CM_RLOOPREN        0x04    // Rec. R-channel enable
         #define CM_RLOOPLEN        0x02    // Rec. L-channel enable
         */
        
        //setUInt8Bit(CM_REG_EXTENT_IND, CM_VPHOM); //unmute pc spacker passtrough?
        
        //if (!cm.hasDualDAC)
        //    setUInt8Bit(CM_REG_EXTENT_IND, CM_RLOOPREN | CM_RLOOPLEN);
	}
	
	writeMixer(0, 0);
	
    //disasble joystick
	clearUInt32Bit(CM_REG_FUNCTRL1, CM_JYSTK_EN);
	
    //disable modem link
	if (cm.chipVersion < 39){
		clearUInt32Bit(CM_REG_MISC_CTRL, CM_FLINKON);
		setUInt32Bit(CM_REG_MISC_CTRL, CM_FLINKOFF);
	}
	
    //set pll values??
	 #if 1
	 if (!cm.chipVersion){
		static const UInt32 rates[] = { 5512, 11025, 22050, 44100, 8000, 16000, 32000, 48000 };
		for (unsigned int val = 0; val < ARRAYSZ(rates); val++)
			set_pll(rates[val], val);
			
		setUInt32Bit(CM_REG_MISC_CTRL, CM_SPDIF48K|CM_SPDF_AC97);
	 }
	 #endif
	
    if (!createAudioEngine()) {
        goto Done;
    }
    
    result = true;
    
Done:

    if (!result) {
        if (cm.deviceMap) {
            cm.deviceMap->release();
            cm.deviceMap = NULL;
        }
    }

    return result;
}

void CMI8738AudioDevice::free()
{
    DBGPRINT("CMI8738AudioDevice[%p]::free()\n", this);
    
    if (cm.deviceMap) {
        cm.deviceMap->release();
        cm.deviceMap = NULL;
    }
    
    super::free();
}


void CMI8738AudioDevice::queryChip()
{
	DBGPRINT("CMI8738AudioDevice[%p]::queryChip()\n", this);
	UInt32 detect;

	/* check reg 0Ch, bit 24-31 */
	detect = readUInt32(CM_REG_INT_HLDCLR) & CM_CHIP_MASK2;
	if (!detect) {
		/* check reg 08h, bit 24-28 */
		detect = readUInt32(CM_REG_CHFORMAT) & CM_CHIP_MASK1;
        /*
         if (!detect) {
             cm.chipVersion = 33;
             cm.maxChannels = 2;
             if (cm.doAC3SW)
                 cm.canAC3SW = true;
             else
                 cm.canAC3HW = true;
             cm.hasDualDAC = true;
         } else {
             cm.chipVersion = 37;
             cm.maxChannels = 2;
             cm.canAC3HW = true;
             cm.hasDualDAC = 1;
         }
         */
        
        switch (detect) {
            case 0:
                cm.chipVersion = 33;
                if (cm.doAC3SW)
                    cm.canAC3SW = true;
                else
                    cm.canAC3HW = true;
                break;
            case CM_CHIP_037:
                cm.chipVersion = 37;
                cm.canAC3HW = true;
                break;
            default:
                cm.chipVersion = 39;
                cm.canAC3HW = true;
                break;
        }
        //cm.hasDualDAC = true;
        cm.maxChannels = 2;
	} else {
		/* check reg 0Ch, bit 26 */
		/*
         if (detect & CM_CHIP_039) {
			cm.chipVersion = 39;
			if (detect & CM_CHIP_039_6CH)
				cm.maxChannels = 6;
			else
				cm.maxChannels = 4;
			cm.canAC3HW = true;
			cm.hasDualDAC = true;
			cm.canMultiChannel = true;
		} else {
			cm.chipVersion = 55; // 4 or 6 channels
			cm.maxChannels = 6;
			cm.canAC3HW = true;
			cm.hasDualDAC = true;
			cm.canMultiChannel = true;
         }
         */
        cm.can96k = true;
        if (detect & CM_CHIP_039) {
            cm.chipVersion = 39;
			cm.can96k = false;
            if (detect & CM_CHIP_039_6CH) /* 4 or 6 channels */
                cm.maxChannels = 6;
            else
                cm.maxChannels = 4;
        } else if (detect & CM_CHIP_8768) {
            cm.chipVersion = 68;
            cm.maxChannels = 8;
        } else {
            cm.chipVersion = 55;
            cm.maxChannels = 6;
        }
        //cm.hasDualDAC = true;
        cm.canAC3HW = true;
        cm.canMultiChannel = true;
    }
    return;
}

void CMI8738AudioDevice::resetChannel(int ch)
{
	UInt32 reset = CM_RST_CH0 << ch;
	writeUInt32(CM_REG_FUNCTRL0, cm.regFUNCTRL0 | reset);
	writeUInt32(CM_REG_FUNCTRL0, cm.regFUNCTRL0 & ~reset);
	IODelay(10);
}

bool CMI8738AudioDevice::createAudioEngine()
{
    bool result = false;
    CMI8738AudioEngine *audioEngine = NULL;
    IOAudioControl *control;
    
    DBGPRINT("CMI8738AudioDevice[%p]::createAudioEngine()\n", this);
    
    audioEngine = new CMI8738AudioEngine;
    if (!audioEngine) {
        goto Done;
    }
    
    // Init the new audio engine with the device registers so it can access them if necessary
    // The audio engine subclass could be defined to take any number of parameters for its
    // initialization - use it like a constructor
    if (!audioEngine->init(&cm)) {
        goto Done;
    }
    
#warning createAudioEngine() incomplete
    
    static const UInt32 ndb = ((-22) << 16) + (32768);
    static const UInt32 pdb = ((22) << 16) + (32768);
    
    // Create a left & right output volume control with an int range from 0 to 65535
    // and a db range from -22.5 to 0.0
    // Once each control is added to the audio engine, they should be released
    // so that when the audio engine is done with them, they get freed properly
    control = IOAudioLevelControl::createVolumeControl(15,	// Initial value
                                                       0,		// min value
                                                       31,		// max value
                                                       ndb,	// -22.5 in IOFixed (16.16)
                                                       0,		// max 0.0 in IOFixed
                                                       kIOAudioControlChannelIDDefaultLeft,
                                                       kIOAudioControlChannelNameLeft,
                                                       0,		// control ID - driver-defined
                                                       kIOAudioControlUsageOutput);
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)volumeChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();
    
    control = IOAudioLevelControl::createVolumeControl(15,	// Initial value
                                                       0,		// min value
                                                       31,		// max value
                                                       ndb,	// min -22.5 in IOFixed (16.16)
                                                       0,		// max 0.0 in IOFixed
                                                       kIOAudioControlChannelIDDefaultRight,	// Affects right channel
                                                       kIOAudioControlChannelNameRight,
                                                       0,		// control ID - driver-defined
                                                       kIOAudioControlUsageOutput);
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)volumeChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();
    
    // Create an output mute control
    control = IOAudioToggleControl::createMuteControl(false,	// initial state - unmuted
                                                      kIOAudioControlChannelIDAll,	// Affects all channels
                                                      kIOAudioControlChannelNameAll,
                                                      0,		// control ID - driver-defined
                                                      kIOAudioControlUsageOutput);
    
    if (!control) {
        goto Done;
    }
    
    control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)outputMuteChangeHandler, this);
    audioEngine->addDefaultAudioControl(control);
    control->release();
    
    if (!cm.hasDualDAC){
        // Create a left & right input gain control with an int range from 0 to 65535
        // and a db range from 0 to 22.5
        control = IOAudioLevelControl::createVolumeControl(65535,	// Initial value
                                                           0,		// min value
                                                           65535,	// max value
                                                           0,		// min 0.0 in IOFixed
                                                           pdb,	// 22.5 in IOFixed (16.16)
                                                           kIOAudioControlChannelIDDefaultLeft,
                                                           kIOAudioControlChannelNameLeft,
                                                           0,		// control ID - driver-defined
                                                           kIOAudioControlUsageInput);
        if (!control) {
            goto Done;
        }
        
        control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)gainChangeHandler, this);
        audioEngine->addDefaultAudioControl(control);
        control->release();
        
        control = IOAudioLevelControl::createVolumeControl(65535,	// Initial value
                                                           0,		// min value
                                                           65535,	// max value
                                                           0,		// min 0.0 in IOFixed
                                                           pdb,	// max 22.5 in IOFixed (16.16)
                                                           kIOAudioControlChannelIDDefaultRight,	// Affects right channel
                                                           kIOAudioControlChannelNameRight,
                                                           0,		// control ID - driver-defined
                                                           kIOAudioControlUsageInput);
        if (!control) {
            goto Done;
        }
        
        control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)gainChangeHandler, this);
        audioEngine->addDefaultAudioControl(control);
        control->release();
        
        // Create an input mute control
        control = IOAudioToggleControl::createMuteControl(false,	// initial state - unmuted
                                                          kIOAudioControlChannelIDAll,	// Affects all channels
                                                          kIOAudioControlChannelNameAll,
                                                          0,		// control ID - driver-defined
                                                          kIOAudioControlUsageInput);
        
        if (!control) {
            goto Done;
        }
        
        control->setValueChangeHandler((IOAudioControl::IntValueChangeHandler)inputMuteChangeHandler, this);
        audioEngine->addDefaultAudioControl(control);
        control->release();
        
    }
    // Active the audio engine - this will cause the audio engine to have start() and initHardware() called on it
    // After this function returns, that audio engine should be ready to begin vending audio services to the system
    activateAudioEngine(audioEngine);
    // Once the audio engine has been activated, release it so that when the driver gets terminated,
    // it gets freed
    audioEngine->release();
    
    result = true;
    
Done:

    if (!result && (audioEngine != NULL)) {
        audioEngine->release();
    }

    return result;
}

IOReturn CMI8738AudioDevice::volumeChangeHandler(IOService *target, IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    CMI8738AudioDevice *audioDevice;
    
    audioDevice = (CMI8738AudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->volumeChanged(volumeControl, oldValue, newValue);
    }
  
    return result;
}

IOReturn CMI8738AudioDevice::volumeChanged(IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
    DBGPRINT("CMI8738AudioDevice[%p]::volumeChanged(%p, %ld, %ld)\n", this, volumeControl, oldValue, newValue);
    
    if (volumeControl) {
        DBGPRINT("\t-> Channel %ld\n", volumeControl->getChannelID());
    }
    
    // Add hardware volume code change 
	if (oldValue != newValue) {
		if (volumeControl->getChannelID() == kIOAudioControlChannelIDDefaultLeft)  {
			writeMixer(0x30, newValue<<3);
		}
		if (volumeControl->getChannelID() == kIOAudioControlChannelIDDefaultRight)  {
			writeMixer(0x31, newValue<<3);
		}
	}
    return kIOReturnSuccess;
}
    
IOReturn CMI8738AudioDevice::outputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    CMI8738AudioDevice *audioDevice;
    
    audioDevice = (CMI8738AudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->outputMuteChanged(muteControl, oldValue, newValue);
    }

	return result;
}

IOReturn CMI8738AudioDevice::outputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    DBGPRINT("CMI8738AudioDevice[%p]::outputMuteChanged(%p, %ld, %ld)\n", this, muteControl, oldValue, newValue);
    
	// Add output mute code here
	
	//clearUInt8Bit(CM_REG_MIXER1, CM_CDPLAY);
	
	if (newValue) {
		clearUInt8Bit(CM_REG_MIXER1, CM_CDPLAY);
		setUInt8Bit(CM_REG_MIXER1, CM_WSMUTE);
	} else {
		clearUInt8Bit(CM_REG_MIXER1, CM_WSMUTE);	
		setUInt8Bit(CM_REG_MIXER1, CM_CDPLAY);
	}

	return kIOReturnSuccess;
}

IOReturn CMI8738AudioDevice::gainChangeHandler(IOService *target, IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    CMI8738AudioDevice *audioDevice;
    
    audioDevice = (CMI8738AudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->gainChanged(gainControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn CMI8738AudioDevice::gainChanged(IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    DBGPRINT("CMI8738AudioDevice[%p]::gainChanged(%p, %ld, %ld)\n", this, gainControl, oldValue, newValue);
    
    if (gainControl) {
        DBGPRINT("\t-> Channel %ld\n", gainControl->getChannelID());
    }
    
    // Add hardware gain change code here 
#warning TODO gainChanged()   
    return kIOReturnSuccess;
}
    
IOReturn CMI8738AudioDevice::inputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    CMI8738AudioDevice *audioDevice;
    
    audioDevice = (CMI8738AudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->inputMuteChanged(muteControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn CMI8738AudioDevice::inputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    DBGPRINT("CMI8738AudioDevice[%p]::inputMuteChanged(%p, %ld, %ld)\n", this, muteControl, oldValue, newValue);
    
    // Add input mute change code here
#warning TODO inputMuteChanged()    
    return kIOReturnSuccess;
}


UInt8 CMI8738AudioDevice::readUInt8(UInt16 reg)
{
	return cm.pciDevice->ioRead8(reg, cm.deviceMap);
}

void CMI8738AudioDevice::writeUInt8(UInt16 reg, UInt8 value)
{
	cm.pciDevice->ioWrite8(reg, value, cm.deviceMap);
}

void CMI8738AudioDevice::setUInt8Bit(UInt16 reg, UInt8 bit)
{
	cm.pciDevice->ioWrite8(reg, cm.pciDevice->ioRead8(reg, cm.deviceMap) | bit, cm.deviceMap);
}

void CMI8738AudioDevice::clearUInt8Bit(UInt16 reg, UInt8 bit)
{
	cm.pciDevice->ioWrite8(reg, cm.pciDevice->ioRead8(reg, cm.deviceMap) & ~bit, cm.deviceMap);
}

UInt32 CMI8738AudioDevice::readUInt16(UInt16 reg)
{
	return cm.pciDevice->ioRead16(reg, cm.deviceMap);
}

void CMI8738AudioDevice::writeUInt16(UInt16 reg, UInt16 value)
{
	cm.pciDevice->ioWrite16(reg, value, cm.deviceMap);
}

UInt32 CMI8738AudioDevice::readUInt32(UInt16 reg)
{
	return cm.pciDevice->ioRead32(reg, cm.deviceMap);
}

void CMI8738AudioDevice::writeUInt32(UInt16 reg, UInt32 value)
{
	cm.pciDevice->ioWrite32(reg, value, cm.deviceMap);
}

void CMI8738AudioDevice::setUInt32Bit(UInt16 reg, UInt32 bit)
{
	cm.pciDevice->ioWrite32(reg, cm.pciDevice->ioRead32(reg, cm.deviceMap) | bit, cm.deviceMap);
}

void CMI8738AudioDevice::clearUInt32Bit(UInt16 reg, UInt32 bit)
{
	cm.pciDevice->ioWrite32(reg, cm.pciDevice->ioRead32(reg, cm.deviceMap) & ~bit, cm.deviceMap);
}

void CMI8738AudioDevice::writeMixer(UInt8 reg, UInt8 value)
{
	cm.pciDevice->ioWrite8(CM_REG_SB16_ADDR, reg, cm.deviceMap);
	cm.pciDevice->ioWrite8(CM_REG_SB16_DATA, value, cm.deviceMap);
}

UInt8 CMI8738AudioDevice::readMixer(UInt8 reg)
{
	cm.pciDevice->ioWrite8(CM_REG_SB16_ADDR, reg, cm.deviceMap);
	return cm.pciDevice->ioRead8(CM_REG_SB16_DATA, cm.deviceMap);
}

void CMI8738AudioDevice::set_pll(UInt32 rate, unsigned int slot){
	unsigned int reg = CM_REG_PLL + slot;
	/*
	 * Guess that this programs at reg. 0x04 the pos 15:13/12:10
	 * for DSFC/ASFC (000 up to 111).
	 */
	
	/* FIXME: Init (Do we've to set an other register first before programming?) */
	
	/* FIXME: Is this correct? Or shouldn't the m/n/r values be used for that? */
	writeUInt8(reg, rate>>8);
	writeUInt8(reg, rate&0xff);
	
	/* FIXME: Setup (Do we've to set an other register first to enable this?) */
}

