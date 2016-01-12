/************************************************************************
** FILE NAME..... MallocLogger.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION.........
** Data structures of loggend files
** 
** NOTES............ 
** 
** 
** ASSUMPTIONS...... 
** 
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
#ifndef _MALLOC_LOGGER_H
#define _MALLOC_LOGGER_H

#ifdef sun
#include <sys/types.h>
#endif
#include <string.h>

#include "CrossPlatform.h"
#include "AccuTrak.h"

#define MagicName "AccuTrak"

// Use 4 bytes to represent elapse time in millisecond
// which is up to roughly 50 days
typedef unsigned int at_timestamp_t;
#define MAX_TIMESTAMP UINT_MAX

// Use 4 bytes for record's sequence number
// This limits max records to 4G to save space
// If need arises, we have to expand it to 8bytes
typedef unsigned int at_seqnum_t;
#define MAX_REC_NUM UINT_MAX

// AccuTrak need to call this ASAP when it wants to do logging.
extern void AT_InitMallocLogger();

// Header of the info file 
// followed by modules names.
struct AT_REC_HEADER
{
	char         mMagic[8];
	unsigned int mLittleEndian:1;
	unsigned int m64Bit:1;
	unsigned int mReserve:30;
	int          mVersionMajor;
	int          mVersionMinor;
	int          mTotalModules;

	// Ctor
	AT_REC_HEADER()
		: mVersionMajor(VER_MAJOR), mVersionMinor(VER_MINOR),
		mReserve(0), mTotalModules(0)
	{
		if(sizeof(long) == 8)
		{
			m64Bit = 1;
		}
		else
		{
			m64Bit = 0;
		}

		const char* lMagicName = MagicName;
		for(int i=0; i<8; i++)
		{
			mMagic[i] = lMagicName[i];
		}
		int lInt = 0x01;
		mLittleEndian = *(char*)&lInt;
	}
	// Verify by magic bytes
	bool CheckMagic()
	{
		const char* lMagicName = MagicName;
		for(int i=0; i<sizeof(mMagic); i++)
		{
			if(mMagic[i] != lMagicName[i])
			{
				return false;
			}
		}
		return true;
	}
	// Check and print out
	bool CheckAndPrintHeader()
	{
		// check magic number
		if(CheckMagic() == false)
		{
			fprintf(stderr, "Wrong magic number of log file, maybe different version\n");
			return false;
		}

		printf("================================================================\n");
		printf("AccuTrak Version %d.%d malloc replay\n", mVersionMajor, mVersionMinor);
		printf("The test machine is %s %s endian\n",
			m64Bit ? "64bit" : "32bit",	mLittleEndian ? "little" : "big");

		return true;
	}
};

// Memory operation type
#define AT_MEM_OP_TYPE_INVALID   0
#define AT_MEM_OP_TYPE_TIMESTAMP 1
#define AT_MEM_OP_TYPE_MALLOC    2
#define AT_MEM_OP_TYPE_FREE      3
#define AT_MEM_OP_TYPE_REALLOC   4

// Make all structs confirm with generic prototype
struct AT_GENERIC_RECORD
{
	unsigned int mType:3;
	unsigned int mModuleNum:9;
	unsigned int mFreeByOtherThread:1;
	unsigned int mAllocatedByOtherThread:1;
	unsigned int mReserve:18;	// 4 bytes
	// In the unit of millisecond, 
	// it is the elapse time since AccuTrak record is started.
	at_timestamp_t mTimeStamp;	// 8 bytes
};

// Record a malloc op
struct AT_MALLOC_RECORD
{
	unsigned int mType:3;
	unsigned int mModuleNum:9;
	unsigned int mFreeByOtherThread:1;
	unsigned int mReserve:19;	// 4 bytes
	// In the unit of millisecond, 
	// it is the elapse time since AccuTrak record is started.
	at_timestamp_t mTimeStamp;	// 8 bytes
	unsigned int mSize;			// 12 bytes, 64bit will have 4 bytes padding here
	void*  mpAllocatedBlock;	// 16/24 bytes

	AT_MALLOC_RECORD(size_t iSize, void* ipBlock, int iModuleNum);
};

// Record free op
struct AT_FREE_RECORD
{
	unsigned int mType:3;
	unsigned int mModuleNum:9;
	unsigned int mAllocatedByOtherThread:1;
	unsigned int mReserve:19;	// 4 bytes
	at_timestamp_t mTimeStamp;	// 8 bytes
	void*  mpFreedBlock;		// 12/16 bytes

	AT_FREE_RECORD(void* ipBlock, int iModuleNum);
};

// Record realloc op
struct AT_REALLOC_RECORD
{
	unsigned int mType:3;
	unsigned int mModuleNum:9;
	unsigned int mFreeByOtherThread:1;
	unsigned int mAllocatedByOtherThread:1;
	unsigned int mReserve:18;	// 4 bytes
	at_timestamp_t mTimeStamp;	// 8 bytes
	unsigned int mNewSize;		// 12 bytes, 64bit will have 4 bytes padding here
	void*  mpOldBlock;			// 16/24 bytes
	void*  mpNewBlock;			// 20/32 bytes

	AT_REALLOC_RECORD(void* ipOldBlock, size_t iNewSize, void* ipNewBlock, int iModuleNum);
};

//////////////////////////////////////////////////////
// Used by post process.
//////////////////////////////////////////////////////
#define K 1024

static size_t gBucketBoundary[] = {
	8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128,
	256, 512, 1*K, 2*K, 3*K, 4*K, 5*K, 6*K, 7*K, 8*K,
	16*K, 32*K, 64*K, 128*K, 256*K, 512*K, 1024*K, 2048*K, 10240*K
};

#define NUM_BUCKET sizeof(gBucketBoundary)/sizeof(size_t)


// Header of the merged file which list all records logged by all threads.
struct AT_CONSOLIDATED_HEADER
{
	char         mMagic[8];			// 8 bytes
	unsigned int mLittleEndian:1;
	unsigned int m64Bit:1;
	unsigned int mReserve:30;		// 12 bytes
	unsigned int mReserve2;			// 16 bytes
	int          mVersionMajor;		// 20 bytes
	int          mVersionMinor;		// 24 bytes
	int          mTotalThreads;		// 28 bytes
	int          mTotalModules;		// 32 bytes
	// Make sure this is 8-bytes aligned
	at_seqnum_t mTotalRecords;		// 36 bytes

	// Constructor
	AT_CONSOLIDATED_HEADER() {}

	AT_CONSOLIDATED_HEADER(int iTotalModules, int iTotalThreads, unsigned int i64Bit, unsigned int iLittleEndian)
		: mLittleEndian(iLittleEndian), m64Bit(i64Bit),	mReserve(0), mReserve2(0),
		mVersionMajor(VER_MAJOR), mVersionMinor(VER_MINOR), 
		mTotalThreads(iTotalThreads), mTotalModules(iTotalModules), mTotalRecords(0)
	{
		const char* lMagicName = MagicName;
		for(int i=0; i<8; i++)
		{
			mMagic[i] = lMagicName[i];
		}
	}

	// Verify by magic bytes
	bool CheckMagic()
	{
		const char* lMagicName = MagicName;
		for(int i=0; i<sizeof(mMagic); i++)
		{
			if(mMagic[i] != lMagicName[i])
			{
				return false;
			}
		}
		return true;
	}
	
	// Check and print out
	bool CheckAndPrintHeader()
	{
		// check magic number
		if(CheckMagic() == false)
		{
			fprintf(stderr, "Wrong magic number of log file, maybe different version\n");
			return false;
		}

		printf("AccuTrak Version %d.%d malloc replay\n", mVersionMajor, mVersionMinor);
		printf("The test machine is %s %s endian\n",
			m64Bit ? "64bit" : "32bit",	mLittleEndian ? "little" : "big");
		printf("Total modules %d\n",  mTotalModules);
		printf("Total threads %d\n",  mTotalThreads);
		printf("Total records %ld\n", mTotalRecords);
		printf("================================================================\n");

		return true;
	}
};

#define MAX_THREAD_ID  (1<<24)
#define MAX_BLOCK_SIZE UINT_MAX

struct AT_CONSOLIDATED_GENERIC
{
	unsigned int mType:3;
	unsigned int mFreeByOtherThread:1;
	unsigned int mReserve:1;
	unsigned int mModuleNum:9;
	unsigned int mThreadID:18;	// 4 bytes
};

// Record a new timestamp
struct AT_CONSOLIDATED_TIMESTAMP
{
	unsigned int mType:3;
	unsigned int mReserve:29;	// 4 bytes
	// TimeStamp in unit of millisecond, use only 4 bytes to save space
	at_timestamp_t mTimeStamp;	// 8 bytes

	AT_CONSOLIDATED_TIMESTAMP(unsigned int iTimeStamp);
};

// Record a malloc op
struct AT_CONSOLIDATED_MALLOC
{
	unsigned int mType:3;
	unsigned int mFreeByOtherThread:1;
	unsigned int mReserve:1;
	unsigned int mModuleNum:9;
	unsigned int mThreadID:18;	// 4 bytes
	unsigned int mSize;			// 8 bytes

	AT_CONSOLIDATED_MALLOC(unsigned int iTID, unsigned int iModuleNum, size_t iSize);
};

// Record free op
struct AT_CONSOLIDATED_FREE
{
	unsigned int mType:3;
	unsigned int mAllocatedByOtherThread:1;
	unsigned int mReserve:1;
	unsigned int mModuleNum:9;
	unsigned int mThreadID:18;		// 4 bytes
	unsigned int mSize;				// 8 bytes
	// This is the sequence number of the malloc record which allocated a block
	// and now freed by this operation
	// Make sure this is 8-bytes aligned
	at_seqnum_t mMatchedMalloc;		// 12 bytes

	AT_CONSOLIDATED_FREE(unsigned int iTID, unsigned int iModuleNum,
		at_seqnum_t iMallocNo, size_t iSize, unsigned int iAllocatedByOtherThread);
};

// Record realloc op
struct AT_CONSOLIDATED_REALLOC
{
	unsigned int mType:3;
	unsigned int mFreeByOtherThread:1;
	unsigned int mAllocatedByOtherThread:1;
	unsigned int mModuleNum:9;
	unsigned int mThreadID:18;		// 4 bytes
	unsigned int   mNewSize;		// 8 bytes
	unsigned int   mOldSize;		// 12 bytes
	// This is the sequence number of the malloc record which allocated a block
	// and realloced by this operation
	at_seqnum_t mMatchedMalloc;		// 16 bytes

	AT_CONSOLIDATED_REALLOC(unsigned int iTID, unsigned int iModuleNum,
		at_seqnum_t iMallocNo, size_t iNewSize, size_t iOldsize,
		unsigned int iAllocatedByOtherThread);
};

#endif // _MALLOC_LOGGER_H
