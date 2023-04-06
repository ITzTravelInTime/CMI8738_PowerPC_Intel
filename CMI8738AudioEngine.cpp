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


#include "CMI8738AudioEngine.h"
#include <IOKit/IOLib.h>

#include <IOKit/IOFilterInterruptEventSource.h>

#include <IOKit/pci/IOPCIDevice.h>

#include <libkern/version.h>

#define INITIAL_SAMPLE_RATE	44100
#define NUM_CHANNELS        2
#define NUM_CHANNELS_MAX    (cm->maxChannels)
#define NUM_SAMPLE_FRAMES	0x4000 //16384
#define BIT_DEPTH			16
#define BIT_DEPTH_MAX       (cm->supports24Bit ? 32 : 16)

#define BUFFER_SIZE_CALC(__channels, __bithDepth) (NUM_SAMPLE_FRAMES * (__channels) * (__bithDepth) / 8)

#define BUFFER_SIZE_MAX     BUFFER_SIZE_CALC(NUM_CHANNELS_MAX, BIT_DEPTH_MAX)
#define BUFFER_SIZE			BUFFER_SIZE_CALC(NUM_CHANNELS, BIT_DEPTH)//(NUM_SAMPLE_FRAMES * NUM_CHANNELS * BIT_DEPTH / 8)

#define ARRAYSZ(ar) ((sizeof(ar))/(sizeof(*ar)))

#define super IOAudioEngine

OSDefineMetaClassAndStructors(CMI8738AudioEngine, IOAudioEngine)

bool CMI8738AudioEngine::init(CMI8738Info* _cm)
{
    bool result = false;
    
    DBGPRINT("CMI8738AudioEngine[%p]::init(%p)\n", this, _cm);

    if (!super::init(NULL)) {
        goto Done;
    }

	if (!_cm) {
		goto Done;
	}	
	cm = _cm;
	
	currentSampleRate = 0;
	currentResolution = 0;
	
	shift_ch0 = 0;
	shift_ch1 = 0;
	dma_size_ch0 = NUM_SAMPLE_FRAMES;
	dma_size_ch1 = NUM_SAMPLE_FRAMES;
	period_size_ch0 = NUM_SAMPLE_FRAMES;
	period_size_ch1 = NUM_SAMPLE_FRAMES;
    
    bzero(&outBuffer, sizeof(Memhandle));
    bzero(&inBuffer, sizeof(Memhandle));
	
    result = true;
    
Done:

    return result;
}

bool CMI8738AudioEngine::initHardware(IOService *provider)
{
    bool result = false;
    IOAudioSampleRate initialSampleRate;
    IOAudioStream *audioStream;
    IOWorkLoop *workLoop;
	char description[128];
    
    DBGPRINT("CMI8738AudioEngine[%p]::initHardware(%p)\n", this, provider);
    
    if (!super::initHardware(provider)) {
        goto Done;
    }
	
    // Setup the initial sample rate for the audio engine
    initialSampleRate.whole = INITIAL_SAMPLE_RATE;
    initialSampleRate.fraction = 0;
    
    bzero(description, 128);
    
    //const char* className = (myMetaClass) ? myMetaClass->getClassName() : NULL;
    getChipDisaplyName(description, 128);
	
    setDescription(description);
    
    setSampleRate(&initialSampleRate);
    
    // Set the number of sample frames in each buffer
    setNumSampleFramesPerBuffer(NUM_SAMPLE_FRAMES);
    
    workLoop = getWorkLoop();
    if (!workLoop) {
        goto Done;
    }
    
    // Create an interrupt event source through which to receive interrupt callbacks
    // In this case, we only want to do work at primary interrupt time, so
    // we create an IOFilterInterruptEventSource which makes a filtering call
    // from the primary interrupt interrupt who's purpose is to determine if
    // our secondary interrupt handler is to be called.  In our case, we
    // can do the work in the filter routine and then return false to
    // indicate that we do not want our secondary handler called
    interruptEventSource = IOFilterInterruptEventSource::filterInterruptEventSource(this, 
                                    CMI8738AudioEngine::interruptHandler, 
                                    CMI8738AudioEngine::interruptFilter,
                                    audioDevice->getProvider());
	
    if (!interruptEventSource) {
        goto Done;
    }
    
	// the interruptEventSource needs to be enabled here, else IRQ sharing doesn't work
	// this 'bug' was found with the help of hydrasworld / hydra
	interruptEventSource->enable();
	    
    // In order to allow the interrupts to be received, the interrupt event source must be
    // added to the IOWorkLoop
    // Additionally, interrupts will not be firing until the interrupt event source is 
    // enabled by calling interruptEventSource->enable() - this probably doesn't need to
    // be done until performAudioEngineStart() is called, and can probably be disabled
    // when performAudioEngineStop() is called and the audio engine is no longer running
    // Although this really depends on the specific hardware 
    workLoop->addEventSource(interruptEventSource);
    
    outBuffer.size = BUFFER_SIZE_MAX;
    inBuffer.size = BUFFER_SIZE;

    if (pci_alloc(&outBuffer)){
        goto Done;
    }
    
    if (pci_alloc(&inBuffer)){
        goto Done;
    }
    
    // Allocate our input and output buffers - a real driver will likely need to allocate its buffers
    // differently
    /*
    outputBuffer = (SInt16 *)IOMallocContiguous(BUFFER_SIZE, 4, &physicalAddressOutput);
    if (!outputBuffer) {
        goto Done;
    }
	
    inputBuffer = (SInt16 *)IOMallocContiguous(BUFFER_SIZE, 4, &physicalAddressInput);
    if (!inputBuffer) {
        goto Done;
    }
     */
    
    // Create an IOAudioStream for each buffer and add it to this audio engine
    //audioStream = createNewAudioStream(kIOAudioStreamDirectionOutput, outputBuffer, BUFFER_SIZE, 0);
    audioStream = createNewAudioStream(kIOAudioStreamDirectionOutput, outBuffer.addr, outBuffer.size, CM_CH_PLAY);
    if (!audioStream) {
        goto Done;
    }
    
    addAudioStream(audioStream);
    audioStream->release();
	
    /*
    audioStream = createNewAudioStream(kIOAudioStreamDirectionInput, inBuffer.addr, inBuffer.size, CM_CH_CAPT);
    if (!audioStream) {
        goto Done;
    }
    
    addAudioStream(audioStream);
    audioStream->release();
	*/
    
	//writeUInt32(CM_REG_CH0_FRAME1, (UInt32)(physicalAddressOutput));
	
	//writeUInt32(CM_REG_CH1_FRAME1, (UInt32)(physicalAddressInput));
	
    writeUInt32(CM_REG_CH0_FRAME1, (UInt32)outBuffer.dma_handle);
    
    writeUInt32(CM_REG_CH1_FRAME1, (UInt32)inBuffer.dma_handle);
    
    result = true;
    
Done:

    return result;
}

void CMI8738AudioEngine::getChipDisaplyName(char* str, const size_t lenght){
    UInt32 modelID = 0x8738;
    
    if (cm->chipVersion < 68){
        const UInt32 deviceID = cm->pciDevice->configRead32(kIOPCIConfigDeviceID) >> 16;
        switch (deviceID) {
            case PCI_DEVICE_ID_CMEDIA_CM8338A:
                modelID = 0x8338A;
                break;
            case PCI_DEVICE_ID_CMEDIA_CM8338B:
                modelID = 0x8338B;
                break;
            case PCI_DEVICE_ID_CMEDIA_CM8738B:
                modelID = 0x8738B;
                break;
            default:
                modelID = 0x8738;
                break;
        }
    }else{
        const UInt8 discriminator = readUInt8(CM_REG_INT_HLDCLR + 3) & 0x03;
        
        switch (discriminator) {
            case 0:
                modelID = 0x8769;
                break;
            case 2:
                modelID = 0x8762;
            default:
                const UInt32 subsystemID = (cm->pciDevice->configRead32(kIOPCIConfigSubSystemID) >> 16) | (cm->pciDevice->configRead32(kIOPCIConfigSubSystemVendorID) << 16);
                
                switch (subsystemID) {
                    case 0x13f69761:
                    case 0x584d3741:
                    case 0x584d3751:
                    case 0x584d3761:
                    case 0x584d3771:
                    case 0x72848384:
                        modelID = 8770;
                        break;
                    default:
                        modelID = 8768;
                        break;
                }
                break;
        }
    }
    
    
    
#if VERSION_MAJOR >= 10
    snprintf(str, lenght, "CMI%x", modelID);
    if (cm->chipVersion < 68){
        snprintf(str + strlen(str), lenght, "-V%d", cm->chipVersion);
    }
    
    if (cm->canMultiChannel) {
        snprintf(str + strlen(str), lenght, "-MC%d", cm->maxChannels);
    }
    
    if (cm->canAC3SW) {
        snprintf(str + strlen(str), lenght, "-SW_AC3");
    }
#else
    sprintf(str, "CMI%x", modelID);
    if (cm->chipVersion < 68){
        sprintf(str + strlen(str), "-V%d", cm->chipVersion);
    }
    
    if (cm->canMultiChannel) {
        sprintf(str + strlen(str), "-MC%d", cm->maxChannels);
    }
    
    if (cm->canAC3SW) {
        sprintf(str + strlen(str), "-SW_AC3");
    }
#endif
}

OSString* CMI8738AudioEngine::getGlobalUniqueID(){

    //const OSMetaClass * const myMetaClass = getMetaClass();

    UInt8 bus,device,function;
    
    bus = cm->pciDevice->getBusNumber();
    device = cm->pciDevice->getDeviceNumber();
    function = cm->pciDevice->getFunctionNumber();

    const UInt16 port = cm->pciDevice->configRead16(kIOPCIConfigBaseAddress0) & 0xFFFE;

    char uniqueIDStr[MAX_STRING];
    char description[MAX_STRING];
    
    bzero(uniqueIDStr, MAX_STRING);
    bzero(description, MAX_STRING);

    //const char* className = (myMetaClass) ? myMetaClass->getClassName() : NULL;
    
    getChipDisaplyName(description, MAX_STRING);
    
    #if VERSION_MAJOR >= 10
    snprintf(uniqueIDStr, MAX_STRING, /*"%s:*/ "%s:%x:%x:%x:%x:%x", /*className,*/ description, bus, device, function, port, (const UInt32)this->index);
    #else
    sprintf(uniqueIDStr, /*"%s:*/ "%s:%x:%x:%x:%x:%x", /*className,*/ description, bus, device, function, port, (const UInt32)this->index);
    #endif
    //OSString* value = super::getGlobalUniqueID();

    OSString* value = OSString::withCString (uniqueIDStr);

    DBGPRINT("CMI8738AudioEngine[%p]::getGlobalUniqueID() -- Returned value: %s\n", this, uniqueIDStr);

    return value;
}

void CMI8738AudioEngine::free()
{
    DBGPRINT("CMI8738AudioEngine[%p]::free()\n", this);
    
    // We need to free our resources when we're going away
    
    if (interruptEventSource) {
	    interruptEventSource->disable();
        interruptEventSource->release();
        interruptEventSource = NULL;
    }
    
    /*
    if (outputBuffer) {
        IOFree(outputBuffer, BUFFER_SIZE);
        outputBuffer = NULL;
    }
    
    if (inputBuffer) {
		IOFree(inputBuffer, BUFFER_SIZE);
        inputBuffer = NULL;
    }*/
    
    pci_free(&outBuffer);
    pci_free(&inBuffer);
    
    super::free();
}

IOAudioStream *CMI8738AudioEngine::createNewAudioStream(IOAudioStreamDirection direction, void *sampleBuffer, UInt32 sampleBufferSize, UInt32 channel)
{
    IOAudioStream *audioStream;
    
    DBGPRINT("CMI8738AudioEngine[%p]::createNewAudioStream()\n", this);
    
    // For this sample device, we are only creating a single format and allowing 44.1KHz and 48KHz
    audioStream = new IOAudioStream;
    if (!audioStream) {
        return audioStream;
    }
    
    if (!audioStream->initWithAudioEngine(this, direction, 1)) {
        audioStream->release();
        return audioStream;
    }
    
    IOAudioSampleRate rate;
    IOAudioStreamFormat format = {
        NUM_CHANNELS,							    	// num channels, was hardcoded to 2
        kIOAudioStreamSampleFormatLinearPCM,			// sample format
        kIOAudioStreamNumericRepresentationSignedInt,	// numeric format
        BIT_DEPTH,										// bit depth
        BIT_DEPTH,										// bit width
        kIOAudioStreamAlignmentHighByte,				// high byte aligned - unused because bit depth == bit width
        kIOAudioStreamByteOrderLittleEndian,			// little endian
        true,											// format is mixable
        channel											// num of channel
    };
    
    // As part of creating a new IOAudioStream, its sample buffer needs to be set
    // It will automatically create a mix buffer should it be needed
    audioStream->setSampleBuffer(sampleBuffer, sampleBufferSize);
    
    const UInt32 rates[] = {8000, 16000, 32000, 48000, 5512, 11025, 22050, 44100};
    
    const UInt32 maxExtended = (cm->chipVersion == 55) ? 128000 : 96000;
    
    const UInt32 extendedRates[] = {8000, 16000, 32000, 48000, /*5512,*/ 11025, 22050, 44100, 88200, 96000, maxExtended};
    
    const UInt32* selectedRates = cm->can96k ? extendedRates : rates;
    const size_t ratesCount = cm->can96k ? ARRAYSZ(extendedRates) : ARRAYSZ(rates);
    
    rate.fraction = 0;
    
    //for(UInt32 chn = NUM_CHANNELS; chn <= (direction == kIOAudioStreamDirectionOutput ? NUM_CHANNELS_MAX : NUM_CHANNELS); chn += 2){
    //    format.fNumChannels = chn;
        for(size_t i = 0; i < ratesCount; i++){
            rate.whole = selectedRates[i];
            
            //avoid duplicate sample rates
            if (/*chn == NUM_CHANNELS &&*/ rate.whole == INITIAL_SAMPLE_RATE)
                continue;
            
            audioStream->addAvailableFormat(&format, &rate, &rate);
            DBGPRINT("\tNew samplig rate: \"%u\" hz @ \"%u\" bits @ \"%u\" chns\n", (unsigned int)rate.whole, (unsigned int)format.fBitDepth, (unsigned int)format.fNumChannels);
        }
    //}
    
    //setup default sample rate
    rate.whole = INITIAL_SAMPLE_RATE;
    currentSampleRate = INITIAL_SAMPLE_RATE;
    format.fNumChannels = NUM_CHANNELS;
    
    //sanity in case we decide to support variable bit depth
    format.fBitDepth = BIT_DEPTH;
    format.fBitWidth = BIT_DEPTH;
    
    audioStream->addAvailableFormat(&format, &rate, &rate);
    DBGPRINT("\tNew default samplig rate: \"%u\" hz @ \"%u\" bits @ \"%u\" chns\n", (unsigned int)rate.whole, (unsigned int)format.fBitDepth, (unsigned int)format.fNumChannels);
    
    // Finally, the IOAudioStream's current format needs to be indicated
    audioStream->setFormat(&format);
    
    return audioStream;
}

void CMI8738AudioEngine::stop(IOService *provider)
{
    DBGPRINT("CMI8738AudioEngine[%p]::stop(%p)\n", this, provider);
    
    // When our device is being stopped and torn down, we should go ahead and remove
    // the interrupt event source from the IOWorkLoop
    // Additionally, we'll go ahead and release the interrupt event source since it isn't
    // needed any more
    if (interruptEventSource) {
        IOWorkLoop *wl;
        
        wl = getWorkLoop();
        if (wl) {
            wl->removeEventSource(interruptEventSource);
        }
        
        interruptEventSource->release();
        interruptEventSource = NULL;
    }
    
    // Add code to shut down hardware (beyond what is needed to simply stop the audio engine)
    // There may be nothing needed here

    super::stop(provider);
}

void CMI8738AudioEngine::setDMASettings(UInt32 bufferTag){
    //values to program the sample counts acording to the new format
    if (bufferTag == CM_CH_PLAY){
        writeUInt16(CM_REG_CH0_FRAME2, dma_size_ch0-1);
        writeUInt16(CM_REG_CH0_FRAME2+2, period_size_ch0-1);
    }else if (bufferTag == CM_CH_CAPT){
        writeUInt16(CM_REG_CH1_FRAME2, dma_size_ch1-1);
        writeUInt16(CM_REG_CH1_FRAME2+2, period_size_ch1-1);
    }
}
    
IOReturn CMI8738AudioEngine::performAudioEngineStart()
{
    DBGPRINT("CMI8738AudioEngine[%p]::performAudioEngineStart()\n", this);
	
	//program the sample counts acording to the new format
	
    setDMASettings(CM_CH_PLAY);
    setDMASettings(CM_CH_CAPT);
	
	dumpRegisters();
	
    // When performAudioEngineStart() gets called, the audio engine should be started from the beginning
    // of the sample buffer.  Because it is starting on the first sample, a new timestamp is needed
    // to indicate when that sample is being read from/written to.  The function takeTimeStamp() 
    // is provided to do that automatically with the current time.
    // By default takeTimeStamp() will increment the current loop count in addition to taking the current
    // timestamp.  Since we are starting a new audio engine run, and not looping, we don't want the loop count
    // to be incremented.  To accomplish that, false is passed to takeTimeStamp(). 
    takeTimeStamp(false);
    
    // Add audio - I/O start code here
    
	setUInt32Bit(CM_REG_INT_HLDCLR, CM_CH0_INT_EN | CM_CH1_INT_EN );
	cm->regFUNCTRL0 |= (CM_CHEN0 | CM_CHEN1);
	writeUInt32(CM_REG_FUNCTRL0, cm->regFUNCTRL0);
    
    return kIOReturnSuccess;
}

IOReturn CMI8738AudioEngine::performAudioEngineStop()
{
    DBGPRINT("CMI8738AudioEngine[%p]::performAudioEngineStop()\n", this);

    // Add audio - I/O stop code here
    
	clearUInt32Bit(CM_REG_INT_HLDCLR, CM_CH0_INT_EN | CM_CH1_INT_EN);
    
	cm->regFUNCTRL0 &= ~(CM_CHEN0 | CM_CHEN1);
    
    UInt32 reset = CM_RST_CH0 | CM_RST_CH0 << 1;
    
	writeUInt32(CM_REG_FUNCTRL0, cm->regFUNCTRL0 | reset);
	writeUInt32(CM_REG_FUNCTRL0, cm->regFUNCTRL0 & ~reset);
    
    return kIOReturnSuccess;
}
    
UInt32 CMI8738AudioEngine::getCurrentSampleFrame()
{
    
    // In order for the erase process to run properly, this function must return the current location of
    // the audio engine - basically a sample counter
    // It doesn't need to be exact, but if it is inexact, it should err towards being before the current location
    // rather than after the current location.  The erase head will erase up to, but not including the sample
    // frame returned by this function.  If it is too large a value, sound data that hasn't been played will be 
    // erased.

    // Change to return the real value
    return (dma_size_ch0 - (readUInt16(CM_REG_CH0_FRAME2) + 1)) >> shift_ch0;
//	return NUM_SAMPLE_FRAMES;
}
    
IOReturn CMI8738AudioEngine::performFormatChange(IOAudioStream *audioStream, const IOAudioStreamFormat *newFormat, const IOAudioSampleRate *newSampleRate)
{
    DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange(%p, %p, %p)\n", this, audioStream, newFormat, newSampleRate);
    
	UInt32 val, fmt = 0, freq = 0, freq_ext = 0, dma_size_reg = NUM_SAMPLE_FRAMES, period_size_reg = NUM_SAMPLE_FRAMES;
	UInt16 currentChShift = 0;
	bool is_dac = audioStream->direction == kIOAudioStreamDirectionOutput;
	bool EnableSPDIF = false;
	
	const IOAudioStreamFormat* format = newFormat ? newFormat : audioStream->getFormat();
	
	if (!format) {
        DBGPRINT(("/t Internal Error - can't determinate the format!.\n"));
		return kIOReturnInternalError;
	}
	
	DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange -- calculating sample rate values\n", this);
    if (newSampleRate) {
		currentSampleRate = newSampleRate->whole;
	} else { 
		if (currentSampleRate == 0) {
			currentSampleRate = INITIAL_SAMPLE_RATE;
		}
	}

    if (currentSampleRate > 48000){
        switch (currentSampleRate) {
            case 88200:  freq_ext = CM_CH0_SRATE_88K; break;
            case 96000:  freq_ext = CM_CH0_SRATE_96K; break;
            case 128000: freq_ext = CM_CH0_SRATE_128K; break;
            default:
                DBGPRINT(("/t Internal Error - unknown extended sample rate selected.\n"));
                return kIOReturnInternalError;
        }
    }else{
        switch (currentSampleRate) {
            case 5512:
                freq = 0;
                break;
            case 11025:
                freq = 1;
                break;
            case 22050:
                freq = 2;
                break;
            case 44100:
                freq = 3;
                break;
            case 8000:
                freq = 4;
                break;
            case 16000:
                freq = 5;
                break;
            case 32000:
                freq = 6;
                break;
            case 48000:
                freq = 7;
                break;
            default:
                DBGPRINT(("/t Internal Error - unknown sample rate selected.\n"));
                return kIOReturnInternalError;
        }
    }
	
		DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange -- setting format buffers values\n", this);
		
		currentResolution = format->fBitDepth;
		currentChShift = 0;
		fmt = 0;
		
        //should be supported just by some later CMI chips, not the case of the CMI8738/CMI8338
		if (currentResolution >= 16) {
			fmt |= 0x02;
			if (currentResolution > 16)
				currentChShift++;
		}
		
		if (format->fNumChannels > 1) {
			fmt |= 0x01;
		}
		
        if (is_dac){
            if (!setDACChannels(format->fNumChannels, fmt)) {
                DBGPRINT(("/t Internal Error - setDACChannels failed, invalid channels/bit width config!\n"));
                return kIOReturnInternalError;
            }
        }
    
        //for variable channel numbers we also need to change the dma buffer size, so the OS can manage it properly.
        if (format){
			const UInt32 bufferSize = BUFFER_SIZE_CALC( format->fNumChannels, format->fBitDepth );
			audioStream->setSampleBuffer( is_dac ? outBuffer.addr : inBuffer.addr, bufferSize );
        }
		
		dma_size_reg = NUM_SAMPLE_FRAMES << currentChShift;
		period_size_reg = NUM_SAMPLE_FRAMES << currentChShift;
		
		if (format->fNumChannels > 2){
			dma_size_reg = (dma_size_reg * format->fNumChannels) / 2;
			period_size_reg = (period_size_reg * format->fNumChannels) / 2;
		}
		
		//values to program the sample counts acording to the new format
		if (format->fDriverTag == CM_CH_PLAY){
			dma_size_ch0 = dma_size_reg;
			period_size_ch0 = period_size_reg;
			shift_ch0 = currentChShift;
		}else if (format->fDriverTag == CM_CH_CAPT){
			dma_size_ch1 = dma_size_reg;
			period_size_ch1 = period_size_reg;
			shift_ch1 = currentChShift;
		}
    
        //setup the playback counter registers
        setDMASettings(format->fDriverTag);
		
		DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange -- setting basic format part 0\n", this);
		val = readUInt32(CM_REG_CHFORMAT);
		if (format->fDriverTag) {
			val &= ~CM_CH1FMT_MASK;
			val |= fmt << CM_CH1FMT_SHIFT;
		} else {
			val &= ~CM_CH0FMT_MASK;
			val |= fmt << CM_CH0FMT_SHIFT;
		}
		
		DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange -- setting extended format\n", this);
		/* set ext format */
		if (cm->can96k) {
			val &= ~(CM_CH0_SRATE_MASK << (format->fDriverTag * 2));
			val |= freq_ext << (format->fDriverTag * 2);
		}
		writeUInt32(CM_REG_CHFORMAT, val);
	
	DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange -- setting dac/adc flag\n", this);
	//set dac/adc flag
	val = format->fDriverTag ? CM_CHADC1 : CM_CHADC0;
	
	if (is_dac)
		cm->regFUNCTRL0 &= ~val;
	else
		cm->regFUNCTRL0 |= val;
	
	writeUInt32(CM_REG_FUNCTRL0, cm->regFUNCTRL0);
    
	DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange -- setting basic format part 2\n", this);
    //set basic format
	val = readUInt32(CM_REG_FUNCTRL1);
	if (format->fDriverTag) {
		val &= ~CM_ASFC_MASK;
		val |= (freq << CM_ASFC_SHIFT) & CM_ASFC_MASK;
	} else {
		val &= ~CM_DSFC_MASK;
		val |= (freq << CM_DSFC_SHIFT) & CM_DSFC_MASK;
	}            
	writeUInt32(CM_REG_FUNCTRL1, val);
	
	DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange -- setting specific adc flag\n", this);
    if (!is_dac && cm->chipVersion) {
		if (currentSampleRate > 44100)
			setUInt32Bit(CM_REG_EXT_MISC, CM_ADC48K44K);
		else
			clearUInt32Bit(CM_REG_EXT_MISC, CM_ADC48K44K);
	}	
	
    //SPDIF
	DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange -- setting spdf\n", this);
	EnableSPDIF = ( currentSampleRate >= 44100 && currentSampleRate <= 48000 /*96000*/ ) && 
				  (currentResolution == 16) && (format->fNumChannels == 2);
	
	setupSPDIFPlayback(EnableSPDIF, false);
        
	dumpRegisters();
	
	DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange -- success\n", this);
    return kIOReturnSuccess;
}

void CMI8738AudioEngine::setupSPDIFPlayback(bool enableSPDIF, bool enableAC3)
{
    DBGPRINT("CMI8738AudioEngine[%p]::setupSPDIFPlayback(%d, %d)\n", this, enableSPDIF, enableAC3);

	if (enableSPDIF) {
		setUInt32Bit(CM_REG_LEGACY_CTRL, CM_ENSPDOUT);
		setUInt32Bit(CM_REG_FUNCTRL1, CM_SPDO2DAC);
        setUInt32Bit(CM_REG_FUNCTRL1, CM_PLAYBACK_SPDF);
		setupAC3Passthru(enableAC3);

		if (currentSampleRate == 48000)
            setUInt32Bit(CM_REG_MISC_CTRL, CM_SPDIF48K | CM_SPDF_AC97);
		else
            clearUInt32Bit(CM_REG_MISC_CTRL, CM_SPDIF48K | CM_SPDF_AC97);

	} else {
        clearUInt32Bit(CM_REG_FUNCTRL1, CM_PLAYBACK_SPDF);
		setupAC3Passthru(false);
    }

 	return;
}

void CMI8738AudioEngine::setupAC3Passthru(bool enableAC3Passthru)
{
    DBGPRINT("CMI8738AudioEngine[%p]::setupAC3Passthru(%d)\n", this, enableAC3Passthru);

	if (enableAC3Passthru) {
		writeUInt8(CM_REG_MIXER1, readUInt8(CM_REG_MIXER1) | CM_WSMUTE);

		// AC3EN for 037
        setUInt32Bit(CM_REG_CHFORMAT, CM_AC3EN1);
		// AC3EN for 039
        setUInt32Bit(CM_REG_MISC_CTRL, CM_AC3EN2);

		if (cm->canAC3HW) {
			/* SPD24SEL for 037, 0x02 */
			/* SPD24SEL for 039, 0x20, but cannot be set */
            setUInt32Bit(CM_REG_CHFORMAT, CM_SPD24SEL);
            clearUInt32Bit(CM_REG_MISC_CTRL, CM_SPD32SEL);
            
            //disabled on linux, enabled here on OS X to also be able to use spdf
			if (cm->chipVersion >= 39)
                writeUInt8(CM_REG_MIXER1, readUInt8(CM_REG_MIXER1) & ~CM_CDPLAY);
		} else {
			/* SPD32SEL for 037 & 039, 0x20 */
            setUInt32Bit(CM_REG_MISC_CTRL, CM_SPD32SEL);
			/* set 176K sample rate to fix 033 HW bug */
			if (cm->chipVersion == 33) {
				if (currentSampleRate >= 48000)
					 setUInt32Bit(CM_REG_CHFORMAT, CM_PLAYBACK_SRATE_176K);
				else
					 clearUInt32Bit(CM_REG_CHFORMAT, CM_PLAYBACK_SRATE_176K);
			}
		}
	} else {
		writeUInt8(CM_REG_MIXER1, readUInt8(CM_REG_MIXER1) & ~CM_WSMUTE);

		clearUInt32Bit(CM_REG_CHFORMAT, CM_AC3EN1);
		clearUInt32Bit(CM_REG_MISC_CTRL, CM_AC3EN2);

		if (cm->canAC3HW)
		{
			/* chip model >= 37 */
			if (currentResolution > 16) {
				setUInt32Bit(CM_REG_MISC_CTRL, CM_SPD32SEL);
				setUInt32Bit(CM_REG_CHFORMAT, CM_SPD24SEL);
			} else {
				clearUInt32Bit(CM_REG_MISC_CTRL, CM_SPD32SEL);
				clearUInt32Bit(CM_REG_CHFORMAT, CM_SPD24SEL);
			}
		} else {
			clearUInt32Bit(CM_REG_MISC_CTRL, CM_SPD32SEL);
			clearUInt32Bit(CM_REG_CHFORMAT, CM_SPD24SEL);
			clearUInt32Bit(CM_REG_CHFORMAT, CM_PLAYBACK_SRATE_176K);
		}
	}
	return;
}

bool CMI8738AudioEngine::setDACChannels(int channels, int format)
{
    DBGPRINT("CMI8738AudioEngine[%p]::setDACChannels(%d, %d)\n", this, channels, format);

    /*
	if (channels > 2) {
		if (!cm->canMultiChannel)
			return false;

		setUInt32Bit(CM_REG_LEGACY_CTRL, CM_NXCHG);
		setUInt32Bit(CM_REG_MISC_CTRL, CM_XCHGDAC);
		if (channels > 4) {
			clearUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D);
			setUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D5C);
		} else {
			clearUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D5C);
			setUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D);
		}
		if (channels == 6) {
			setUInt32Bit(CM_REG_LEGACY_CTRL, CM_CHB3D6C);
			setUInt32Bit(CM_REG_MISC_CTRL, CM_ENCENTER);
		} else {
			clearUInt32Bit(CM_REG_LEGACY_CTRL, CM_CHB3D6C);
			clearUInt32Bit(CM_REG_MISC_CTRL, CM_ENCENTER);
		}
	} else {
		if (cm->canMultiChannel) {
			clearUInt32Bit(CM_REG_LEGACY_CTRL, CM_NXCHG);
			clearUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D);
			clearUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D5C);
			clearUInt32Bit(CM_REG_LEGACY_CTRL, CM_CHB3D6C);
			clearUInt32Bit(CM_REG_MISC_CTRL, CM_ENCENTER);
			clearUInt32Bit(CM_REG_MISC_CTRL, CM_XCHGDAC);
		}
	}*/
    
    //sanity check
    if (channels > 2) {
        if (!cm->canMultiChannel)
            return false;
        if (format != 0x03) /* stereo 16bit only */
            return false;
    }
    
    if (!cm->canMultiChannel) {
        return true;
    }
    
    //spin_lock_irq(&cm->reg_lock);
    if (channels > 2) {
        setUInt32Bit(CM_REG_LEGACY_CTRL, CM_NXCHG);
        setUInt32Bit(CM_REG_MISC_CTRL, CM_XCHGDAC);
    } else {
        clearUInt32Bit(CM_REG_LEGACY_CTRL, CM_NXCHG);
        clearUInt32Bit(CM_REG_MISC_CTRL, CM_XCHGDAC);
    }
    if (channels == 8)
        setUInt32Bit(CM_REG_EXT_MISC, CM_CHB3D8C);
    else
        clearUInt32Bit(CM_REG_EXT_MISC, CM_CHB3D8C);
    if (channels == 6) {
        setUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D5C);
        setUInt32Bit(CM_REG_LEGACY_CTRL, CM_CHB3D6C);
    } else {
        clearUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D5C);
        clearUInt32Bit(CM_REG_LEGACY_CTRL, CM_CHB3D6C);
    }
    if (channels == 4)
        setUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D);
    else
        clearUInt32Bit(CM_REG_CHFORMAT, CM_CHB3D);
    //spin_unlock_irq(&cm->reg_lock);
    
    return true;
}

void CMI8738AudioEngine::interruptHandler(OSObject *owner, IOInterruptEventSource *source, int count)
{
    // Since our interrupt filter always returns false, this function will never be called
    // If the filter returned true, this function would be called on the IOWorkLoop
    return;
}

bool CMI8738AudioEngine::interruptFilter(OSObject *owner, IOFilterInterruptEventSource *source)
{
    CMI8738AudioEngine *audioEngine = OSDynamicCast(CMI8738AudioEngine, owner);
    
    // We've cast the audio engine from the owner which we passed in when we created the interrupt
    // event source
    if (audioEngine) {
        // Then, filterInterrupt() is called on the specified audio engine
        audioEngine->filterInterrupt(source->getIntIndex());
    }
    
    return false;
}

void CMI8738AudioEngine::filterInterrupt(int index)
{
    // In the case of our simple device, we only get interrupts when the audio engine loops to the
    // beginning of the buffer.  When that happens, we need to take a timestamp and increment
    // the loop count.  The function takeTimeStamp() does both of those for us.  Additionally,
    // if a different timestamp is to be used (other than the current time), it can be passed
    // in to takeTimeStamp()
	UInt32 status, mask = 0;
	
	status = readUInt32(CM_REG_INT_STATUS);
	if (!(status & CM_INTR)) {
		return;
	}
	
    /* acknowledge interrupt */
	if (status & CM_CHINT0) {
		mask |= CM_CH0_INT_EN;
		//DBGPRINT("CMI8738AudioEngine[%p]::filterInterrupt(%x) Playback interrupt acknowledgement\n", this, index);
	}
	if (status & CM_CHINT1) {
		mask |= CM_CH1_INT_EN;
		//DBGPRINT("CMI8738AudioEngine[%p]::filterInterrupt(%x) Recording interrupt acknowledgement\n", this, index);
	}
	clearUInt32Bit(CM_REG_INT_HLDCLR, mask);
	setUInt32Bit(CM_REG_INT_HLDCLR, mask);
	
	

	if (status & CM_CHINT0) {
	    takeTimeStamp();
	}
	
	return;
}


UInt8 CMI8738AudioEngine::readUInt8(UInt16 reg)
{
	return cm->pciDevice->ioRead8(reg, cm->deviceMap);
}

void CMI8738AudioEngine::writeUInt8(UInt16 reg, UInt8 value)
{
	cm->pciDevice->ioWrite8(reg, value, cm->deviceMap);
}

UInt32 CMI8738AudioEngine::readUInt16(UInt16 reg)
{
	return (cm->pciDevice->ioRead16(reg, cm->deviceMap));
}

void CMI8738AudioEngine::writeUInt16(UInt16 reg, UInt16 value)
{
	cm->pciDevice->ioWrite16(reg, (value), cm->deviceMap);
}

UInt32 CMI8738AudioEngine::readUInt32(UInt16 reg)
{
	return (cm->pciDevice->ioRead32(reg, cm->deviceMap));
}

void CMI8738AudioEngine::writeUInt32(UInt16 reg, UInt32 value)
{
	cm->pciDevice->ioWrite32(reg, (value), cm->deviceMap);
}

void CMI8738AudioEngine::setUInt32Bit(UInt16 reg, UInt32 bit)
{
	cm->pciDevice->ioWrite32(reg, cm->pciDevice->ioRead32(reg, cm->deviceMap) | bit, cm->deviceMap);
}

void CMI8738AudioEngine::clearUInt32Bit(UInt16 reg, UInt32 bit)
{
	cm->pciDevice->ioWrite32(reg, cm->pciDevice->ioRead32(reg, cm->deviceMap) & ~bit, cm->deviceMap);
}

void CMI8738AudioEngine::dumpRegisters()
{
	
    DBGPRINT("CMI8738AudioEngine[%p]::dumpRegisters()\n", this);
    DBGPRINT("  FUNCTRL0:    0x%lx\n", (UInt32)readUInt32(CM_REG_FUNCTRL0));
    DBGPRINT("  FUNCTRL1:    0x%lx\n", (UInt32)readUInt32(CM_REG_FUNCTRL1));
    DBGPRINT("  CHFORMAT:    0x%lx\n", (UInt32)readUInt32(CM_REG_CHFORMAT));
    DBGPRINT("  INT_HLDCLR:  0x%lx\n", (UInt32)readUInt32(CM_REG_INT_HLDCLR));
    DBGPRINT("  MISC_CTRL:   0x%lx\n", (UInt32)readUInt32(CM_REG_MISC_CTRL));
    DBGPRINT("  LEGACY_CTRL: 0x%lx\n", (UInt32)readUInt32(CM_REG_LEGACY_CTRL));
    DBGPRINT("  CH0_FRAME1:  0x%lx\n", (UInt32)readUInt32(CM_REG_CH0_FRAME1));
    DBGPRINT("  CH0_FRAME2:  0x%lx\n", (UInt32)readUInt32(CM_REG_CH0_FRAME2));
    DBGPRINT("  bit shift c0:%ld\n", (SInt32)shift_ch0);
	DBGPRINT("  bit shift c1:%ld\n", (SInt32)shift_ch1);
    DBGPRINT("  bit depth:   %ld\n", (SInt32)currentResolution);
    DBGPRINT("  sample rate: %ld\n", (SInt32)currentSampleRate);

}

#if defined(ARM)
bool CMI8738AudioEngine::driverDesiresHiResSampleIntervals(){
    return TRUE; //to be changed probably, idk
}
#endif
