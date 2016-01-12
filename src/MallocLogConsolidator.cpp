/************************************************************************
** FILE NAME..... MallocLogConsolidator.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Consolidate all records(scattered in per-thread files)
** in one file with memory allocate/deallocate matched.
**
** NOTES............  
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
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <vector>
#include <map>
#include <list>
#include <set>

#include "CrossPlatform.h"
#include "AccuTrak.h"
#include "MallocLogger.h"
#include "MmapFile.h"
#include "Utility.h"

#define LINE_DIVIDER \
"===================================================================\n"

#define WRITE_FILE(buffer, size, n, file, filename) \
	if(1 != ::fwrite((buffer), (size), (n), (file))) \
	{ \
		fprintf(stderr, "Failed to write record into file %s\n", (filename)); \
		return false; \
	}

////////////////////////////////////////////////////////////////////
// Global vars
////////////////////////////////////////////////////////////////////
AT_REC_HEADER gInfoHeader;

// Flags 64bit machine is processing 32bit log files
static bool gb64BitProcess32Bit = false;

// Cached value of struct size of logged records
// They are test machine size specifed by log info file
size_t gMallocRecordSz = sizeof(AT_MALLOC_RECORD);
size_t gFreeRecordSz = sizeof(AT_FREE_RECORD);
size_t gReallocRecordSz = sizeof(AT_REALLOC_RECORD);

// Vecotr of all module names specifed by log info file
std::vector<const char*> gModuleNames;

// Collection of mmap opened log files
std::vector<MmapPerThreadLogFile*> gMmapFiles;

// Collection of allocated blocks sorted by address.
// raw block address in log file ==> consolidated record's sequence number
struct ALLOCATED_BLOCK
{
	void* mpRawAddr;
	struct ALLOCATED_BLOCK* mNext;
	at_timestamp_t mTimeStamp;
	at_seqnum_t mSeqNo;
	unsigned int  mThreadID; // Thread that allocated this block
	unsigned int  mSize;

	ALLOCATED_BLOCK(void* p, at_timestamp_t t, at_seqnum_t n, unsigned int id, unsigned int sz)
		: mpRawAddr(p), mTimeStamp(t), mSeqNo(n), mThreadID(id), mSize(sz), mNext(NULL) {}
};

// Compare function of ALLOCATED_BLOCK set
struct lt_ALLOCATED_BLOCK
{
  bool operator()(const ALLOCATED_BLOCK* b1, const ALLOCATED_BLOCK* b2) const
  {
    return (b1->mpRawAddr < b2->mpRawAddr);
  }
};

typedef std::set<ALLOCATED_BLOCK*, lt_ALLOCATED_BLOCK> ALLOCATED_BLOCK_SET;
typedef std::set<void*> BLOCK_SET;

// These blocks are allocated, but not freed before the same address is
// allocated to another block.
//ALLOCATED_BLOCK_SET gErrorBlocks;

// Freed blocks that can't find its allocator
BLOCK_SET gUnknownFreeBlocks;

// Remember blocks' seq_no that are allocated by one thread,
// and freed by other thread.
// We'll reset the record's mAllocatedByOtherThread bit in the 2nd pass
std::set<at_seqnum_t> gBlocksFreedByOtherThread;

// Limit the size of outstanding block set
// If it is too big, which is not unusual, the performance suffers a lot
// Hash Raw block address to the array index.
#define TOTAL_BUCKETS 8192
// Hash function
#define ADDR2INDEX(p) (int)(((unsigned long)(p) >> 4) & (TOTAL_BUCKETS - 1))
// Array of sets indexed by hashed raw address
ALLOCATED_BLOCK_SET* gBlockSetArray[TOTAL_BUCKETS];

////////////////////////////////////////////////////////////////////
// Class implementation
////////////////////////////////////////////////////////////////////

// Thread id is typed 24bit unsigned int
static unsigned int CheckThreadID(unsigned int iTID)
{
	if(iTID >= MAX_THREAD_ID)
	{
		fprintf(stderr, "There are too many threads %d, combine extra to thread [%d]\n",
			iTID, MAX_THREAD_ID-1);
		return MAX_THREAD_ID-1;
	}
	return iTID;
}

// Block size is typed unsigned int instead of size_t
static unsigned int CheckBlockSize(size_t iSize)
{
	if(iSize >= MAX_BLOCK_SIZE)
	{
		fprintf(stderr, "The malloc size [0x%lx] is too big to handle\n", iSize);
		return UINT_MAX;
	}
	return iSize;
}

// ctor
AT_CONSOLIDATED_TIMESTAMP::AT_CONSOLIDATED_TIMESTAMP(unsigned int iTimeStamp)
	: mType(AT_MEM_OP_TYPE_TIMESTAMP), mReserve(0), mTimeStamp(iTimeStamp)
{
}

// ctor
AT_CONSOLIDATED_MALLOC::AT_CONSOLIDATED_MALLOC(unsigned int iTID, unsigned int iModuleNum, size_t iSize)
	: mType(AT_MEM_OP_TYPE_MALLOC), mFreeByOtherThread(0), mReserve(0),
	mModuleNum(iModuleNum), mThreadID(CheckThreadID(iTID)), mSize(CheckBlockSize(iSize))
{	
}

// ctor
AT_CONSOLIDATED_FREE::AT_CONSOLIDATED_FREE(unsigned int iTID, unsigned int iModuleNum,
										   at_seqnum_t iMallocNo, size_t iSize,
										   unsigned int iAllocatedByOtherThread)
	: mType(AT_MEM_OP_TYPE_FREE), mAllocatedByOtherThread(iAllocatedByOtherThread), mReserve(0),
	mModuleNum(iModuleNum), mMatchedMalloc(iMallocNo), mSize(iSize), mThreadID(CheckThreadID(iTID))
{
}

// ctor
AT_CONSOLIDATED_REALLOC::AT_CONSOLIDATED_REALLOC(unsigned int iTID, unsigned int iModuleNum,
												 at_seqnum_t iMallocNo, size_t iNewSize, size_t iOldSize,
												 unsigned int iAllocatedByOtherThread)
	: mType(AT_MEM_OP_TYPE_REALLOC), mAllocatedByOtherThread(iAllocatedByOtherThread), mFreeByOtherThread(0),
	mModuleNum(iModuleNum), mThreadID(CheckThreadID(iTID)), mNewSize(CheckBlockSize(iNewSize)), mOldSize(iOldSize),
	mMatchedMalloc(iMallocNo)
{
}

////////////////////////////////////////////////////////////////////
// Helper
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// Read info file
// Print some basics and set a few global vars related.
////////////////////////////////////////////////////////////////////
static bool ReadInfoFile(const char* iInfoFileName)
{
	bool rc = true;

	MmapFile lMmapFile(iInfoFileName);

	if(lMmapFile.InitSucceed() == false)
	{
		return false;
	}

	AT_REC_HEADER* lpInfoHeader = (AT_REC_HEADER*)lMmapFile.GetStartAddr();

	// check endianess
	int lInt = 0x01;
	if(lpInfoHeader->mLittleEndian != *(char*)&lInt)
	{
		fprintf(stderr, "This machine has different endianess than the tested machine\n");
		fprintf(stderr, "Currently it can't be processed by this machine.\n");
		return false;
	}

	// print header
	if(lpInfoHeader->CheckAndPrintHeader() == false)
	{
		return false;
	}
	// save the header for later use
	::memcpy(&gInfoHeader, lpInfoHeader, sizeof(AT_REC_HEADER));

	// Special case where we use 64bit machine to consolidate 32bit machine's log files
	if(sizeof(long)==8 && !lpInfoHeader->m64Bit)
	{
		gb64BitProcess32Bit = true;
		gMallocRecordSz  -= 8;
		gFreeRecordSz    -= 4;
		gReallocRecordSz -= 12;
	}

	printf("The following %d modules are tracked for malloc/free operations:\n", lpInfoHeader->mTotalModules);
	const char* lpModuleName = (const char*)(lpInfoHeader+1);
	for(int i=0; i<lpInfoHeader->mTotalModules; i++)
	{
		if(lpModuleName > lMmapFile.GetEndAddr())
		{
			::fprintf(stderr, "Error to read module names in file %s\n", iInfoFileName);
			rc = false;
			break;
		}
		printf("[%d]: %s\n", i, lpModuleName);
		gModuleNames.push_back(::strdup(lpModuleName));
		lpModuleName += ::strlen(lpModuleName)+1;
	}

	return rc;
}

////////////////////////////////////////////////////////////////////
// Unload mmaped files on the global list
////////////////////////////////////////////////////////////////////
static bool UnloadLogFiles()
{
	for(int i=0; i< gMmapFiles.size(); i++)
	{
		MmapFile* lpMmapFile = gMmapFiles[i];
		delete lpMmapFile;
	}
	gMmapFiles.clear();

	return true;
}

////////////////////////////////////////////////////////////////////
// Open a per-thread log file,
////////////////////////////////////////////////////////////////////
static bool OpenOneThreadLogFile(const char* iLogFileName)
{
	long lThreadID;
	::sscanf(GetBaseName(iLogFileName), "TID_%ld.log", &lThreadID);

	MmapPerThreadLogFile* lpMmapFile = new MmapPerThreadLogFile(lThreadID, gMmapFiles.size()+1, iLogFileName);

	if(lpMmapFile->InitSucceed())
	{
		gMmapFiles.push_back(lpMmapFile);
	}
	else
	{
		::fprintf(stderr, "Failed to mmap file %s\n", iLogFileName);
		delete lpMmapFile;
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////
// Find which per-thread log file contains this record address
// Input: ipRecAddr is the record address in core of the mmaped log file
////////////////////////////////////////////////////////////////////
static unsigned int GetThreadIDByRecordAddr(void* ipRecAddr)
{
	for(int i=0; i<gMmapFiles.size(); i++)
	{
		MmapPerThreadLogFile* lpMmapFile = gMmapFiles[i];
		if(lpMmapFile->AddrWithinMmapFile((char*)ipRecAddr))
		{
			return lpMmapFile->GetMappedTID();
		}
	}
	// something is wrong
	fprintf(stderr, "Can't find the mmap file that contains this addr 0x%lx\n", ipRecAddr);

	return UINT_MAX;
}

////////////////////////////////////////////////////////////////////
// Choose the outstanding block sets by hashed index
////////////////////////////////////////////////////////////////////
inline ALLOCATED_BLOCK_SET* GetTheSet(void* ipRawAddr)
{
	// Hash the block address
	int lIndex = ADDR2INDEX(ipRawAddr);
	ALLOCATED_BLOCK_SET* lpSet = gBlockSetArray[lIndex];
	if(lpSet == NULL)
	{
		lpSet = new ALLOCATED_BLOCK_SET;
		gBlockSetArray[lIndex] = lpSet;
	}
	return lpSet;
}

////////////////////////////////////////////////////////////////////
// Remember the block just allocated in a global map of addr=>seq no.
////////////////////////////////////////////////////////////////////
static bool AddOutstandingBlock(void* ipRawAddr,
								at_timestamp_t iTimeStamp,
								at_seqnum_t iSeqNo,
								unsigned int iAllocThreadID,
								unsigned int iSize)
{
	ALLOCATED_BLOCK_SET* lpSet = GetTheSet(ipRawAddr);

	ALLOCATED_BLOCK* lpNewBlock = new ALLOCATED_BLOCK(ipRawAddr, iTimeStamp, iSeqNo, iAllocThreadID, iSize);
	// Insert the block into this set
	ALLOCATED_BLOCK_SET::iterator it2;
	// remember the record sequence number mapped by allocated block's address
	if( (lpSet->insert(lpNewBlock)).second == false)
	{
		// There is already an outstanding block with the same address.		
		// We need to remember the new one
		it2	= lpSet->find(lpNewBlock);
		if(it2 == lpSet->end())
		{
			// something goes really wrong.
			fprintf(stderr, "Impossible condition.\n");
			::abort();
		}
		// Put the new block at the end of existing block's list, they have the same raw addr
		ALLOCATED_BLOCK* lpOldBlock = *it2;
		while(lpOldBlock->mNext)
		{
			lpOldBlock = lpOldBlock->mNext;
		}
		lpOldBlock->mNext = lpNewBlock;

		//fprintf(stderr, "Block 0x%lx is never freed before the same address is reused\n", ipRawAddr);
		// Put this block into error map
		//if( (gErrorBlocks.insert(*it2)).second == false)
		//{
		//	fprintf(stderr, "Error Block 0x%lx is already in set\n", (*it2)->mpRawAddr);
		//}
	}

	return true;
}

////////////////////////////////////////////////////////////////////
// Find the sequence no (malloc no) using the block address.
////////////////////////////////////////////////////////////////////
static at_seqnum_t GetMallocNumber(void* ipRawAddr,
								  unsigned int iFreeThreadID,
								  unsigned int& orAllocatedByOtherThread,
								  unsigned int& orSize)
{
	if(!ipRawAddr)
	{
		//::fprintf(stderr, "!Freed block has address NULL\n");
		return 0;
	}

	// Init result
	orAllocatedByOtherThread = 0;
	orSize = UINT_MAX;
	at_seqnum_t lMallocSeqNo = 0;

	ALLOCATED_BLOCK_SET* lpSet = GetTheSet(ipRawAddr);
	ALLOCATED_BLOCK_SET::iterator it2;
	ALLOCATED_BLOCK lBlock(ipRawAddr, 0, 0, 0, 0);
	it2	= lpSet->find(&lBlock);
	// Find the record sequence number corresponding to the allocated block's address
	if(it2 != lpSet->end())
	{
		ALLOCATED_BLOCK* lpBlock = *it2;
		// Normally there is only one block found
		if(lpBlock->mNext == NULL)
		{
			// Remove this node as it is freed
			lpSet->erase(it2);
		}
		// if there are multiple blocks with the same addr, 
		// get the one allocated by the same thread and the oldest
		else
		{
			ALLOCATED_BLOCK* lpCursor = lpBlock;
			ALLOCATED_BLOCK* lpPrev = NULL;
			while(lpCursor)
			{
				if(lpCursor->mThreadID == iFreeThreadID)
				{
					// unlink the found block
					if(lpPrev)
					{
						lpPrev->mNext = lpCursor->mNext;
					}
					break;
				}
				lpPrev = lpCursor;
				lpCursor = lpCursor->mNext;
			}
			// If all blocks are allocate by other thread, just get the oldest one.
			if(lpCursor == NULL)
			{
				lpCursor = lpBlock;
			}
			// So far we have decided which block to free.
			if(lpCursor == lpBlock)
			{
				// if found block is the head of the list.
				// The global set needs to be changed to point to the new head list
				ALLOCATED_BLOCK* lpHeadBlock = lpBlock->mNext;
				lpSet->erase(it2);
				if((lpSet->insert(lpHeadBlock)).second == false)
				{
					::fprintf(stderr, "Impossible condition\n");
					::abort();
				}
			}
			// Set the found pointer
			lpBlock = lpCursor;
		}
		// The following is common to single/multiple blocks
		lMallocSeqNo = lpBlock->mSeqNo;
		orSize       = lpBlock->mSize;
		if(lpBlock->mThreadID != iFreeThreadID)
		{
			gBlocksFreedByOtherThread.insert(lpBlock->mSeqNo);
			orAllocatedByOtherThread = 1;
		}
		delete lpBlock;
	}
	else
	{	
		/*it2 = gErrorBlocks.find(&lBlock);
		if(it2 != gErrorBlocks.end())
		{
			// The freed block is in error block set.
			ALLOCATED_BLOCK* lpBlock = *it2;
			lMallocSeqNo = lpBlock->mSeqNo;
			orSize       = lpBlock->mSize;
			if(lpBlock->mThreadID != iFreeThreadID)
			{
				gBlocksFreedByOtherThread.insert(lpBlock->mSeqNo);
				orAllocatedByOtherThread = 1;
			}
			// Remove this node as it is freed
			delete lpBlock;
			gErrorBlocks.erase(it2);
		}
		else*/
		{
			if((gUnknownFreeBlocks.insert(ipRawAddr)).second == false)
			{
				//::fprintf(stderr, "2nd unknown Free blocks 0x%lx\n", ipRawAddr);
			}
		}
	}

	return lMallocSeqNo;
}

////////////////////////////////////////////////////////////////////
// Helpers to bridge 32bit log files processed by 64bit tool
// NOTE: this is hard-coded by struct's memory layout, 
//       keep sync with MallocLogger.h
////////////////////////////////////////////////////////////////////
static void* GetAllocatedBlockPtr(AT_MALLOC_RECORD* ipMalloc)
{
	if(gb64BitProcess32Bit)
	{
		char* lpCursor = (char*)&ipMalloc->mSize;
		lpCursor += 4;
		return (void*)(*(unsigned int*)lpCursor);
	}
	else
	{
		return ipMalloc->mpAllocatedBlock;
	}
}

static void* GetFreedBlockPtr(AT_FREE_RECORD* ipFree)
{
	if(gb64BitProcess32Bit)
	{
		char* lpCursor = (char*)&ipFree->mpFreedBlock;
		return (void*)(*(unsigned int*)lpCursor);
	}
	else
	{
		return ipFree->mpFreedBlock;
	}
}

static void* GetAllocatedBlockPtr(AT_REALLOC_RECORD* ipRealloc)
{
	if(gb64BitProcess32Bit)
	{
		char* lpCursor = (char*)&ipRealloc->mNewSize;
		lpCursor += 8;
		return (void*)(*(unsigned int*)lpCursor);
	}
	else
	{
		return ipRealloc->mpNewBlock;
	}
}

static void* GetFreedBlockPtr(AT_REALLOC_RECORD* ipRealloc)
{
	if(gb64BitProcess32Bit)
	{
		char* lpCursor = (char*)&ipRealloc->mpOldBlock;
		return (void*)(*(unsigned int*)lpCursor);
	}
	else
	{
		return ipRealloc->mpOldBlock;
	}
}

static AT_GENERIC_RECORD* GetNextRecord(AT_MALLOC_RECORD* ipMalloc)
{
	if(gb64BitProcess32Bit)
	{
		return (AT_GENERIC_RECORD*) ((char*)ipMalloc + gMallocRecordSz);
	}
	else
	{
		return (AT_GENERIC_RECORD*) (ipMalloc+1);
	}
}

static AT_GENERIC_RECORD* GetNextRecord(AT_FREE_RECORD* ipFree)
{
	if(gb64BitProcess32Bit)
	{
		return (AT_GENERIC_RECORD*) ((char*)ipFree + gFreeRecordSz);
	}
	else
	{
		return (AT_GENERIC_RECORD*) (ipFree+1);
	}
}

static AT_GENERIC_RECORD* GetNextRecord(AT_REALLOC_RECORD* ipRealloc)
{
	if(gb64BitProcess32Bit)
	{
		return (AT_GENERIC_RECORD*) ((char*)ipRealloc + gReallocRecordSz);
	}
	else
	{
		return (AT_GENERIC_RECORD*) (ipRealloc+1);
	}
}

////////////////////////////////////////////////////////////////////
// Core function of the program
// Return true if succeed.
// It takes two passes to generate the consolidated log file.
// (1) generate most of the info
// (2) rewind the consolidated log file and mark those malloc/realloc
//     that will be freed by another thread.
////////////////////////////////////////////////////////////////////
static bool GenConsolidatedFile2(const char* ipOutputFileName)
{	
	printf("Processing Pass One ...\n");

	// Create output file.
	FILE* lpFile = ::fopen(ipOutputFileName, "wb");
	if(!lpFile)
	{
		fprintf(stderr, "Failed to open file %s\n", ipOutputFileName);
		return false;
	}

	// First write header onto disk
	AT_CONSOLIDATED_HEADER lHeader(gModuleNames.size(), gMmapFiles.size(), gInfoHeader.m64Bit, gInfoHeader.mLittleEndian);
	if(1 != ::fwrite(&lHeader, sizeof(lHeader), 1, lpFile))
	{
		fprintf(stderr, "Failed to write header into file %s\n", ipOutputFileName);
		return false;
	}

	// Preamble: Get the first record of all per-thread log files
	::memset(gBlockSetArray, 0, sizeof(gBlockSetArray));
	int lTotalLogFiles = gMmapFiles.size();
	AT_GENERIC_RECORD** lpNextRecordsArray = new AT_GENERIC_RECORD*[lTotalLogFiles];
	at_timestamp_t* lpNextTimeStampsArray = new at_timestamp_t [lTotalLogFiles];
	int i;
	for(i=0; i<lTotalLogFiles; i++)
	{
		MmapFile* lpMmapFile = gMmapFiles[i];
		if(lpMmapFile->GetStartAddr() < lpMmapFile->GetEndAddr())
		{
			lpNextRecordsArray[i] = (AT_GENERIC_RECORD*)lpMmapFile->GetStartAddr();
			AT_GENERIC_RECORD* lpRec = (AT_GENERIC_RECORD*)lpMmapFile->GetStartAddr();
			lpNextTimeStampsArray[i] = lpRec->mTimeStamp;
		}
		else
		{
			
			lpNextRecordsArray[i] = NULL;
			lpNextTimeStampsArray[i] = MAX_TIMESTAMP;
		}
	}

	// Record number 0 is reserved for unknown block (unmatched free block)
	at_seqnum_t lTotalRecords = 1;
	at_timestamp_t lPrevTimeStamp = MAX_TIMESTAMP;
	at_timestamp_t lPrevTimeStampSeqNo = 0;

	int lFinishedFiles = 0;
	// Grand loop until all records of all modules are written out to disk file
	while(lFinishedFiles < lTotalLogFiles)
	{
		at_timestamp_t lSmallestTimeStamp = MAX_TIMESTAMP;
		int lFileIndex = -1;
		// Now pick the record with smallest time stamp among all threaded log files
		for(i=0; i<lTotalLogFiles; i++)
		{
			if(lpNextRecordsArray[i] && lpNextTimeStampsArray[i] < lSmallestTimeStamp)
			{
				lSmallestTimeStamp = lpNextTimeStampsArray[i];
				lFileIndex = i;
			}
		}

		// If new timestamp, create a timestamp record
		if(lSmallestTimeStamp != lPrevTimeStamp)
		{
			AT_CONSOLIDATED_TIMESTAMP lTimeStamp(lSmallestTimeStamp);
			WRITE_FILE(&lTimeStamp, sizeof(lTimeStamp), 1, lpFile, ipOutputFileName);
			lPrevTimeStamp = lSmallestTimeStamp;
			lPrevTimeStampSeqNo = lTotalRecords;
		}

		// Process the next record with smallest time stamp
		AT_GENERIC_RECORD* lpRecord = lpNextRecordsArray[lFileIndex];
		unsigned int  lOpThreadID = GetThreadIDByRecordAddr(lpRecord);

		at_seqnum_t lMallocSeqNo = 0;
		unsigned int  lAllocatedByOtherThread = 0;
		unsigned int  lOldSize = 0;
		// Write out the record with smallest time stamp
		if(lpRecord->mType == AT_MEM_OP_TYPE_MALLOC)
		{
			AT_MALLOC_RECORD* lpMalloc = (AT_MALLOC_RECORD*)lpRecord;
			// Remember the allocated block in global map
			if(AddOutstandingBlock(GetAllocatedBlockPtr(lpMalloc), lSmallestTimeStamp, lTotalRecords, lOpThreadID, lpMalloc->mSize) == false)
			{
				return false;
			}
			// Write out record to disk
			AT_CONSOLIDATED_MALLOC lMallocRec(lOpThreadID, lpMalloc->mModuleNum, lpMalloc->mSize);
			WRITE_FILE(&lMallocRec, sizeof(lMallocRec), 1, lpFile, ipOutputFileName);
			// Move to next log record.
			lpNextRecordsArray[lFileIndex] = GetNextRecord(lpMalloc);
		}
		else if(lpRecord->mType == AT_MEM_OP_TYPE_FREE)
		{
			AT_FREE_RECORD* lpFree = (AT_FREE_RECORD*)lpRecord;
			// Find who allocated the to-be-freed block
			void* lpFreedBlockPtr = GetFreedBlockPtr(lpFree);
			if(lpFreedBlockPtr)
			{
				lMallocSeqNo = GetMallocNumber(lpFreedBlockPtr, lOpThreadID, lAllocatedByOtherThread, lOldSize);
				// special case if the block is allocated by other thread
				// Insert a timestamp record here to avoid free is executed before paired malloc at runtime
				if(lAllocatedByOtherThread && lMallocSeqNo>=lPrevTimeStampSeqNo)
				{
					AT_CONSOLIDATED_TIMESTAMP lTimeStamp(lSmallestTimeStamp);
					WRITE_FILE(&lTimeStamp, sizeof(lTimeStamp), 1, lpFile, ipOutputFileName);
					lPrevTimeStampSeqNo = lTotalRecords;
				}
				AT_CONSOLIDATED_FREE lFreeRec(lOpThreadID, lpFree->mModuleNum, lMallocSeqNo, lOldSize, lAllocatedByOtherThread);
				// Write out record to disk
				WRITE_FILE(&lFreeRec, sizeof(lFreeRec), 1, lpFile, ipOutputFileName);
			}
			else
			{
				// The freed block points to NULL, skip it.
				lTotalRecords--; // Need to adjust record no. here.
			}
			// Move to next log record.
			lpNextRecordsArray[lFileIndex] = GetNextRecord(lpFree);
		}
		else if(lpRecord->mType == AT_MEM_OP_TYPE_REALLOC)
		{
			AT_REALLOC_RECORD* lpRealloc = (AT_REALLOC_RECORD*)lpRecord;
			// Find who allocated the to-be-freed old block
			lMallocSeqNo = GetMallocNumber(GetFreedBlockPtr(lpRealloc), lOpThreadID, lAllocatedByOtherThread, lOldSize);
			// special case if the block is allocated by other thread
			// Insert a timestamp record here to avoid free is executed before malloc at runtime
			if(lAllocatedByOtherThread && lMallocSeqNo>=lPrevTimeStampSeqNo)
			{
				AT_CONSOLIDATED_TIMESTAMP lTimeStamp(lSmallestTimeStamp);
				WRITE_FILE(&lTimeStamp, sizeof(lTimeStamp), 1, lpFile, ipOutputFileName);
				lPrevTimeStampSeqNo = lTotalRecords;
			}
			AT_CONSOLIDATED_REALLOC lReallocRec(lOpThreadID, lpRealloc->mModuleNum, lMallocSeqNo,
												lpRealloc->mNewSize, lOldSize, lAllocatedByOtherThread);
			// Remember the allocated new block in global map
			if(AddOutstandingBlock(GetAllocatedBlockPtr(lpRealloc), lSmallestTimeStamp, lTotalRecords, lOpThreadID, lpRealloc->mNewSize) == false)
			{
				return false;
			}
			// Write out record to disk
			WRITE_FILE(&lReallocRec, sizeof(lReallocRec), 1, lpFile, ipOutputFileName);
			// Move to next log record.
			lpNextRecordsArray[lFileIndex] = GetNextRecord(lpRealloc);
		}
		else
		{
			// Error
			fprintf(stderr, "Fatal: Unknown record type\n");
			return false;
		}

		// bump number of records and check its limit
		lTotalRecords++;
		if(lTotalRecords == MAX_REC_NUM)
		{
			fprintf(stderr, "There are more than [%ld] logged records\n");
			fprintf(stderr, "It exceeds the program's current capability\n");
			fprintf(stderr, "Please contact author to enhance this\n");
			break;
		}

		// Check if the next record has reached the end of the file
		if((char*)lpNextRecordsArray[lFileIndex] >= gMmapFiles[lFileIndex]->GetEndAddr())
		{
			lpNextRecordsArray[lFileIndex] = NULL;
			lFinishedFiles++;
		}
		// or log file is truncated, peek next record's type and see if the record might exeeds file size
		else if( ((lpNextRecordsArray[lFileIndex])->mType==AT_MEM_OP_TYPE_MALLOC && (char*)lpNextRecordsArray[lFileIndex]+gMallocRecordSz>gMmapFiles[lFileIndex]->GetEndAddr())
			|| ((lpNextRecordsArray[lFileIndex])->mType==AT_MEM_OP_TYPE_FREE && (char*)lpNextRecordsArray[lFileIndex]+gFreeRecordSz>gMmapFiles[lFileIndex]->GetEndAddr())
			|| ((lpNextRecordsArray[lFileIndex])->mType==AT_MEM_OP_TYPE_REALLOC && (char*)lpNextRecordsArray[lFileIndex]+gReallocRecordSz>gMmapFiles[lFileIndex]->GetEndAddr()) )
		{
			fprintf(stderr, "File [%s] is truncated\n", gMmapFiles[lFileIndex]->GetFileName());
			lpNextRecordsArray[lFileIndex] = NULL;
			lFinishedFiles++;
		}
		else
		{
			// All appear fine. Update next record's timestamp
			AT_GENERIC_RECORD* lpRec = (AT_GENERIC_RECORD*)lpNextRecordsArray[lFileIndex];
			lpNextTimeStampsArray[lFileIndex] = lpRec->mTimeStamp;
			gMmapFiles[lFileIndex]->AdjustMmapArea((char*)lpNextRecordsArray[lFileIndex]);
		}
	} // Grand loop until all records of all modules are written out to disk file

	// Close consolidated log file
	::fclose(lpFile);
	// log files are not needed any more.
	UnloadLogFiles();

	// Error blocks are allocated blocks 
	// but not freed and its address is used by another allocation.
	// Either the runtime logger fails to log the free transaction
	// or this consolidator fails to match it for some reason.
	//printf("==> Total Blocks allocated but its address reused by another malloc before freed: %ld\n", gErrorBlocks.size());
	/*ALLOCATED_BLOCK_SET::iterator ErrIt;
	for(ErrIt=gErrorBlocks.begin(); ErrIt!=gErrorBlocks.end(); ErrIt++)
	{
		void* lpErrorBlock = (*ErrIt)->mpRawAddr;

		// Search it in the unknown freed blocks.
		BLOCK_SET::iterator ErrFreedBlockIt = gUnknownFreeBlocks.find(lpErrorBlock);
		if(ErrFreedBlockIt != gUnknownFreeBlocks.end())
		{
			// This block is freed down the stream. We missed it.
			//fprintf(stderr, "Block 0x%lx is freed later\n", lpErrorBlock);
			lTotalErrorBlockMatchedLater++;
		}
		else
		{
			// It must be a real runtime logger problem.
			//fprintf(stderr, "Error block 0x%lx\n", lpErrorBlock);
		}
	}*/

	unsigned long lTotalUnmatchedMalloc = 0;
	unsigned long lTotalErrorBlockMatchedLater = 0;
	for(i=0; i<TOTAL_BUCKETS; i++)
	{
		ALLOCATED_BLOCK_SET* lpSet = gBlockSetArray[i];
		if(lpSet)
		{
			lTotalUnmatchedMalloc += lpSet->size();

			ALLOCATED_BLOCK_SET::iterator it;
			for(it=lpSet->begin(); it!=lpSet->end(); it++)
			{
				void* lpUnMatchedMallocBlock = (*it)->mpRawAddr;
				// Search it in the unknown freed blocks.
				BLOCK_SET::iterator ErrFreedBlockIt = gUnknownFreeBlocks.find(lpUnMatchedMallocBlock);
				if(ErrFreedBlockIt != gUnknownFreeBlocks.end())
				{
					if( (*it)->mNext)
					{
						::fprintf(stderr, "Multiple unmatched malloc blocks 0x%lx found only one unmatched free\n", (*it)->mpRawAddr);
					}
					// This block is freed down the stream. We missed it.
					//fprintf(stderr, "Block 0x%lx is freed later\n", lpErrorBlock);
					lTotalErrorBlockMatchedLater++;
				}
			}
		}
	}

	printf("==> Total allocated Blocks without matched free: %ld\n", lTotalUnmatchedMalloc);
	printf("==> Total freed Blocks without matched malloc: %ld\n", gUnknownFreeBlocks.size());
	printf("==> Total Blocks' address reused and freed later: %ld\n", lTotalErrorBlockMatchedLater);

	// Now for 2nd pass, mark those mallocs that are freed by other threads.
	printf("Processing Pass Two ...\n");
	MmapFile lOutputMmapFile(ipOutputFileName, true);

	// First update header for total records
	AT_CONSOLIDATED_HEADER* lpHeader = (AT_CONSOLIDATED_HEADER*)lOutputMmapFile.GetStartAddr();
	lpHeader->mTotalRecords = lTotalRecords-1;

	// Next move cursor to the first record, skip timestamp.
	AT_CONSOLIDATED_GENERIC* lpNextRecord = (AT_CONSOLIDATED_GENERIC*)(lpHeader+1);
	at_seqnum_t lCurrentSeqNo = 1;
	while(lpNextRecord->mType == AT_MEM_OP_TYPE_TIMESTAMP)
	{
		lpNextRecord = (AT_CONSOLIDATED_GENERIC*)((char*)lpNextRecord + sizeof(AT_CONSOLIDATED_TIMESTAMP));
	}

	// Fix each block freed-by-other-thread on the list
	std::set<at_seqnum_t>::iterator it;
	for(it=gBlocksFreedByOtherThread.begin(); it!=gBlocksFreedByOtherThread.end(); it++)
	{
		at_seqnum_t lSeqNo = *it;
		// something is wrong if the cursor has passed the desired block
		if(lSeqNo < lCurrentSeqNo)
		{
			fprintf(stderr, "Blocks on Freed-By-Other-Thread list are not sorted correctly\n");
			continue;
		}

		// Move the cursor to the record matching Seq no.
		while(lCurrentSeqNo != lSeqNo && (char*)lpNextRecord < lOutputMmapFile.GetEndAddr())
		{
			if(lpNextRecord->mType == AT_MEM_OP_TYPE_TIMESTAMP)
			{
				lpNextRecord = (AT_CONSOLIDATED_GENERIC*)((char*)lpNextRecord + sizeof(AT_CONSOLIDATED_TIMESTAMP));
			}
			else if(lpNextRecord->mType == AT_MEM_OP_TYPE_MALLOC)
			{
				lpNextRecord = (AT_CONSOLIDATED_GENERIC*)((char*)lpNextRecord + sizeof(AT_CONSOLIDATED_MALLOC));
				lCurrentSeqNo++;
			}
			else if(lpNextRecord->mType == AT_MEM_OP_TYPE_FREE)
			{
				lpNextRecord = (AT_CONSOLIDATED_GENERIC*)((char*)lpNextRecord + sizeof(AT_CONSOLIDATED_FREE));
				lCurrentSeqNo++;
			}
			else if(lpNextRecord->mType == AT_MEM_OP_TYPE_REALLOC)
			{
				lpNextRecord = (AT_CONSOLIDATED_GENERIC*)((char*)lpNextRecord + sizeof(AT_CONSOLIDATED_REALLOC));
				lCurrentSeqNo++;
			}
			else
			{
				fprintf(stderr, "Wrong type\n");
				::abort();
			}
		}

		// Skip timestamp.
		while(lpNextRecord->mType == AT_MEM_OP_TYPE_TIMESTAMP)
		{
			lpNextRecord = (AT_CONSOLIDATED_GENERIC*)((char*)lpNextRecord + sizeof(AT_CONSOLIDATED_TIMESTAMP));
		}

		if(lCurrentSeqNo == lSeqNo)
		{
			// Verify the type
			if(lpNextRecord->mType != AT_MEM_OP_TYPE_MALLOC && lpNextRecord->mType != AT_MEM_OP_TYPE_REALLOC)
			{
				fprintf(stderr, "Error: Record %ld is neither malloc nor realloc\n", lSeqNo);
			}
			else
			{
				// Mark the record
				lpNextRecord->mFreeByOtherThread = 1;
				// unmap part of the file if necessary
				lOutputMmapFile.AdjustMmapArea((char*)lpNextRecord);
			}
		}
		else
		{
			fprintf(stderr, "Failed to find the record %ld\n", lSeqNo);
		}
	}

	return true;
}

// !FIX! consider parallel processing
/*#define MAX_THREADS 4

struct PreProcessParams
{
	int mThreadIndex;
	const char* mLogFileName;
};

// Array of flags set by working threads that signal work has be done.
bool* gThreadFinished = NULL;

typedef std::map<void*,AT_GENERIC_RECORD*> ADDRESS_MAP;
typedef std::pair<void*,AT_GENERIC_RECORD*> ADDRESS_PAIR;

inline ADDRESS_MAP* GetAddressMap(void* ipRawAddr, ADDRESS_MAP* iMapArray[])
{
	// Hash the block address
	int lIndex = ADDR2INDEX(ipRawAddr);
	ADDRESS_MAP* lpMap = iMapArray[lIndex];
	if(lpMap == NULL)
	{
		lpMap = new ADDRESS_MAP;
		iMapArray[lIndex] = lpMap;
	}
	return lpMap;
}

inline bool AddOneBlock(void* ipRawAddr, ADDRESS_MAP* iMapArray[], AT_GENERIC_RECORD* ipRecord)
{
	ADDRESS_MAP* lpMap = GetAddressMap(ipRawAddr, iMapArray);
	// Remember the allocated block in global map
	if(lpMap->insert(ADDRESS_PAIR(ipRawAddr, ipRecord)).second == false)
	{
		fprintf(stderr, "Duplicated address is used before freed.\n");
		return false;
	}
	return true;
}

inline AT_GENERIC_RECORD* FindBlockRecord(void* ipRawAddr, ADDRESS_MAP* iMapArray[])
{
	AT_GENERIC_RECORD* lpResult = NULL;

	ADDRESS_MAP* lpMap = GetAddressMap(ipRawAddr, iMapArray);
	if(lpMap)
	{
		ADDRESS_MAP::iterator it = lpMap->find(ipRawAddr);
		if(it != lpMap->end())
		{
			lpResult = (*it).second;
			lpMap->erase(it);
		}
	}
	return lpResult;
}*/

////////////////////////////////////////////////////////////////////
// Fun starts here.
// User supply the folder that has all the raw log files.
// Consolidator set its working dir to that folder and generate
// one consolidated log file: accutrak.dat
////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
	// help message
	if(argc < 2 || 0==::strcmp(argv[1], "-h"))
	{
		printf("Usage: %s Directory_of_Log_Files\n", argv[0]);
		exit(0);
	}

	// Check input directory is valid
	const char* lpDirPath = argv[1];
	if(IsDirectory(lpDirPath) == false)
	{
		::fprintf(stderr, "%s is not a directory as expected\n", lpDirPath);
		return(-1);
	}

	// Get all per-thread log files
	std::vector<char*> lLogFileNames;
#ifdef WIN32
	if(GetAllFilesInDirectory(lpDirPath, "\\TID_*.log", lLogFileNames) == false)
#else
	if(GetAllFilesInDirectory(lpDirPath, "TID_", lLogFileNames) == false)
#endif
	{
		::fprintf(stderr, "Failed to get log files under directory %s\n", lpDirPath);
		return(-1);
	}
	if(lLogFileNames.size() < 1)
	{
		::fprintf(stderr, "No log file(s) are found under directory %s\n", lpDirPath);
		return(-1);
	}

	// Change working directory
	SetCurrentWorkingDirectory(lpDirPath);

	// Get info file
	const char* lpInfoFileName = "AccuTrak.rec";
	if(!ReadInfoFile(lpInfoFileName))
	{
		return -1;
	}

	// Open all log files in mmap mode
	for(int i=0; i<lLogFileNames.size(); i++)
	{
		printf("Load file %s\n", lLogFileNames[i]);
		if(false == OpenOneThreadLogFile(lLogFileNames[i]) )
		{
			return -1;
		}
	}

	const char* lpOutputFileName = "accutrak.dat";
	// The real work
	GenConsolidatedFile2(lpOutputFileName);

	return 0;
}

