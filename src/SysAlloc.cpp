/************************************************************************
** FILE NAME..... SysAlloc.cpp
**
** (c) COPYRIGHT
**
** FUNCTION......... Allocate and manage system page
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
#include <malloc.h>
#else
#include <pthread.h>
#include <sys/mman.h>
#endif

#include "CrossPlatform.h"
#include "ReplacementMallocImpl.h"
#include "SysAlloc.h"
#include "PatchManager.h"

//////////////////////////////////////////////////////
// Macros to define how various platforms
// get system page:
// either mmap or valloc
//////////////////////////////////////////////////////
#if defined(_AIX) || defined(__hpux) || defined(sun) || defined(linux)

// AIX says
// "The behavior of mprotect is unspecified if the mapping was not established by a call to the mmap subroutine"
// Hence we have to use mmap to get the pages.
#define GET_SYSTEM_PAGE(iBlockSize,iPageSize) System_mmap((iBlockSize))
#define RELEASE_SYSTEM_PAGE(ipRawBlock, iBlockSize) System_munmmap((ipRawBlock), (iBlockSize))
//#define GET_SYSTEM_PAGE(iBlockSize) valloc((iBlockSize))
//#define RELEASE_SYSTEM_PAGE(ipRawBlock, iBlockSize) free((ipRawBlock))

static pthread_mutex_t gCacheBlockListLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gFreeBlockTagLock = PTHREAD_MUTEX_INITIALIZER;

#elif defined(WIN32)

#define GET_SYSTEM_PAGE(iBlockSize,iPageSize) System_VirtualAlloc((iBlockSize))
#define RELEASE_SYSTEM_PAGE(ipRawBlock, iBlockSize) System_VirtualFree((ipRawBlock), (iBlockSize))
//extern "C" void * _aligned_malloc(size_t size, size_t alignment);
//extern "C" void _aligned_free (void *memblock);
//#define GET_SYSTEM_PAGE(iBlockSize,iPageSize) _aligned_malloc((iBlockSize),(iPageSize))
//#define RELEASE_SYSTEM_PAGE(ipRawBlock, iBlockSize) _aligned_free((ipRawBlock))

CRITICAL_SECTION gCacheBlockListLock;
CRITICAL_SECTION gFreeBlockTagLock;

CRITICAL_SECTION gSystemChunkLock;

// Init routines called when dll is attached to process
void AT_Init_SysAlloc_cpp()
{
	::InitializeCriticalSection(&gCacheBlockListLock);
	::InitializeCriticalSection(&gFreeBlockTagLock);
	::InitializeCriticalSection(&gSystemChunkLock);
}

void AT_Fini_SysAlloc_cpp()
{
	::DeleteCriticalSection(&gCacheBlockListLock);
	::DeleteCriticalSection(&gFreeBlockTagLock);
	::DeleteCriticalSection(&gSystemChunkLock);
}

#endif

// This is the link node for freed blocks that are check for invalid access
// Because these pages are set dead to trap invalid access, the link node
// has to use additional buffer.
struct FreeBlock
{
	FreeBlock* mpNextFreeBlock;
	void* mpRawAddr;
};

//////////////////////////////////////////////////////////
// Relessed blocks are cached on this single linked list
//////////////////////////////////////////////////////////

// No access free checking
// link list node embedded in free page
// cached blocks are reused in LIFO order
static BlockWithDeadPageHeader* gCacheBlockHead = NULL;

// Check access free, released blocks are set dead.
// link nodes have to use out-of-page buffer.
// When maximum number reached, cached blocks are resued in FIFO order
static FreeBlock* gCacheBlockTagHead = NULL;
static FreeBlock* gCacheBlockTagTail = NULL;

// Link list size of cached free blocks
static unsigned long gBlocksInCache = 0;

// In case we want to check invalid access of freed block
// the released blocks on cached list are protected by PROT_NONE
// Hence embeding linking pointers inside the page is impossible.
// So we use a dedicated buffer for out-of-page block headers
static FreeBlock* gFreeBlockTagHead = NULL;
static const size_t gTagBufferSize = 64 * 1024; // Get 64KBytes from MM a time for the buffer

// Put the out-of-page block header on free list
static void ReleaseFreeBlockTag(FreeBlock* ipFreeBlockTag)
{
	LOCK_MUTEX(&gFreeBlockTagLock);
	ipFreeBlockTag->mpNextFreeBlock = gFreeBlockTagHead;
	gFreeBlockTagHead = ipFreeBlockTag;
	UNLOCK_MUTEX(&gFreeBlockTagLock);
}

// Get one out-of-page block header from free list
static FreeBlock* GetFreeBlockTag()
{
	FreeBlock* lpResult = NULL;

	LOCK_MUTEX(&gFreeBlockTagLock);
	if(!gFreeBlockTagHead)
	{
		FreeBlock* lpBuffer;
		// There is not free block header available
		// Get a big chunck of buffer from MM and use it for following needs
		// Note: We don't intend to release these buffers for the life of process
		if(lpBuffer = (FreeBlock*)::malloc(gTagBufferSize))
		{
			gFreeBlockTagHead = lpBuffer;
			// Remember the memory directly got from MM
			// split the buffer into pieces and put them on free list.
			FreeBlock* lpLastTag = (FreeBlock*)((char*)lpBuffer + gTagBufferSize - sizeof(FreeBlock));
			do
			{
				lpBuffer->mpNextFreeBlock = lpBuffer+1;
				lpBuffer++;
			} while (lpBuffer+1 <= lpLastTag);
			lpBuffer->mpNextFreeBlock = NULL;

			UNLOCK_MUTEX(&gFreeBlockTagLock);
			// It is convenient to recurse here.
			return GetFreeBlockTag();
		}
		else
		{
			OUTPUT_MSG1(AT_FATAL, "Can't get %ld bytes memory from system\n", gTagBufferSize);
		}
	}
	else
	{
		lpResult = gFreeBlockTagHead;
		gFreeBlockTagHead = gFreeBlockTagHead->mpNextFreeBlock;
	}
	UNLOCK_MUTEX(&gFreeBlockTagLock);
	return lpResult;
}

#if defined(_AIX) || defined(__hpux) || defined(sun) || defined(linux)

// Create mmaped pages
// Assume anonymous mmap is allowed
static void* System_mmap(size_t iBlockSize)
{
	void* lpResult;

	lpResult = ::mmap(NULL, iBlockSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

	if(lpResult == (void*)-1)
	{
		static int gNumWarning = 0;
		if(gNumWarning == 0)
		{
			gNumWarning++;
			fprintf(stderr, "Failed to mmap a block of size [%ld] bytes\n", iBlockSize);
			OUTPUT_MSG1(AT_FATAL, "Failed to mmap a block of size [%ld] bytes\n", iBlockSize);
		}
		return NULL;
	}

	// mmap shall return page aligned memory region.
	size_t lPageSize = gPatchMgr->GetPageSize();
	if((unsigned long)lpResult & (lPageSize-1))
	{
		fprintf(stderr, "!Panic: mmap returns an region that is not on page boundary\n");
		::abort();
	}

	return lpResult;
}

// This is the counterpart of SystemMMap
static void System_munmmap(void* ipRawBlock, size_t iLength)
{
	int lRC = ::munmap((char*)ipRawBlock, iLength);

	if(lRC == -1)
	{
		fprintf(stderr, "!Panic: munmap region 0x%lx with length %ld bytes failed\n", ipRawBlock, iLength);
	}
}

#elif defined(WIN32)

////////////////////////////////////////////////////////////////
// VirtualAlloc ALWAYS returns 64KB aligned memory
// Assume page size is 4KB
//
// !!NOTICE!! be sure the assumption is true !!NOTICE!!
////////////////////////////////////////////////////////////////
#define VIRTUAL_GRANULARITY 0x10000ul
#define VIRTUAL_MASK        (VIRTUAL_GRANULARITY-1)
#define SYS_PAGE_SZ         0x1000ul

struct SysChunkHeader
{
	void* mpChunkBaseAddr;
	// 16 pages of 64KB chunk can be mapped with 16bit
	// A set bit means the corresponding pages is in use
	unsigned short mInUsePageMap;
};

// Array of 64KB chunk headers, it is very CPU cache conscious
static SysChunkHeader* gAllChunks = NULL;
// Total number of 64kb chunks in use
static size_t gNumChunks = 0;
// The maximum chunk headers avaible
static size_t gHeaderBufSz = 0;

// Find a header to manage this new system chunk
static SysChunkHeader* AddSystemChunk(void* ipChunk, size_t iRequestSize)
{
	SysChunkHeader* lpChunkHdr = NULL;

	LOCK_MUTEX(&gSystemChunkLock);
	// Find a header slot
	for (size_t i=0; i<gHeaderBufSz; i++)
	{
		if (gAllChunks[i].mpChunkBaseAddr == NULL)
		{
			lpChunkHdr = &gAllChunks[i];
			break;
		}
	}
	// Create more buffer if current buffer is exhausted
	if (lpChunkHdr == NULL)
	{
		size_t lNewBufBytes = gHeaderBufSz * sizeof(SysChunkHeader) + VIRTUAL_GRANULARITY;
		lNewBufBytes = (lNewBufBytes + VIRTUAL_MASK) & (~VIRTUAL_MASK);
		SysChunkHeader* lNewHdrs = (SysChunkHeader*) ::VirtualAlloc(NULL, lNewBufBytes, MEM_COMMIT, PAGE_READWRITE);
		if (lNewHdrs == NULL)
		{
			return NULL;
		}
		else if (gHeaderBufSz > 0)
		{
			memcpy(lNewHdrs, gAllChunks, gHeaderBufSz * sizeof(SysChunkHeader));
			::VirtualFree(gAllChunks, 0, MEM_RELEASE);
		}
		gAllChunks = lNewHdrs;
		gHeaderBufSz = lNewBufBytes / sizeof(SysChunkHeader);
		lpChunkHdr = &gAllChunks[0];
	}
	// Now we have a free header slot
	lpChunkHdr->mpChunkBaseAddr = ipChunk;
	lpChunkHdr->mInUsePageMap = (1 << (iRequestSize/SYS_PAGE_SZ)) - 1;

	UNLOCK_MUTEX(&gSystemChunkLock);
	return lpChunkHdr;
}

// Search all small chunks to see if there are contiguous free pages of desired size
static void* GetReservedPages(size_t iBlockSize)
{
	void* result = NULL;
	// 0001 for one page, 0011 for two pages, 0111 for three pages
	unsigned int bitPatten = (1 << (iBlockSize/SYS_PAGE_SZ)) - 1;
	// search all chunks that have free pages
	size_t lNumChunks = 0;

	LOCK_MUTEX(&gSystemChunkLock);

	for (size_t i=0; i<gHeaderBufSz && lNumChunks<gNumChunks; i++)
	{
		// This header owns a chunk
		if (gAllChunks[i].mpChunkBaseAddr)
		{
			lNumChunks++;
			// The chunk has free pages
			if (gAllChunks[i].mInUsePageMap < 0xFFFF)
			{
				register unsigned short FreePageMap = ~gAllChunks[i].mInUsePageMap;
				for(int k=0; k<16; k++)
				{
					// If there are enough contiguous pages
					if ((FreePageMap & bitPatten) == bitPatten)
					{
						result = (char*)gAllChunks[i].mpChunkBaseAddr + SYS_PAGE_SZ * k;
						// Set bits that are returned.
						gAllChunks[i].mInUsePageMap |= (bitPatten << k);
						// done
						return result;
					}
					FreePageMap >>= 1;
				}
			}
		}
	}

	UNLOCK_MUTEX(&gSystemChunkLock);

	return result;
}

// Some pages of a small chunk are freed
// if the whole small chunk become free, return it back to caller,
// Return chunk base if all pages of it are free.
// Otherwise, returns NULL.
static void* FreeSmallChunkPages(void* ipPageAddr, size_t iLength)
{
	void* result = NULL;
	void* lpBase = (void*) ((unsigned long)ipPageAddr & ~VIRTUAL_MASK);

	LOCK_MUTEX(&gSystemChunkLock);

	// !!FIX!! Linear search maybe too slow
	for (size_t i=0; i<gHeaderBufSz; i++)
	{
		if (gAllChunks[i].mpChunkBaseAddr == lpBase)
		{
			unsigned int offset = ((unsigned long)ipPageAddr - (unsigned long)lpBase) / SYS_PAGE_SZ;
			unsigned int bitPatten = (1 << (iLength/SYS_PAGE_SZ)) - 1;
			gAllChunks[i].mInUsePageMap &= ~(bitPatten << offset);
			if (gAllChunks[i].mInUsePageMap == 0)
			{
				// The whole chunk is free
				gAllChunks[i].mpChunkBaseAddr = 0;
				result = lpBase;
			}
			break;
		}
	}

	UNLOCK_MUTEX(&gSystemChunkLock);

	return result;
}

static void* System_VirtualAlloc(size_t iBlockSize)
{
	void* result = NULL;

	// For small chunk, search free chunks for reserved pages
	if (iBlockSize < VIRTUAL_GRANULARITY)
	{
		result = GetReservedPages(iBlockSize);
		// If no free pages for small chunk, get from OS
		if (result == NULL)
		{
			result = ::VirtualAlloc(NULL, VIRTUAL_GRANULARITY, MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE);
			if (result)
				AddSystemChunk(result, iBlockSize);
			else
				return NULL;
		}
		// commit just enough memory
		if (NULL == ::VirtualAlloc(result, iBlockSize, MEM_COMMIT, PAGE_READWRITE))
		{
			fprintf(stderr, "!Panic: Can't commit %ld pages at 0x%lx  fails with error code: %ld\n",
				iBlockSize/SYS_PAGE_SZ, result, GetLastError());
			return NULL;
		}
	}
	// Big chunk always get directly from VMM
	else
	{
		result = ::VirtualAlloc(NULL, iBlockSize, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
	}

	return result;
}

static void System_VirtualFree(void* ipRawBlock, size_t iLength)
{
	void* ipChunkBase = ipRawBlock;

	// Small chunk < 64KB
	if (iLength < VIRTUAL_GRANULARITY)
	{
		// if chunk manager decides to release the whole chunk back to OS,
		// it should return its base. Otherwise, it returns NULL here.
		ipChunkBase = FreeSmallChunkPages(ipRawBlock, iLength);
	}
	// It is either Big chink >= 64KB or whole small chunk, return to OS
	if (ipChunkBase)
	{
		// On Window, return 0 means failure.
		if(0 == ::VirtualFree(ipChunkBase, 0, MEM_RELEASE))
		{
			fprintf(stderr, "!Panic: VirtualFree fails with error code: %ld\n", GetLastError());
		}
	}
}

#else
#error "PLATFORM NOT SUPPORTED"
#endif // Unix platforms

// Retrive call stack and write into buffer
static void FillBackTracePage(void* ipAddr, int nFrames, int skipFrames)
{
	// Embed a short string in the dead page for identification
	StampDeadPage(ipAddr);
	if (nFrames > 0)
	{
		GetBackTrace((address_t*)ipAddr+1, nFrames, skipFrames);
	}
}

// Get the memory pages from default MM or my cached blocks
// Handle the dead page too, since the cached one retain the original dead page.
void* GetPages(size_t iBlockSize, bool ibSetFirstPageDead)
{
	void* lpRawBlock = NULL;
	size_t lPageSize = gPatchMgr->GetPageSize();
	// Check my cache list if user requests two pages
	if( (iBlockSize>>1) == lPageSize)
	{
		LOCK_MUTEX(&gCacheBlockListLock);

		// In order to check access free error, we need to keep configured number of blocks in cache.
		// Reuse them in FIFO order and only when max cached number is reached.
		if(gPatchMgr->CheckAccessFreedBlock() && gBlocksInCache >= gPatchMgr->GetMaxCachedBlocks())
		{
			lpRawBlock = gCacheBlockTagHead->mpRawAddr;
			// Need to relax the protection of user page if check_freed is on
			void* lpUserPage = ibSetFirstPageDead ? ((char*)lpRawBlock+lPageSize) : lpRawBlock;
			// When Linux kernel limit vm.max_map_count is exceeded,
			// we can't set the page protection bit (which will split the segment into two)
			if(SetPageLive((char*)lpUserPage, lPageSize) == false)
			{
				OUTPUT_MSG1(AT_FATAL, "Failed to set page at 0x%lx read/write\n", lpUserPage);
				lpRawBlock = NULL;
			}
			else
			{
				gBlocksInCache--;

				FreeBlock* lpCurrentTag = gCacheBlockTagHead;
				if(gCacheBlockTagHead == gCacheBlockTagTail)
				{
					gCacheBlockTagHead = gCacheBlockTagTail = NULL;
				}
				else
				{
					gCacheBlockTagHead = gCacheBlockTagHead->mpNextFreeBlock;
				}
				// Return it to the tag pool
				ReleaseFreeBlockTag(lpCurrentTag);
			}
		}
		// If don't want to check access free, reuse cached block if there is one.
		else if(gCacheBlockHead)
		{
			lpRawBlock = gCacheBlockHead->mRawBlockAddr;
			gCacheBlockHead = gCacheBlockHead->p.mNextHeader;
			gBlocksInCache--;
		}
		UNLOCK_MUTEX(&gCacheBlockListLock);

		if(lpRawBlock)
		{
			// User request back trace
			if (gPatchMgr->GetBackTraceDepth() > 0 && !gPatchMgr->GetBackTraceFree())
			{
				// change dead page to live first
				void* lpDeadPageAddr = ibSetFirstPageDead ? lpRawBlock : ((char*)lpRawBlock + lPageSize);
				if(SetPageLive((char*)lpDeadPageAddr, lPageSize) == false)
				{
					OUTPUT_MSG1(AT_FATAL, "Failed to set page at 0x%lx read/write\n", lpDeadPageAddr);
				}
				else
				{
					FillBackTracePage(lpDeadPageAddr, gPatchMgr->GetBackTraceDepth(), 3);
					if(SetPageDead(lpDeadPageAddr, lPageSize) == false)
					{
						OUTPUT_MSG1(AT_FATAL, "Failed to set page at 0x%lx to non-access\n", lpDeadPageAddr);
					}
				}
			}
			return lpRawBlock;
		}
	}

	// Otherwise get memory from default MM
	lpRawBlock = GET_SYSTEM_PAGE(iBlockSize,lPageSize);

	if(!lpRawBlock)
	{
		// Allocation failed,
		OUTPUT_MSG1(AT_FATAL, "Failed to get a block of size [%ld] bytes from kernel VMM\n", iBlockSize);
	}
	else
	{
		// Set the dead page
		// Does it make sense to release RAM/swap by calling madvise ????
		void* lpDeadPageAddr = ibSetFirstPageDead ? lpRawBlock : ((char*)lpRawBlock + iBlockSize - lPageSize);

		// User request back trace
		if(gPatchMgr->GetBackTraceDepth() > 0 && !gPatchMgr->GetBackTraceFree())
		{
			FillBackTracePage(lpDeadPageAddr, gPatchMgr->GetBackTraceDepth(), 3);
		}

		if(SetPageDead(lpDeadPageAddr, lPageSize) == false)
		{
			OUTPUT_MSG1(AT_FATAL, "Failed to set page at 0x%lx to non-access\n", lpDeadPageAddr);
		}
	}

	return lpRawBlock;
}

void  ReleasePages(void* ipRawBlock, size_t iBlockSize, BlockWithDeadPageHeader* ipHeader, void* ipDeadPageAddr)
{
	size_t lPageSize = gPatchMgr->GetPageSize();
	bool lbReleaseToVMM = true;

	// We cache blocks with two system pages (one for user, one for dead page)
	// This should meet majority of memory requests, which are <= pagesize
	if( (iBlockSize>>1) == lPageSize)
	{
		ipHeader->mRawBlockAddr = ipRawBlock;
		ipHeader->p.mNextHeader = NULL;

		// User request back trace
		if(gPatchMgr->GetBackTraceDepth() > 0 && gPatchMgr->GetBackTraceFree())
		{
			if(SetPageLive((char*)ipDeadPageAddr, lPageSize) == false)
			{
				OUTPUT_MSG1(AT_FATAL, "Failed to set page at 0x%lx read/write\n", ipDeadPageAddr);
			}
			else
			{
				FillBackTracePage(ipDeadPageAddr, gPatchMgr->GetBackTraceDepth(), 3);
				if(SetPageDead(ipDeadPageAddr, lPageSize) == false)
				{
					OUTPUT_MSG1(AT_FATAL, "Failed to set page at 0x%lx to non-access\n", ipDeadPageAddr);
				}
			}
		}

		// User choose to debug invalid read/write of freed blocks,
		if(gPatchMgr->CheckAccessFreedBlock())
		{
			size_t lPageMask = ~(lPageSize - 1);
			size_t lpUserPage = (size_t)ipHeader & lPageMask;
			// Set user page to PROT_NONE (dead)
			// remember to reset it back to PROT_READ|PROT_WRITE when we reuse it
			if(SetPageDead((char*)lpUserPage, lPageSize) == false)
			{
				OUTPUT_MSG1(AT_FATAL, "Failed to mprotect page at 0x%lx\n", lpUserPage);
			}

			LOCK_MUTEX(&gCacheBlockListLock);

			// Push the freed block at the end of link list
			FreeBlock* lpFreeBlockTag = GetFreeBlockTag();
			if(lpFreeBlockTag)
			{
				lpFreeBlockTag->mpRawAddr = ipRawBlock;
				lpFreeBlockTag->mpNextFreeBlock = NULL;
				if(gCacheBlockTagTail)
				{
					gCacheBlockTagTail->mpNextFreeBlock = lpFreeBlockTag;
				}
				gCacheBlockTagTail = lpFreeBlockTag;
				if(!gCacheBlockTagHead)
				{
					gCacheBlockTagHead = lpFreeBlockTag;
				}
				gBlocksInCache++;
				lbReleaseToVMM = false;
			}

			// There are enough cached blocks already,
			// release the oldest one which is the front of the link list
			if (gBlocksInCache > gPatchMgr->GetMaxCachedBlocks())
			{
				ipHeader = NULL;
				ipRawBlock = gCacheBlockTagHead->mpRawAddr;
				gBlocksInCache--;
				lbReleaseToVMM = true;

				FreeBlock* lpCurrentTag = gCacheBlockTagHead;
				if(gCacheBlockTagHead == gCacheBlockTagTail)
				{
					gCacheBlockTagHead = gCacheBlockTagTail = NULL;
				}
				else
				{
					gCacheBlockTagHead = gCacheBlockTagHead->mpNextFreeBlock;
				}
				// Return it to the tag pool
				ReleaseFreeBlockTag(lpCurrentTag);
			}

			UNLOCK_MUTEX(&gCacheBlockListLock);
		}
		// Without checking invalid access of freed blocks, linking pointers are embeded in page itself
		else if (gBlocksInCache < gPatchMgr->GetMaxCachedBlocks())
		{
			LOCK_MUTEX(&gCacheBlockListLock);
			gBlocksInCache++;
			// Put the free block in front. LIFO
			ipHeader->p.mNextHeader = gCacheBlockHead;
			gCacheBlockHead = ipHeader;
			UNLOCK_MUTEX(&gCacheBlockListLock);
			lbReleaseToVMM = false;
		}
	}

	if (lbReleaseToVMM)
	{
		// I shall also relax the pretection of the dead page
		/*if(ipDeadPageAddr)
		{
			if(SetPageLive((char*)ipDeadPageAddr, lPageSize) == false)
			{
				OUTPUT_MSG1(AT_FATAL, "Failed to mprotect page at 0x%lx\n", ipDeadPageAddr);
			}
			// Erase the signature string of the dead page
			// !!FIX!! This will cost me one physical page, don't do this by default
			UnstampDeadPage(ipDeadPageAddr);
		}
		else
		{
			OUTPUT_MSG1(AT_FATAL, "Dead page is NIL. Raw block 0x%lx\n", ipRawBlock);
		}*/
		RELEASE_SYSTEM_PAGE(ipRawBlock, iBlockSize);
	}
}

void PrintBlocksInCache()
{
	size_t lPageSize = gPatchMgr->GetPageSize();
	printf("Cached Blocks Follows: \n");

	LOCK_MUTEX(&gCacheBlockListLock);

	BlockWithDeadPageHeader* lpHeader = gCacheBlockHead;
	for(unsigned long i=0; i<gBlocksInCache && lpHeader; i++)
	{
		printf("[%ld] 0x%lx - 0x%lx\n", i, lpHeader->mRawBlockAddr, (char*)lpHeader->mRawBlockAddr+lPageSize*2);
		lpHeader = lpHeader->p.mNextHeader;
	}

	UNLOCK_MUTEX(&gCacheBlockListLock);
}

BlockWithDeadPageHeader* FindAddrInCache(void* ipAddr)
{
	BlockWithDeadPageHeader* lpResult = NULL;
	size_t lPageSize = gPatchMgr->GetPageSize();

	LOCK_MUTEX(&gCacheBlockListLock);

	BlockWithDeadPageHeader* lpHeader = gCacheBlockHead;
	for(unsigned long i=0; i<gBlocksInCache && lpHeader; i++)
	{
		if(lpHeader->mRawBlockAddr <= ipAddr && (char*)lpHeader->mRawBlockAddr+lPageSize*2 >= ipAddr)
		{
			lpResult = lpHeader;
			break;
		}
		lpHeader = lpHeader->p.mNextHeader;
	}

	UNLOCK_MUTEX(&gCacheBlockListLock);

	return lpResult;
}



#if defined(linux) || defined(sun) || defined(__hpux) || defined(_AIX)
// Return true if success
bool SetPageDead(void* ipDeadPageAddr, size_t iPageSize)
{
	return ::mprotect((char*)ipDeadPageAddr, iPageSize, PROT_NONE) == 0;
}

bool SetPageLive(void* ipDeadPageAddr, size_t iPageSize)
{
	return ::mprotect((char*)ipDeadPageAddr, iPageSize, PROT_READ|PROT_WRITE) == 0;
}

#elif defined(WIN32)

// Return true if success
bool SetPageDead(void* ipDeadPageAddr, size_t iPageSize)
{
	DWORD lOldProt;
	return ::VirtualProtect(ipDeadPageAddr, iPageSize, PAGE_NOACCESS, &lOldProt);
}

bool SetPageLive(void* ipDeadPageAddr, size_t iPageSize)
{
	DWORD lOldProt;
	return ::VirtualProtect(ipDeadPageAddr, iPageSize, PAGE_READWRITE, &lOldProt);
}

#else
#error Unknown platform
#endif
