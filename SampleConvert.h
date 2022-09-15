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
 
#include "CMI8738AudioEngine.h"

#include <IOKit/IOLib.h>
//#include <TargetConditionals.h>
#include <libkern/OSTypes.h>
#include <libkern/OSByteOrder.h>

inline static UInt32 correctEndianess32(const UInt32 number){
    return OSSwapHostToLittleInt32(number);
}

inline static UInt16 correctEndianess16(const UInt16 number){
    return OSSwapHostToLittleInt16(number);
}

//#define NO_ASM

static const float clipMax = 1.0f;
static const float clipMin = -1.0f;

static const float clipPosMul8 = 127.0f;
static const float clipNegMul8 = 128.0f;

static const float clipPosMulDiv8 = 1 / clipPosMul8;
static const float clipNegMulDiv8 = 1 / clipNegMul8;

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

static inline int32_t FloatToInt32(const double val){
    
#if !defined(PPC) || defined(NO_ASM)
    static const double maxInt32 = 2147483648.0;    // 1 << 31
    static const double round = 128.0;
    static const double max32 = maxInt32 - 1.0 - round;
#endif
    
#if defined(PPC) && !defined(NO_ASM)
    int32_t i;
    union {
        double     d;
        uint32_t i[2];
    } u;
    
    // fctiw rounds, doesn't truncate towards zero like fctiwz
    __asm__ volatile ("fctiw %0, %1"
                      : "=f" (u.d)
                      : "f" (val));
    
    i = u.i[1];
    return i;
    
#elif defined(X86)
    /*
     if (val >= max32) return 0x7FFFFFFF;
     return (SInt32)val; //the compiler handles it better on x86, hence the missing check compared to the generic version
     */
    
    return ((val >= max32) * 0x7FFFFFFF) + ((val < max32) * ((int32_t)val)); //branchless, so the cpu is happy
#else
    //Generic slow version
    static const double min32 = -2147483648.0;
    
    if (val >= max32) return 0x7FFFFFFF;
    else if (val <= min32) return 0x80000000;
    return (SInt32)val;
    
#endif
    
}

static inline double fabs(const double val){
    
#if defined(PPC) && !defined(NO_ASM)
    double d;
    
    __asm__ volatile ("fabs %0, %1"
                      : "=f" (d)
                      : "f" (val));
    
    return d;
#else
    return val * ((val > 0) - (val < 0));
#endif
    
}

static inline double fmax(const double a, const double b){
    return (a + b + fabs(a - b)) / 2;
}

static inline double fmin(const double a, const double b){
    return (a + b - fabs(a - b)) / 2;
}

static inline double clamp(const double val){
    return fmin(fmax(val, clipMin), clipMax);
}


static void Float32ToSInt32_portable( const float* floatMixBuf, SInt32* destBuf, const size_t end, const size_t start){
    register float inSample;
    register size_t sampleIndex = start, sampleDestinationMemoryIndex = (start * sizeof(*destBuf));
    
    // Loop through the mix/sample buffers one sample at a time and perform the clip and conversion operations
    for (; sampleIndex < end; sampleIndex++, sampleDestinationMemoryIndex += sizeof(*destBuf)){
        // Fetch the floating point mix sample
        inSample = (clamp(floatMixBuf[sampleIndex]) * clipPosMul32) + 128;
        
        // Scale the -1.0 to 1.0 range to the appropriate scale for signed 32-bit samples and then
        // convert to SInt32 and store in the hardware sample buffer
        
        OSWriteLittleInt32(destBuf, sampleDestinationMemoryIndex, FloatToInt32(inSample));
    }
    
}

static void Float32ToSInt16Aligned32_portable( const float* floatMixBuf, SInt32* destBuf, const size_t end, const size_t start){
    register float inSample;
    register size_t sampleIndex = start, sampleDestinationMemoryIndex = (start * sizeof(*destBuf));
    
    // Loop through the mix/sample buffers one sample at a time and perform the clip and conversion operations
    for (; sampleIndex < end; sampleIndex++, sampleDestinationMemoryIndex += sizeof(*destBuf)){
        // Fetch the floating point mix sample
        inSample = (clamp(floatMixBuf[sampleIndex]) * clipPosMul16);
        
        // Scale the -1.0 to 1.0 range to the appropriate scale for signed 16-bit samples and then
        // convert to SInt16 and shit upwards of 16 bits (to have the correct volume) and store in the hardware sample buffer
        
        OSWriteLittleInt32(destBuf, sampleDestinationMemoryIndex, ((int16_t)inSample) << 16);
    }
    
}

static void Float32ToSInt16_portable( const float* floatMixBuf, SInt16* destBuf, const size_t end, const size_t start){
    register float inSample;
    register size_t sampleIndex = start, sampleDestinationMemoryIndex = (start * sizeof(*destBuf));
    
    // Loop through the mix/sample buffers one sample at a time and perform the clip and conversion operations
    for (; sampleIndex < end; sampleIndex++, sampleDestinationMemoryIndex += sizeof(*destBuf)){
        // Fetch the floating point mix sample
        inSample = (clamp(floatMixBuf[sampleIndex]) * clipPosMul16);
        
        // Scale the -1.0 to 1.0 range to the appropriate scale for signed 16-bit samples and then
        // convert to SInt16 and store in the hardware sample buffer
        
        OSWriteLittleInt16(destBuf, sampleDestinationMemoryIndex, ((int16_t)inSample));
    }
}

static void Float32ToSInt32_no_array_little_endian( const float* mixBuf, int32_t* destBuf, const size_t end, const size_t start){
    register uintptr_t inSample = (uintptr_t)&mixBuf[start];
    register uintptr_t outSample = (uintptr_t)&destBuf[start];
    
    const uintptr_t endSample = (uintptr_t)&mixBuf[end - 1];
    
    for (;inSample <= endSample; inSample += sizeof(*mixBuf), outSample += sizeof(*destBuf)){
        (*(volatile int32_t *)outSample) = FloatToInt32((clamp(*(volatile float *)inSample) * clipPosMul32) + 128);
    }
    
}

static void Float32ToSInt16Aligned32_no_array_little_endian( const float* mixBuf, int32_t* destBuf, const size_t end, const size_t start){
    register uintptr_t inSample = (uintptr_t)&mixBuf[start];
    register uintptr_t outSample = (uintptr_t)&destBuf[start];
    
    const uintptr_t endSample = (uintptr_t)&mixBuf[end - 1];
    
    for (;inSample <= endSample; inSample += sizeof(*mixBuf), outSample += sizeof(*destBuf)){
        (*(volatile int32_t *)outSample) = ((int16_t)(clamp(*(volatile float *)inSample) * clipPosMul16)) << 16;
    }
}

static void Float32ToSInt16_no_array_little_endian( const float* mixBuf, int16_t* destBuf, const size_t end, const size_t start){
    register uintptr_t inSample = (uintptr_t)&mixBuf[start];
    register uintptr_t outSample = (uintptr_t)&destBuf[start];
    
    const uintptr_t endSample = (uintptr_t)&mixBuf[end - 1];
    
    for (;inSample <= endSample; inSample += sizeof(*mixBuf), outSample += sizeof(*destBuf)){
        (*(volatile int16_t *)outSample) = (int16_t)(clamp(*(volatile float *)inSample) * clipPosMul16);
    }
}

static void Float32ToSInt32_no_array_portable( const float* mixBuf, int32_t* destBuf, const size_t end, const size_t start){
    register uintptr_t inSample = (uintptr_t)&mixBuf[start];
    register size_t outSampleIndex = start * sizeof(*destBuf);
    
    const uintptr_t endSample = (uintptr_t)&mixBuf[end - 1];
    
    for (;inSample <= endSample; inSample += sizeof(*mixBuf), outSampleIndex += sizeof(*destBuf)){
        OSWriteLittleInt32(destBuf, outSampleIndex, FloatToInt32((clamp(*(volatile float *)inSample) * clipPosMul32) + 128) );
    }
}

static void Float32ToSInt16Aligned32_no_array_portable( const float* mixBuf, int32_t* destBuf, const size_t end, const size_t start){
    register uintptr_t inSample = (uintptr_t)&mixBuf[start];
    register size_t outSampleIndex = start * sizeof(*destBuf);
    
    const uintptr_t endSample = (uintptr_t)&mixBuf[end - 1];
    
    for (;inSample <= endSample; inSample += sizeof(*mixBuf), outSampleIndex += sizeof(*destBuf)){
        OSWriteLittleInt32(destBuf, outSampleIndex ,((int16_t)(clamp(*(volatile float *)inSample) * clipPosMul16)) << 16);
    }
}

static void Float32ToSInt16_no_array_portable( const float* mixBuf, int16_t* destBuf, const size_t end, const size_t start){
    register uintptr_t inSample = (uintptr_t)&mixBuf[start];
    register size_t outSampleIndex = start * sizeof(*destBuf);
    
    const uintptr_t endSample = (uintptr_t)&mixBuf[end - 1];
    
    for (;inSample <= endSample; inSample += sizeof(*mixBuf), outSampleIndex += sizeof(*destBuf)){
        OSWriteLittleInt16(destBuf, outSampleIndex ,(int16_t)(clamp(*(volatile float *)inSample) * clipPosMul16));
    }
}


#if defined(PPC)

#define Float32ToSInt16_optimized Float32ToSInt16_no_array_portable
#define Float32ToSInt32_optimized Float32ToSInt32_no_array_portable
#define Float32ToSInt16Aligned32_optimized Float32ToSInt16Aligned32_no_array_portable

#elif defined(TARGET_RT_LITTLE_ENDIAN) || defined(X86)
#if TARGET_RT_LITTLE_ENDIAN || defined(X86)

#define Float32ToSInt16_optimized Float32ToSInt16_no_array_little_endian
#define Float32ToSInt32_optimized Float32ToSInt32_no_array_little_endian
#define Float32ToSInt16Aligned32_optimized Float32ToSInt16Aligned32_no_array_little_endian

#else

#define Float32ToSInt16_optimized Float32ToSInt16_portable
#define Float32ToSInt32_optimized Float32ToSInt32_portable
#define Float32ToSInt16Aligned32_optimized Float32ToSInt16Aligned32_portable

#endif

#else

#define Float32ToSInt16_optimized Float32ToSInt16_portable
#define Float32ToSInt32_optimized Float32ToSInt32_portable
#define Float32ToSInt16Aligned32_optimized Float32ToSInt16Aligned32_portable

#endif


#endif
