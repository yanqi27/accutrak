/************************************************************************
** FILE NAME..... MallocReplay.cpp
**
** (c) COPYRIGHT
**
** FUNCTION......... Replay mallo/free by previously logged file
**
** NOTES............ One controller thread read-in consolidated log file
**  then dispatch malloc/free/realloc jobs to specified number of working
**  threads, through array of queues.
**  The tracking information, f.g. block's sequence#, are imbeded in the
**  allocated memory block itself. This is an effor to best replay the original
**  memory footprint.
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
#pragma warning(disable:4786) // debug symbol is longer than 255
#else
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#ifdef sun
#include <thread.h>
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <map>

#include "CrossPlatform.h"
#include "MmapFile.h"
#include "AccuTrak.h"
#include "MallocLogger.h"
#include "PerfCounter.h"
#include "CustomMalloc.h"
#include "Utility.h"
#include "Atomic_op.h"
#include "Tree.h"

#ifdef _SMARTHEAP
#include "smrtheap.h"
#endif

const bool _verbose_check = false;

#define FATAL_ERROR(msg,rc) \
	fprintf(stderr, "Fatal: %s\n", msg); \
	exit(rc)

// Forward declaration of internal buffer for tree to track small allocated blocks
static void* GetMemoryFromVMM(size_t sz);
static size_t GetInternalBufferSz();
static void* GetBlockInternal(size_t sz);
static void  ReleaseBlockInternal(void* p, size_t sz);
static void FreeInternalBuffer();
static void FreeMemoryToVMM(void* addr, size_t sz);

static size_t gPageSize = 0;

/////////////////////////////////////////////////////////////////////////
// Declare constants and private structures
/////////////////////////////////////////////////////////////////////////
#define QUEUE_RECORD_BUFFER_SZ 32*1024

struct QueueJob
{
	QueueJob()
		: mSequenceNo(0), mTotalJobs(0), mNextRecordOffset(0),
			mpRecordBuffer(NULL)
	{}

	void Reset(int offset)
	{
		mSequenceNo = mTotalJobs = 0;
		mNextRecordOffset = offset;
	}

	at_seqnum_t mSequenceNo;
	int  mTotalJobs;
	int  mNextRecordOffset;
	char* mpRecordBuffer;
};

struct AllocatedBlock
{
	at_seqnum_t  mSeqNo;
	unsigned int mSize;
	void*		    mpBlock;
	AllocatedBlock* mpNext;
};

// all blocks are aligned on 8 bytes boundary, even Win32
#define ALIGNMENT 8
#define ALIGN_MASK (ALIGNMENT-1)
#define ALIGN_EIGHT_BYTE(sz) (((sz)+ALIGN_MASK) & ~ALIGN_MASK)

// Convenient macros
#define LINE_DIVIDER \
"===================================================================\n"

/////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////
QueueJob  gpJobs;
size_t    gJobQueueBufferSz = ALIGN_EIGHT_BYTE(QUEUE_RECORD_BUFFER_SZ);

// No real replay, just print out what we should do
bool gbPrintOnly      = false;
bool gbTimingAnalysis = false;
bool gbLogMemUsage    = false;

at_seqnum_t gStartRecNo = 0;
at_seqnum_t gEndRecNo   = MAX_REC_NUM;

// Touch allocated memory in order to trigger page-on-demand.
bool gTouchMemory = false;

// Free memory before exit for oustanding blocks not explicitly released by log file
bool gFreeAllAtEnd = false;

// Replay w/o actual malloc/free in order to measure overhead like file i/o to build a baseline performance
bool gTestBaseline = false;

// Don't synchronize by time stamp, let all threads finish their jobs in the job queue
// make sure the memory usage pattern is not distorted.
bool gIgnoreTimeStamp = false;

// Instrument memory manager to search for the module fragments heap the most
bool gHuntFragmentor = false;
int* gModules        = NULL;

// Dump snapshot of heap profile into a disk file
bool gLogHeapProfile = false;


#ifdef WIN32
//////////////////////////////////////////////////////////////////////////////
// window's CRITICAL_SECTION is recursive mutex, controller thread can't block
// itself this way. Use EVENT alternatively
//////////////////////////////////////////////////////////////////////////////
HANDLE* gWorkingThreadsEventArry = NULL;
HANDLE gControllerEvent = NULL;
CRITICAL_SECTION  gFreeByOtherThreadBlocksLock;

static void AT_Init_MallocReplay_cpp()
{
	gControllerEvent = ::CreateEvent(NULL,
									false, // Auto reset
									false, // Initial state, not signaled
									NULL); // unnamed event
	if(gControllerEvent == NULL)
	{
		FATAL_ERROR("Failed to create event object", 0);
	}
	::InitializeCriticalSection(&gFreeByOtherThreadBlocksLock);
}

static void AT_Fini_MallocReplay_cpp()
{
	::CloseHandle(gControllerEvent);
	::DeleteCriticalSection(&gFreeByOtherThreadBlocksLock);
}

#else

// Wake up only threads that have job on queue.
pthread_mutex_t* gWorkingThreadsLockArry = NULL;
// Once controller setup jobs for all threads, it will be blocked by this lock
// The last thread that finish its job is responsible to unlock it to kick off the next cycle
pthread_mutex_t gControllerLock = PTHREAD_MUTEX_INITIALIZER;
// Lock to protect above map
pthread_mutex_t gFreeByOtherThreadBlocksLock = PTHREAD_MUTEX_INITIALIZER;
static void AT_Init_MallocReplay_cpp() {}
static void AT_Fini_MallocReplay_cpp() {}

#endif

// Number of threads working on queued jobs
// It is first set to TotalThreads by controller.
// Each working thread decrement it by one when it finishes jobs on its queue
long gNumWorkingThreads = 0;

// Flag set by controller that it is time to go home
// all threads shall listen to this
bool gAllDone = false;

// Blocks allocated by one thread and freed by other will stay here.
#define DEFAULT_PER_THREAD_HASH_TABLE_SZ 16*8*1024 // multiple of system page size

static unsigned int gPerThreadHashTableSize = DEFAULT_PER_THREAD_HASH_TABLE_SZ;

static unsigned int gFreeByOtherThreadHashSize = DEFAULT_PER_THREAD_HASH_TABLE_SZ*8;
static AllocatedBlock** gFreeByOtherThreadHash = NULL;

// Number of threads doing malloc/free dictated by log file
int gTotalThreads = 0;
int gTotalModules = 0;

/////////////////////////////////////////////////////////////////////
// Code starts here
/////////////////////////////////////////////////////////////////////

// Insert a newly allocated block into hash
// The caller shall protect hash table's thread-safty
void HashInsertAllocatedBlock(AllocatedBlock** iHashTable,
							  unsigned int iHashTableSz,
							  at_seqnum_t iSeqNo,
							  void* ipBlock,
							  unsigned int iSize)
{
	unsigned int index = iSeqNo % iHashTableSz;

	AllocatedBlock* lpBlock;
	if (iSize < sizeof(AllocatedBlock))
		lpBlock = (AllocatedBlock*) GetBlockInternal(sizeof(AllocatedBlock));
	else
		lpBlock = (AllocatedBlock*) ipBlock;

	lpBlock->mSeqNo = iSeqNo;
	lpBlock->mSize  = iSize;
	lpBlock->mpBlock = ipBlock;

	lpBlock->mpNext  = iHashTable[index];
	iHashTable[index] = lpBlock;
}

// Find a previously allocated block in the hash by sequence number
void* HashFindAllocatedBlock(AllocatedBlock** iHashTable,
							 unsigned int iHashTableSz,
							 at_seqnum_t iSeqNo,
							 unsigned int iSize)
{
	unsigned int index = iSeqNo % iHashTableSz;

	AllocatedBlock* lpHashEntry = iHashTable[index];
	AllocatedBlock* lpPrevEntry = NULL;

	while (lpHashEntry)
	{
		// found the block in hash
		if (lpHashEntry->mSeqNo == iSeqNo)
		{
			// sanity check
			if (_verbose_check && lpHashEntry->mSize != iSize)
			{
				FATAL_ERROR("Impossible switch: malloc_size doesn't match free_size", -1);
			}

			// copy result
			void* lpBlock = lpHashEntry->mpBlock;

			// remove from hash
			if (lpPrevEntry)
			{
				// sanity check
				if (_verbose_check && lpPrevEntry->mpNext != lpHashEntry)
				{
					FATAL_ERROR("Impossible switch: Hash table entry list is broken", index);
				}
				lpPrevEntry->mpNext = lpHashEntry->mpNext;
			}
			else
				iHashTable[index] = lpHashEntry->mpNext;

			// release the node
			if (iSize < sizeof(AllocatedBlock))
				ReleaseBlockInternal(lpHashEntry, sizeof(AllocatedBlock));
			// sanity check
			else if (_verbose_check && lpHashEntry != lpHashEntry->mpBlock)
			{
				FATAL_ERROR("Impossible switch: Hash table entry block is inconsistent", -1);
			}

			return lpBlock;
		}
		else
		{
			lpPrevEntry = lpHashEntry;
			lpHashEntry = lpHashEntry->mpNext;
		}
	}
	return NULL;
}

void StashFreeByOtherThreadBlock(at_seqnum_t iSeqNo, void* ipBlock, size_t iSize)
{
	LOCK_MUTEX(&gFreeByOtherThreadBlocksLock);

	HashInsertAllocatedBlock(gFreeByOtherThreadHash,
		gFreeByOtherThreadHashSize, iSeqNo, ipBlock, iSize);

	UNLOCK_MUTEX(&gFreeByOtherThreadBlocksLock);
}

void* GetBlockAllocatedByOtherThread(at_seqnum_t iSeqNo, size_t iSize)
{
	do
	{
		void* lpBlock = NULL;

		LOCK_MUTEX(&gFreeByOtherThreadBlocksLock);

		lpBlock = HashFindAllocatedBlock(gFreeByOtherThreadHash,
										gFreeByOtherThreadHashSize,
										iSeqNo, iSize);

		UNLOCK_MUTEX(&gFreeByOtherThreadBlocksLock);

		if (lpBlock)
		{
			// Just for fun
			::memset(lpBlock, 0x55, iSize);
			return lpBlock;
		}
		else
		{
			YIELD();
		}
	} while (1);

	//return NULL;
}

inline void FinishQueuedJobs()
{
	long lThreadsLeft = SYS_INTERLOCKED_DECREMENT(&gNumWorkingThreads);
	if (lThreadsLeft == 0)
	{
#ifdef WIN32
		if(false == ::SetEvent(gControllerEvent))
		{
			FATAL_ERROR("Failed to SetEvent", 0);
		}
#else
		UNLOCK_MUTEX(&gControllerLock);
#endif
	}
}

/////////////////////////////////////////////////////////////////////
// The core functions
/////////////////////////////////////////////////////////////////////
static void OpMalloc(AT_CONSOLIDATED_MALLOC* ipMalloc,
					 AllocatedBlock** iHashTable,
					 unsigned int iHashTableSz,
					 at_seqnum_t iCurrentSeqNo)
{
	// Allocate block
	void* lpBlock = (CustomMallocs[ipMalloc->mModuleNum])(ipMalloc->mSize);
	if (lpBlock)
	{
		if(gTouchMemory)
		{
			::memset(lpBlock, 0xCF, ipMalloc->mSize);
		}
		/*if(gHuntFragmentor)
		{
			// Mark the module number at the beginning of the memory block
			*(int*)lpBlock = ipMalloc->mModuleNum;
		}*/
	}
	else
	{
		FATAL_ERROR("Failed to malloc", ipMalloc->mSize);
	}

	// Stash allocated block in the per-thread map if it is going to be freed by this thread
	if(ipMalloc->mFreeByOtherThread == 0)
	{
		HashInsertAllocatedBlock(iHashTable, iHashTableSz, iCurrentSeqNo, lpBlock, ipMalloc->mSize);
	}
	// Otherwise insert it into global outstanding blocks map.
	else
	{
		StashFreeByOtherThreadBlock(iCurrentSeqNo, lpBlock, ipMalloc->mSize);
	}
}

static void OpFree(AT_CONSOLIDATED_FREE* ipFree,
				   AllocatedBlock** iHashTable,
				   unsigned int iHashTableSz,
				   at_seqnum_t iCurrentSeqNo)
{
	void* lpBlock = NULL;

	// Find the to-be-freed block in the per-thread map
	if(ipFree->mAllocatedByOtherThread == 0)
	{
		lpBlock = HashFindAllocatedBlock(iHashTable, iHashTableSz, ipFree->mMatchedMalloc, ipFree->mSize);
	}
	// The block is allocated by other thread
	// and user wants to do cross-thread alloc
	else
	{
		lpBlock = GetBlockAllocatedByOtherThread(ipFree->mMatchedMalloc, ipFree->mSize);
	}

	// free it if found it
	if(lpBlock)
	{
		if(gTouchMemory)
			memset(lpBlock, 0xFD, ipFree->mSize);
		(CustomFrees[ipFree->mModuleNum])(lpBlock);
	}
	else if (_verbose_check)
	{
		fprintf(stderr, "[%ld] Free: Failed to find the block[%ld]\n",
			iCurrentSeqNo, ipFree->mMatchedMalloc);
	}
}

static void OpRealloc(AT_CONSOLIDATED_REALLOC* ipRealloc,
					  AllocatedBlock** iHashTable,
					  unsigned int iHashTableSz,
					 at_seqnum_t iCurrentSeqNo)
{
	void* lpBlock = NULL;

	// If we know which old block to free
	if(ipRealloc->mMatchedMalloc)
	{
		// Find the to-be-freed block in the per-thread map
		if(ipRealloc->mAllocatedByOtherThread == 0)
		{
			lpBlock = HashFindAllocatedBlock(iHashTable, iHashTableSz, ipRealloc->mMatchedMalloc, ipRealloc->mOldSize);

			if (lpBlock)
			{
				// Find the old block, do realloc
				lpBlock = (CustomReallocs[ipRealloc->mModuleNum])(lpBlock, ipRealloc->mNewSize);
			}
			else
			{
				// Can't find the old block, just malloc(new_size)
				fprintf(stderr, "[%ld] Realloc: Failed to find the old block[%ld] allocated by my thread for realloc\n",
						iCurrentSeqNo, ipRealloc->mMatchedMalloc);
				lpBlock = (CustomMallocs[ipRealloc->mModuleNum])(ipRealloc->mNewSize);
			}
		}
		// The old block is allocated by another thread
		else
		{
			// The old block is not stashed if user choose not to exercise cross-thread alloc/dealloc
			lpBlock = GetBlockAllocatedByOtherThread(ipRealloc->mMatchedMalloc, ipRealloc->mOldSize);
			if(lpBlock)
			{
				lpBlock = (CustomReallocs[ipRealloc->mModuleNum])(lpBlock, ipRealloc->mNewSize);
			}
			else
			{
				// Can't find the old block
				fprintf(stderr, "[%ld] Realloc: Failed to find the old block[%ld] allocated by other thread for realloc\n",
						iCurrentSeqNo, ipRealloc->mMatchedMalloc);
				lpBlock = (CustomMallocs[ipRealloc->mModuleNum])(ipRealloc->mNewSize);
			}
		}
	}
	else
	{
		// We don't know the old block from the consolidated log file
		lpBlock = (CustomMallocs[ipRealloc->mModuleNum])(ipRealloc->mNewSize);
	}

	// check allocated block
	if(lpBlock)
	{
		if(gTouchMemory)
		{
			::memset(lpBlock, 0xCF, ipRealloc->mNewSize);
		}
		/*if(gHuntFragmentor)
		{
			// Mark the module number at the beginning of the memory block
			*(int*)lpBlock = ipRealloc->mModuleNum;
		}*/
	}
	else
	{
		FATAL_ERROR("Failed to malloc %ld bytes", ipRealloc->mNewSize);
	}

	// Stash allocated block in the per-thread map
	if(ipRealloc->mFreeByOtherThread == 0)
	{
		HashInsertAllocatedBlock(iHashTable, iHashTableSz, iCurrentSeqNo, lpBlock, ipRealloc->mNewSize);
	}
	else
	{
		StashFreeByOtherThreadBlock(iCurrentSeqNo, lpBlock, ipRealloc->mNewSize);
	}
}

/// All working threads run this function for malloc/free
static void DoMallocFree(unsigned int iThreadID)
{
	unsigned int lHashTableSz = gPerThreadHashTableSize;
	AllocatedBlock** lHashTable = (AllocatedBlock**) GetMemoryFromVMM(lHashTableSz * sizeof(AllocatedBlock*));
	::memset(lHashTable, 0, lHashTableSz * sizeof(AllocatedBlock*));

	while(1)
	{
		// Will be wake up by controller.
#ifdef WIN32
		if(WAIT_FAILED == ::WaitForSingleObject(gWorkingThreadsEventArry[iThreadID], INFINITE))
		{
			FATAL_ERROR("Thread Failed to wait for event", GetLastError());
		}
#else
		if (LOCK_MUTEX(&gWorkingThreadsLockArry[iThreadID]))
		{
			FATAL_ERROR("Failed to lock mutex", -1);
		}
#endif

		// It is time to call it quit.
		if(gAllDone)
			break;

		QueueJob* lpQueueJob = &gpJobs;
		AT_CONSOLIDATED_GENERIC* lpNextRecord = (AT_CONSOLIDATED_GENERIC*)(&lpQueueJob->mpRecordBuffer[lpQueueJob->mNextRecordOffset]);
		at_seqnum_t lCurrentSeqNo = lpQueueJob->mSequenceNo;
		int lTotalJobs = lpQueueJob->mTotalJobs;

		// Innter loop to process all ops by this thread.
		while(lTotalJobs)
		{
			// Malloc
			if(lpNextRecord->mType == AT_MEM_OP_TYPE_MALLOC)
			{
				AT_CONSOLIDATED_MALLOC* lpMalloc = (AT_CONSOLIDATED_MALLOC*)lpNextRecord;
				if (iThreadID==lpMalloc->mThreadID && !gTestBaseline)
				{
					OpMalloc(lpMalloc, lHashTable, lHashTableSz,
							lCurrentSeqNo);
				}
				lpNextRecord = (AT_CONSOLIDATED_GENERIC*)(lpMalloc+1);
			}
			// Free
			else if(lpNextRecord->mType == AT_MEM_OP_TYPE_FREE)
			{
				AT_CONSOLIDATED_FREE* lpFree = (AT_CONSOLIDATED_FREE*)lpNextRecord;
				// If we know which block to free
				if (iThreadID==lpFree->mThreadID && lpFree->mMatchedMalloc && !gTestBaseline)
				{
					OpFree(lpFree, lHashTable, lHashTableSz, lCurrentSeqNo);
				}
				lpNextRecord = (AT_CONSOLIDATED_GENERIC*)(lpFree+1);
			}
			// Realloc
			else if(lpNextRecord->mType ==  AT_MEM_OP_TYPE_REALLOC)
			{
				AT_CONSOLIDATED_REALLOC* lpRealloc = (AT_CONSOLIDATED_REALLOC*)lpNextRecord;
				if (iThreadID==lpRealloc->mThreadID && !gTestBaseline)
				{
					OpRealloc(lpRealloc, lHashTable, lHashTableSz, lCurrentSeqNo);
				}
				lpNextRecord = (AT_CONSOLIDATED_GENERIC*)(lpRealloc+1);
			}
			// Time stamp is NOT accounted for a job, don't bump sequence number
			else if (lpNextRecord->mType ==  AT_MEM_OP_TYPE_TIMESTAMP)
			{
				AT_CONSOLIDATED_TIMESTAMP* lpTimestamp = (AT_CONSOLIDATED_TIMESTAMP*)lpNextRecord;
				lpNextRecord = (AT_CONSOLIDATED_GENERIC*)(lpTimestamp+1);
				continue;
			}
			else
			{
				FATAL_ERROR("Unknown job type in queue.", 0);
			}
			lCurrentSeqNo++;
			lTotalJobs--;
		}

		// Done all jobs currently on my queue
		FinishQueuedJobs();
	}

	unsigned long lTotalOutstandingBlocks = 0;
	// error report
	for (int i=0; i<lHashTableSz; i++)
	{
		AllocatedBlock* lpHashEntry = lHashTable[i];
		while (lpHashEntry)
		{
			lTotalOutstandingBlocks++;

			// release internal buffer
			if (gFreeAllAtEnd)
			{
				AllocatedBlock* nextEntry = lpHashEntry->mpNext;

				if (lpHashEntry->mSize < sizeof(AllocatedBlock) )
				{
					::free(lpHashEntry->mpBlock);
					ReleaseBlockInternal(lpHashEntry, sizeof(AllocatedBlock));
				}
				else
					::free(lpHashEntry->mpBlock);

				lpHashEntry = nextEntry;
			}
			else
				lpHashEntry = lpHashEntry->mpNext;
		}
	}
	FreeMemoryToVMM(lHashTable, lHashTableSz * sizeof(AllocatedBlock*));

	if(_verbose_check && lTotalOutstandingBlocks)
	{
		::fprintf(stderr, "[Info] there are %ld outstanding blocks for thread %d\n",
			lTotalOutstandingBlocks, iThreadID);
	}
}

/// Thread entry function
#ifdef WIN32
DWORD WINAPI ThreadFunc(LPVOID arg)
#else
static void *ThreadFunc(void *arg)
#endif
{
	int lThreadID = *(int*)arg;

	try	{
		DoMallocFree(lThreadID);
	}
	catch (...)
	{
		::fprintf(stderr, "Caught exception in thread %d\n", lThreadID);
		::fprintf(stderr, "Maybe memory is used up\n");
	}

	return NULL;
}


enum DISK_JOB_STATUS
{
	DISK_JOB_STATUS_IDLE,
	DISK_JOB_STATUS_READ,
	DISK_JOB_STATUS_EXIT
};

struct DiskIOJob
{
	DiskIOJob(FILE* fp) : mFilePtr(fp), mStatus(DISK_JOB_STATUS_IDLE), mpBuffer(NULL), mReadSz(0)
	{
		MUTEX_INIT(&mCmdLock);
		MUTEX_INIT(&mSleepLock);
	}

	void Stop()
	{
		LOCK_MUTEX(&mCmdLock);
		mStatus  = DISK_JOB_STATUS_EXIT;
		UNLOCK_MUTEX(&mCmdLock);

		// wake up working thread
		UNLOCK_MUTEX(&mSleepLock);
	}

	void StartRead(char* ipBuffer, size_t iReadSz)
	{
		LOCK_MUTEX(&mCmdLock);
		mpBuffer = ipBuffer;
		mReadSz  = iReadSz;
		mStatus  = DISK_JOB_STATUS_READ;
		UNLOCK_MUTEX(&mCmdLock);

		// wake up working thread
		UNLOCK_MUTEX(&mSleepLock);
	}

	void ReadFile()
	{
		LOCK_MUTEX(&mCmdLock);
		if (mReadSz > 0)
		{
			if(1 != ::fread(mpBuffer, mReadSz, 1, mFilePtr))
			{
				FATAL_ERROR("Failed to read file", 0);
			}
		}
		mStatus = DISK_JOB_STATUS_IDLE;
		UNLOCK_MUTEX(&mCmdLock);

		// Go back to sleep
		LOCK_MUTEX(&mSleepLock);
	}

	DISK_JOB_STATUS mStatus;
	MUTEX_t mCmdLock;
	MUTEX_t mSleepLock;
	FILE* mFilePtr;
	char* mpBuffer;
	size_t mReadSz;
};

#ifdef WIN32
DWORD WINAPI DiskIOFunc(LPVOID arg)
#else
static void *DiskIOFunc(void *arg)
#endif
{
	DiskIOJob* ipDiskIO = (DiskIOJob*) arg;

	LOCK_MUTEX(&ipDiskIO->mSleepLock);

	while (1)
	{
		DISK_JOB_STATUS status;

		LOCK_MUTEX(&ipDiskIO->mCmdLock);
		status = ipDiskIO->mStatus;
		UNLOCK_MUTEX(&ipDiskIO->mCmdLock);

		if (status == DISK_JOB_STATUS_EXIT)
			break;
		else if (status == DISK_JOB_STATUS_READ)
		{
			ipDiskIO->ReadFile();
		}
		else if (status == DISK_JOB_STATUS_IDLE)
		{
			LOCK_MUTEX(&ipDiskIO->mSleepLock);
		}
	}

	UNLOCK_MUTEX(&ipDiskIO->mSleepLock);

	return NULL;
}

/////////////////////////////////////////////////////////////////////////
// Postmortem analysis of logged data
/////////////////////////////////////////////////////////////////////////
struct OutstandingBlock
{
	void Init(at_seqnum_t n, size_t s, OutstandingBlock* ipNext)
	{
		mSeqNo=n; mSize=s; mpNext=ipNext;
	}

	at_seqnum_t       mSeqNo;
	unsigned int      mSize;
	OutstandingBlock* mpNext;
};

#define DEFAULT_HASH_TABLE_SZ 1024*1024

// Make sure hash table size is multiple of system page size
static unsigned long gHashSize = DEFAULT_HASH_TABLE_SZ;
static OutstandingBlock** gHashTable = NULL;

static void HashInsertBlock(at_seqnum_t n, size_t s)
{
	unsigned long index = n % gHashSize;
	OutstandingBlock* lpHashEntry = (OutstandingBlock*) GetBlockInternal(sizeof(OutstandingBlock));
	lpHashEntry->Init(n, s, gHashTable[index]);
	gHashTable[index] = lpHashEntry;
}

static bool HashFindBlock(at_seqnum_t n, OutstandingBlock* ipBlock)
{
	unsigned long index = n % gHashSize;
	OutstandingBlock* lpHashEntry = gHashTable[index];
	OutstandingBlock* lpPrevEntry = NULL;

	while (lpHashEntry)
	{
		// found the block in hash
		if (lpHashEntry->mSeqNo == n)
		{
			// copy result
			*ipBlock = *lpHashEntry;
			// remove from hash
			if (lpPrevEntry)
				lpPrevEntry->mpNext = lpHashEntry->mpNext;
			else
				gHashTable[index] = lpHashEntry->mpNext;
			// release the node
			ReleaseBlockInternal(lpHashEntry, sizeof(OutstandingBlock));
			return true;
		}
		else
		{
			lpPrevEntry = lpHashEntry;
			lpHashEntry = lpHashEntry->mpNext;
		}
	}
	return false;
}

// Given block size, find the bucket it belongs to
static int BlockSize2BucketIndex(size_t iSize)
{
	for(int i=NUM_BUCKET-1; i>=0; i--)
	{
		if(iSize > gBucketBoundary[i])
		{
			return i+1;
		}
	}
	return 0;
}

static void PrintAllBuckets()
{
	for(int i=0; i<NUM_BUCKET+1; i++)
	{
		unsigned long lSize1, lSize2;
		if(i==0)
		{
			lSize1 = 0;
			lSize2 = gBucketBoundary[0];
		}
		else if(i==NUM_BUCKET)
		{
			lSize1 = gBucketBoundary[NUM_BUCKET-1]+1;
			lSize2 = LONG_MAX;
		}
		else
		{
			lSize1 = gBucketBoundary[i-1]+1;
			lSize2 = gBucketBoundary[i];
		}
		printf("\t[%ld - %ld]", lSize1, lSize2);
	}
	printf("\n");
}

static void AnalyzeTiming(const char* ipLogFileName)
{
	int i;
	// Open file and mmap it into process
	MmapFile lMmapConsolidatedFile(ipLogFileName);

	if(lMmapConsolidatedFile.InitSucceed() == false)
	{
		return;
	}

	// Process log file header
	AT_CONSOLIDATED_HEADER* lpHeader = (AT_CONSOLIDATED_HEADER*)lMmapConsolidatedFile.GetStartAddr();
	if(lpHeader->CheckAndPrintHeader() == false)
	{
		return;
	}

	// Per-module block sizes
	int TotalModules = lpHeader->mTotalModules;
	long* lOutstandingSizeArray = new long [TotalModules];
	for (i=0; i<TotalModules; i++)
		lOutstandingSizeArray[i] = 0;

	// Per-thread blocks
	// Thread id is 1...TotalThreads
	int TotalThreads = lpHeader->mTotalThreads;
	long* lOutstandingBlocksPerThread = new long[TotalThreads+1];
	long* lPeakOutstandBlocksPerThread = new long[TotalThreads+1];
	for (i=0; i<TotalThreads+1; i++)
	{
		lOutstandingBlocksPerThread[i] = lPeakOutstandBlocksPerThread[i] = 0;
	}

	// Allocation counts by Size
	unsigned long TotalBuckets[NUM_BUCKET+1];
	for (i=0; i<NUM_BUCKET+1; i++)
		TotalBuckets[i] = 0;

	// Sequence numuber starts with 1, reserve 0 as unmatched.
	at_seqnum_t lRecSeqNumber = 1;
	char* lpCursor = (char*)(lpHeader+1);

	size_t lOutstandingBlockSz = 0;
	size_t lPeakOutstandBytes = 0;
	at_seqnum_t lPeakSeqNo = 0;

	long lOutstandingBlocks = 0;
	long lPeakOutstandBlocks = 0;
	at_seqnum_t lPeakBlocksSeqNo = 0;

	// hash table to remember outstanding blocks.
	// Don't need to zero-initialize the memory
	gHashTable = (OutstandingBlock**)GetMemoryFromVMM(gHashSize*sizeof(OutstandingBlock*));

	// Go through all records
	while(lpCursor < lMmapConsolidatedFile.GetEndAddr())
	{
		// Unmap the file page we just used in order to save memory
		// and not meddle with simulating behavior.
		if(lMmapConsolidatedFile.AdjustMmapArea(lpCursor) == false)
		{
			break;
		}

		AT_CONSOLIDATED_GENERIC* lpNextRecord = (AT_CONSOLIDATED_GENERIC*)lpCursor;
		AT_CONSOLIDATED_MALLOC*  lpMalloc;
		AT_CONSOLIDATED_FREE*    lpFree;
		AT_CONSOLIDATED_REALLOC* lpRealloc;

		//size_t                   lBlockSize;
		OutstandingBlock         lBlock;
		int                      lThreadIndex = -1;
		int lBucketIndex = -1;

		if(lpNextRecord->mType == AT_MEM_OP_TYPE_TIMESTAMP)
		{
			lpCursor += sizeof(AT_CONSOLIDATED_TIMESTAMP);
			continue;
		}
		else if(lpNextRecord->mType == AT_MEM_OP_TYPE_MALLOC)
		{
			lpMalloc = (AT_CONSOLIDATED_MALLOC*)lpCursor;

			lOutstandingBlockSz += lpMalloc->mSize;
			lOutstandingSizeArray[lpMalloc->mModuleNum] += lpMalloc->mSize;

			HashInsertBlock(lRecSeqNumber, lpMalloc->mSize);
			lOutstandingBlocks++;
			if (lpMalloc->mFreeByOtherThread == 0)
				lThreadIndex = lpMalloc->mThreadID;
			else
				lThreadIndex = 0;
			lOutstandingBlocksPerThread[lThreadIndex]++;

			lBucketIndex = BlockSize2BucketIndex(lpMalloc->mSize);
			TotalBuckets[lBucketIndex]++;

			lpCursor += sizeof(AT_CONSOLIDATED_MALLOC);
			lRecSeqNumber++;
		}
		else if(lpNextRecord->mType == AT_MEM_OP_TYPE_FREE)
		{
			lpFree = (AT_CONSOLIDATED_FREE*)lpCursor;

			// If we know who allocated this block
			if(lpFree->mMatchedMalloc)
			{
				if (HashFindBlock(lpFree->mMatchedMalloc, &lBlock))
				{
					lOutstandingBlockSz -= lBlock.mSize;
					lOutstandingSizeArray[lpFree->mModuleNum] -= lBlock.mSize;
					lOutstandingBlocks--;
					if (lpFree->mAllocatedByOtherThread == 0)
						lThreadIndex = lpFree->mThreadID;
					else
						lThreadIndex = 0;
					lOutstandingBlocksPerThread[lThreadIndex]--;
				}
				else
				{
					fprintf(stderr, "[%d] Free: Failed to find the block[%ld] allocated by my thread for free\n",
						lRecSeqNumber, lpFree->mMatchedMalloc);
					continue;
				}
			}

			lpCursor += sizeof(AT_CONSOLIDATED_FREE);
			lRecSeqNumber++;
		}
		else if(lpNextRecord->mType == AT_MEM_OP_TYPE_REALLOC)
		{
			lpRealloc = (AT_CONSOLIDATED_REALLOC*)lpCursor;

			// If we know which old block to free
			if(lpRealloc->mMatchedMalloc)
			{
				// Find the old block
				if (HashFindBlock(lpRealloc->mMatchedMalloc, &lBlock))
				{
					lOutstandingBlockSz -= lBlock.mSize;
					lOutstandingSizeArray[lpRealloc->mModuleNum] -= lBlock.mSize;
					lOutstandingBlocks--;
					if (lpRealloc->mAllocatedByOtherThread == 0)
						lThreadIndex = lpRealloc->mThreadID;
					else
						lThreadIndex = 0;
					lOutstandingBlocksPerThread[lThreadIndex]--;
				}
				else
				{
					// Can't find the old block
					fprintf(stderr, "[%ld] Realloc: Failed to find the old block[%ld] allocated by my thread for realloc\n",
							lRecSeqNumber, lpRealloc->mMatchedMalloc);
					continue;
				}
			}
			HashInsertBlock(lRecSeqNumber, lpRealloc->mNewSize);

			lOutstandingBlocks++;
			if (lpRealloc->mFreeByOtherThread == 0)
				lThreadIndex = lpRealloc->mThreadID;
			else
				lThreadIndex = 0;
			lOutstandingBlocksPerThread[lThreadIndex]++;

			lOutstandingBlockSz += lpRealloc->mNewSize;
			lOutstandingSizeArray[lpRealloc->mModuleNum] += lpRealloc->mNewSize;

			lBucketIndex = BlockSize2BucketIndex(lpRealloc->mNewSize);
			TotalBuckets[lBucketIndex]++;

			lpCursor += sizeof(AT_CONSOLIDATED_REALLOC);
			lRecSeqNumber++;
		}
		else
		{
			::fprintf(stderr, "Unknown type [%d]\n", lpNextRecord->mType);
			return;
		}

		if (lPeakOutstandBytes < lOutstandingBlockSz)
		{
			lPeakOutstandBytes = lOutstandingBlockSz;
			lPeakSeqNo = lRecSeqNumber;
		}

		if (lPeakOutstandBlocks < lOutstandingBlocks)
		{
			lPeakOutstandBlocks = lOutstandingBlocks;
			lPeakBlocksSeqNo = lRecSeqNumber;
		}

		if (lThreadIndex >= 0
			&& lPeakOutstandBlocksPerThread[lThreadIndex] < lOutstandingBlocksPerThread[lThreadIndex])
			lPeakOutstandBlocksPerThread[lThreadIndex] = lOutstandingBlocksPerThread[lThreadIndex];

		// If user only wants to replay part of the records.
		if(lRecSeqNumber > gEndRecNo)
		{
			break;
		}
	}

	// Print the per-module statistics
	printf("Outstanding bytes distributed in each modules:\n");
	for(i=0; i<TotalModules; i++)
	{
		printf("Module[%d]=%ld\n", i, lOutstandingSizeArray[i]);
	}

	// Size distribution
	printf("\nAllocation histogram by size\n");
	//PrintAllBuckets();
	for(i=0; i<NUM_BUCKET+1; i++)
	{
		if (i == NUM_BUCKET)
		{
			printf(">%ld\t%ld\n", gBucketBoundary[NUM_BUCKET-1], TotalBuckets[i]);
		}
		else
		{
			printf("%ld\t%ld\n", gBucketBoundary[i], TotalBuckets[i]);
		}
	}

	// Print peak memory usage
	printf("\nPeak Outstanding Bytes=%ld when SeqNo=%ld\n", lPeakOutstandBytes, (unsigned long)lPeakSeqNo);
	printf("\nPeak Outstanding Blocks=%ld when SeqNo=%ld\n", lPeakOutstandBlocks, (unsigned long)lPeakBlocksSeqNo);

	long lMaxOutstandBlockThread = 0;
	int  lMaxOutstandBlockTID = -1;
	for (i=0; i<TotalThreads+1; i++)
	{
		if (lMaxOutstandBlockThread < lPeakOutstandBlocksPerThread[i])
		{
			lMaxOutstandBlockThread = lPeakOutstandBlocksPerThread[i];
			lMaxOutstandBlockTID = i;
		}
	}
	printf("\nPeak Outstanding Blocks Per_Thread=%ld of TID=%d\n", lMaxOutstandBlockThread, lMaxOutstandBlockTID);

	lMaxOutstandBlockThread = 0;
	lMaxOutstandBlockTID = -1;
	for (i=1; i<TotalThreads+1; i++)
	{
		if (lMaxOutstandBlockThread < lPeakOutstandBlocksPerThread[i])
		{
			lMaxOutstandBlockThread = lPeakOutstandBlocksPerThread[i];
			lMaxOutstandBlockTID = i;
		}
	}
	printf("Peak Outstanding Blocks Per_Thread=%ld of TID=%d\n", lMaxOutstandBlockThread, lMaxOutstandBlockTID);

	// clean up
	delete [] lOutstandingSizeArray;

	FreeMemoryToVMM(gHashTable, gHashSize*sizeof(OutstandingBlock*));
}

/////////////////////////////////////////////////////////////////////////
// Print each record in detail
/////////////////////////////////////////////////////////////////////////

static size_t ReadNextRecord(char* ipBuffer, FILE* ipFile)
{
	size_t lSizeRead;
	// First read the common part of all kinds of records
	if(1 != ::fread(ipBuffer, sizeof(AT_CONSOLIDATED_GENERIC), 1, ipFile))
	{
		::fprintf(stderr, "Failed to read record from log file\n");
		return 0;
	}

	// find out the type
	AT_CONSOLIDATED_GENERIC* lpGenericRecord = (AT_CONSOLIDATED_GENERIC*)ipBuffer;
	switch(lpGenericRecord->mType)
	{
	case AT_MEM_OP_TYPE_TIMESTAMP:
		lSizeRead = sizeof(AT_CONSOLIDATED_TIMESTAMP);
		break;
	case AT_MEM_OP_TYPE_MALLOC:
		lSizeRead = sizeof(AT_CONSOLIDATED_MALLOC);
		break;
	case AT_MEM_OP_TYPE_FREE:
		lSizeRead = sizeof(AT_CONSOLIDATED_FREE);
		break;
	case AT_MEM_OP_TYPE_REALLOC:
		lSizeRead = sizeof(AT_CONSOLIDATED_REALLOC);
		break;
	default:
		fprintf(stderr, "Error: invalid or unknown type [%d]\n", lpGenericRecord->mType);
		return 0;
	}

	// Then read in the rest of the record depending on its type
	if(1 != ::fread(ipBuffer+sizeof(AT_CONSOLIDATED_GENERIC), lSizeRead-sizeof(AT_CONSOLIDATED_GENERIC), 1, ipFile))
	{
		::fprintf(stderr, "Failed to read record from log file\n");
		return 0;
	}

	return lSizeRead;
}

static bool PrintAll(const char* ipLogFileName)
{
	// file stat
	struct stat lLogFileStat;
	if(::stat(ipLogFileName, &lLogFileStat))
	{
		::fprintf(stderr, "Failed to stat file %s\n", ipLogFileName);
		return false;
	}

	// Open the file
	FILE* fp = ::fopen(ipLogFileName, "rb");
	if(!fp)
	{
		::fprintf(stderr, "Failed to open log file %s\n", ipLogFileName);
		return false;
	}

	// Track total bytes read
	size_t lTotalReadSize = 0;

	// First read in header
	AT_CONSOLIDATED_HEADER lLogHeader;
	if(1 != ::fread(&lLogHeader, sizeof(lLogHeader), 1, fp))
	{
		::fprintf(stderr, "Failed to read log file's header %s\n", ipLogFileName);
		return false;
	}
	lTotalReadSize += sizeof(lLogHeader);

	// Print out header info
	if(lLogHeader.CheckAndPrintHeader() == false)
	{
		return false;
	}

	// Transaction Stats
	size_t lTotalOutstadningBytes = 0;
	at_seqnum_t lTotalMalloc = 0;
	at_seqnum_t lTotalFree = 0;
	at_seqnum_t lTotalRealloc = 0;
	at_seqnum_t lTotalUnknownFree = 0;

	// Sequence numuber starts with 1, 0 is reserved for unmatched.
	at_seqnum_t lRecSeqNumber = 1;

	// One recrod buffer on stack, big enough for any kind of record
	char lRecordBuffer[sizeof(AT_CONSOLIDATED_REALLOC)];
	AT_CONSOLIDATED_GENERIC* lpNextRecord = (AT_CONSOLIDATED_GENERIC*)lRecordBuffer;

	// Go through all records
	while(lTotalReadSize < lLogFileStat.st_size)
	{
		size_t lSizeRead = ReadNextRecord(lRecordBuffer, fp);
		lTotalReadSize += lSizeRead;
		// Leave if we reach the end of file
		if(lSizeRead == 0)
		{
			break;
		}

		AT_CONSOLIDATED_MALLOC* lpMalloc;
		AT_CONSOLIDATED_FREE*   lpFree;
		AT_CONSOLIDATED_REALLOC* lpRealloc;
		AT_CONSOLIDATED_TIMESTAMP* lpTimeStamp;

		switch(lpNextRecord->mType)
		{
		case AT_MEM_OP_TYPE_TIMESTAMP:
			lpTimeStamp = (AT_CONSOLIDATED_TIMESTAMP*)lpNextRecord;
			if(lRecSeqNumber>=gStartRecNo && lRecSeqNumber<=gEndRecNo)
			{
				printf("\nTime %ldms\n", lpTimeStamp->mTimeStamp);
			}
			lRecSeqNumber--;	// timestamp is just pseudo record.
			break;

		case AT_MEM_OP_TYPE_MALLOC:
			lpMalloc = (AT_CONSOLIDATED_MALLOC*)lpNextRecord;
			if(lRecSeqNumber>=gStartRecNo && lRecSeqNumber<=gEndRecNo)
			{
				printf("[%ld][thread %d]: malloc %ld bytes, will be freed by %s thread\n",
					lRecSeqNumber, lpMalloc->mThreadID, lpMalloc->mSize, lpMalloc->mFreeByOtherThread ? "other" : "same");
			}
			lTotalMalloc++;
			break;

		case AT_MEM_OP_TYPE_FREE:
			lpFree = (AT_CONSOLIDATED_FREE*)lpNextRecord;
			if(lRecSeqNumber>=gStartRecNo && lRecSeqNumber<=gEndRecNo)
			{
				printf("[%ld][thread %d]: free [%ld]th block %ld bytes allocated by %s thread\n",
					lRecSeqNumber, lpFree->mThreadID, lpFree->mMatchedMalloc, lpFree->mSize,
					lpFree->mAllocatedByOtherThread ? "other" : "same");
			}
			if(lpFree->mMatchedMalloc == 0)
			{
				lTotalUnknownFree++;
			}
			else
			{
				lTotalFree++;
			}
			break;

		case AT_MEM_OP_TYPE_REALLOC:
			lpRealloc = (AT_CONSOLIDATED_REALLOC*)lpNextRecord;
			if(lRecSeqNumber>=gStartRecNo && lRecSeqNumber<=gEndRecNo)
			{
				printf("[%ld][thread %d]: realloc [%ld]th block %ld bytes allocated by %s thread with new_size %ld bytes, will be freed by %s thread\n",
					lRecSeqNumber, lpRealloc->mThreadID, lpRealloc->mMatchedMalloc, lpRealloc->mOldSize,
					lpRealloc->mAllocatedByOtherThread ? "other" : "same",
					lpRealloc->mNewSize, lpRealloc->mFreeByOtherThread ? "other" : "same");
			}
			if(lpRealloc->mMatchedMalloc == 0)
			{
				lTotalUnknownFree++;
			}
			lTotalRealloc++;
			break;

		default:
			::fprintf(stderr, "Unknown type [%d]\n", lpNextRecord->mType);
			return false;
		}

		lRecSeqNumber++;
		// If user only wants to replay part of the records.
		if(lRecSeqNumber > gEndRecNo)
		{
			break;
		}

	}
	// Display some stats
	printf("\n==> Total_Malloc: %ld\n", lTotalMalloc);
	printf("==> Total_Free: %ld\n", lTotalFree);
	printf("==> Total_Realloc: %ld\n", lTotalRealloc);
	printf("==> Total_UnMatched_Free: %ld\n", lTotalUnknownFree);

	if(::fclose(fp))
	{
		::fprintf(stderr, "Failed to close log file %s\n", ipLogFileName);
		return false;
	}

	return true;
}

// All counters are in KB
static void PrintMemFootPrint(at_timestamp_t iTimeStamp, size_t iOutstandingBytes, bool ibCloseFile=false)
{
	static FILE* lpFile = NULL;
	const char* lpOutFileName = "accutrak.log";

	if(!lpFile)
	{
		if(!(lpFile = ::fopen(lpOutFileName, "w")))
		{
			fprintf(stderr, "Failed to open file %s to write\n", lpOutFileName);
			FATAL_ERROR("", -1);
		}
		fprintf(lpFile, "Time(ms)\tOutstanding(KB)\tVSS(KB)\tRSS(KB)\tHeap(KB)\n");
	}

	size_t lVSS, lRSS;
	GetProcessMemory(&lVSS, &lRSS);

	// Take into account of memory used for internal bookkeep
	size_t lBufSz = GetInternalBufferSz();
	lVSS -= lBufSz/1024;
	lRSS -= lBufSz/1024;

	size_t lHeapSz = 0;
#if defined(_SMARTHEAP) && defined(_HEAP_PROFILE)
	lHeapSz = (size_t)(MemHeapTop() - MemHeapBottom())/1024;
#endif

	fprintf(lpFile, "%ld\t%ld\t%ld\t%ld\t%ld\n", iTimeStamp, iOutstandingBytes, lVSS, lRSS, lHeapSz);
	//::fflush(lpFile);

	if(ibCloseFile && lpFile)
	{
		::fclose(lpFile);
		lpFile = NULL;
		return;
	}
}

/////////////////////////////////////////////////////////////////////////
// Replay all malloc/free/realloc operation in the consolidated log file
/////////////////////////////////////////////////////////////////////////
bool ReplayAll(const char* ipLogFileName)
{
	int i;
#ifdef _SMARTHEAP
	MemRegisterTask();
#ifdef _AIX
	MemPatchProcess();
#endif
	//MemPoolSetMaxSubpools(MEM_DEFAULT_POOL, 1);
	//MEM_SIZET newSize = 64 * 1024;
	//MEM_SIZET oldSize = MemProcessSetGrowIncrement(newSize);
	//printf("Change Heap Grow increment from %ldKB to %ldKB\n", oldSize/1024, newSize/1024);
	//oldSize = MemProcessSetLargeBlockThreshold(newSize);
	//printf("Change Large Block Threshold from %ldKB to %ldKB\n", oldSize/1024, newSize/1024);
#endif
	// file stat
	struct stat lLogFileStat;
	if(::stat(ipLogFileName, &lLogFileStat))
	{
		::fprintf(stderr, "Failed to stat file %s\n", ipLogFileName);
		return false;
	}

	// Open the file
	FILE* fp = ::fopen(ipLogFileName, "rb");
	if(!fp)
	{
		::fprintf(stderr, "Failed to open log file %s\n", ipLogFileName);
		return false;
	}
	//::setvbuf(fp, NULL, _IONBF, 0);

	// Track total bytes read
	size_t lTotalReadSize = 0;

	// First read in header
	AT_CONSOLIDATED_HEADER lLogHeader;
	if(1 != ::fread(&lLogHeader, sizeof(lLogHeader), 1, fp))
	{
		::fprintf(stderr, "Failed to read log file's header %s\n", ipLogFileName);
		return false;
	}
	lTotalReadSize += sizeof(lLogHeader);

	// Print out header info
	if(lLogHeader.CheckAndPrintHeader() == false)
	{
		return false;
	}

	gTotalModules = lLogHeader.mTotalModules;
	gModules = new int [gTotalModules];
	::memset(gModules, 0, sizeof(int)*gTotalModules);

	// Prepare global data structures for the upcoming replay
	// total threads
	gTotalThreads = lLogHeader.mTotalThreads;

	gFreeByOtherThreadHash = (AllocatedBlock**)
		GetMemoryFromVMM(gFreeByOtherThreadHashSize * sizeof(AllocatedBlock*));


	// Synchronize all working threads per time stamp
	int* lpActiveWorkingThreadsMap = new int[gTotalThreads+1];

#ifdef WIN32
	gWorkingThreadsEventArry = new HANDLE [gTotalThreads+1];
	for(i=0; i<gTotalThreads+1; i++)
	{
		gWorkingThreadsEventArry[i] = ::CreateEvent(NULL,
													false, // Auto reset
													false, // Initial state, not signaled
													NULL); // unnamed event
		if(gWorkingThreadsEventArry[i] == NULL)
		{
			FATAL_ERROR("Failed to create event object", 0);
		}
		lpActiveWorkingThreadsMap[i] = 0;
	}
#else
	gWorkingThreadsLockArry = new pthread_mutex_t[gTotalThreads+1];
	for(i=0; i<gTotalThreads+1; i++)
	{
		if(::pthread_mutex_init(&gWorkingThreadsLockArry[i], NULL))
		{
			FATAL_ERROR("Failed to init mutex", -1);
		}
		if(LOCK_MUTEX(&gWorkingThreadsLockArry[i]))
		{
			FATAL_ERROR("Failed to lock mutex", -1);
		}
		lpActiveWorkingThreadsMap[i] = 0;
	}
#endif

	// Create required number of threads
	ThreadID_t* ptids = new ThreadID_t[gTotalThreads];
	int* lThreadIndex = new int[gTotalThreads];
	for(i=0; i<gTotalThreads; i++)
	{
		lThreadIndex[i] = i+1;
#ifdef WIN32
		ptids[i] = ::CreateThread(0, 0,
									LPTHREAD_START_ROUTINE(ThreadFunc),
									(LPVOID)&lThreadIndex[i], 0, NULL);
		if(ptids[i] == NULL)
#else
		if(::pthread_create(&ptids[i], NULL, ThreadFunc, &lThreadIndex[i]) )
#endif
		{
			FATAL_ERROR("Error: creating thread", i);
		}
	}

	// Transaction Stats
	size_t lTotalOutstadningBytes = 0;
	at_seqnum_t lTotalMalloc = 0;
	at_seqnum_t lTotalFree = 0;
	at_seqnum_t lTotalRealloc = 0;
	at_seqnum_t lTotalUnknownFree = 0;

	// Sequence numuber starts with 1, 0 is reserved for unmatched.
	at_seqnum_t lRecSeqNumber = 1;
	const int FIN_STEP = 10;
	int FinishedPercentage = FIN_STEP;

	// Lock once
	// Window uses event which is created with non-signaled as initial state
#ifndef WIN32
	LOCK_MUTEX(&gControllerLock);
#endif

	DiskIOJob diskjob(fp);
	// Spawn a thread to read records from disk
	ThreadID_t diskTID;
#ifdef WIN32
	diskTID = ::CreateThread(0, 0, LPTHREAD_START_ROUTINE(DiskIOFunc),
									(LPVOID)&diskjob, 0, NULL);
	if(diskTID == NULL)
#else
	if(::pthread_create(&diskTID, NULL, DiskIOFunc, &diskjob) )
#endif
	{
		FATAL_ERROR("Error: creating thread", 0);
	}

	// Get the global job queue
	const int numQueueBuffer = 2;
	char*     pBuffer        = (char*) malloc(gJobQueueBufferSz*numQueueBuffer);
	char*     pQueueBuffers[numQueueBuffer];
	for (i=0; i<numQueueBuffer; i++)
	{
		pQueueBuffers[i] = &pBuffer[i*gJobQueueBufferSz];
	}

	size_t lReadSize = gJobQueueBufferSz - sizeof(AT_CONSOLIDATED_REALLOC);
	if (lReadSize + lTotalReadSize > lLogFileStat.st_size)
		lReadSize = lLogFileStat.st_size - lTotalReadSize;
	// Fill one buffer
	diskjob.StartRead(pQueueBuffers[0] + sizeof(AT_CONSOLIDATED_REALLOC), lReadSize);
	lTotalReadSize += lReadSize;

	//gpJobs.mpRecordBuffer = pQueueBuffers[1];
	// Set job queue buffer
	char* lpBufferStart = NULL; //&gpJobs.mpRecordBuffer[0];
	//char* lpBufferEnd   = NULL; //&gpJobs.mpRecordBuffer[gJobQueueBufferSz];

	char* lpActiveBufStart = lpBufferStart;
	char* lpActiveBufEnd   = lpBufferStart;

	bool  lbRefillBuffer = true;

	// Go through all records
	while(lTotalReadSize < lLogFileStat.st_size || lpActiveBufEnd != lpActiveBufStart || lRecSeqNumber-1 < lLogHeader.mTotalRecords)
	{
		// Prepare the buffer
		if (lbRefillBuffer || lpActiveBufEnd - lpActiveBufStart < sizeof(AT_CONSOLIDATED_REALLOC) )
		{
			// Get the buffer from the thread
			while (diskjob.mStatus == DISK_JOB_STATUS_READ)
			{
				YIELD();
			}

			// Move the leftover records to the beginning of the buffer
			size_t lLeftOverSz = lpActiveBufEnd - lpActiveBufStart;

			lpBufferStart = gpJobs.mpRecordBuffer = diskjob.mpBuffer - lLeftOverSz;
			if (lLeftOverSz > 0)
			{
				::memcpy(lpBufferStart, lpActiveBufStart, lLeftOverSz);
			}
			lpActiveBufStart = lpBufferStart;
			lpActiveBufEnd   = diskjob.mpBuffer + diskjob.mReadSz;

			// Calc how many bytes to read from file, align it on 16 bytes
			lReadSize = gJobQueueBufferSz - sizeof(AT_CONSOLIDATED_REALLOC);
			lReadSize &= ~((size_t)(16-1));

			if (lTotalReadSize + lReadSize > lLogFileStat.st_size)
			{
				lReadSize = lLogFileStat.st_size - lTotalReadSize;
			}
			// Read from log file
			if (lReadSize > 0)
			{
				char* pBuffer;
				// Choose the buffer
				if (diskjob.mpBuffer == pQueueBuffers[0] + sizeof(AT_CONSOLIDATED_REALLOC))
					pBuffer = pQueueBuffers[1] + sizeof(AT_CONSOLIDATED_REALLOC);
				else if (diskjob.mpBuffer == pQueueBuffers[1] + sizeof(AT_CONSOLIDATED_REALLOC))
					pBuffer = pQueueBuffers[0] + sizeof(AT_CONSOLIDATED_REALLOC);
				else
				{
					FATAL_ERROR("Impossible switch: queue buffer", 0);
				}

				// Read in a chunk
				diskjob.StartRead(pBuffer, lReadSize);
				lTotalReadSize += lReadSize;
			}
			// done refill buffer
			lbRefillBuffer = false;
		}

		// Clean up map
		for(i=0; i<gTotalThreads+1; i++)
		{
			lpActiveWorkingThreadsMap[i] = 0;
		}

		// Init the job queue buffer
		gpJobs.Reset(lpActiveBufStart-lpBufferStart);

		// The inner loop go through all records between current timestamp and next timestamp.
		// or when queue buffer is full
		while(lpActiveBufStart + sizeof(AT_CONSOLIDATED_GENERIC) < lpActiveBufEnd)
		{
			size_t lRecordSz = 0;

			AT_CONSOLIDATED_GENERIC* lpNextRecord = (AT_CONSOLIDATED_GENERIC*)lpActiveBufStart;

			// It is a timestamp
			if (lpNextRecord->mType == AT_MEM_OP_TYPE_TIMESTAMP)
			{
				// Log memory footprint as required
				if (gbLogMemUsage)
				{
					PrintMemFootPrint(((AT_CONSOLIDATED_TIMESTAMP*)lpNextRecord)->mTimeStamp,
										lTotalOutstadningBytes/1024);
				}

				lRecordSz = sizeof(AT_CONSOLIDATED_TIMESTAMP);
			}
			// It is either malloc/free/realloc
			else
			{
				// Mark the thread that the job belongs to
				lpActiveWorkingThreadsMap[lpNextRecord->mThreadID] = 1;
				// remember first record's sequence no.
				if (gpJobs.mTotalJobs == 0)
				{
					gpJobs.mSequenceNo = lRecSeqNumber;
				}
				gpJobs.mTotalJobs++;

				lRecSeqNumber++;
				// print out status of fulfilled jobs
				if ((unsigned long)lRecSeqNumber*100/(unsigned long)lLogHeader.mTotalRecords >= FinishedPercentage)
				{
					fprintf(stderr, "Processing [%ld]th records. %d%%\n", lRecSeqNumber, FinishedPercentage);
					FinishedPercentage += FIN_STEP;
				}

				// Bookkeeping memory usage by application, not memory manager.
				if(lpNextRecord->mType == AT_MEM_OP_TYPE_MALLOC)
				{
					lRecordSz = sizeof(AT_CONSOLIDATED_MALLOC);
					lTotalMalloc++;
					lTotalOutstadningBytes += ALIGN_EIGHT_BYTE(((AT_CONSOLIDATED_MALLOC*)lpNextRecord)->mSize);
				}
				else if(lpNextRecord->mType == AT_MEM_OP_TYPE_FREE)
				{
					lRecordSz = sizeof(AT_CONSOLIDATED_FREE);
					lTotalFree++;
					// If we know the allocator
					if(((AT_CONSOLIDATED_FREE*)lpNextRecord)->mMatchedMalloc)
					{
						lTotalOutstadningBytes -= ALIGN_EIGHT_BYTE(((AT_CONSOLIDATED_FREE*)lpNextRecord)->mSize);
					}
				}
				else if(lpNextRecord->mType == AT_MEM_OP_TYPE_REALLOC)
				{
					lRecordSz = sizeof(AT_CONSOLIDATED_REALLOC);
					lTotalRealloc++;
					// If we know the allocator
					if(((AT_CONSOLIDATED_REALLOC*)lpNextRecord)->mMatchedMalloc)
					{
						lTotalOutstadningBytes -= ALIGN_EIGHT_BYTE(((AT_CONSOLIDATED_REALLOC*)lpNextRecord)->mOldSize);
					}
					lTotalOutstadningBytes += ALIGN_EIGHT_BYTE(((AT_CONSOLIDATED_REALLOC*)lpNextRecord)->mNewSize);
				}
				else
				{
					FATAL_ERROR("Fatal: Unknown type", -1);
				}
			}

			lpActiveBufStart += lRecordSz;
			if (lpActiveBufStart > lpActiveBufEnd)
			{
				lbRefillBuffer = true;
				lpActiveBufStart -= lRecordSz;
				if (lpNextRecord->mType != AT_MEM_OP_TYPE_TIMESTAMP)
				{
					gpJobs.mTotalJobs--;
					lRecSeqNumber--;
				}
				break;
			}

			if (lpNextRecord->mType == AT_MEM_OP_TYPE_TIMESTAMP)
			{
				if (gIgnoreTimeStamp)
				{
					continue;
				}
				else
				{
					break;
				}
			}
		}

		// Find threads that have job to do
		int lNumActiveThreads = 0;
		for(i=0; i<gTotalThreads+1; i++)
		{
			if(lpActiveWorkingThreadsMap[i])
			{
				lNumActiveThreads++;
			}
		}
		// Back to loop if no job is to be done
		if(lNumActiveThreads == 0)
		{
			continue;
		}
		// Set this number to signal me (control thread) when all working threads are through
		gNumWorkingThreads = lNumActiveThreads;

		// Now wake up all threads that have jobs to do
		for(i=0; i<gTotalThreads+1; i++)
		{
			if(lpActiveWorkingThreadsMap[i])
			{
#ifdef WIN32
				if(false == ::SetEvent(gWorkingThreadsEventArry[i]))
				{
					FATAL_ERROR("Failed to SetEvent", GetLastError());
				}
#else
				if (UNLOCK_MUTEX(&gWorkingThreadsLockArry[i]))
				{
					FATAL_ERROR("Failed to unlock mutex", -1);
				}
#endif
			}
		}

		// Lock for the 2nd time, I am going to block myself
		// Wait until all threads finish their jobs
		// The last thread that finishes its job shall release me
#ifdef WIN32
		if(WAIT_FAILED == ::WaitForSingleObject(gControllerEvent, INFINITE))
		{
			FATAL_ERROR("Failed to wait for event", GetLastError());
		}
#else
		LOCK_MUTEX(&gControllerLock);
#endif

		// If user only wants to replay part of the records.
		if(lRecSeqNumber > gEndRecNo)
		{
			break;
		}
	}

	diskjob.Stop();

	// All done, send flag to all threads to exit now.
	gAllDone = true;
	for(i=0; i<gTotalThreads+1; i++)
	{
#ifdef WIN32
		::SetEvent(gWorkingThreadsEventArry[i]);
#else
		if (UNLOCK_MUTEX(&gWorkingThreadsLockArry[i]))
		{
			FATAL_ERROR("Failed to unlock mutex", -1);
		}
#endif
	}

	// Done with my lock.
#ifndef WIN32
	UNLOCK_MUTEX(&gControllerLock);
#endif

	// Display some stats
	if(gbLogMemUsage)
	{
		printf("Total_Malloc: %ld\n", lTotalMalloc);
		printf("Total_Free: %ld\n", lTotalFree);
		printf("Total_Realloc: %ld\n", lTotalRealloc);
		printf("Total_UnMatched_Free: %ld\n", lTotalUnknownFree);
	}

	// check how much we did
	if(lTotalReadSize != lLogFileStat.st_size)
	{
		::fprintf(stderr, "Error: Log file has size [%ld] while we only read [%ld]\n",
			lLogFileStat.st_size, lTotalReadSize);
	}

	if(lRecSeqNumber-1 != lLogHeader.mTotalRecords)
	{
		::fprintf(stderr, "Error: Log file says [%ld] records while we only process [%ld] ones\n",
			lLogHeader.mTotalRecords, lRecSeqNumber-1);
	}

	// done, join all threads
	for(i=0; i<gTotalThreads; i++)
	{
		if(THREAD_JOIN(ptids[i]))
		{
			FATAL_ERROR("Error join thread", i);
		}
	}

	if(THREAD_JOIN(diskTID))
	{
		FATAL_ERROR("Error join thread", i);
	}

	if(::fclose(fp))
	{
		::fprintf(stderr, "Failed to close log file %s\n", ipLogFileName);
		return false;
	}

	// Left over in the tree for cross-thread outstanding memory
	if (gFreeAllAtEnd)
	{
		for (int i=0; i<gFreeByOtherThreadHashSize; i++)
		{
			AllocatedBlock* lpHashEntry = gFreeByOtherThreadHash[i];
			while (lpHashEntry)
			{
				::free(lpHashEntry->mpBlock);
				lpHashEntry = lpHashEntry->mpNext;
			}
		}
		FreeInternalBuffer();
	}
	FreeMemoryToVMM(gFreeByOtherThreadHash, gFreeByOtherThreadHashSize * sizeof(AllocatedBlock*));

	// done.
	//free(gpJobs.mpRecordBuffer);
	delete[] pBuffer;
	return true;
}

#if defined(_SMARTHEAP) && defined(_HEAP_PROFILE)
extern "C" void HeapEventHandler(MEM_SIZET size, MEM_POOL context)
{
	if(gLogHeapProfile)
	{
		MemLogHeap(size, context);
	}

	/*if(gHuntFragmentor)
	{
		const int MaxModules = 256;
		int lModules[MaxModules];

		if(MaxModules < gTotalModules)
		{
			::fprintf(stderr, "sorry, can't handle %d modules for fragmentation search\n", gTotalModules);
		}
		else if(MemGetFragmentor(size, context, lModules, MaxModules))
		{
			for(int i=0; i<gTotalModules; i++)
			{
				if(lModules[i])
				{
					gModules[i]++;
				}
			}
		}
	}*/
}
#endif

typedef struct _ModuleCount
{
	short mModuleNum;
	short mCount;
} ModuleCount;

int IntCompFunc(const void* i1, const void* i2)
{
	int left  = ((ModuleCount*)i1)->mCount;
	int right = ((ModuleCount*)i2)->mCount;
	return left-right;
}

////////////////////////////////////////////////////////////////////
// Fun starts here.
////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
	AT_Init_MallocReplay_cpp();

	if(argc < 2 || 0==::strcmp(argv[1], "-h")
		|| (argv[1][0]=='-' && argc < 3) )
	{
		printf("Usage: %s [-p|-t|-h] LogFile\n", argv[0]);
		printf("\t[no arguments] Redo all malloc/free in log file\n");
		printf("\t[-p] Print out all operations without execution.\n");
		printf("\t[-t] Timing analysis. \n");
		printf("\t[-h] This help message.\n");
		printf("\tEnvironment Variables under various mode:\n");
		printf("\t\tAT_LOG_MEM_USAGE=1    [replay mode] log process vss/rss in a file while replaying\n");
		printf("\t\tAT_LOG_HEAP_PROFILE=1 [replay mode] enable heap profile logging\n");
		/*printf("\t\tAT_HUNT_FRAGMENTOR=1  [replay mode] try to find which module fragments heap the most\n");*/
		printf("\t\tAT_TEST_BASELINE=1    [replay mode} replay without actual malloc/free in order to measure the baseline time\n");
		printf("\t\tAT_TOUCH_MEMORY=1     [replay mode] allocated memory is written to trigger page-on-demand.\n");
		printf("\t\tAT_FREE_ALL_AT_END=1  [replay mode] free all outstanding blocks before exit.\n");
		printf("\t\tAT_IGNORE_TIMESTAMP=1 [replay mode] roughly follow timestamp but not exact\n");
		printf("\t\tAT_QUEUE_BUFFER_SIZE=nnn     [replay mode] specify the buffer size other than default %d\n", QUEUE_RECORD_BUFFER_SZ);
		printf("\t\tAT_START_RECORD=n     [print mode] start from record number n.\n");
		printf("\t\tAT_END_RECORD=n       [print mode] last record number n.\n");
		exit(0);
	}

	const char* LogFileName;

	if(0==::strcmp(argv[1], "-p"))
	{
		gbPrintOnly = true;
		LogFileName = argv[2];
	}
	else if(0==::strcmp(argv[1], "-t"))
	{
		gbTimingAnalysis = true;
		LogFileName = argv[2];
	}
	else
	{
		LogFileName = argv[1];
	}

	// env var
	const char* lEnvStr = ::getenv("AT_START_RECORD");
	if(lEnvStr)
	{
		gStartRecNo = ::atol(lEnvStr);
	}
	lEnvStr = ::getenv("AT_END_RECORD");
	if(lEnvStr)
	{
		gEndRecNo = ::atol(lEnvStr);
	}

	// Get system page size
	GET_SYSTEM_PAGE_SIZE(gPageSize);

	// The real work
	if(gbPrintOnly)
	{
		PrintAll(LogFileName);
	}
	else if(gbTimingAnalysis)
	{
		AnalyzeTiming(LogFileName);
	}
	else
	{
		// env var relating to replay
		if(::getenv("AT_LOG_MEM_USAGE"))
		{
			gbLogMemUsage = true;
#ifdef WIN32
			InitPerfCounter();
#endif
		}

		if(::getenv("AT_TOUCH_MEMORY"))
		{
			gTouchMemory = true;
		}

		if (::getenv("AT_FREE_ALL_AT_END"))
		{
			gFreeAllAtEnd = true;
		}

		if (::getenv("AT_IGNORE_TIMESTAMP"))
		{
			gIgnoreTimeStamp = true;
		}

		if (::getenv("AT_TEST_BASELINE"))
		{
			gTestBaseline = true;
		}

		lEnvStr = ::getenv("AT_QUEUE_BUFFER_SIZE");
		if (lEnvStr)
		{
			gJobQueueBufferSz = ::atol(lEnvStr);
			// sanity check
			if (gJobQueueBufferSz < 16*sizeof(AT_CONSOLIDATED_REALLOC))
			{
				gJobQueueBufferSz = ALIGN_EIGHT_BYTE(QUEUE_RECORD_BUFFER_SZ);
			}
			gJobQueueBufferSz = ALIGN_EIGHT_BYTE(gJobQueueBufferSz);
		}

#if defined(_SMARTHEAP) && defined(_HEAP_PROFILE)
		if(::getenv("AT_LOG_HEAP_PROFILE"))
		{
			gLogHeapProfile = true;
		}
		/*if(::getenv("AT_HUNT_FRAGMENTOR"))
		{
			gHuntFragmentor = true;
		}*/

		/*if(gLogHeapProfile || gHuntFragmentor)
		{
			// Make sure default pool is initialized before start heap logging.
			// in order to prevent recursive infinite loop
			void* lpDummy = ::malloc(1);
			MemSetHeapEventCallBack(HeapEventHandler);
		}*/
#endif

		ReplayAll(LogFileName);

		/*if(gHuntFragmentor && gModules)
		{
			int i;
			for(i=0; i<gTotalModules; i++)
			{
				short count = gModules[i];
				ModuleCount* lpModuleCount = (ModuleCount*) &gModules[i];
				lpModuleCount->mModuleNum = i;
				lpModuleCount->mCount     = count;
			}
			::qsort(gModules, gTotalModules, sizeof(int), IntCompFunc);
			printf("The following modules maybe the cause of fragmentation\n");
			for(i=0; i<gTotalModules; i++)
			{
				ModuleCount* lpModuleCount = (ModuleCount*) &gModules[i];
				printf("\tModule[%d] ==> %d\n", lpModuleCount->mModuleNum, lpModuleCount->mCount);
			}
		}*/
	}

	AT_Fini_MallocReplay_cpp();

#ifdef WIN32
	if(gbLogMemUsage)
	{
		FiniPerfCounter();
	}
#endif

#if defined(_SMARTHEAP) && defined(_HEAP_PROFILE)
	if(gbLogMemUsage)
	{
		PrintMemFootPrint(0, 0, true);
	}
	if (gLogHeapProfile)
		MemLogHeap(0, NULL);
#endif

	if (gFreeAllAtEnd && GetInternalBufferSz() > 0)
	{
		fprintf(stderr, "Internal buffer has %ld bytes leaked\n", GetInternalBufferSz());
	}

	//printf("pause ... hit [ret] key to contiune\n");
	//char c = ::getchar();

	return 0;
}

/////////////////////////////////////////////////////////////////////////
// Internal buffer to build tree of small allocated memory block
/////////////////////////////////////////////////////////////////////////
struct SINGLE_LINK_LIST_NODE
{
	SINGLE_LINK_LIST_NODE* mpNext;
	size_t                 mSize;
};

#define SINGLE_LINK_LIST_INSERT(head, node) \
	{ \
		if((head) == NULL) \
		{ \
			(head) = (SINGLE_LINK_LIST_NODE*)(node); \
			(head)->mpNext = NULL; \
		} \
		else \
		{ \
			((SINGLE_LINK_LIST_NODE*)(node))->mpNext = (head); \
			(head) = (SINGLE_LINK_LIST_NODE*)(node); \
		} \
	}

#define SINGLE_LINK_LIST_POP(head, node) \
	{ \
		(node) = (head); \
		if ((head)) \
			(head) = (head)->mpNext; \
	}

static size_t gTotalBufferSz = 0;

static SINGLE_LINK_LIST_NODE* gRawChuncks = NULL;

size_t GetInternalBufferSz()
{
	return gTotalBufferSz;
}

// Assume there is no reference of this buffer any more
void FreeInternalBuffer()
{
	while(gRawChuncks)
	{
		SINGLE_LINK_LIST_NODE* node = gRawChuncks;
		gRawChuncks = gRawChuncks->mpNext;
		FreeMemoryToVMM((void*)node, node->mSize);
	}
}

void FreeMemoryToVMM(void* addr, size_t sz)
{
#if defined(_AIX) || defined(__hpux) || defined(sun) || defined(linux)
		::munmap((char*)addr, sz);
#elif defined(WIN32)
		::VirtualFree(addr, 0, MEM_RELEASE);
#endif
		gTotalBufferSz -= sz;
}

// memory for internal usage. avoid allocating from memory manager
// so that we can keep correct counters
void* GetMemoryFromVMM(size_t sz)
{
	void* lpResult;
	bool  bFailed;

	// align size to page boundary
	sz = (sz + gPageSize - 1) & ~(gPageSize - 1);

#if defined(_AIX) || defined(__hpux) || defined(sun) || defined(linux)
	lpResult = ::mmap(NULL, sz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	bFailed  = (lpResult == (void*)-1);

#elif defined(WIN32)
	lpResult = ::VirtualAlloc(NULL, sz, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
	bFailed  = (lpResult == NULL);
#else
#pragma error Platform not supported
#endif

	if(bFailed)
	{
		FATAL_ERROR("Failed to get a chunk from VMM", sz);
	}
	else
	{
		gTotalBufferSz += sz;
	}

	return lpResult;
}


static SINGLE_LINK_LIST_NODE* gFreeInternalBlocks = NULL;

static spinlock_t gInternalBufferLock = SPINLOCK_UNLOCKED;

// Add a big chunk of memory to the free tree node buffer
static void GrowInternalBuffer()
{
	const size_t sz = 1024*1024;
	SINGLE_LINK_LIST_NODE* node = (SINGLE_LINK_LIST_NODE*) GetMemoryFromVMM(sz);
	node->mSize = sz;
	SINGLE_LINK_LIST_INSERT(gRawChuncks, node);

	node++;
	node->mSize = sz - sizeof(SINGLE_LINK_LIST_NODE);
	SINGLE_LINK_LIST_INSERT(gFreeInternalBlocks, node);
}

static void* GetBlockInternal(size_t sz)
{
	SINGLE_LINK_LIST_NODE* node;

	spinlock_lock(gInternalBufferLock);

	// If current buffer is depleted, add more
	if (gFreeInternalBlocks == NULL)
	{
		GrowInternalBuffer();
	}

	// Pop the first free node, split it if it is big
	SINGLE_LINK_LIST_POP(gFreeInternalBlocks, node);
	if (node->mSize >= sz*2)
	{
		SINGLE_LINK_LIST_NODE* new_node = (SINGLE_LINK_LIST_NODE*)((char*)node + sz);
		new_node->mSize = node->mSize - sz;
		SINGLE_LINK_LIST_INSERT(gFreeInternalBlocks, new_node);
	}

	spinlock_unlock(gInternalBufferLock);

	return (void*) node;
}

static void  ReleaseBlockInternal(void* p, size_t sz)
{
	spinlock_lock(gInternalBufferLock);

	((SINGLE_LINK_LIST_NODE*)p)->mSize = sz;
	SINGLE_LINK_LIST_INSERT(gFreeInternalBlocks, (SINGLE_LINK_LIST_NODE*)p);

	spinlock_unlock(gInternalBufferLock);
}
