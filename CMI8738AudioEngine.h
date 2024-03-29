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


#ifndef _CMI8738AudioEngine_H
#define _CMI8738AudioEngine_H

#include <IOKit/audio/IOAudioEngine.h>

#include "ArchDetect.h"
#include "CMI8738AudioDevice.h"
#include "Memhandle.h"

#define CMI8738AudioEngine com_CMI8738AudioEngine

class IOFilterInterruptEventSource;
class IOInterruptEventSource;

static const int MAX_STRING = 128;

class CMI8738AudioEngine : public IOAudioEngine
{
    OSDeclareDefaultStructors(CMI8738AudioEngine)
    
public:

    virtual bool	init(CMI8738Info* _cm);
    virtual void	free();
    
    virtual bool	initHardware(IOService *provider);
    virtual void	stop(IOService *provider);
	bool	setDACChannels(int channels, int format);
	    
	UInt8	readUInt8(UInt16 reg);
	void	writeUInt8(UInt16 reg, UInt8 value);
	UInt32	readUInt16(UInt16 reg);
	void	writeUInt16(UInt16 reg, UInt16 value);
	UInt32	readUInt32(UInt16 reg);
	void	writeUInt32(UInt16 reg, UInt32 value);
	void	clearUInt32Bit(UInt16 reg, UInt32 bit);
	void	setUInt32Bit(UInt16 reg, UInt32 bit);
    
    void    setDMASettings(const UInt32 bufferTag);
    
    virtual OSString* getGlobalUniqueID();
	
	virtual void	dumpRegisters();

	void	setupSPDIFPlayback(bool enableSPDIF, bool enableAC3);
	void	setupSPDIFCapture(bool enableSPDIF);
	void	setupAC3Passthru(bool enableAC3Passthru);
    
    void    getChipDisaplyName(char* str, const size_t lenght);

	virtual IOAudioStream *createNewAudioStream(IOAudioStreamDirection direction, void *sampleBuffer, UInt32 sampleBufferSize, UInt32 channel);

    virtual IOReturn performAudioEngineStart();
    virtual IOReturn performAudioEngineStop();
    
    virtual UInt32 getCurrentSampleFrame();
    
    virtual IOReturn performFormatChange(IOAudioStream *audioStream, const IOAudioStreamFormat *newFormat, const IOAudioSampleRate *newSampleRate);

    virtual IOReturn clipOutputSamples(const void *mixBuf, void *sampleBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream);
    virtual IOReturn convertInputSamples(const void *sampleBuf, void *destBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream);
    
    virtual void filterInterrupt(int index);
    
#if defined(ARM)
    virtual bool driverDesiresHiResSampleIntervals(void);
#endif

private:
	CMI8738Info						*cm;
	UInt32                          modelID;
	UInt32                          currentSampleRate, currentResolution;
    UInt32                          shift_ch0, dma_size_ch0, period_size_ch0, shift_ch1, dma_size_ch1, period_size_ch1;
	
    /*
    SInt16							*outputBuffer;
    SInt16							*inputBuffer;
	IOPhysicalAddress               physicalAddressOutput;
	IOPhysicalAddress               physicalAddressInput;
	*/
    
    Memhandle inBuffer;
    Memhandle outBuffer;
    
    IOFilterInterruptEventSource	*interruptEventSource;
    
    static void interruptHandler(OSObject *owner, IOInterruptEventSource *source, int count);
    static bool interruptFilter(OSObject *owner, IOFilterInterruptEventSource *source);
	
};

#endif /* _CMI8738AudioEngine_H */
