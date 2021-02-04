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

static const float clipMax = 1.0f;
static const float clipMin = -1.0f;

static const float clipPosMul = 32767.0f;
static const float clipNegMul = 32768.0f;

static const float clipPosMulDiv = 1 / clipPosMul;
static const float clipNegMulDiv = 1 / clipNegMul;

// The function clipOutputSamples() is called to clip and convert samples from the float mix buffer into the actual
// hardware sample buffer.  The samples to be clipped, are guaranteed not to wrap from the end of the buffer to the
// beginning.
// This implementation is very inefficient, but illustrates the clip and conversion process that must take place.
// Each floating-point sample must be clipped to a range of -1.0 to 1.0 and then converted to the hardware buffer
// format


 #ifdef __BIG_ENDIAN__
 //#if 0
 //#warning Experimental code
 
 static UInt32 littleToBig32(const UInt32 num){
 
 UInt32 res = 0;
 
 res |= (num & 0xff000000) >> 24;
 res |= (num & 0x00ff0000) >> 8 ;
 res |= (num & 0x0000ff00) << 8 ;
 res |= (num & 0x000000ff) << 24;
 
 return res;
 
 }
 
 static UInt16 littleToBig16(const UInt16 num){
 
 UInt16 res = 0;
 
 res |= (num & 0xff00) >> 8 ;
 res |= (num & 0x00ff) << 8 ;
 
 return res;
 
 }
 
 #define correctEndian32(num) (littleToBig32(num))
 #define correctEndian16(num) (littleToBig16(num))
 
 #else
 
 #define correctEndian32(num) ((UInt32)(num))
 #define correctEndian16(num) ((UInt16)(num))
 
 #endif
 
 

// The parameters are as follows:
//		mixBuf - a pointer to the beginning of the float mix buffer - its size is based on the number of sample frames
// 					times the number of channels for the stream
//		sampleBuf - a pointer to the beginning of the hardware formatted sample buffer - this is the same buffer passed
//					to the IOAudioStream using setSampleBuffer()
//		firstSampleFrame - this is the index of the first sample frame to perform the clipping and conversion on
//		numSampleFrames - the total number of sample frames to clip and convert
//		streamFormat - the current format of the IOAudioStream this function is operating on
//		audioStream - the audio stream this function is operating on
IOReturn CMI8738AudioEngine::clipOutputSamples(const void *mixBuf, void *sampleBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream)
{
    UInt32 sampleIndex, maxSampleIndex;
    float *floatMixBuf;
    SInt16 *outputSInt16Buf = (SInt16 *)sampleBuf;
    // Start by casting the void * mix and sample buffers to the appropriate types - float * for the mix buffer
    // and SInt16 * for the sample buffer (because our sample hardware uses signed 16-bit samples)
    floatMixBuf = (float *)mixBuf;

    
    // We calculate the maximum sample index we are going to clip and convert
    // This is an index into the entire sample and mix buffers
    maxSampleIndex = (firstSampleFrame + numSampleFrames) * streamFormat->fNumChannels;
    
    // Loop through the mix/sample buffers one sample at a time and perform the clip and conversion operations
    for (sampleIndex = (firstSampleFrame * streamFormat->fNumChannels); sampleIndex < maxSampleIndex; sampleIndex++) {
        float inSample;
        
        // Fetch the floating point mix sample
        inSample = floatMixBuf[sampleIndex];
        
        // Clip that sample to a range of -1.0 to 1.0
        // A softer clipping operation could be done here
		
        if (inSample > clipMax) {
            inSample = clipMax;
        } else if (inSample < clipMin) {
            inSample = clipMin;
        }
        
        // Scale the -1.0 to 1.0 range to the appropriate scale for signed 16-bit samples and then
        // convert to SInt16 and store in the hardware sample buffer
		
		outputSInt16Buf[sampleIndex] = ((SInt16) (inSample * ((inSample >= 0) ? clipPosMul : clipNegMul)));
		outputSInt16Buf[sampleIndex] = (SInt16)correctEndian16((UInt16)outputSInt16Buf[sampleIndex]);
		
		/*
		if (inSample >= 0) {
			outputSInt16Buf[sampleIndex] = (SInt16) (inSample * clipPosMul);
		} else {
			outputSInt16Buf[sampleIndex] = (SInt16) (inSample * clipNegMul);
		}
		*/
    }
    
    return kIOReturnSuccess;
}

// The function convertInputSamples() is responsible for converting from the hardware format 
// in the input sample buffer to float samples in the destination buffer and scale the samples 
// to a range of -1.0 to 1.0.  This function is guaranteed not to have the samples wrapped
// from the end of the buffer to the beginning.
// This function only needs to be implemented if the device has any input IOAudioStreams

// This implementation is very inefficient, but illustrates the conversion and scaling that needs to take place.

// The parameters are as follows:
//		sampleBuf - a pointer to the beginning of the hardware formatted sample buffer - this is the same buffer passed
//					to the IOAudioStream using setSampleBuffer()
//		destBuf - a pointer to the float destination buffer - this is the buffer that the CoreAudio.framework uses
//					its size is numSampleFrames * numChannels * sizeof(float)
//		firstSampleFrame - this is the index of the first sample frame to the input conversion on
//		numSampleFrames - the total number of sample frames to convert and scale
//		streamFormat - the current format of the IOAudioStream this function is operating on
//		audioStream - the audio stream this function is operating on
IOReturn CMI8738AudioEngine::convertInputSamples(const void *sampleBuf, void *destBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream)
{
    UInt32 numSamplesLeft;
    float *floatDestBuf;
    SInt16 *inputBuf;
    
    // Start by casting the destination buffer to a float *
    floatDestBuf = (float *)destBuf;
    // Determine the starting point for our input conversion 
    inputBuf = &(((SInt16 *)sampleBuf)[firstSampleFrame * streamFormat->fNumChannels]);
    
    // Calculate the number of actual samples to convert
    numSamplesLeft = numSampleFrames * streamFormat->fNumChannels;
    
    // Loop through each sample and scale and convert them
    while (numSamplesLeft > 0) {
        SInt16 inputSample;
        
        // Fetch the SInt16 input sample
        inputSample = *inputBuf;
    
        // Scale that sample to a range of -1.0 to 1.0, convert to float and store in the destination buffer
        // at the proper location
        if (inputSample >= 0) {
            (*floatDestBuf) = correctEndian16( inputSample * clipPosMulDiv );// / 32767.0;
        } else {
            (*floatDestBuf) = correctEndian16( inputSample * clipNegMulDiv );// / 32768.0;
        }
        
        // Move on to the next sample
        ++inputBuf;
        ++floatDestBuf;
        --numSamplesLeft;
    }
    
    return kIOReturnSuccess;
}
