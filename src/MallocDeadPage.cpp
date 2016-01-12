/************************************************************************
** FILE NAME..... MallocDeadPage.cpp
**
** (c) COPYRIGHT
**
** FUNCTION.........
** Replacement malloc by adding protected system page before OR after user
** block. Catch overrun, underrun, dangling pointer, double free instantly
**
** NOTES............
**
**
** ASSUMPTIONS......
**
**
** RESTRICTIONS.....
** Blocks allocated by me MUST be freed by me.
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

#include "BackTrace.h"
#include "ReplacementMallocImpl.h"
#include "PatchManager.h"
#include "SysAlloc.h"


//////////////////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////////////////

#if defined(linux) || defined(sun) || defined(__hpux) || defined(_AIX)

static pthread_mutex_t gOtherBlocksLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gBlockHeaderBufferLock = PTHREAD_MUTEX_INITIALIZER;

#elif defined(WIN32)

CRITICAL_SECTION gOtherBlocksLock;
CRITICAL_SECTION gBlockHeaderBufferLock;

void AT_Init_ReplacementMalloc_cpp()
{
	::InitializeCriticalSection(&gOtherBlocksLock);
	::InitializeCriticalSection(&gBlockHeaderBufferLock);
}

void AT_Fini_ReplacementMalloc_cpp()
{
	::DeleteCriticalSection(&gOtherBlocksLock);
	::DeleteCriticalSection(&gBlockHeaderBufferLock);
}

#else
#error Unknown platform
#endif

// The list remember the raw block address of a few blocks allocated by me
// that can't fit the block header at the beginning of the same page
// because the use size is so close to page size
static std::list<void*> gOtherBlocks;

// We use a buffer to satify the need for out-of-page block headers
static BlockWithDeadPageHeader* gFreeBlockHeader = NULL;
static const size_t gBufferSize = 1024 * 1024; // Get 1MB from MM a time for the headers
static std::list<void*> gBlockHeaderBufferList;


//////////////////////////////////////////////////////////////////////////
// Help functions
//////////////////////////////////////////////////////////////////////////

// Put the out-of-page block header on free list
static void ReleaseBlockHeader(BlockWithDeadPageHeader* ipHeader)
{
	LOCK_MUTEX(&gBlockHeaderBufferLock);
	ipHeader->p.mNextHeader = gFreeBlockHeader;
	gFreeBlockHeader = ipHeader;
	UNLOCK_MUTEX(&gBlockHeaderBufferLock);
}

// Get one out-of-page block header from free list
static BlockWithDeadPageHeader* GetBlockHeader()
{
	BlockWithDeadPageHeader* lpResult = NULL;

	LOCK_MUTEX(&gBlockHeaderBufferLock);
	if(!gFreeBlockHeader)
	{
		BlockWithDeadPageHeader* lpBuffer;
		// There is not free block header available
		// Get a big chunck of buffer from MM and use it for following needs
		// Note: We don't intend to release these buffers for the life of process
		if(lpBuffer = (BlockWithDeadPageHeader*)::malloc(gBufferSize))
		{
			gFreeBlockHeader = lpBuffer;
			// Remember the memory directly got from MM
			gBlockHeaderBufferList.push_front(gFreeBlockHeader);
			// split the buffer into pieces and put them on free list.
			BlockWithDeadPageHeader* lpLastHeader = (BlockWithDeadPageHeader*)
									((char*)lpBuffer + gBufferSize - sizeof(BlockWithDeadPageHeader));
			do
			{
				lpBuffer->p.mNextHeader = lpBuffer+1;
				lpBuffer++;
			} while (lpBuffer+1 <= lpLastHeader);
			lpBuffer->p.mNextHeader = NULL;

			UNLOCK_MUTEX(&gBlockHeaderBufferLock);
			// It is convenient to recurse here.
			return GetBlockHeader();
		}
		else
		{
			OUTPUT_MSG1(AT_FATAL, "Can't get %ld bytes memory from system\n", gBufferSize);
			return NULL;
		}
	}
	else
	{
		lpResult = gFreeBlockHeader;
		gFreeBlockHeader = gFreeBlockHeader->p.mNextHeader;
	}
	UNLOCK_MUTEX(&gBlockHeaderBufferLock);
	return lpResult;
}

// Check if the block was allocated by me
// Return true if yes and update RawBlock and DeatPageAddr accordingly
static bool VerifyBlockWithDeadPage(void*  ipUserSpace,
									void*& orRawBlock,
									void*& orDeadPageAddr,
									size_t& orSize,
									size_t& orRawSize)
{
	bool rc = false;

	// Get the settings
	size_t lPageSize = gPatchMgr->GetPageSize();
	bool   lbCheckUnderrun = gPatchMgr->CheckUnderrun();
	size_t lPageMask = ~(lPageSize - 1);

	BlockWithDeadPageHeader* lpHeader;
	char* lpRawBlockCandidate;

	if(lbCheckUnderrun)
	{
		// block is prepared for under run check
		// user address has to be on page boundary if it is allocated by me
		if((size_t)ipUserSpace & (lPageSize-1))
		{
			// Not aligned on page, no chance it is my block
		}
		else
		{
			// user space is aligned on page
			// There is a good chance that it is my block, except
			// (1) it is allocated before patching
			// (2) it is allocated by other unpatched module
			//     and turned out ask being freed by me (none self-contained)
			lpRawBlockCandidate = (char*)ipUserSpace - lPageSize;
			// The only possible place of the block header is at the bottom of the same page
			// as the user space starting page.
			lpHeader = (BlockWithDeadPageHeader*)
				((char*)ipUserSpace+lPageSize-sizeof(BlockWithDeadPageHeader));

			// Releasing an already freed block
			// !FIX!
			// We may peek into user space and generate false alarm
			//		e.g., a small memory is allocated and released, the page with have the "FREEBLK!" marker at
			//		the end of the page. The page could be reallocated later on. If the new memory request is 4KB (page size)
			//		the "FREEBLK!" marker is passed to user space. If application doesn't modify this marker before
			//		freeing this block, we are tricked into believing this is a DOUBLE_FREE
			//
			//		A potential fix is to clean the marker when a new page is allocated from cache
			//
			//if (FREE_BLOCK(lpHeader))
			//{
			//	gPatchMgr->ProcessMemError(ipUserSpace, PatchManager::AT_MEM_ERR_DOUBLE_FREE);
			//	return false;
			//}

			// Check signature and size
			if(IS_MY_BLOCK(lpHeader,ipUserSpace,lpRawBlockCandidate))
			{
				// Hooray, it is mine after all
				orDeadPageAddr = orRawBlock = lpRawBlockCandidate;
				orSize = lpHeader->mUserSize;
				orRawSize = (lPageSize + orSize + lPageSize - 1) & lPageMask;
				rc = true;
				// Clean up stuff to avoid weird problem
				GROUND_HEADER(lpHeader);
			}
			else
			{
				// signature doesn't match, check global list as the last resort
				std::list<void*>::iterator it;
				LOCK_MUTEX(&gOtherBlocksLock);
				for(it=gOtherBlocks.begin(); it!=gOtherBlocks.end(); it++)
				{
					lpHeader = (BlockWithDeadPageHeader*)(*it);
					if(IS_MY_BLOCK(lpHeader,ipUserSpace,lpRawBlockCandidate))
					{
						orDeadPageAddr = orRawBlock = lpRawBlockCandidate;
						orSize = lpHeader->mUserSize;
						orRawSize = (lPageSize + orSize + lPageSize - 1) & lPageMask;
						rc = true;
						// free the out of page header
						// Clean up stuff to avoid weird problem
						GROUND_HEADER(lpHeader);
						gOtherBlocks.erase(it);
						ReleaseBlockHeader(lpHeader);
						break;
					}
				}
				UNLOCK_MUTEX(&gOtherBlocksLock);
			}
		}
	}
	else
	{
		// block is protected for over run
		// First find the place we stash our header which is the closest page boundary
		lpHeader = (BlockWithDeadPageHeader*)((size_t)ipUserSpace & lPageMask);
		lpRawBlockCandidate = (char*)lpHeader;
		// Is there enough space for the header at all
		if((char*)ipUserSpace - (char*)lpHeader >= sizeof(BlockWithDeadPageHeader))
		{
			// Releasing an already freed block
			if (FREE_BLOCK(lpHeader))
			{
				OUTPUT_MSG1(AT_FATAL, "Block 0x%lx is double freed\n", ipUserSpace);
				::abort();
			}

			// There is enough space, now check the signature
			if(IS_MY_BLOCK(lpHeader,ipUserSpace,lpRawBlockCandidate))
			{
				// Hooray, it is mine after all
				orRawBlock = lpRawBlockCandidate;
				orSize = lpHeader->mUserSize;
				orRawSize = (lPageSize + orSize + lPageSize - 1) & lPageMask;
				orDeadPageAddr = (char*)orRawBlock + orRawSize - lPageSize;
				rc = true;
				// Clean up stuff to avoid weird problem
				GROUND_HEADER(lpHeader);
			}
			else
			{
				// signature doesn't match, not mine
			}
		}
		else
		{
			// The header is impossible to fit in the same page as user space.
			// Check our special block container
			std::list<void*>::iterator it;
			LOCK_MUTEX(&gOtherBlocksLock);
			if (gOtherBlocks.size() > 0)
			{
				for (it=gOtherBlocks.begin(); it!=gOtherBlocks.end(); it++)
				{
					lpHeader = (BlockWithDeadPageHeader*)(*it);
					if(IS_MY_BLOCK(lpHeader,ipUserSpace,lpRawBlockCandidate))
					{
						orRawBlock = lpRawBlockCandidate;
						orSize = lpHeader->mUserSize;
						orRawSize = (lPageSize + orSize + lPageSize - 1) & lPageMask;
						orDeadPageAddr = (char*)orRawBlock + orRawSize - lPageSize;
						rc = true;
						// clear this list node
						// Clean up stuff to avoid weird problem
						GROUND_HEADER(lpHeader);
						gOtherBlocks.erase(it);
						// free the out of page header
						ReleaseBlockHeader(lpHeader);
						break;
					}
				}
			}
			UNLOCK_MUTEX(&gOtherBlocksLock);
		}
	}
	return rc;
}

//////////////////////////////////////////////////////////////////////////
// Replacement routines
//////////////////////////////////////////////////////////////////////////

void*  ReallocBlockWithDeadPage(void* ipUserSpace, size_t iNewSize)
{
	void* lpResult = NULL;
	// Old block is nil, just allocate a new one
	if(!ipUserSpace)
	{
		return MallocBlockWithDeadPage(iNewSize);
	}

	// Check if the old block is mine
	size_t lPageSize = gPatchMgr->GetPageSize();
	void* lpDeadPageAddr = NULL;
	void* lpRawBlock = ipUserSpace;
	size_t lOldSize = 0;
	size_t lRawSize = 0;
	if(VerifyBlockWithDeadPage(ipUserSpace, lpRawBlock, lpDeadPageAddr, lOldSize, lRawSize))
	{
		// It is my block
		// I shall expand the original block if all possible
		// !!FIX!! For now, I will just create another one for simplicty
		size_t lCopySize = iNewSize;
		if(lOldSize < iNewSize)
		{
			lCopySize = lOldSize;
		}
		if(lpResult = MallocBlockWithDeadPage(iNewSize))
		{
			::memcpy(lpResult, ipUserSpace, lCopySize);
		}

		// Release the block, we may choose to cache it for later use
		BlockWithDeadPageHeader* lpHeader;
		if(lpDeadPageAddr == lpRawBlock)
		{
			lpHeader = (BlockWithDeadPageHeader*) ((char*)lpRawBlock + lPageSize);
		}
		else
		{
			lpHeader = (BlockWithDeadPageHeader*) lpRawBlock;
		}

		ReleasePages(lpRawBlock, lRawSize, lpHeader, lpDeadPageAddr);

		gPatchMgr->ProcessEvent(PatchManager::AT_EVENT_FREE, (unsigned long)ipUserSpace, (unsigned long)lpRawBlock, lRawSize);
	}
	else
	{
		// The original block was allocated by default malloc
		lpResult = ::realloc(ipUserSpace, iNewSize);
	}

	OUTPUT_MSG2(AT_ALL, "ReallocBlockWithDeadPage %d bytes at 0x%lx successfully\n", lOldSize, ipUserSpace);

	return lpResult;
}

void  FreeBlockWithDeadPage(void* ipUserSpace)
{
	// Protect
	if(!ipUserSpace)
	{
		//if(gPatchMgr->ReportFreeNull())
		//{
		//	OUTPUT_MSG0(ERROR, "Free null memory block.\n");
		//}
		return;
	}

	size_t lPageSize = gPatchMgr->GetPageSize();
	void* lpDeadPageAddr = NULL;
	void* lpRawBlock = ipUserSpace;
	size_t lSize = 0;
	size_t lRawSize = 0;

	// The most tricky thing is to verify that the block is realy mine
	if(VerifyBlockWithDeadPage(ipUserSpace, lpRawBlock, lpDeadPageAddr, lSize, lRawSize))
	{
		// Release the block, we may choose to cache it for later use
		BlockWithDeadPageHeader* lpHeader;
		if(lpDeadPageAddr == lpRawBlock)
		{
			lpHeader = (BlockWithDeadPageHeader*) ((char*)lpRawBlock + lPageSize);
		}
		else
		{
			lpHeader = (BlockWithDeadPageHeader*) lpRawBlock;
		}
		ReleasePages(lpRawBlock, lRawSize, lpHeader, lpDeadPageAddr);
	}
	else
	{
		// The block is directly allocated by default memory allocator
#ifdef WIN32
		if (_CrtIsValidHeapPointer(lpRawBlock))
#endif
			::free(lpRawBlock);
	}

	gPatchMgr->ProcessEvent(PatchManager::AT_EVENT_FREE, (unsigned long)ipUserSpace, (unsigned long)lpRawBlock, lRawSize);

	OUTPUT_MSG2(AT_ALL, "FreeBlockWithDeadPage %d bytes at 0x%lx successfully\n", lSize, ipUserSpace);
}

void*  CallocBlockWithDeadPage(size_t iNumElement, size_t iElementSize)
{
	size_t lTotalSize = iNumElement*iElementSize;
	void* lpResult = MallocBlockWithDeadPage(lTotalSize);
	if(lpResult && lTotalSize)
	{
		::memset(lpResult, 0, lTotalSize);
	}
	return lpResult;
}

void*  MallocBlockWithDeadPage(size_t iSize)
{
	// If size==0. we will return the  address of dead page instead of NULL

	if (iSize < gPatchMgr->GetMinBlockSize() || iSize > gPatchMgr->GetMaxBlockSize())
	{
		void* lpUserAddr = ::malloc(iSize);
		if (lpUserAddr)
		{
			gPatchMgr->ProcessEvent(PatchManager::AT_EVENT_MALLOC, (unsigned long)lpUserAddr, (unsigned long)lpUserAddr, iSize);
		}
		return lpUserAddr;
	}

	// Fetch the specifications
	unsigned int lAlignment = gPatchMgr->GetAlignment();
	size_t lPageSize = gPatchMgr->GetPageSize();
	bool   lbCheckUnderrun = gPatchMgr->CheckUnderrun();

	size_t Remainder;
	// Adjust the requested size
	//
	// Alignment, for overrun check only
	if(!lbCheckUnderrun && lAlignment>1)
	{
		Remainder = iSize % lAlignment;
		if(Remainder > 0)
		{
			iSize += lAlignment - Remainder;
		}
	}
	// And add the dead page
	size_t lRealSize = iSize + lPageSize;
	// Round up to page
	Remainder = lRealSize % lPageSize;
	if(Remainder > 0)
	{
		lRealSize += lPageSize - Remainder;
	}

	// At this point, lRealSize must be multiple of system page size
	// The returned block should have dead page set properly alreday.
	void* lpRawBlock = GetPages(lRealSize, lbCheckUnderrun);

	if(!lpRawBlock)
	{
		return NULL;
	}

	char* lpUserAddr;
	BlockWithDeadPageHeader* lpHeader = NULL;
	bool lbUseEmbeddedHeader = false;
	// Default, check overrun
	if(!lbCheckUnderrun)
	{
		// Get proper user address
		lpUserAddr = (char*)lpRawBlock + lRealSize - lPageSize - iSize;
		// Set up the header
		if(lpUserAddr - (char*)lpRawBlock >= sizeof(BlockWithDeadPageHeader))
		{
			// We got enough space to stash the header at the very beginning of the block
			// which is also on the immediate page boundary of user space
			lpHeader = (BlockWithDeadPageHeader*)lpRawBlock;
			lbUseEmbeddedHeader = true;
		}
	}
	// Otherwise check underrun
	else
	{
		// Get proper user address
		lpUserAddr = (char*)lpRawBlock + lPageSize;
		// Set up the header
		// Since we have to put header in the same page as user space starting address,
		// otherwise when we free, we have to access other page which may not belong to the process.
		// This means that we can only embed header if use size is small enough (fit in one page)
		if(lPageSize>iSize && lRealSize-lPageSize-iSize>=sizeof(BlockWithDeadPageHeader))
		{
			// We got enough space to stash the header at the bottom of the page
			lpHeader = (BlockWithDeadPageHeader*)(lpUserAddr+lPageSize-sizeof(BlockWithDeadPageHeader));
			lbUseEmbeddedHeader = true;
		}
	}

	if(!lbUseEmbeddedHeader)
	{
		// Oh no, we don't have the space for the header
		// allocate a out-of-page header object
		lpHeader = GetBlockHeader();
		// Remember the address in a global object so that we can free it later
		LOCK_MUTEX(&gOtherBlocksLock);
		gOtherBlocks.push_back(lpHeader);
		UNLOCK_MUTEX(&gOtherBlocksLock);
	}

	// Now, the header pointer should point to either in-page or out-page header structure
	lpHeader->mRawBlockAddr = lpRawBlock;
	lpHeader->p.mUserAddr = lpUserAddr;
	lpHeader->mUserSize = iSize;
	SET_SIGNATURE(lpHeader);

	OUTPUT_MSG2(AT_ALL, "MallocBlockWithDeadPage allocates %d bytes at 0x%lx successfully\n", iSize, lpUserAddr);

	gPatchMgr->ProcessEvent(PatchManager::AT_EVENT_MALLOC, (unsigned long)lpUserAddr, (unsigned long)lpRawBlock, lRealSize);
	return lpUserAddr;
}

void*  NewBlockWithDeadPage(size_t iBlockSize)
{
	void* lpResult = MallocBlockWithDeadPage(iBlockSize);
	if(!lpResult)
	{
		throw std::bad_alloc();
	}
	return lpResult;
}

//void  DeleteBlockWithDeadPage(void* ipUserSpace)
//{
//	return FreeBlockWithDeadPage(ipUserSpace);
//}
