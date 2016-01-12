/************************************************************************
** FILE NAME..... MallocPrivateHeap.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION.........
** Replacement malloc by allocate blocks from a private heap other than
** the default one. This will reduce memory fragmentation by isolating
** the most offending modules.
** 
** NOTES............ 
** 
** 
** ASSUMPTIONS...... 
** A small subset of modules in core uses this allocator to achieve overall
** memory conservation.
** 
** RESTRICTIONS..... 
** Allocation is heap-handle based.
** 
** LIMITATIONS...... 
** 
** DEVIATIONS....... 
** 
** RETURN VALUES.... 0  - successful
**                   !0 - error
** 
** AUTHOR(S)........ Michael Q Yan
** 
** CHANGES:
** 
************************************************************************/
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <dlfcn.h>
#endif

#include "ReplacementMallocImpl.h"
#include "PatchManager.h"

typedef void* HeapHandle;

// Global functions for heap-based memory alloc/dealloc
HeapHandle (*AT_HeapCreate)(unsigned int iFlags)    = NULL;
void*      (*AT_HeapAlloc)(HeapHandle iHeap, size_t iSize, unsigned int iFlags) = NULL;
int        (*AT_HeapFree)(void* ipBlock) = NULL;
void*      (*AT_HeapReAlloc)(void* ipOldBlock, size_t iNewSize, unsigned int iFlags) = NULL;

// private heap handle
static HeapHandle gHeap = NULL;

// Call this before patching any of the following functions.
// This will prepare the private heap for subsequent malloc/free
// Return true, if initialized succussfully
bool InitPrivateHeaps()
{
	// We need a portable way to find out if the underneath MM support
	// private-heap (aka regions/arena, etc) allocations

#ifndef WIN32
	// Fisrt find out the default MM
	// By checking the address of resolved malloc function of my lib
	address_t lMallocPtr = (address_t) malloc;
	//const char* lpMallocModuleName = GetModuleNameByAddr(lMallocPtr);
	const char* lpMallocModuleName = LIBSH;

	if(::strstr(lpMallocModuleName, LIBSH) )
	{
		// It is SH
		void* lpHandle = ::dlopen(LIBSH, RTLD_LAZY | RTLD_GLOBAL);
		if(lpHandle == NULL)
			return false;

		AT_HeapCreate = (HeapHandle(*)(unsigned int)) ::dlsym(lpHandle, "MemPoolInit");
		if(AT_HeapCreate == NULL)
			return false;

		AT_HeapAlloc = (void*(*)(HeapHandle,size_t,unsigned int))
					::dlsym(lpHandle, "MemAllocPtr");
		if(AT_HeapAlloc == NULL)
			return false;

		AT_HeapFree = (int(*)(void*)) ::dlsym(lpHandle, "MemFreePtr");
		if(AT_HeapFree == NULL)
			return false;

		AT_HeapReAlloc = (void*(*)(void*,size_t,unsigned int))
					::dlsym(lpHandle, "MemReAllocPtr");
		if(AT_HeapReAlloc == NULL)
			return false;

		// So far, we have located all functions we need.
		// Create the private heap now.
		gHeap = AT_HeapCreate(0);
		if(gHeap == NULL)
			return false;

		// done with the library
		::dlclose(lpHandle);
		return true;
	}
#endif

	return false;
}

void*  MallocBlockFromPrivateHeap(size_t iBlockSize)
{
	return AT_HeapAlloc(gHeap, iBlockSize, 0);
}

void   FreeBlockFromPrivateHeap(void* ipBlock)
{
	if(ipBlock)
	{
		AT_HeapFree(ipBlock);
	}
}

void*  ReallocBlockFromPrivateHeap(void* ipBlock, size_t iNewSize)
{
	if(ipBlock)
	{
		return AT_HeapReAlloc(ipBlock, iNewSize, 0);
	}
	else
	{
		// The old block is NIL, we don't know which heap to use for new block
		// Hence use default heap for the sake of safety.
		return malloc(iNewSize);
	}
}

void*  CallocBlockFromPrivateHeap(size_t iNumElement, size_t iElementSize)
{
	return AT_HeapAlloc(gHeap, iNumElement*iElementSize, 0);
}

void*  NewBlockFromPrivateHeap(size_t iBlockSize)
{
	void* lpResult = MallocBlockFromPrivateHeap(iBlockSize);
	if(!lpResult)
	{
		throw std::bad_alloc();
	}
	return lpResult;
}

