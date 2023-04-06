//
//  Memhandle.cpp
//  CMI8738AudioDriver
//
//  Created by ITzTravelInTime on 06/04/23.
//

#include "Memhandle.h"
#include <IOKit/IOLib.h>
#if !defined(OLD_ALLOC)
#include <IOKit/IOBufferMemoryDescriptor.h>
#endif
#include <libkern/version.h>

int pci_alloc(Memhandle *h)
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
    h->dma_handle = (uintptr_t)h->desc->getPhysicalAddress();
#endif
    
    //buffer cleaning
    bzero((unsigned char*)h->addr, h->size);
    
    return 0;
}

void pci_free(Memhandle *h)
{
    const mach_vm_size_t size = h->size;
#if defined(OLD_ALLOC)
    #warning "Using old dma memory allocation method"
    IOFreeContiguous(h->addr,h->size);
#else
    h->desc->release();
#endif
    memset(h, 0, sizeof(*h));
    h->size = size;
}
