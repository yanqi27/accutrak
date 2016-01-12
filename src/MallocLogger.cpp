/************************************************************************
** FILE NAME..... MallocLogger.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION.........
** Replacement malloc in order to record all transactions into disk file
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
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sys/time.h>
#endif

#include "CrossPlatform.h"
#include "ReplacementMalloc.h"
#include "PatchManager.h"
#include "MallocLogger.h"

const char* gLogPathEnvName = "AT_LOG_PATH";

#define BUFFER_SIZE 4096

#ifdef WIN32
// Start time
static LARGE_INTEGER gStartTime;
static LARGE_INTEGER gFrequency;
#else
// Start time
static struct timeval gStartTime;
#endif

// Each thread holds one following TLS structure to facilitate the logging
typedef struct _FileControl
{
	FILE* mpFile;
	int   mTouched;
#ifndef WIN32	// Non-buffer for window for now. !FIX!
	char  mBuffer[BUFFER_SIZE];
	size_t mBufferOffset;
#endif
} FileControl;

/////////////////////////////////////////////////////////////////////
// Global vars.
/////////////////////////////////////////////////////////////////////

/// Key for the thread-specific buffer
static TLS_key_t buffer_key;

static bool gProcessExitting = false;

// Original Routines to allocate/deallocate blocks
// This is to work around mixed msvcrt.dll/msvcrtd.dll secnario.
// Block allocated by one dll has to be deallocated by the same dll
void* (* OldMalloc[MAX_MODULES_TO_RECORD]) (size_t) = {NULL};
void  (* OldFree[MAX_MODULES_TO_RECORD]) (void*)    = {NULL};
void* (* OldRealloc[MAX_MODULES_TO_RECORD]) (void*, size_t) = {NULL};
void* (* OldCalloc[MAX_MODULES_TO_RECORD]) (size_t, size_t) = {NULL};
void* (* OldNew[MAX_MODULES_TO_RECORD]) (size_t)    = {NULL};
void  (* OldDelete[MAX_MODULES_TO_RECORD]) (void*)  = {NULL};
void* (* OldNewArray[MAX_MODULES_TO_RECORD]) (size_t)   = {NULL};
void  (* OldDeleteArray[MAX_MODULES_TO_RECORD]) (void*) = {NULL};

/////////////////////////////////////////////////////////////////////
// class AT_REC_HEADER
/////////////////////////////////////////////////////////////////////

// Elapse time in ms since AccuTrak record is started.
static at_timestamp_t AT_CalcElapseTimeInMilliSecond()
{
#ifdef WIN32
	LARGE_INTEGER lCounter;
	if(false == QueryPerformanceCounter(&lCounter))
	{
		return 0;
	}
	return (1000 * (lCounter.QuadPart - gStartTime.QuadPart)) / gFrequency.QuadPart;
#else
	timeval lNow;

	::gettimeofday(&lNow, NULL);

	return ((long)lNow.tv_sec - (long)gStartTime.tv_sec)*1000 + ((long)lNow.tv_usec - (long)gStartTime.tv_usec)/1000;
#endif
}

AT_MALLOC_RECORD::AT_MALLOC_RECORD(size_t iSize, void* ipBlock, int iModuleNum)
	: mType(AT_MEM_OP_TYPE_MALLOC), mModuleNum(iModuleNum),
	mSize(iSize), mpAllocatedBlock(ipBlock), mFreeByOtherThread(0), mReserve(0)
{
	mTimeStamp = AT_CalcElapseTimeInMilliSecond();
}

AT_FREE_RECORD::AT_FREE_RECORD(void* ipBlock, int iModuleNum)
	: mType(AT_MEM_OP_TYPE_FREE), mModuleNum(iModuleNum),
	mpFreedBlock(ipBlock), mAllocatedByOtherThread(0), mReserve(0)
{
	mTimeStamp = AT_CalcElapseTimeInMilliSecond();
}

AT_REALLOC_RECORD::AT_REALLOC_RECORD(void* ipOldBlock, size_t iNewSize, void* ipNewBlock, int iModuleNum)
	: mType(AT_MEM_OP_TYPE_REALLOC), mModuleNum(iModuleNum),
	mpOldBlock(ipOldBlock), mNewSize(iNewSize),	mpNewBlock(ipNewBlock),
	mFreeByOtherThread(0), mAllocatedByOtherThread(0), mReserve(0)
{
	mTimeStamp = AT_CalcElapseTimeInMilliSecond();
}

/////////////////////////////////////////////////////////////////////
// Thread functions starts here.
/////////////////////////////////////////////////////////////////////

// Free the thread-specific buffer
// This is the callback function passed to pthread_key_create()
static void buffer_destroy(void * buf)
{
	FileControl* lpFileCtl = (FileControl*)buf;
	if(lpFileCtl->mpFile)
	{
		// Linux, at this point the key value is NULL
		// But fclose will call free() which will windup in AT_FreeLog
		// It will then create a new key value and open the same per-thread log file.
		//
		// Reinstall the key value here to prevent this mess
		// After we fclose the log file, ground the key value manually.
		if(TLS_SET_VALUE(buffer_key, lpFileCtl))
		{
			OUTPUT_MSG1(AT_FATAL, "Thread [%ld] failed to set thread specific at thread exit\n", GET_THREADID())
		}
		lpFileCtl->mTouched = 1;
#ifndef WIN32
		// write the rest of record into disk file
		if(lpFileCtl->mBufferOffset > 0)
		{
			if(1 != ::fwrite(lpFileCtl->mBuffer, lpFileCtl->mBufferOffset, 1, lpFileCtl->mpFile))
			{
				OUTPUT_MSG1(AT_FATAL, "Thread [%ld] failed to Log the last malloc/free.\n", GET_THREADID())
			}
			lpFileCtl->mBufferOffset = 0;
		}
#endif
		::fclose(lpFileCtl->mpFile);
		lpFileCtl->mpFile = NULL;
		// Now set the key value to NULL
		if(TLS_SET_VALUE(buffer_key, NULL))
		{
			OUTPUT_MSG1(AT_FATAL, "Thread [%ld] failed to set thread specific at thread exit\n", GET_THREADID())
		}

	}
	lpFileCtl->mTouched = 0;
	::free(lpFileCtl);
}

void AT_FiniMallocLogger ()
{
	// It appears the main thread doesn't call the TLS destructor
	// We got to do it explicitly here
	FileControl* lpFileCtl = (FileControl*) TLS_GET_VALUE(buffer_key);
	if(lpFileCtl)
	{
		buffer_destroy(lpFileCtl);
	}
	gProcessExitting = true;
}

// Create the key to the TLS
void AT_InitMallocLogger ()
{
	TLS_CREATE_KEY(buffer_key, buffer_destroy);

#ifdef WIN32
	if (0 == ::QueryPerformanceFrequency(&gFrequency))
	{
		// Bail out if the system doesn't support this
		fprintf(stderr, "Failed to QueryPerformanceFrequency\n");
		exit(0);
	}

	if(false == ::QueryPerformanceCounter(&gStartTime))
	{
		::fprintf(stderr, "Failed to QueryPerformanceCounter\n");
	}
#else
	if(::gettimeofday(&gStartTime, NULL))
	{
		::fprintf(stderr, "Failed to gettimeofday\n");
	}
#endif

	::atexit(AT_FiniMallocLogger);
}

// Create and update the information file for recording
bool PatchManager::UpdateRecordInfoFile()
{
	static AT_REC_HEADER gRecHeader;
	if(mModuleConfiguration.mMode != ModuleConfig::AT_MODE_ADD_RECORD)
	{
		return false;
	}

	if(gRecHeader.mTotalModules == mModuleConfiguration.NumModuleToPatch())
	{
		return true;
	}
	
	// We need to update the file now, 
	// This could be the first time we create the file
	// or maybe some modules are just dynamically loaded in
	gRecHeader.mTotalModules = mModuleConfiguration.NumModuleToPatch();	
	
	const char* lBaseFileName = "AccuTrak.rec";
	// Get usr-chosen path
	const char* lLogDirectory = ::getenv(gLogPathEnvName);
	char* lFileName;
	if(lLogDirectory)
	{
		// Note: strlen(NULL) will core dump on Solaris.
		lFileName = (char*) malloc(strlen(lLogDirectory)+16);
		sprintf(lFileName, "%s/%s", lLogDirectory, lBaseFileName);
	}
	else
	{
		lFileName = strdup(lBaseFileName);
	}

	FILE* lpFile = ::fopen(lFileName, "wb");
	if(!lpFile)
	{
		::fprintf(stderr, "Failed to fopen file %s\n", lFileName);
		return false;
	}
	free(lFileName);

	// Write header
	if(1 != ::fwrite(&gRecHeader, sizeof(AT_REC_HEADER), 1, lpFile))
	{
		::fprintf(stderr, "Failed to write to file %s\n", lFileName);
		fclose(lpFile);
		return false;
	}
	// Write modules names
	std::list<ModuleConfig::ModuleToPatch*>::iterator it2;
	for(it2=mModuleConfiguration.mModules.begin(); it2!=mModuleConfiguration.mModules.end(); it2++)
	{
		const char* lModuleName = (*it2)->mModuleName;
#ifdef linux
		if(strlen(lModuleName) == 0 && mProgramName)
		{
			lModuleName = mProgramName;
		}
#endif
		if(1 != ::fwrite(lModuleName, strlen(lModuleName)+1, 1, lpFile))
		{
			::fprintf(stderr, "Failed to write to file %s\n", lFileName);
			fclose(lpFile);
			return false;
		}
	}

	// Done
	fclose(lpFile);
	return true;
}

// Log one record of transaction
static void AT_LogRecord(FileControl* ipFileCtl,
						 const void*  ipBuffer,
						 size_t       iRecordSize)
{
	if(!ipFileCtl)
	{
		return;
	}

#ifdef WIN32
	if(1 != ::fwrite(ipBuffer, iRecordSize, 1, ipFileCtl->mpFile))
	{
		OUTPUT_MSG1(AT_FATAL, "Thread [%ld] failed to Log a malloc/free.\n", GET_THREADID())
		return;
	}
	// !FIX! flush at the end
	::fflush(ipFileCtl->mpFile);
#else
	// If the buffer is full, write it out first.
	if(ipFileCtl->mBufferOffset + iRecordSize > BUFFER_SIZE)
	{
		if(1 != ::fwrite(ipFileCtl->mBuffer, ipFileCtl->mBufferOffset, 1, ipFileCtl->mpFile))
		{
			OUTPUT_MSG1(AT_FATAL, "Thread [%ld] failed to Log a malloc/free.\n", GET_THREADID())
			return;
		}
		ipFileCtl->mBufferOffset = 0;
	}

	// Just put the record into buffer.
	::memcpy(ipFileCtl->mBuffer+ipFileCtl->mBufferOffset, ipBuffer, iRecordSize);
	ipFileCtl->mBufferOffset += iRecordSize;
#endif
}

static FileControl* CreateAndSetFileControlTLS()
{
	// If the process is winding down, don't create new TLS
	if(gProcessExitting)
	{
		return NULL;
	}

	// New and fill the file control structure
	FileControl* lpFileCtl= (FileControl*) malloc(sizeof(FileControl));
	if(lpFileCtl)
	{
		lpFileCtl->mTouched = 0;
		lpFileCtl->mpFile   = NULL;
#ifndef WIN32
		lpFileCtl->mBufferOffset = 0;
#endif
		// Set this object as TLS for known key
		if(TLS_SET_VALUE(buffer_key, lpFileCtl))
		{
			OUTPUT_MSG1(AT_FATAL, "Thread [%ld] failed to set thread specific\n", GET_THREADID())
		}
	}
	else
	{
		OUTPUT_MSG1(AT_FATAL, "Thread [%ld] failed to create TLS\n", GET_THREADID())
	}

	return lpFileCtl;
}

static FILE* OpenPerThreadLogFile()
{
	ThreadID_t lMyThreadID = (ThreadID_t) GET_THREADID();
	
	// Generate the log file name
	// which includes usr-chosen path and mangled with threadID
	const char* lLogDirectory = ::getenv(gLogPathEnvName);
	char* lFileName;
	if(lLogDirectory)
	{
		// Note: strlen(NULL) will core dump on Solaris.
		lFileName = (char*) malloc(strlen(lLogDirectory)+32);
		sprintf(lFileName, "%s/TID_%ld.log", lLogDirectory, lMyThreadID);
	}
	else
	{
		lFileName = (char*) malloc(32);
		sprintf(lFileName, "TID_%ld.log", lMyThreadID);
	}

#if defined(sun) || defined(linux)
	// Solaris threads will malloc even after buffer_destroy is called
	// Hence server reopen this file and overwrite old content.
	FILE* lpFile = ::fopen(lFileName, "ab");
#else
	FILE* lpFile = ::fopen(lFileName, "wb");
#endif
	if(!lpFile)
	{
		OUTPUT_MSG2(AT_FATAL, "Thread [%ld] failed to fopen file %s\n", lMyThreadID, lFileName)
	}
	free(lFileName);

	return lpFile;
}

#ifdef WIN32
#define DEFAULT_ALLOC(size,OldAlloc,modNum)  (OldAlloc[modNum])((size))
#define DEFAULT_DEALLOC(p,OldDealloc,modNum) (OldDealloc[modNum])((p))
#define DEFAULT_REALLOC(p,size,modNum)       (OldRealloc[modNum])((p),(size))
#else
#define DEFAULT_ALLOC(size,OldAlloc,modNum)  ::malloc((size))
#define DEFAULT_DEALLOC(p,OldDealloc,modNum) ::free((p))
#define DEFAULT_REALLOC(p,size,modNum)       ::realloc((p),(size))
#endif

/////////////////////////////////////////////////////////////////////
// Combined malloc/operator new/operator new[] 
//          free/operator delete/operator delete[]
/////////////////////////////////////////////////////////////////////
static void* AT_AllocateLog(size_t size, int iModuleNum, void*(*OldAlloc[])(size_t))
{
	void *result;

	// Get thread specific data
	FileControl* lpFileCtl = (FileControl*) TLS_GET_VALUE(buffer_key);
	if(lpFileCtl == NULL)
	{
		lpFileCtl = CreateAndSetFileControlTLS();
		// Uh, You must be kidding, something really bad happens
		// Let default malloc handle it
		if(!lpFileCtl)
		{
			return DEFAULT_ALLOC(size, OldAlloc, iModuleNum);
		}
	}

	// Avoid infinite recursive loop if this function is already entered before
	if(lpFileCtl->mTouched)
	{
		return DEFAULT_ALLOC(size, OldAlloc, iModuleNum);
	}
	else
	{
		lpFileCtl->mTouched = 1;

		result = DEFAULT_ALLOC(size, OldAlloc, iModuleNum);

		// Log file is not created yet
		if(lpFileCtl->mpFile == NULL)
		{
			lpFileCtl->mpFile = OpenPerThreadLogFile();
		}
		/// Log the transaction
		AT_MALLOC_RECORD lMallocRecord(size, result, iModuleNum);
		AT_LogRecord(lpFileCtl, &lMallocRecord, sizeof(lMallocRecord));

		lpFileCtl->mTouched = 0;
	}
	return result;
}

static void AT_DeallocateLog(void* ptr, int iModuleNum, void(*OldDealloc[])(void*))
{
	// Get thread specific data
	FileControl* lpFileCtl = (FileControl*) TLS_GET_VALUE(buffer_key);
	if(lpFileCtl == NULL)
	{
		lpFileCtl = CreateAndSetFileControlTLS();
		if(!lpFileCtl)
		{
			DEFAULT_DEALLOC(ptr, OldDealloc, iModuleNum);
			return;
		}
	}

	if(lpFileCtl->mTouched)
	{
		// Avoid recursive my_malloc
		DEFAULT_DEALLOC(ptr, OldDealloc, iModuleNum);
		return;
	}
	else
	{
		lpFileCtl->mTouched = 1;

		DEFAULT_DEALLOC(ptr, OldDealloc, iModuleNum);

		// Log file is not created yet
		if(lpFileCtl->mpFile == NULL)
		{
			lpFileCtl->mpFile = OpenPerThreadLogFile();
		}
		/// Log the transaction
		AT_FREE_RECORD lFreeRecord(ptr, iModuleNum);
		AT_LogRecord(lpFileCtl, &lFreeRecord, sizeof(lFreeRecord));

		lpFileCtl->mTouched = 0;
	}
}

/////////////////////////////////////////////////////////////////////
// Replacement routines called by instrumented program.
/////////////////////////////////////////////////////////////////////
static void* AT_MallocLog(size_t size, int iModuleNum)
{
	return AT_AllocateLog(size, iModuleNum, OldMalloc);
}

static void AT_FreeLog(void* ptr, int iModuleNum)
{
	AT_DeallocateLog(ptr, iModuleNum, OldFree);
}

static void* AT_CallocLog(size_t iNumElement, size_t iElementSize, int iModuleNum)
{
	size_t lTotalSize = iNumElement*iElementSize;
	void* lpResult = AT_MallocLog(lTotalSize, iModuleNum);
	if(lpResult && lTotalSize)
	{
		::memset(lpResult, 0, lTotalSize);
	}
	return lpResult;
};

static void* AT_ReallocLog(void* ipBlock, size_t iNewSize, int iModuleNum)
{
	void* result;

	// Get thread specific data
	FileControl* lpFileCtl = (FileControl*) TLS_GET_VALUE(buffer_key);
	if(lpFileCtl == NULL)
	{
		lpFileCtl = CreateAndSetFileControlTLS();
		if(!lpFileCtl)
		{
			return DEFAULT_REALLOC(ipBlock, iNewSize, iModuleNum);
		}
	}

	if(lpFileCtl->mTouched)
	{
		// Avoid recursive my_malloc
		return DEFAULT_REALLOC(ipBlock, iNewSize, iModuleNum);
	}
	else
	{
		lpFileCtl->mTouched = 1;

		result = DEFAULT_REALLOC(ipBlock, iNewSize, iModuleNum);

		// Log file is not created yet
		if(lpFileCtl->mpFile == NULL)
		{
			lpFileCtl->mpFile = OpenPerThreadLogFile();
		}
		/// Log the transaction
		AT_REALLOC_RECORD lReallocRecord(ipBlock, iNewSize, result, iModuleNum);
		AT_LogRecord(lpFileCtl, &lReallocRecord, sizeof(lReallocRecord));

		lpFileCtl->mTouched = 0;
	}
	return result;
}

static void* AT_NewLog(size_t size, int iModuleNum)
{
	void* lpResult = AT_AllocateLog(size, iModuleNum, OldNew);
	if(!lpResult)
	{
		throw std::bad_alloc();
	}
	return lpResult;
}

static void AT_DeleteLog(void* ipBlock, int iModuleNum)
{
	AT_DeallocateLog(ipBlock, iModuleNum, OldDelete);
}

static void* AT_NewArrayLog(size_t size, int iModuleNum)
{
	void* lpResult = AT_AllocateLog(size, iModuleNum, OldNewArray);
	if(!lpResult)
	{
		throw std::bad_alloc();
	}
	return lpResult;
}

static void AT_DeleteArrayLog(void* ipBlock, int iModuleNum)
{
	AT_DeallocateLog(ipBlock, iModuleNum, OldDeleteArray);
}

/********************************************************************
* This file is mechanically generated by GenMallocLogFuncs.pl script
* which has the pattern below.
* Note: these routines are repeated with suffix _x from 0 to max
********************************************************************/
#include "MallocLogFuncs.inc"

/*
/////////////////////////////////////////////////////////////////////
// Per Module malloc/free routines
/////////////////////////////////////////////////////////////////////
static void* MallocLog_0(size_t iSize)
{
	return AT_MallocLog(iSize, 0);
}

static void FreeLog_0(void* ipBlock)
{
	return AT_FreeLog(ipBlock, 0);
}

static void* ReallocLog_0(void* ipBlock, size_t iNewSize)
{
	return AT_ReallocLog(ipBlock, iNewSize, 0);
}

static void* CallocLog_0(size_t iNumElement, size_t iElementSize)
{
	size_t lTotalSize = iNumElement*iElementSize;
	void* lpResult = AT_MallocLog(lTotalSize, 0);
	if(lpResult && lTotalSize)
	{
		::memset(lpResult, 0, lTotalSize);
	}
	return lpResult;
}

static void* NewLog_0(size_t iBlockSize)
{
	return NewLog(iBlockSize, 0);
}

static void DeleteLog_0(void* ipBlock)
{
	AT_DeleteLog(ipBlock, 0);
};

static void* NewArrayLog_0(size_t iBlockSize)
{
	return AT_NewArrayLog(iBlockSize, 0);
};

static void DeleteArrayLog_0(void* ipBlock)
{
	AT_DeleteArrayLog(ipBlock, 0);
};

/////////////////////////////////////////////////////////////////////
// Array of routines index by module number
// These routines are patched into correspondig modules
/////////////////////////////////////////////////////////////////////
void* (* MallocBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t) =
{
	MallocLog_0,
};

void  (* FreeBlockWithLog[MAX_MODULES_TO_RECORD]) (void*) = 
{
	FreeLog_0,
};

void* (* ReallocBlockWithLog[MAX_MODULES_TO_RECORD]) (void*, size_t) =
{
	ReallocLog_0,
};

void* (* CallocBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t, size_t) =
{
	CallocLog_0,
};

void* (* NewBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t) = 
{
	NewLog_0,
};
*/
