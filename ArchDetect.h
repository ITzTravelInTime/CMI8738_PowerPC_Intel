//
//  ArchDetect.h
//  CMI8738AudioDriver
//
//  Created by ITzTravelInTime on 06/04/23.
//

#ifndef ArchDetect_h
#define ArchDetect_h

#if defined(i386) || defined(I386) || defined(IX86) || defined(__I386__) || defined(_IX86) || defined(_M_IX86) || defined(AMD64) || defined(__x86_64__) || defined(__i386__)
    #define X86
#elif defined(__PPC__) || defined(__ppc__) || defined(_ARCH_PPC) || defined(__POWERPC__) || defined(__powerpc) || defined(__powerpc__)
    #define PPC
#elif(defined(__ARM__) || defined(__arm__) || defined(_ARCH_ARM) || defined(_ARCH_ARM64) || defined(__aarch64e__) || defined(__arm) || defined(__arm64e__) || defined(__aarch64__))
    #define ARM
#else
    #error "Unknown processor architecture"
#endif

#endif /* ArchDetect_h */
