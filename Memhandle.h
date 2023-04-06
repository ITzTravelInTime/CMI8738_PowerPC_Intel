//
//  Memhandle.hpp
//  CMI8738AudioDriver
//
//  Created by ITzTravelInTime on 06/04/23.
//

#ifndef Memhandle_h
#define Memhandle_h

#include <IOKit/IOLib.h>

#ifndef _IOBUFFERMEMORYDESCRIPTOR_H
class IOBufferMemoryDescriptor;
#endif

typedef struct _memhandle
{
    mach_vm_size_t size;
    void * addr;          // virtual
    uintptr_t dma_handle; // physical
    
#if !defined(OLD_ALLOC)
    IOBufferMemoryDescriptor *desc;
#endif
} Memhandle;

#define allocation_mask ((0x000000007FFFFFFFULL) & (~((PAGE_SIZE) - 1)))

int pci_alloc(Memhandle *h);
void pci_free(Memhandle *h);

#endif /* Memhandle_hpp */
