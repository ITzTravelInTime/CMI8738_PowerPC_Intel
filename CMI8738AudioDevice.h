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

#include "CMI8738-register.h"

#ifndef _CMI8738AudioDevice_H
#define _CMI8738AudioDevice_H

#include <IOKit/audio/IOAudioDevice.h>

#ifdef DEBUG
	#define DBGPRINT(msg,...)    IOLog(msg, ##__VA_ARGS__)
#else
	#define DBGPRINT(msg,...)
#endif

class IOPCIDevice;
class IOMemoryMap;

#define CMI8738AudioDevice com_homebrew_driver_CMI8738Audio

typedef struct CMI8738Info {
	int			chipVersion;
	UInt32			maxChannels;
	UInt32		regFUNCTRL0;
    bool        supports24Bit;
	bool		canAC3HW, canAC3SW;
	bool		canMultiChannel;
    bool        can96k;
	bool		doAC3SW;
	bool		hasDualDAC;
	IOPCIDevice *pciDevice;
	IOMemoryMap *deviceMap;
} CMI8738Info;

class CMI8738AudioDevice : public IOAudioDevice
{
    friend class CMI8738AudioEngine;
    
public:
    OSDeclareDefaultStructors(CMI8738AudioDevice)
    
    virtual bool	initHardware(IOService *provider);
    virtual bool	createAudioEngine();
    virtual void	free();
    
	void	queryChip();
    void	resetChannel(int ch);
	
	UInt8	readUInt8(UInt16 reg);
	void	writeUInt8(UInt16 reg, UInt8 value);
	void	clearUInt8Bit(UInt16 reg, UInt8 bit);
	void	setUInt8Bit(UInt16 reg, UInt8 bit);

	UInt32	readUInt16(UInt16 reg);
	void	writeUInt16(UInt16 reg, UInt16 value);

	UInt32	readUInt32(UInt16 reg);
	void	writeUInt32(UInt16 reg, UInt32 value);
	void	clearUInt32Bit(UInt16 reg, UInt32 bit);
	void	setUInt32Bit(UInt16 reg, UInt32 bit);
	
	UInt8	readMixer(UInt8 reg);
	void	writeMixer(UInt8 reg, UInt8 value);	
	
	void    set_pll(UInt32 rate, unsigned int slot);
	
    virtual IOReturn volumeChanged(IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue);
    
    virtual IOReturn outputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);

    virtual IOReturn gainChanged(IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue);
    
    virtual IOReturn inputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);
    
private:
    static IOReturn volumeChangeHandler(IOService *target, IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue);
    static IOReturn outputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);
    static IOReturn gainChangeHandler(IOService *target, IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue);
    static IOReturn inputMuteChangeHandler(IOService *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue);
    
    CMI8738Info cm;
};

#endif /* _CMI8738AudioDevice_H */
