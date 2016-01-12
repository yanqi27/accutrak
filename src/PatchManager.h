
#ifndef _PATCH_MANAGER_H
#define _PATCH_MANAGER_H
/************************************************************************
** FILE NAME..... PatchManager.h
**
** (c) COPYRIGHT
**
** FUNCTION.........
** The central data structure of the program. It is given too much power
** though convenient.
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
#pragma warning(disable:4800)
#pragma warning(disable:4786)
#endif

#include <list>

#include "CrossPlatform.h"
#include "FuncTypes.h"
#include "BackTrace.h"

// This class remember all the configuration of debuggee
// on the level of modules(DSOs).
//
// This class shall know how to intercept calls like malloc/free/realloc
class PatchManager
{
	// Each module or a group of modules can choose their own MM behavior.
	// The rational is: while developing code, we are most likely concerned about
	// memory problem of a specific library instead of all DSOs. By injecting debugging code
	// into a few dlls instead of all, we make program runs significantly faster. In many
	// cases, it is impractical to run off-shelf tools like purify, efence because of perfromance.
	//
	// Besides debugging, we can reduce memory fragmentation by using private heap for
	// certain DSOs that we know use memory intensively and memory blocks' life span vary
	// a lot which is a sure recipe for fragmentation.
	class ModuleConfig
	{
	public:
		// Debug mode, we can choose:
		// (1) Hardware way, insert dead page before OR after user space
		// (2) Software way by padding some bytes before AND after user space
		//     less overhead(memory and speed), but also less effective
		// (3) Private heap, will let a subset of modules to use a heap other than default
		//     This may decrease fragmentation overall.
		// (4) Record all memory allocation/release in disk files for post analysis
		enum AT_MODE
		{
			AT_MODE_ADD_NONE,
			AT_MODE_ADD_DEAD_PAGE,
			AT_MODE_ADD_PADDING,
			AT_MODE_ADD_PRIVATE_HEAP,
			AT_MODE_ADD_RECORD
		};

		class ModuleToPatch
		{
		private:
			ModuleToPatch() {}
			ModuleToPatch(ModuleToPatch& iModule) {}

		public:
			ModuleToPatch(const char* iModuleName);
			~ModuleToPatch();

			bool mPatched;
			char* mModuleName;

			// malloc
			MallocFuncPtr  mOldMalloc;
			MallocFuncPtr  mNewMalloc;

			// free
			FreeFuncPtr    mOldFree;
			FreeFuncPtr    mNewFree;

			// calloc
			CallocFuncPtr  mOldCalloc;
			CallocFuncPtr  mNewCalloc;

			// realloc
			ReallocFuncPtr mOldRealloc;
			ReallocFuncPtr mNewRealloc;

			// operator new
			OperatorNewPtr mNewOperatorNew;
			OperatorNewPtr mOldOperatorNew;

			// operator delete
			OperatorDeletePtr mNewOperatorDelete;
			OperatorDeletePtr mOldOperatorDelete;

			// operator new[]
			OperatorNewArrayPtr mNewOperatorNewArray;
			OperatorNewArrayPtr mOldOperatorNewArray;

			// operator delete[]
			OperatorDeleteArrayPtr mNewOperatorDeleteArray;
			OperatorDeleteArrayPtr mOldOperatorDeleteArray;
		};

		ModuleConfig();
		~ModuleConfig();

		bool AddModule(const char* iModuleName);

		int  NumModuleToPatch() { return mModules.size(); }

		//ModuleConfig* GetConfigByName(const char* iModuleName);

		void InstallReplacementRoutines(ModuleToPatch* lpModule, int iModuleNum=0);

	public:
		// Mode determines how to instrument selected modules
		AT_MODE mMode;
		// Collection of to be patched modules
		std::list<ModuleToPatch*> mModules;
	};

	/// This class record the patch status of individual module
	/// in order to implement incremental patching
	class ModulePatchStatus
	{
	#define PatchedNone   0x00000000
	#define PatchedDlopen 0x00000001
	#define PatchedFree   0x00000002

	public:
		ModulePatchStatus(const char* ipModuleName);
		~ModulePatchStatus();

		const char* GetModuleName() { return mModuleName; }

		bool DlopenIsPatched() { return mStatus & PatchedDlopen; }
		bool FreeIsPatched()   { return mStatus & PatchedFree;   }

		void SetDlopenPatched() { mStatus |= PatchedDlopen; }
		void SetFreePatched()   { mStatus |= PatchedFree;   }

		void ClearPatchStatus() { mStatus = PatchedNone; }

	private:
		char* mModuleName;
		int mStatus;
	};

public:

	// User can choose different level of message detail
	enum MESSAGE_LEVEL
	{
		AT_NONE,
		AT_FATAL,
		AT_ERROR,
		AT_INFO,
		AT_ALL
	};

	// Error code of invalid memory access
	enum AT_MEM_ERROR
	{
		AT_MEM_ERR_OK,			// 0
		AT_MEM_ERR_OVERRUN,		// 1
		AT_MEM_ERR_UNDERRUN,	// 2
		AT_MEM_ERR_DOUBLE_FREE,	// 3
		AT_MEM_ERR_ACCESS_FREE	// 4
	};

	// Memory related operation event
	enum AT_EVENT
	{
		AT_EVENT_MALLOC,
		AT_EVENT_MALLOC_AROUND,
		AT_EVENT_MALLOC_SIZE,
		AT_EVENT_FREE,
		AT_EVENT_FREE_AROUND,
		AT_EVENT_REALLOC
	};

	// What action to tacke when an event occurs
	enum AT_ACTION
	{
		AT_ACTION_OUTPUT,
		AT_ACTION_PAUSE,
		AT_ACTION_BACKTRACE
	};

	// We can trace certain event. f.g. a block with specified address or size, etc
	struct BlockEvent
	{
		AT_EVENT mType;
		unsigned long mValue;
	};

	PatchManager(const char* iConfigFile);
	~PatchManager();

	// This shall do as the configuration file specify
	// Return true when all required modules are found and patched
	bool Patch();

	// Alignment requirement of user block
	unsigned GetAlignment() { return mAlignment; }

	// System page size
	size_t GetPageSize() { return mPageSize; }

	// Detect underrun
	bool CheckUnderrun() { return mCheckUnderrun; }

	// Detect read/write already freed user block
	bool CheckAccessFreedBlock() { return mCheckAccessFreedBlock; }

	// Flag patch manager is ready
	bool IsInitialized() { return mInitialized; }

	// Flag all modules are patched
	bool IsPatchFinished() { return mPatchFinished; }

	// Flag if dlopen needs be patched
	bool PatchDlopen();

	// Flag if we need to patch all modules' free routines, none-self-contained
	bool PatchCatchAllFree();

	// Flag to report free(NULL) as error
	//bool ReportFreeNull() { return mReportFreeNull; }

	// Flag user's opinion as if selected modules are self-contained, default false
	bool IsSelfContained() { return mSelfContained; }

	// Flag patch manager to spit out detail of patching process
	bool PatchDebug() { return mPatchDebug; }

	// Batch mode
	bool IsBatchMode() { return mBatchMode; }

	// Flag if inremental patch is safe, default YES
	// Two scenarios come into my mind:
	// a. DSO is loaded and unloaded, then later on it is loaded in again.
	// b. Another MM is contending with me to patch malloc/free routines
	bool IncrementalPatch() { return mIncrementalPatch; }

	// Return true if the given named module has already been patched with dlopen/free
	bool DlopenIsPatched(const char* ipModuleName);
	bool FreeIsPatched(const char* ipModuleName);

	// Mark the given named module has already been patched with dlopen/free
	void SetDlopenPatched(const char* ipModuleName);
	void SetFreePatched(const char* ipModuleName);

	// Return number of modules that are cleared with patch status
	int ClearPatchStatus();

	// User selected number of maximun depth of stack frames
	int GetBackTraceDepth()  { return mBackTraceDepth; }
	bool GetBackTraceFree() { return mBackTraceFree; }

	// Flags level of message output user is interested in
	MESSAGE_LEVEL GetMessageLevel() { return mOutputLevel; }

	// Get method of outpout desination
	FILE* GetOutputFile() { return mpOutput; }

	// Call back method when a malloc/free transaction occurs
	void ProcessEvent(AT_EVENT iType, unsigned long iValue, unsigned long iRawBlockAddr, size_t iRawBlockSize);

	// User selected maximum number of freed blocks that are cached for later use
	unsigned long GetMaxCachedBlocks() { return mMaxCachedBlocks; }

	// Return number of bits to shift to calculate number of pages quickly
	//unsigned int GetPageShift() { return mPageShift; }

	// Output the patch manager's view of patching specification
	void PrintConfigDefinition();

	// An event happens, see if we want to check the integrity of all block on in-use list
	void CheckAll(AT_EVENT iType);

	// User choice to fill newly allocated block with a byte pattern
	bool InitUserSpace() { return mInitUserSpace; }

	// User choice to fill just freed block with a byte pattern
	bool FloodFreeSpace() { return mFloodFreeSpace; }

	// Call back method if a memory error is detected
	void ProcessMemError(void* ipUserSpace, AT_MEM_ERROR iError);

	// Dump the in-use blocks we know of into disk files
	void DumpInUseBlocks();

	// Actions after a successful dlopen
	void PostDlopen(const char* iFileName, void* iHandle);

	// Actions after a successful dlclose
	void PostDlclose(void* iHandle);

	// Executable name
	const char* GetProgramName() { return mProgramName; }

	// Block size to track
	unsigned long GetMinBlockSize() { return mMinBlockSize; }
	unsigned long GetMaxBlockSize() { return mMaxBlockSize; }

	// Maximum bytes to defer free
	size_t GetMaxDeferFreeBytes() { return mMaxDeferFreeBytes; }

private:
	// Read the configuration file
	// Return true if config file is valid and consistent
	bool ParseConfigFile(const char* iConfigFile);

	// After reading in all config settings,
	// check to ensure the integrity of them, spit out conflicts
	// prepare for patching, f.g. find out "all" modules, open output stream, etc.
	// Return true if all go well
	bool CheckConfigOptions();

	// We want to patch all modules currently in core
	// Return true if there is new module added
	bool AddAllModulesInCore();

	// Create or update this info file if mode=record is set.
	bool UpdateRecordInfoFile();

#ifdef WIN32
	// User these helper methods on Windows because the patching on window is epic
	void PatchLoadLibraryW(HMODULE ihModule, const char* ipModuleName);
	void PatchFreeW(HMODULE ihModule, const char* ipModuleName);
	void PatchMallocW(HMODULE ihModule, const char* ipModuleName);
	// We never patch some dlls for malloc/free/load
	bool SkipDllToPatchW(const char* ipModuleName);
#endif

	///////////////////////////////////////////////////////////////////////////
	// Data members
	///////////////////////////////////////////////////////////////////////////

	// Configuration of modules to be patched
	ModuleConfig mModuleConfiguration;

	// Number of attempts to patch process
	int mNumPatchAttempts;

	// A list of modules and their patch status
	// May have more modules than config files because of none-self-contained
	std::list<ModulePatchStatus*> mPatchedList;

	// Name of library that provides default mallo/free implementation
	// In general, it is libc.so on UNIX and msvcrt.dll on Window.
	//char* mDefaultMallocModuleName;

	// The list of events to trace
	std::list<BlockEvent*> mEventList;

	// What to do when an event happens
	AT_ACTION mActionType;

	// system default page size
	size_t mPageSize;

	// Maximum bytes to keep in deferred free block list
	size_t mMaxDeferFreeBytes;

	// Maximum number of blocks to be cached
	unsigned long mMaxCachedBlocks;

	// Size range of blocks to track
	unsigned long mMinBlockSize;
	unsigned long mMaxBlockSize;

	// Alignment of allocated block, default to sizeof(int)
	unsigned int mAlignment;

	// Log the caller's back trace, maximum depth of stack frames
	int mBackTraceDepth;

	// How often do we want to check all blocks on the in-use list
	// Zero or Negative number means never ever
	// Positive number means do checking evey so many malloc/free
	int mCheckFrequency;

	// Output stream, either disk file or stdout/stderr
	FILE* mpOutput;

	// Which information to go out
	MESSAGE_LEVEL mOutputLevel;

	// Executable name
	// The linux link_map doesn't show this name for unknown reason.
	char* mProgramName;

	// If yes, we only patch specified modules
	// otherwise, patch free and realloc to all modules (excluding libaccutrak.so and sys libs)
	bool mSelfContained;

	// We can only check either overrun(default) or underrun, not both
	bool mCheckUnderrun;

	// check invalid access of freed block
	// Note: we can only check for last mMaxCachedBlocks
	bool mCheckAccessFreedBlock;

	// Flags we are ready
	bool mInitialized;

	// Flags we have tried patching at least once
	bool mPatchTried;

	// Flags we have patched all DSOs specified
	bool mPatchFinished;

	// If user will also do patching after dlopen, he shall disable my attemp to patch dlopen
	bool mPatchDlopen;

	// If we want to patch operator new/new[]/delete/delete[]
	// Although operator new will in turn call malloc to get memory block,
	// it is defined in c++ runtime which we usually don't want to patch the runtime library.
	// So it is best to patch operator new in application if c++ is used intensively
	//bool mPatchNew;

	// We want every detail of the patching process
	bool mPatchDebug;

	// Batch mode disable any message on console
	bool mBatchMode;

	// If incremental patch is enabled. Yes, by default
	// No for special case where another memory manager is doing the same thing, then we need to patch all module again
	bool mIncrementalPatch;

	// Just patch all user modules
	bool mPatchAllUserModules;

	// If the user want to init a fresh block to certain pattern
	bool mInitUserSpace;

	// Flood the user space of freed block with certain pattern
	bool mFloodFreeSpace;

	// Retrieve call stack when memory block is freed.
	bool mBackTraceFree;

	// Flags we have patched dlopen to all currently loaded DSOs (excluding sys libs)
	//bool mDlopenInstalled;

	// User choice: if we report free(NULL) as an error. default(false)
	//bool mReportFreeNull;
};

// Singleton object to facilitate patching
extern PatchManager* gPatchMgr;

#define AT_FATAL PatchManager::AT_FATAL
#define AT_ERROR PatchManager::AT_ERROR
#define AT_INFO  PatchManager::AT_INFO
#define AT_ALL   PatchManager::AT_ALL

// System library names we need to be care of when patching
#define LIBPTHREAD "libpthread"
#if defined(linux) || defined(sun) || defined(__hpux)
#define MYLIBNAME "libaccutrak.so"
#define LIBC      "libc.so"
#define LIBDL     "libdl.so"
#define LIBSH     "libsmartheap_smp64.so"
#elif defined(_AIX)
#define MYLIBNAME "libaccutrak.a"
#define LIBC      "libc.a"
#define LIBDL     "libdl.a"
#define LIBSH     "libsmartheap_smp64.a"
#elif defined(WIN32)
#define MYLIBNAME "AccuTrak.dll"
#define LIBSH     "SHSMP.dll"
#else
#error Unknown platform
#endif

typedef void* address_t;

// Unfortunately only gcc and aCC support this macro, SunOS's CC and AIX's xlC don't
//#define OUTPUT_MSG(iLevel,ipFormat,args...) \
//	if(gPatchMgr->GetOutputFile() && gPatchMgr->GetMessageLevel() >= (iLevel)) \
//	{ \
//		fprintf(gPatchMgr->GetOutputFile(), ipFormat, ##args); \
//		fflush(gPatchMgr->GetOutputFile()); \
//	}

#define OUTPUT_MSG0(iLevel,ipFormat) \
	if(gPatchMgr->GetOutputFile() && gPatchMgr->GetMessageLevel() >= (iLevel)) \
	{ \
		fprintf(gPatchMgr->GetOutputFile(), (ipFormat)); \
		fflush(gPatchMgr->GetOutputFile()); \
	}

#define OUTPUT_MSG1(iLevel,ipFormat,arg1) \
	if(gPatchMgr->GetOutputFile() && gPatchMgr->GetMessageLevel() >= (iLevel)) \
	{ \
		fprintf(gPatchMgr->GetOutputFile(), (ipFormat), (arg1)); \
		fflush(gPatchMgr->GetOutputFile()); \
	}

#define OUTPUT_MSG2(iLevel,ipFormat,arg1,arg2) \
	if(gPatchMgr->GetOutputFile() && gPatchMgr->GetMessageLevel() >= (iLevel)) \
	{ \
		fprintf(gPatchMgr->GetOutputFile(), (ipFormat), (arg1), (arg2)); \
		fflush(gPatchMgr->GetOutputFile()); \
	}

#define OUTPUT_MSG3(iLevel,ipFormat,arg1,arg2,arg3) \
	if(gPatchMgr->GetOutputFile() && gPatchMgr->GetMessageLevel() >= (iLevel)) \
	{ \
		fprintf(gPatchMgr->GetOutputFile(), (ipFormat), (arg1), (arg2), (arg3)); \
		fflush(gPatchMgr->GetOutputFile()); \
	}

#define OUTPUT_MSG4(iLevel,ipFormat,arg1,arg2,arg3,arg4) \
	if(gPatchMgr->GetOutputFile() && gPatchMgr->GetMessageLevel() >= (iLevel)) \
	{ \
		fprintf(gPatchMgr->GetOutputFile(), (ipFormat), (arg1), (arg2), (arg3), (arg4)); \
		fflush(gPatchMgr->GetOutputFile()); \
	}

#define OUTPUT_MSG5(iLevel,ipFormat,arg1,arg2,arg3,arg4,arg5) \
	if(gPatchMgr->GetOutputFile() && gPatchMgr->GetMessageLevel() >= (iLevel)) \
	{ \
		fprintf(gPatchMgr->GetOutputFile(), (ipFormat), (arg1), (arg2), (arg3), (arg4), (arg5)); \
		fflush(gPatchMgr->GetOutputFile()); \
	}

#endif //_PATCH_MANAGER_H
