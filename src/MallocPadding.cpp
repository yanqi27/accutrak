/************************************************************************
** FILE NAME..... MallocPadding.cpp
**
** (c) COPYRIGHT
**
** FUNCTION.........
** Replacement malloc by adding extra bytes before and after user block
**
** NOTES............
**
**
** ASSUMPTIONS......
**
**
** RESTRICTIONS.....
** Block allocated by me MUST be freed by me.
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
#include <stdio.h>
#include <crtdbg.h>
#else
#include <pthread.h>
#include <sys/mman.h>
#include <stdio.h>
#endif

#include <new>
#include <vector>

#include "BackTrace.h"
#include "ReplacementMallocImpl.h"
#include "PatchManager.h"
#include "SysAlloc.h"

//////////////////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////////////////

#if defined(linux) || defined(sun) || defined(__hpux) || defined(_AIX)

static pthread_mutex_t gBlockListLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gDeferredBlockListLock = PTHREAD_MUTEX_INITIALIZER;

#elif defined(WIN32)

static CRITICAL_SECTION gBlockListLock;
static CRITICAL_SECTION gDeferredBlockListLock;

// This func is called in DllMain() when dll is loaded in.
void AT_Init_MallocPadding_cpp()
{
	::InitializeCriticalSection(&gBlockListLock);
	::InitializeCriticalSection(&gDeferredBlockListLock);
}

void AT_Fini_MallocPadding_cpp()
{
	::DeleteCriticalSection(&gBlockListLock);
	::DeleteCriticalSection(&gDeferredBlockListLock);
}

#else
#error Unknown platform
#endif


static BlockWithPaddingHeader* gBlockHeaderAnchor = NULL;

static BlockWithPaddingHeader* gDeferredFreeBlocks = NULL;
static size_t gTotalDeferred = 0;

#define Link_Block(pHeader) \
	LOCK_MUTEX(&gBlockListLock); \
	pHeader->mPrevHeader = NULL; \
	pHeader->mNextHeader = gBlockHeaderAnchor; \
	gBlockHeaderAnchor = pHeader; \
	if(pHeader->mNextHeader != NULL) \
	{ \
		pHeader->mNextHeader->mPrevHeader = pHeader; \
	} \
	UNLOCK_MUTEX(&gBlockListLock)

#define Unlink_Header(pHeader) \
	LOCK_MUTEX(&gBlockListLock); \
	if(pHeader->mNextHeader != NULL) \
	{ \
		pHeader->mNextHeader->mPrevHeader = pHeader->mPrevHeader; \
	} \
	if(pHeader->mPrevHeader != NULL) \
	{ \
		pHeader->mPrevHeader->mNextHeader = pHeader->mNextHeader; \
	} \
	else \
	{ \
		gBlockHeaderAnchor = pHeader->mNextHeader; \
	} \
	UNLOCK_MUTEX(&gBlockListLock)

// Defer freeing a block to catch access free, 
// retain more data for post-mortem analysis.
static void LinkDeferredBlock(BlockWithPaddingHeader* ipHeader)
{
	DEFER_SIGNATURE(ipHeader);

	LOCK_MUTEX(&gDeferredBlockListLock);
	if (gDeferredFreeBlocks != NULL)
	{
		gDeferredFreeBlocks->mPrevHeader->mNextHeader = ipHeader;
		ipHeader->mPrevHeader = gDeferredFreeBlocks->mPrevHeader;
		gDeferredFreeBlocks->mPrevHeader = ipHeader;
		ipHeader->mNextHeader = gDeferredFreeBlocks;

		gTotalDeferred += ipHeader->mUserSize;

		// pop blocks if FIFO order if derferred bytes exceeds limit
		while (gTotalDeferred > gPatchMgr->GetMaxDeferFreeBytes())
		{
			BlockWithPaddingHeader* lpHeader = gDeferredFreeBlocks;
			if (lpHeader == lpHeader->mNextHeader)
				break;
			else
				gDeferredFreeBlocks = lpHeader->mNextHeader;

			lpHeader->mPrevHeader->mNextHeader = lpHeader->mNextHeader;
			lpHeader->mNextHeader->mPrevHeader = lpHeader->mPrevHeader;

			gTotalDeferred -= lpHeader->mUserSize;
			SCRATCH_SIGNATURE(lpHeader);
			void* lpRawBlock = (char*)lpHeader - gPatchMgr->GetBackTraceDepth()*sizeof(address_t);
			::free(lpRawBlock);
		}
	}
	else
	{
		gDeferredFreeBlocks = ipHeader->mPrevHeader = ipHeader->mNextHeader = ipHeader;
	}
	UNLOCK_MUTEX(&gDeferredBlockListLock);
}

static void UnlinkDeferredBlock(BlockWithPaddingHeader* ipHeader)
{
	LOCK_MUTEX(&gDeferredBlockListLock);
	{
		gTotalDeferred -= ipHeader->mUserSize;

		if (ipHeader == gDeferredFreeBlocks)
		{
			gDeferredFreeBlocks = ipHeader->mNextHeader;
		}

		if(ipHeader->mNextHeader == ipHeader)
		{
			gDeferredFreeBlocks = ipHeader->mNextHeader = ipHeader->mPrevHeader = NULL;
		}
		else
		{
			ipHeader->mPrevHeader->mNextHeader = ipHeader->mNextHeader;
			ipHeader->mNextHeader->mPrevHeader = ipHeader->mPrevHeader;
			ipHeader->mNextHeader = ipHeader->mPrevHeader = NULL;
		}
	}
	UNLOCK_MUTEX(&gDeferredBlockListLock);
}

///////////////////////////////////////////////////////////////////////////////
//
// Replace routines
//
///////////////////////////////////////////////////////////////////////////////
void*  MallocBlockWithPadding(size_t iSize)
{
	// Most MM promote 0 size to minimum size
	// We don't need to do that because we will allocate the header at least

	// Check all blocks at every entry
	// Let patch mgr decide if it wants to skip it.
	gPatchMgr->CheckAll(PatchManager::AT_EVENT_MALLOC);

	void* lpUserAddr;
	void* lpRawAddr;
	size_t lRealSize;

	// Only pad blocks within selected size range
	if (iSize < gPatchMgr->GetMinBlockSize() || iSize > gPatchMgr->GetMaxBlockSize())
	{
		lpUserAddr = ::malloc(iSize);
		if (lpUserAddr == NULL)
			return NULL;

		lpRawAddr = lpUserAddr;
		lRealSize = iSize;
	}
	else
	{
		size_t lBackTraceSz = gPatchMgr->GetBackTraceDepth()*sizeof(address_t);
		lRealSize = lBackTraceSz + sizeof(BlockWithPaddingHeader) + iSize + TAIL_MAGIC_SZ;
		lpRawAddr = ::malloc(lRealSize);
		BlockWithPaddingHeader* lpHeader;

		if(lpRawAddr == NULL)
			return NULL;

		if(lBackTraceSz > 0)
		{
			// Get the back trace now.
			GetBackTrace((address_t*)lpRawAddr, gPatchMgr->GetBackTraceDepth(), 2);
		}
		lpHeader = (BlockWithPaddingHeader*)((char*)lpRawAddr + lBackTraceSz);

		lpUserAddr = (char*)(lpHeader+1);

		lpHeader->mUserSize = iSize;
		//lpHeader->mCallStackDepth = gPatchMgr->GetBackTraceDepth();

		// Write front magic strings
		SET_SIGNATURE(lpHeader);
		// Write tail magic
		::memset((char*)lpUserAddr+iSize, TAIL_MAGIC_BYTE, TAIL_MAGIC_SZ);
		// Initialize user space if desired
		if(gPatchMgr->InitUserSpace())
		{
			::memset(lpUserAddr, USER_INIT_BYTE, iSize);
		}
		// Put the block on global list
		Link_Block(lpHeader);

		// Spit out the transaction if desired
		OUTPUT_MSG2(AT_ALL, "MallocBlockWithPadding allocates %d bytes at 0x%lx successfully\n", iSize, lpUserAddr);
	}

	// Give it a chance to catch this event
	gPatchMgr->ProcessEvent(PatchManager::AT_EVENT_MALLOC, (unsigned long)lpUserAddr, (unsigned long)lpRawAddr, lRealSize);
	return lpUserAddr;
}

static void CheckBlockWithPaddingHeader(BlockWithPaddingHeader* ipHeader)
{
	// It is hard to check underrun because we do module level patching
	// But it is worth sacrificing this capability for speed

	// Check overrun
	char* lpTailBytes = (char*)(ipHeader+1) + ipHeader->mUserSize;
	for(int i=0; i<TAIL_MAGIC_SZ; i++)
	{
		if(*lpTailBytes != TAIL_MAGIC_BYTE)
		{
			// Tail byte pattern is broken, memory overrun
			gPatchMgr->ProcessMemError(ipHeader+1, PatchManager::AT_MEM_ERR_OVERRUN);
			break;
		}
		lpTailBytes++;
	}
}

void   FreeBlockWithPadding(void* ipUserSpace)
{
	// Check all blocks at every entry
	// Let patch mgr decide if it wants to skip it.
	gPatchMgr->CheckAll(PatchManager::AT_EVENT_FREE);

	if(ipUserSpace == NULL)
	{
		return;
	}

	void* lpRawBlock;
	size_t lRealSize = 0;
	bool lbDeferFree = false;
	BlockWithPaddingHeader* lpHeader = ((BlockWithPaddingHeader*)ipUserSpace) - 1;
	if (SIGNATURE_DEFERRED(lpHeader))
	{
		// Verify if the block is really on the defer list ??
		gPatchMgr->ProcessMemError(ipUserSpace, PatchManager::AT_MEM_ERR_DOUBLE_FREE);
		return;
	}
	else if(SIGNATURE_MATCH(lpHeader))
	{
		CheckBlockWithPaddingHeader(lpHeader);
		Unlink_Header(lpHeader);
		//lpHeader->mPrevHeader = lpHeader->mNextHeader = NULL;
		if(gPatchMgr->FloodFreeSpace())
		{
			::memset(ipUserSpace, USER_FREE_BYTE, lpHeader->mUserSize);
		}

		// Defer free of the block
		if (gPatchMgr->GetMaxDeferFreeBytes() > 0)
		{
			LinkDeferredBlock(lpHeader);
			lbDeferFree = true;
		}
		else
			SCRATCH_SIGNATURE(lpHeader);

		lpRawBlock = (char*)lpHeader - gPatchMgr->GetBackTraceDepth()*sizeof(address_t);
		lRealSize = ((char*)ipUserSpace - (char*)lpRawBlock) + lpHeader->mUserSize;
	}
	else
	{
		lpRawBlock = ipUserSpace;
	}

	// Give it a chance to catch this event
	gPatchMgr->ProcessEvent(PatchManager::AT_EVENT_FREE, (unsigned long)ipUserSpace, (unsigned long)lpRawBlock, lRealSize);

	// free the block
	if (!lbDeferFree)
	{
#ifdef WIN32
		if (_CrtIsValidHeapPointer(lpRawBlock))
#endif
			::free(lpRawBlock);
	}
}

void*  CallocBlockWithPadding(size_t iNumElement, size_t iElementSize)
{
	size_t lTotalSize = iNumElement*iElementSize;
	void* lpResult = MallocBlockWithPadding(lTotalSize);
	if(lpResult && lTotalSize)
	{
		::memset(lpResult, 0, lTotalSize);
	}
	return lpResult;
}

void*  ReallocBlockWithPadding(void* ipUserSpace, size_t iNewSize)
{
	// Check all blocks at every entry
	// Let patch mgr decide if it wants to skip it.
	gPatchMgr->CheckAll(PatchManager::AT_EVENT_REALLOC);

	size_t lOldSize = 0;
	BlockWithPaddingHeader* lpHeader = NULL;
	if(ipUserSpace)
	{
		lpHeader = ((BlockWithPaddingHeader*)ipUserSpace) - 1;
		if(SIGNATURE_MATCH(lpHeader))
		{
			lOldSize = lpHeader->mUserSize;
			CheckBlockWithPaddingHeader(lpHeader);
			Unlink_Header(lpHeader);
			if(iNewSize<lOldSize && gPatchMgr->FloodFreeSpace())
			{
				::memset((char*)ipUserSpace, USER_FREE_BYTE, lOldSize-iNewSize);
			}
			SCRATCH_SIGNATURE(lpHeader);
		}
		else
		{
			lpHeader = NULL;
		}
	}

	char* lpUserAddr = NULL;
	// Call system realoc
	if(lpHeader)
	{
		// The original block is my block, allocate my block again
		size_t lBackTraceSz = gPatchMgr->GetBackTraceDepth()*sizeof(address_t);
		size_t lRealSize = lBackTraceSz + sizeof(BlockWithPaddingHeader) + iNewSize + TAIL_MAGIC_SZ;
		// Take into consider of backtrace buffer
		lpHeader = (BlockWithPaddingHeader*)::realloc((char*)lpHeader - gPatchMgr->GetBackTraceDepth()*sizeof(address_t), lRealSize);

		if(lpHeader == NULL)
		{
			return NULL;
		}

		if(lBackTraceSz > 0)
		{
			// Get the back trace now.
			GetBackTrace((address_t*)lpHeader, gPatchMgr->GetBackTraceDepth(), 2);
			lpHeader = (BlockWithPaddingHeader*)((char*)lpHeader + lBackTraceSz);
		}

		lpUserAddr = (char*)(lpHeader+1);
		lpHeader->mUserSize = iNewSize;
		// Write front magic strings
		SET_SIGNATURE(lpHeader);
		// Write tail magic
		::memset(lpUserAddr+iNewSize, TAIL_MAGIC_BYTE, TAIL_MAGIC_SZ);
		// Initialize user space if desired
		if(iNewSize>lOldSize && gPatchMgr->InitUserSpace())
		{
			::memset(lpUserAddr+lOldSize, USER_INIT_BYTE, iNewSize-lOldSize);
		}
		// Put the block on global list
		Link_Block(lpHeader);

		// Spit out the transaction if desired
		OUTPUT_MSG2(AT_ALL, "MallocBlockWithPadding allocates %d bytes at 0x%lx successfully\n", iNewSize, lpUserAddr);

		// Give it a chance to catch this event
		gPatchMgr->ProcessEvent(PatchManager::AT_EVENT_REALLOC, (unsigned long)lpUserAddr, (unsigned long)lpHeader, lRealSize);
	}
	else
	{
		lpUserAddr = (char*)::realloc(ipUserSpace, iNewSize);
	}

	return lpUserAddr;
}

void*  NewBlockWithPadding(size_t iBlockSize)
{
	void* lpResult = MallocBlockWithPadding(iBlockSize);
	if(!lpResult)
	{
		throw std::bad_alloc();
	}
	return lpResult;
}

///////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//
///////////////////////////////////////////////////////////////////////////////

// Iterate all blocks on list and verify their integrety
void AT_CheckAllBlocks()
{
	LOCK_MUTEX(&gBlockListLock);
	BlockWithPaddingHeader* lpHeader = gBlockHeaderAnchor;
	while(lpHeader)
	{
		if(SIGNATURE_MATCH(lpHeader))
		{
			CheckBlockWithPaddingHeader(lpHeader);
		}
		else
		{
			// Front byte pattern is broken, memory underrun
			gPatchMgr->ProcessMemError(lpHeader+1, PatchManager::AT_MEM_ERR_UNDERRUN);
		}
		// move to the next in-use block on list
		lpHeader = lpHeader->mNextHeader;
	}
	UNLOCK_MUTEX(&gBlockListLock);
}

// The following structure describes all blocks that share the same back trace
// f.g. The same code logic(flow of control) request memory many times
struct AggrBlocks
{
	unsigned long mCallCount;
	size_t        mTotalBytes;
	address_t*    mBackTrace;
};

// Spit out outstanding blocks on list
void AT_DumpInUseBlocks(FILE* ipFile)
{
	// title
	::fprintf(ipFile, "=========== Start of In-use Blocks ===========\n");

	LOCK_MUTEX(&gBlockListLock);

	// First consolidate blocks with same back trace
	std::vector<AggrBlocks> lBlocks;
	std::vector<AggrBlocks>::iterator it;

	BlockWithPaddingHeader* lpHeader = gBlockHeaderAnchor;
	while(lpHeader)
	{
		// Compare this block with those in consolidated vector
		bool lDuplicate = false;
		if(gPatchMgr->GetBackTraceDepth() > 0)
		{
			address_t* lpStackBuffer = (address_t*)lpHeader - gPatchMgr->GetBackTraceDepth();
			for(it=lBlocks.begin(); it!=lBlocks.end(); it++)
			{
				address_t* lpKnownStack = (*it).mBackTrace;
				bool lSameBatckTrace = true;
				// Compare stack frames
				for(int k=0; k<gPatchMgr->GetBackTraceDepth(); k++)
				{
					if(lpStackBuffer[k] != lpKnownStack[k])
					{
						lSameBatckTrace = false;
						break;
					}
				}
				if(lSameBatckTrace)
				{
					lDuplicate = true;
					(*it).mCallCount++;
					(*it).mTotalBytes += lpHeader->mUserSize;
					break;
				}
			}
		}

		if(!lDuplicate)
		{
			AggrBlocks lNewBlock;
			lNewBlock.mCallCount = 1;
			lNewBlock.mTotalBytes = lpHeader->mUserSize;
			lNewBlock.mBackTrace = (address_t*)lpHeader - gPatchMgr->GetBackTraceDepth();
			lBlocks.push_back(lNewBlock);
		}
		// move to the next in-use block on list
		lpHeader = lpHeader->mNextHeader;
	}

	unsigned long lTotalBlocks = 0;
	size_t lTotalBytes  = 0;
	// Ready to print out the consolidated results
	for(it=lBlocks.begin(); it!=lBlocks.end(); it++)
	{
		lTotalBlocks += (*it).mCallCount;
		lTotalBytes  += (*it).mTotalBytes;

		lpHeader = (BlockWithPaddingHeader*) ((*it).mBackTrace + gPatchMgr->GetBackTraceDepth());
		// block address, size
		::fprintf(ipFile, "\nAddr=0x%lx  Count=%ld  Size=%ld\n", lpHeader+1, (*it).mCallCount, (*it).mTotalBytes);
		// back trace if any
		if(gPatchMgr->GetBackTraceDepth() > 0)
		{
			for(int i=0; i<gPatchMgr->GetBackTraceDepth(); i++)
			{
				//::fprintf(ipFile, "0x%lx\n", *lpStackBuffer);
				PrintFuncName(ipFile, ((*it).mBackTrace)[i]);
			}
		}

	}

	UNLOCK_MUTEX(&gBlockListLock);

	::fprintf(ipFile, "\nTotal in-use blocks [%ld],   Total bytes [%ld]\n", lTotalBlocks, lTotalBytes);
}

