/************************************************************************
** FILE NAME..... StaticBufferMgr.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Use static storage as the buffer to STL class in
**                   order to avoid disturbance to heap profile
** 
** NOTES............ 
** 
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS..... 
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
#include <unistd.h>
#ifdef sun
#include <thread.h>
#endif
#endif

#include <stdio.h>
#include <stdlib.h>

#include "CrossPlatform.h"
#include "StaticBufferMgr.h"

#if defined(_SMARTHEAP) && defined(_USE_STATIC_BUFFER)

#include "smrtheap.h"

#ifdef WIN32
#define BUFFER_SIZE 64*1024*1024
#else
#define BUFFER_SIZE 1024*1024*1024
#endif

// Big chunk of memory in bss segment of executable
// to be carved out and used in small pieces
static char gBuffer[BUFFER_SIZE];

// Memory pool created from above buffer
MEM_POOL gStaticBuffrePool = NULL;

// !FIX! serialize buffer-based pool
#include "Atomic_op.h"
//pthread_mutex_t gUserPoolLock = PTHREAD_MUTEX_INITIALIZER;
spinlock_t gUserPoolLock = SPINLOCK_UNLOCKED;

void* GetBlock(size_t size)
{
#if defined(sun) || defined(linux)
	//LOCK_MUTEX(&gUserPoolLock);
	SpinLock(&gUserPoolLock);
#endif

	void* lpBlock = MemAllocPtr(gStaticBuffrePool, size, 0);
	
#if defined(sun) || defined(linux)
	//UNLOCK_MUTEX(&gUserPoolLock);
	SpinUnLock(&gUserPoolLock);
#endif

	if(!lpBlock)
	{
		static bool once = false;
		if(once == false)
		{
			fprintf(stderr, "Static buffer is used up, use heap from now on\n");
			once = true;
		}
		lpBlock = malloc(size);
	}
	return lpBlock;
}

void FreeBlock(void* ipBlock)
{
#if defined(sun) || defined(linux)
	//LOCK_MUTEX(&gUserPoolLock);
	SpinLock(&gUserPoolLock);
#endif

	MemFreePtr(ipBlock);

#if defined(sun) || defined(linux)
	//UNLOCK_MUTEX(&gUserPoolLock);
	SpinUnLock(&gUserPoolLock);
#endif
}

#define ALIGN_64K_MASK  0xFFFFul
#define ALIGN_ON_64K(p) (((unsigned long)p+ALIGN_64K_MASK)&(~ALIGN_64K_MASK))

// Init
void  InitStaticBuffer()
{
	// Get around a bug in memory manager where user-provided region has to be 
	// aligned on 64KB, but it doesn't do it correctly. It will assert in debug mode.
	char* lpAlignedBuffer = (char*)ALIGN_ON_64K(gBuffer);
	size_t lBufferSize    = BUFFER_SIZE - (lpAlignedBuffer - gBuffer);

	// Touch all pages of static buffer
	// This way, process gets all physical pages of the buffer
	// so that any RSS increase later must be from heap
	size_t lSystemPageSize;
	GET_SYSTEM_PAGE_SIZE(lSystemPageSize);
	char* lCursor = lpAlignedBuffer;
	while(lCursor < lpAlignedBuffer + lBufferSize)
	{
		*lCursor = (char)0xFF;
		lCursor += lSystemPageSize;
	}

	// Use static buffer to create a pool
	gStaticBuffrePool = MemPoolInitRegion(lpAlignedBuffer, lBufferSize, MEM_POOL_DEFAULT | MEM_POOL_SERIALIZE | MEM_POOL_REGION);
	if(gStaticBuffrePool == NULL)
	{
		fprintf(stderr, "Failed to create pool from static buffer\n");
		exit(-1);
	}
	// Disable sub pools so that we use less memory from buffer
	MemPoolSetMaxSubpools(gStaticBuffrePool, 1);
}

// Done with the buffer
void ReleaseStaticBuffer()
{
	if(gStaticBuffrePool)
	{
		MemPoolFree(gStaticBuffrePool);
		gStaticBuffrePool = NULL;
	}
}

#else

void  InitStaticBuffer()
{
}

void* GetBlock(size_t size)
{
	return malloc(size);
}

void FreeBlock(void* ipBlock)
{
	free(ipBlock);
}

#endif // _SMARTHEAP
