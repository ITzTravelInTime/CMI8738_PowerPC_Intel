/*
 *  SampleConvert.h
 *  Envy24HT_64bit_Driver
 *
 *  Created by ITzTravelInTime on 12/2/21.
 *  Copyright 2021 __MyCompanyName__. All rights reserved.
 *
 */
 
 #if !defined(SAMPLE_CONVERT_H)
 #define SAMPLE_CONVERT_H
 
 //#include "AudioEngine.h"

#include <IOKit/IOLib.h>
//#include <TargetConditionals.h>
#include <libkern/OSTypes.h>
#include <libkern/OSByteOrder.h>

#if defined(i386) || defined(I386) || defined(IX86) || defined(__I386__) || defined(_IX86) || defined(_M_IX86) || defined(AMD64) || defined(__x86_64__) || defined(__i386__)
    #define X86
#elif defined(__PPC__) || defined(__ppc__) || defined(_ARCH_PPC) || defined(__POWERPC__) || defined(__powerpc) || defined(__powerpc__)
    #define PPC
#else
    #error "Unknown processor architecture"
#endif

static UInt32 correctEndianess32(const UInt32 number){
    return OSSwapHostToLittleInt32(number);
}

static UInt16 correctEndianess16(const UInt16 number){
    return OSSwapHostToLittleInt16(number);
}

static const float clipMax = 1.0f;
static const float clipMin = -1.0f;

static const float clipPosMul16 = 32767.0f;
static const float clipNegMul16 = 32768.0f;

static const float clipPosMulDiv16 = 1 / clipPosMul16;
static const float clipNegMulDiv16 = 1 / clipNegMul16;

static const float clipPosMul24 = 8388607.0f;
static const float clipNegMul24 = 8388608.0f;

static const float clipPosMulDiv24 = 1 / clipPosMul24;
static const float clipNegMulDiv24 = 1 / clipNegMul24;

static const float clipPosMul32 = 2147483647.0f;
static const float clipNegMul32 = 2147483648.0f;

static const float clipPosMulDiv32 = 1 / clipPosMul32;
static const float clipNegMulDiv32 = 1 / clipNegMul32;

static inline SInt32 FloatToInt32(double val){

#if !defined(PPC)
    static const double maxInt32 = 2147483648.0;	// 1 << 31
	static const double round = 128.0;
	static const double max32 = maxInt32 - 1.0 - round;
#endif
    
#if defined(PPC)
	SInt32 i;
	union {
		double	d;
		UInt32	i[2];
	} u;
    
	// fctiw rounds, doesn't truncate towards zero like fctiwz
	__asm__ ("fctiw %0, %1" 
			 /* outputs:  */ : "=f" (u.d) 
			 /* inputs:   */ : "f" (val));
    
	i = u.i[1];
	return i;
#elif defined(X86)
	
	if (val >= max32) return 0x7FFFFFFF;
	return (SInt32)val;
    
#else
    
    static const double min32 = -2147483648.0;
    
	if (val >= max32) return 0x7FFFFFFF;
	else if (val <= min32) return 0x80000000;
	return (SInt32)val;
    
#endif
    
}

#define Float32ToSInt24Unpacked_optimized Float32ToSInt24Unpacked_portable
#define Float32ToSInt16_optimized Float32ToSInt16_portable
#define Float32ToSInt32_optimized Float32ToSInt32_portable

static void Float32ToSInt32_portable( const float* floatMixBuf, SInt32* destBuf, const UInt32 end, const UInt32 start){
    register float inSample;
    UInt32 sampleIndex;
    
    // Loop through the mix/sample buffers one sample at a time and perform the clip and conversion operations
    for (sampleIndex = start; sampleIndex < end; sampleIndex++){
        // Fetch the floating point mix sample
        inSample = floatMixBuf[sampleIndex];
        
        // Clip that sample to a range of -1.0 to 1.0
        // A softer clipping operation could be done here
        if (inSample > clipMax){
            inSample = clipMax;
        }else if (inSample < clipMin){
            inSample = clipMin;
        }
        
        // Scale the -1.0 to 1.0 range to the appropriate scale for signed 32-bit samples and then
        // convert to SInt32 and store in the hardware sample buffer
        
        //destBuf[sampleIndex] = (SInt32)correctEndianess32( (word)(inSample * clipPosMul16) << 16 ); //no artifacts 16 bit aprrox.
        destBuf[sampleIndex] = (SInt32)correctEndianess32( FloatToInt32( inSample * clipPosMul32 + 128) );
        
    }
    
}

static void Float32ToSInt24Unpacked_portable( const float* floatMixBuf, SInt32* destBuf, const UInt32 end, const UInt32 start){
    register float inSample;
    UInt32 sampleIndex;
    
    
    
    // Loop through the mix/sample buffers one sample at a time and perform the clip and conversion operations
    for (sampleIndex = start; sampleIndex < end; sampleIndex++){
        // Fetch the floating point mix sample
        inSample = floatMixBuf[sampleIndex];
        
        // Clip that sample to a range of -1.0 to 1.0
        // A softer clipping operation could be done here
        if (inSample > clipMax){
            inSample = clipMax;
        }else if (inSample < clipMin){
            inSample = clipMin;
        }
        
        // Scale the -1.0 to 1.0 range to the appropriate scale for signed 32-bit samples and then
        // convert to SInt32 and store in the hardware sample buffer
        
        //destBuf[sampleIndex] = (SInt32)correctEndianess32( (word)(inSample * clipPosMul16) << 16 ); //no artifacts 16 bit aprrox.
        destBuf[sampleIndex] = (SInt32)correctEndianess32( FloatToInt32( inSample * clipPosMul24 + 128 ) << 8);
        
    }
    
}

static void Float32ToSInt16_portable( const float* floatMixBuf, SInt16* destBuf, const UInt32 end, const UInt32 start){
    
    register float inSample;
    UInt32 sampleIndex;
    
    // Loop through the mix/sample buffers one sample at a time and perform the clip and conversion operations
    for (sampleIndex = start; sampleIndex < end; sampleIndex++){
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
        
        destBuf[sampleIndex] = (SInt16)correctEndianess16((UInt16)(inSample * clipPosMul16));
    }
}

/*
 UInt32 numSamplesLeft;
 float *floatDestBuf;
 
 // Calculate the number of actual samples to convert
 numSamplesLeft = numSampleFrames * streamFormat->fNumChannels;
 
 // Start by casting the destination buffer to a float *
 floatDestBuf = (float *)destBuf;
 
 if(bps==32){
 SInt32 *inputBuf;
 SInt32 inputSample;
 
 // Determine the starting point for our input conversion
 inputBuf = &(((SInt32 *)sampleBuf)[firstSampleFrame * streamFormat->fNumChannels]);
 
 // Loop through each sample and scale and convert them
 for (UInt32 i = 0; i < numSamplesLeft; i++) {
 // Fetch the SInt32 input sample
 inputSample = *inputBuf;
 
 // Scale that sample to a range of -1.0 to 1.0, convert to float and store in the destination buffer
 // at the proper location
 *floatDestBuf = (SInt32)correctEndianess32((UInt32)(inputSample * ( inputSample>=0 ? clipPosMulDiv24 : clipNegMulDiv24 )));
 
 // Move on to the next sample
 ++inputBuf;
 ++floatDestBuf;
 }
 
 }else if(bps==16){
 SInt16 *inputBuf;
 SInt16 inputSample;
 
 // Determine the starting point for our input conversion
 inputBuf = &(((SInt16 *)sampleBuf)[firstSampleFrame * streamFormat->fNumChannels]);
 
 // Loop through each sample and scale and convert them
 for (UInt32 i = 0; i < numSamplesLeft; i++) {
 
 // Fetch the SInt16 input sample
 inputSample = *inputBuf;
 
 // Scale that sample to a range of -1.0 to 1.0, convert to float and store in the destination buffer
 // at the proper location
 
 (*floatDestBuf) = (SInt16)correctEndianess16((UInt16)(inputSample * ( inputSample>=0 ? clipPosMulDiv16 : clipNegMulDiv16 )));
 
 // Move on to the next sample
 ++inputBuf;
 ++floatDestBuf;
 }
 }
 */

void SInt32ToFloat32_portable(const SInt32* sourceBuf, float* destBuf, const UInt32 end, const UInt32 start){
    UInt32 sampleIndex;
    
    SInt32 inputSample;
    float  convertedSample;
    UInt32* destBufu = (UInt32*)destBuf;
    
    for (sampleIndex=start; sampleIndex<end; sampleIndex++){
        
        convertedSample = ((float)correctEndianess32(sourceBuf[sampleIndex])) * clipPosMulDiv32 - 128;
        
        destBufu[sampleIndex] = *((UInt32*)(&convertedSample));
    }
}


#endif
