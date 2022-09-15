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

#define INITIAL_SAMPLE_RATE	44100
#define NUM_SAMPLE_FRAMES	16384
#define NUM_CHANNELS		2
#define BIT_DEPTH			16

#define BUFFER_SIZE			(NUM_SAMPLE_FRAMES * NUM_CHANNELS * BIT_DEPTH / 8)

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
    
    bzero(&outBuffer, sizeof(struct memhandle));
    bzero(&inBuffer, sizeof(struct memhandle));
	
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
    
	sprintf(description, "CMI8738-v%d", cm->chipVersion);
	if (cm->canMultiChannel) {
		sprintf(description + strlen(description), "-MC%d", cm->maxChannels);
	}	
	
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
    
    outBuffer.size = BUFFER_SIZE;
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
    audioStream = createNewAudioStream(kIOAudioStreamDirectionOutput, outBuffer.addr, outBuffer.size, 0);
    if (!audioStream) {
        goto Done;
    }
    
    addAudioStream(audioStream);
    audioStream->release();
    
	/*
    audioStream = createNewAudioStream(kIOAudioStreamDirectionInput, inputBuffer, BUFFER_SIZE, 1);
    if (!audioStream) {
        goto Done;
    }
    
    addAudioStream(audioStream);
    audioStream->release();
    */
	
	//writeUInt32(CM_REG_CH0_FRAME1, (UInt32)(physicalAddressOutput));
	
	//writeUInt32(CM_REG_CH1_FRAME1, (UInt32)(physicalAddressInput));
	
    writeUInt32(CM_REG_CH0_FRAME1, outBuffer.dma_handle);
    
    writeUInt32(CM_REG_CH1_FRAME1, inBuffer.dma_handle);
    
    result = true;
    
Done:

    return result;
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
    if (audioStream) {
        if (!audioStream->initWithAudioEngine(this, direction, 1)) {
            audioStream->release();
        } else {
            IOAudioSampleRate rate;
            IOAudioStreamFormat format = {
                2,												// num channels
                kIOAudioStreamSampleFormatLinearPCM,			// sample format
                kIOAudioStreamNumericRepresentationSignedInt,	// numeric format
                BIT_DEPTH,										// bit depth
                BIT_DEPTH,										// bit width
                kIOAudioStreamAlignmentHighByte,				// high byte aligned - unused because bit depth == bit width
                kIOAudioStreamByteOrderBigEndian,				// big endian
                true,											// format is mixable
                channel											// number of channel
            };
            
            // As part of creating a new IOAudioStream, its sample buffer needs to be set
            // It will automatically create a mix buffer should it be needed
            audioStream->setSampleBuffer(sampleBuffer, sampleBufferSize);
            /*
			 
			 case 8000:
			 freq = 4;
			 break;
			 case 16000:
			 freq = 5;
			 break;
			 case 32000:
			 
			 case 5512:
			 freq = 0;
			 break;
			 case 11025:
			 freq = 1;
			 break;
			 case 22050:
			 
			 */
			 
			const UInt32 rates[] = {/*8000, 16000, 32000,*/ 48000, /*5512, 11025, 22050,*/ 44100};
			rate.fraction = 0;
			for(UInt8 w = 8; w <= BIT_DEPTH; w += 8){
				format.fBitDepth = w;
				for(size_t i = 0; i < ARRAYSZ(rates); i++){
					rate.whole = rates[i];
					audioStream->addAvailableFormat(&format, &rate, &rate);
					DBGPRINT("	New samplig rate: Bits \"%u\" Sample Rate \"%u\"\n", (unsigned int)format.fBitDepth, (unsigned int)rate.whole);
					
				}
			}
			
			currentSampleRate = INITIAL_SAMPLE_RATE;
			
			/*
            // This device only allows a single format and a choice of 2 different sample rates
            rate.fraction = 0;
            rate.whole = 44100;
			currentSampleRate = 44100;
			
            audioStream->addAvailableFormat(&format, &rate, &rate);
            rate.whole = 48000;
            audioStream->addAvailableFormat(&format, &rate, &rate);
			*/
			
			// Finally, the IOAudioStream's current format needs to be indicated
            audioStream->setFormat(&format);
        }
    }
    
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
    
IOReturn CMI8738AudioEngine::performAudioEngineStart()
{
    DBGPRINT("CMI8738AudioEngine[%p]::performAudioEngineStart()\n", this);

	writeUInt16(CM_REG_CH0_FRAME2, NUM_SAMPLE_FRAMES-1);
	writeUInt16(CM_REG_CH0_FRAME2+2, NUM_SAMPLE_FRAMES-1);
		
	writeUInt16(CM_REG_CH1_FRAME2, NUM_SAMPLE_FRAMES-1);
	writeUInt16(CM_REG_CH1_FRAME2+2, NUM_SAMPLE_FRAMES-1);

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
    
	setUInt32Bit(CM_REG_INT_HLDCLR, (CM_CH0_INT_EN << 0) | (CM_CH0_INT_EN << 1) );
	cm->regFUNCTRL0 |= (CM_CHEN0 << 0) | (CM_CHEN0 << 1);
	writeUInt32(CM_REG_FUNCTRL0, cm->regFUNCTRL0); 
	
    return kIOReturnSuccess;
}

IOReturn CMI8738AudioEngine::performAudioEngineStop()
{
    DBGPRINT("CMI8738AudioEngine[%p]::performAudioEngineStop()\n", this);

    // Add audio - I/O stop code here
    
	clearUInt32Bit(CM_REG_INT_HLDCLR, (CM_CH0_INT_EN << 0) | (CM_CH0_INT_EN << 1));
	cm->regFUNCTRL0 &= ~((CM_CHEN0 << 1) | (CM_CHEN0 << 0));
	writeUInt32(CM_REG_FUNCTRL0, cm->regFUNCTRL0 | ((CM_RST_CH0 << 0) | (CM_RST_CH0 << 1))); 
	writeUInt32(CM_REG_FUNCTRL0, cm->regFUNCTRL0 & ~((CM_RST_CH0 << 0) | (CM_RST_CH0 << 1))); 
	
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
    return (NUM_SAMPLE_FRAMES - (readUInt16(CM_REG_CH0_FRAME2) + 1)) >> shift;
//	return NUM_SAMPLE_FRAMES;
}
    
IOReturn CMI8738AudioEngine::performFormatChange(IOAudioStream *audioStream, const IOAudioStreamFormat *newFormat, const IOAudioSampleRate *newSampleRate)
{
    DBGPRINT("CMI8738AudioEngine[%p]::peformFormatChange(%p, %p, %p)\n", this, audioStream, newFormat, newSampleRate);
    
	UInt32 val, fmt = 0, freq = 0;
	bool EnableSPDIF = false;
	
    // Since we only allow one format, we only need to be concerned with sample rate changes
    // In this case, we only allow 2 sample rates - 44100 & 48000, so those are the only ones
    // that we check for
	if (newFormat) {
		currentResolution = newFormat->fBitWidth;
		shift = 0;
		
		if (newFormat->fNumChannels > 1) {
			fmt |= 0x01;
		}				
		if (newFormat->fBitWidth >= 16) {
			fmt |= 0x02;
			if (newFormat->fBitWidth > 16)
				shift++;
		}
		
		if (!setDACChannels(newFormat->fNumChannels, fmt)) {
			DBGPRINT(("/t Internal Error - setDACChannels failed.\n"));
			return kIOReturnInternalError;			
		}	
				
		val = readUInt32(CM_REG_CHFORMAT);
		if (newFormat->fDriverTag) {
			val &= ~CM_CH1FMT_MASK;
			val |= fmt << CM_CH1FMT_SHIFT;
		} else {
			val &= ~CM_CH0FMT_MASK;
			val |= fmt << CM_CH0FMT_SHIFT;
		}
		writeUInt32(CM_REG_CHFORMAT, val);	
	
		val = newFormat->fDriverTag ? CM_CHADC1 : CM_CHADC0;
		cm->regFUNCTRL0 &= ~val;
		writeUInt32(CM_REG_FUNCTRL0, cm->regFUNCTRL0);
					
	}
	
    if (newSampleRate) {
		currentSampleRate = newSampleRate->whole;
	} else { 
		if (currentSampleRate == 0) {
			currentSampleRate = INITIAL_SAMPLE_RATE;
		}
	}

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

	val = newFormat->fDriverTag ? CM_CHADC1 : CM_CHADC0;
	cm->regFUNCTRL0 &= ~val;
	writeUInt32(CM_REG_FUNCTRL0, cm->regFUNCTRL0);
			
	val = readUInt32(CM_REG_FUNCTRL1);
	if (newFormat->fDriverTag) {
		val &= ~CM_ASFC_MASK;
		val |= (freq << CM_ASFC_SHIFT) & CM_ASFC_MASK;
	} else {
		val &= ~CM_DSFC_MASK;
		val |= (freq << CM_DSFC_SHIFT) & CM_DSFC_MASK;
	}            
	writeUInt32(CM_REG_FUNCTRL1, val);
   
	EnableSPDIF = ( (currentSampleRate == 48000) || (currentSampleRate == 44100) ) && 
				  ( (currentResolution == 16) );
	
	setupSPDIFPlayback(true, false);
	
	dumpRegisters();
	
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
	}
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
	
	if (status & CM_CHINT0) {
		mask |= CM_CH0_INT_EN;
	}
	if (status & CM_CHINT1) {
		mask |= CM_CH1_INT_EN;
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
	/*
    DBGPRINT("CMI8738AudioEngine[%p]::dumpRegisters()\n", this);
    DBGPRINT("  FUNCTRL0:    0x%u\n", (UInt32)readUInt32(CM_REG_FUNCTRL0));
    DBGPRINT("  FUNCTRL1:    0x%u\n", (UInt32)readUInt32(CM_REG_FUNCTRL1));
    DBGPRINT("  CHFORMAT:    0x%u\n", (UInt32)readUInt32(CM_REG_CHFORMAT));
    DBGPRINT("  INT_HLDCLR:  0x%u\n", (UInt32)readUInt32(CM_REG_INT_HLDCLR));
    DBGPRINT("  MISC_CTRL:   0x%u\n", (UInt32)readUInt32(CM_REG_MISC_CTRL));
    DBGPRINT("  LEGACY_CTRL: 0x%u\n", (UInt32)readUInt32(CM_REG_LEGACY_CTRL));
    DBGPRINT("  CH0_FRAME1:  0x%u\n", (UInt32)readUInt32(CM_REG_CH0_FRAME1));
    DBGPRINT("  CH0_FRAME2:  0x%u\n", (UInt32)readUInt32(CM_REG_CH0_FRAME2));
    DBGPRINT("  bit shift:   %d\n", (SInt32)shift);
    DBGPRINT("  bit depth:   %d\n", (SInt32)currentResolution);
    DBGPRINT("  sample rate: %d\n", (SInt32)currentSampleRate);
*/

}

#if defined(ARM)
bool CMI8738AudioEngine::driverDesiresHiResSampleIntervals(){
    return TRUE; //to be changed probably, idk
}
#endif

int pci_alloc(struct memhandle *h)
{
    
#if defined(OLD_ALLOC)
    
#warning "Using old dma memory allocation method"
    
    IOPhysicalAddress physical;
    h->addr=IOMallocContiguous((vm_size_t)h->size,PAGE_SIZE,&physical);
    h->dma_handle = (dword)physical;
    
    if (!(h->addr) || !(h->dma_handle))
        return -1;
#else
    
    //h->addr=IOMallocContiguous(h->size,PAGE_SIZE,&phys_addr);
    mach_vm_address_t mask = allocation_mask; //0x000000007FFFFFFFULL & ~(PAGE_SIZE - 1);
    
    h->desc = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(
                                                               kernel_task,
                                                               kIODirectionInOut | kIOMemoryPhysicallyContiguous,
                                                               h->size,
                                                               mask);
    
    if (!h->desc)
        return -1;
    
    h->desc->prepare();
    
    h->addr =              h->desc->getBytesNoCopy    ();
    h->dma_handle = (UInt32)h->desc->getPhysicalAddress();
#endif
    
    //buffer cleaning
    bzero((unsigned char*)h->addr, h->size);
    
    return 0;
}

void pci_free(struct memhandle *h)
{
    const size_t size = h->size;
#if defined(OLD_ALLOC)
    #warning "Using old dma memory allocation method"
    IOFreeContiguous(h->addr,h->size);
#else
    h->desc->release();
#endif
    memset(h, 0, sizeof(*h));
    h->size = size;
}
