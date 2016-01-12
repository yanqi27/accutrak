/************************************************************************
** FILE NAME..... MallocLogViewer.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Print out the logged records in readable format.
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
#include <dirent.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <map>
#include <list>

#include "AccuTrak.h"
#include "MallocLogger.h"
#include "MmapFile.h"
#include "Utility.h"

#define LINE_DIVIDER \
"===================================================================\n"

typedef std::multimap<unsigned long, AT_GENERIC_RECORD*> RECMULTIMAP;
typedef std::pair<const unsigned long,AT_GENERIC_RECORD*>      RECPAIR;

///////////////////////////////////////////////////////////////////////
// Global vars
///////////////////////////////////////////////////////////////////////
std::vector<const char*> gModuleNames;

RECMULTIMAP gRecordsByModule;

std::list<MmapPerThreadLogFile*> gMmapFileList;

// Print the overall info.
bool PrintInfoFile(const char* iInfoFileName)
{
	bool rc = true;

	MmapFile lMmapFile(iInfoFileName);
	if(lMmapFile.InitSucceed() == false)
	{
		return false;
	}

	AT_REC_HEADER* lpInfoHeader = (AT_REC_HEADER*)lMmapFile.GetStartAddr();
	// print header
	if(lpInfoHeader->CheckAndPrintHeader() == false)
	{
		return false;
	}
	// print modules
	printf("The following %d modules are tracked for malloc/free operations:\n", lpInfoHeader->mTotalModules);
	const char* lpModuleName = (const char*)(lpInfoHeader+1);
	for(int i=0; i<lpInfoHeader->mTotalModules; i++)
	{
		if( lpModuleName > lMmapFile.GetEndAddr())
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


///////////////////////////////////////////////////////////////////
// Helper function to handle various(malloc/free/realloc) records
///////////////////////////////////////////////////////////////////
const char* GetOpString(AT_GENERIC_RECORD* ipRecord)
{
	static char gOpStrBuf[1024];
	AT_MALLOC_RECORD*  lpMallocRec = NULL;
	AT_FREE_RECORD*    lpFreeRec = NULL;
	AT_REALLOC_RECORD* lpReallocRec = NULL;

	switch(ipRecord->mType)
	{
	case AT_MEM_OP_TYPE_MALLOC:
		lpMallocRec = (AT_MALLOC_RECORD*)ipRecord;
		sprintf(gOpStrBuf, "time=%ld malloc %ld bytes at 0x%lx from module %s[%d]",
			lpMallocRec->mTimeStamp, lpMallocRec->mSize, lpMallocRec->mpAllocatedBlock, 
			gModuleNames[lpMallocRec->mModuleNum], lpMallocRec->mModuleNum);
		break;
	case AT_MEM_OP_TYPE_FREE:
		lpFreeRec = (AT_FREE_RECORD*)ipRecord;
		sprintf(gOpStrBuf, "time=%ld free 0x%lx from module %s[%d]",
			lpFreeRec->mTimeStamp, lpFreeRec->mpFreedBlock, 
			gModuleNames[lpFreeRec->mModuleNum], lpFreeRec->mModuleNum);
		break;
	case AT_MEM_OP_TYPE_REALLOC:
		lpReallocRec = (AT_REALLOC_RECORD*)ipRecord;
		sprintf(gOpStrBuf, "time=%ld realloc 0x%lx new_size %ld bytes at 0x%lx from module %s[%d]",
			lpReallocRec->mTimeStamp, lpReallocRec->mpOldBlock, lpReallocRec->mNewSize, lpReallocRec->mpNewBlock,
			gModuleNames[lpReallocRec->mModuleNum], lpReallocRec->mModuleNum);
		break;
	default:
		printf("Error: invalid or unknown type [%d]\n", ipRecord->mType);
		gOpStrBuf[0] = '\0';
		break;
	}

	return gOpStrBuf;
}

inline bool ValidateModuleNum(AT_GENERIC_RECORD* ipRecord)
{
	if(ipRecord->mModuleNum<0 || ipRecord->mModuleNum>=gModuleNames.size())
	{
		printf("Error: module number is invalid [%d]\n", ipRecord->mModuleNum);
		return false;
	}
	return true;
}

static AT_GENERIC_RECORD* GetNextRecord(AT_GENERIC_RECORD* ipRecord, MmapFile* lpMmapFile)
{
	AT_GENERIC_RECORD* lpNextRecord;

	switch(ipRecord->mType)
	{
	case AT_MEM_OP_TYPE_MALLOC:
		lpNextRecord = (AT_GENERIC_RECORD*)((char*)ipRecord + sizeof(AT_MALLOC_RECORD));
		break;
	case AT_MEM_OP_TYPE_FREE:
		lpNextRecord = (AT_GENERIC_RECORD*)((char*)ipRecord + sizeof(AT_FREE_RECORD));
		break;
	case AT_MEM_OP_TYPE_REALLOC:
		lpNextRecord = (AT_GENERIC_RECORD*)((char*)ipRecord + sizeof(AT_REALLOC_RECORD));
		break;
	default:
		printf("Error: invalid or unknown type [%d]\n", ipRecord->mType);
		lpNextRecord = (AT_GENERIC_RECORD*)lpMmapFile->GetEndAddr();
		break;
	}

	// Check if the next record is truncated
	if((char*)lpNextRecord < lpMmapFile->GetEndAddr()
		&& ( (lpNextRecord->mType==AT_MEM_OP_TYPE_MALLOC && (char*)lpNextRecord+sizeof(AT_MALLOC_RECORD)>lpMmapFile->GetEndAddr())
		|| (lpNextRecord->mType==AT_MEM_OP_TYPE_FREE && (char*)lpNextRecord+sizeof(AT_FREE_RECORD)>lpMmapFile->GetEndAddr())
		|| (lpNextRecord->mType==AT_MEM_OP_TYPE_REALLOC && (char*)lpNextRecord+sizeof(AT_REALLOC_RECORD)>lpMmapFile->GetEndAddr()) ) )
	{
		fprintf(stderr, "File [%s] is truncated\n", lpMmapFile->GetFileName());
		lpNextRecord = (AT_GENERIC_RECORD*)lpMmapFile->GetEndAddr();
	}
	return lpNextRecord;
}

bool PrintOneThreadLogFile(const char* iLogFileName)
{
	long lThreadID=0;
	::sscanf(GetBaseName(iLogFileName), "TID_%ld.log", &lThreadID);

	MmapFile lMmapFile(iLogFileName);

	unsigned long lTotalRecs = 0;

	// print thread header
	printf("%sThread %ld malloc/free operations:\n", LINE_DIVIDER, lThreadID);

	// print out each record of this thread
	AT_GENERIC_RECORD* lpNextRecord = (AT_GENERIC_RECORD*)lMmapFile.GetStartAddr();
	while((char*)lpNextRecord < lMmapFile.GetEndAddr())
	{

		lTotalRecs++;

		if(ValidateModuleNum(lpNextRecord) == false)
		{
			return false;
		}

		printf("%s\n", GetOpString(lpNextRecord));

		lpNextRecord = GetNextRecord(lpNextRecord, &lMmapFile);
	}
	printf("Total records for this thread %ld\n", lTotalRecs);

	return true;
}

bool UnloadLogFiles()
{
	std::list<MmapPerThreadLogFile*>::iterator it;
	for(it=gMmapFileList.begin(); it!=gMmapFileList.end(); it++)
	{
		MmapPerThreadLogFile* lpMmapFile = *it;
		delete lpMmapFile;
	}
	gMmapFileList.clear();

	return true;
}

// Put all records into a global multimap sorted by time stamp
// This could use a lot, lot of memory. 
// BE WARNED
bool PreprocessAllRecords()
{
	std::list<MmapPerThreadLogFile*>::iterator it;
	for(it=gMmapFileList.begin(); it!=gMmapFileList.end(); it++)
	{
		MmapPerThreadLogFile* lpMmapFile = *it;

		// Go through each record and push them into a multimap.
		AT_GENERIC_RECORD* lpNextRecord = (AT_GENERIC_RECORD*)lpMmapFile->GetStartAddr();
		while((char*)lpNextRecord < lpMmapFile->GetEndAddr())
		{
			if(ValidateModuleNum(lpNextRecord) == false)
			{
				return false;
			}

			unsigned long lTimeStamp = lpNextRecord->mTimeStamp;
			gRecordsByModule.insert(RECPAIR(lTimeStamp, lpNextRecord));
			lpNextRecord = GetNextRecord(lpNextRecord, lpMmapFile);
		}
	}
	return true;
}

bool LoadOneThreadLogFile(const char* iLogFileName)
{
	long lThreadID;
	::sscanf(GetBaseName(iLogFileName), "TID_%ld.log", &lThreadID);

	MmapPerThreadLogFile* lpMmapFile = new MmapPerThreadLogFile(lThreadID, gMmapFileList.size()+1, iLogFileName);

	gMmapFileList.push_back(lpMmapFile);

	return true;
}

long GetThreadIDByRecordAddr(void* ipRecAddr)
{
	std::list<MmapPerThreadLogFile*>::iterator it;
	for(it=gMmapFileList.begin(); it!=gMmapFileList.end(); it++)
	{
		MmapPerThreadLogFile* lpMmapFile = *it;
		if(lpMmapFile->AddrWithinMmapFile((char*)ipRecAddr))
		{
			return lpMmapFile->GetMappedTID();
		}
	}
	return (long)(-1);
}

void PrintLogPerModule()
{
	printf("Total %ld records\n", gRecordsByModule.size());

	// For each module
	for(int i=0; i<gModuleNames.size(); i++)
	{
		printf("%s", LINE_DIVIDER);
		printf("Module[%d] %s malloc/free operations:\n", i, gModuleNames[i]);

		// Go through all records for module i
		RECMULTIMAP::iterator it;
		for(it=gRecordsByModule.begin(); it!=gRecordsByModule.end(); it++)
		{
			// print out the record
			AT_GENERIC_RECORD* lpRecord = (AT_GENERIC_RECORD*)(*it).second;
			if(lpRecord->mModuleNum == i)
			{
				printf("%s by thread %ld\n",
					GetOpString(lpRecord), GetThreadIDByRecordAddr(lpRecord));
			}
		}
	}
}


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

struct MallocStatPerModule
{
	unsigned long mBuckets[NUM_BUCKET+1];
	unsigned long mTotalMalloc;
	unsigned long mTotalFree;
	unsigned long mTotalRealloc;

	MallocStatPerModule() : mTotalMalloc(0),
		mTotalFree(0), mTotalRealloc(0) 
	{
		::memset(mBuckets, 0, sizeof(mBuckets));
	}
};

void PrintStatsPerModule()
{
	unsigned long lTotalMalloc  = 0;
	unsigned long lTotalFree    = 0;
	unsigned long lTotalRealloc = 0;

	MallocStatPerModule* lpStatsArray = new MallocStatPerModule[gModuleNames.size()];
	unsigned long TotalBuckets[NUM_BUCKET+1];
	::memset(TotalBuckets, 0, sizeof(TotalBuckets));

	// Go through all per-thread log files and each record in them
	std::list<MmapPerThreadLogFile*>::iterator it;
	for(it=gMmapFileList.begin(); it!=gMmapFileList.end(); it++)
	{
		MmapPerThreadLogFile* lpMmapFile = *it;
		printf("Processing %s ...\n", lpMmapFile->GetFileName());

		AT_GENERIC_RECORD* lpNextRecord = (AT_GENERIC_RECORD*)lpMmapFile->GetStartAddr();
		while((char*)lpNextRecord < lpMmapFile->GetEndAddr())
		{
			AT_MALLOC_RECORD*  lpMallocRec = NULL;
			AT_FREE_RECORD*    lpFreeRec = NULL;
			AT_REALLOC_RECORD* lpReallocRec = NULL;

			if(ValidateModuleNum(lpNextRecord) == false)
			{
				break;
			}

			int lBucketIndex = -1;
			switch(lpNextRecord->mType)
			{
			case AT_MEM_OP_TYPE_MALLOC:
				lpMallocRec = (AT_MALLOC_RECORD*)lpNextRecord;
				lTotalMalloc++;
				lpStatsArray[lpNextRecord->mModuleNum].mTotalMalloc++;
				lBucketIndex = BlockSize2BucketIndex(lpMallocRec->mSize);
				lpStatsArray[lpNextRecord->mModuleNum].mBuckets[lBucketIndex]++;
				TotalBuckets[lBucketIndex]++;
				break;
			case AT_MEM_OP_TYPE_FREE:
				lpFreeRec = (AT_FREE_RECORD*)lpNextRecord;
				lTotalFree++;
				lpStatsArray[lpNextRecord->mModuleNum].mTotalFree++;
				break;
			case AT_MEM_OP_TYPE_REALLOC:
				lpReallocRec = (AT_REALLOC_RECORD*)lpNextRecord;
				lTotalRealloc++;
				lpStatsArray[lpNextRecord->mModuleNum].mTotalRealloc++;
				lBucketIndex = BlockSize2BucketIndex(lpReallocRec->mNewSize);
				lpStatsArray[lpNextRecord->mModuleNum].mBuckets[lBucketIndex]++;
				TotalBuckets[lBucketIndex]++;
				break;
			default:
				printf("Error: invalid or unknown type [%d]\n", lpNextRecord->mType);
				break;
			}

			lpNextRecord = GetNextRecord(lpNextRecord, lpMmapFile);
		}
		delete lpMmapFile;
	}
	gMmapFileList.clear();

	// Print out the stats
	printf("%s", LINE_DIVIDER);
	printf("Total Malloc: %ld    Total Free: %ld    Total Realloc: %ld\n", 
		lTotalMalloc, lTotalFree, lTotalRealloc);
	// Print total histogram stat
	printf("Memory Allocation Size Histogram (bytes):\n");
	PrintAllBuckets();

	int i;
	for(i=0; i<NUM_BUCKET+1; i++)
	{
		printf("\t%ld", TotalBuckets[i]);
	}
	printf("\n");
	// Per module stats
	printf("%s", LINE_DIVIDER);
	printf("Per-module statistics follows:\n");

	printf("Module_Name\tTotal_Malloc    Total_Free    Total_Realloc\n");
	for(i=0; i<gModuleNames.size(); i++)
	{
		printf("%s\t%ld    %ld    %ld\n", gModuleNames[i],
			lpStatsArray[i].mTotalMalloc, lpStatsArray[i].mTotalFree, lpStatsArray[i].mTotalRealloc);
	}
	
	printf("\nModule_Name");
	PrintAllBuckets();
	for(i=0; i<gModuleNames.size(); i++)
	{
		printf("%s", gModuleNames[i]);
		for(int k=0; k<NUM_BUCKET+1; k++)
		{
			printf("\t%ld", lpStatsArray[i].mBuckets[k]);
		}
		printf("\n");
	}
}

void PrintLogByTime()
{
	printf("Total %ld records\n", gRecordsByModule.size());
	
	// Go through all records for module i
	RECMULTIMAP::iterator it;
	for(it=gRecordsByModule.begin(); it!=gRecordsByModule.end(); it++)
	{
		AT_GENERIC_RECORD* lpRecord = (AT_GENERIC_RECORD*)(*it).second;
		
		printf("%s by thread %ld\n",
			GetOpString(lpRecord), GetThreadIDByRecordAddr(lpRecord));
	}
}


///////////////////////////////////////////////////////////////////////
// Fun starts from here
///////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
	bool lbViewPerModule = false;
	bool lbViewPerThread = false;
	bool lbViewStats     = false;
	const char* lpDirPath;

	if(argc < 2 || 0==::strcmp(argv[1], "-h"))
	{
		printf("Usage: %s [-h|-m|-t|-s] Directory_of_Log_Files\n", argv[0]);
		printf("\t[-h] ==> print this message.\n");
		printf("\t[-m] ==> print memory operations per-module.\n");
		printf("\t[-t] ==> print memory operations per-thread.\n");
		printf("\t[-s] ==> print memory operations statistics.\n");
		exit(0);
	}

	if(0 == ::strcmp(argv[1], "-m"))
	{
		lbViewPerModule = true;
		lpDirPath = argv[2];
	}
	else if(0 == ::strcmp(argv[1], "-t"))
	{
		lbViewPerThread = true;
		lpDirPath = argv[2];
	}
	else if(0 == ::strcmp(argv[1], "-s"))
	{
		lbViewStats = true;
		lpDirPath = argv[2];
	}
	else
	{
		lpDirPath = argv[1];
	}

	// Check input directory is valid
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

	// Print out the log info file
	const char* lpInfoFileName = "AccuTrak.rec";
	if(false == PrintInfoFile(lpInfoFileName))
	{
		return(-1);
	}

	int i;
	// Print details by option
	// Print out ops per-module
	if(lbViewPerModule)
	{
		for(i=0; i<lLogFileNames.size(); i++)
		{
			LoadOneThreadLogFile(lLogFileNames[i]);
		}
		PreprocessAllRecords();
		// sorted by time in module
		PrintLogPerModule();
		// clean up
		UnloadLogFiles();
	}
	// print out ops per-thread
	else if(lbViewPerThread)
	{
		for(i=0; i<lLogFileNames.size(); i++)
		{
			PrintOneThreadLogFile(lLogFileNames[i]);
		}
	}
	// print out stats
	else if(lbViewStats)
	{
		for(i=0; i<lLogFileNames.size(); i++)
		{
			LoadOneThreadLogFile(lLogFileNames[i]);
		}
		// only stats
		PrintStatsPerModule();
		// clean up
		UnloadLogFiles();
	}
	// Print by time stamp by default
	else
	{
		for(i=0; i<lLogFileNames.size(); i++)
		{
			LoadOneThreadLogFile(lLogFileNames[i]);
		}
		PreprocessAllRecords();
		// Print all
		PrintLogByTime();
		// clean up
		UnloadLogFiles();
	}

	return 0;
}
