/************************************************************************
** FILE NAME..... PatchManager.cpp
**
** (c) COPYRIGHT
**
** FUNCTION......... The commander of all the magics. Control mostly
** patching operations.
**
** NOTES............
**
** ASSUMPTIONS......
**
** RESTRICTIONS.....  Not thread-safe.
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
#include <stdio.h>
#include <dlfcn.h>
#endif

#include "CrossPlatform.h"
#include "PatchManager.h"
#include "ReplacementMalloc.h"
#include "PatchSym.h"
#include "Utility.h"

// Mangled symbol name of new/delete/new[]/delete[]
#if defined(linux) || defined(__hpux)
#define MANGLED_OPERATOR_NEW       "_Znwm"
#define MANGLED_OPERATOR_NEW_ARRAY "_Znam"
#define MANGLED_OPERATOR_DELETE    "_ZdlPv"
#define MANGLED_OPERATOR_DELETE_ARRAY "_ZdaPv"
#elif defined(sun)
#define MANGLED_OPERATOR_NEW       "__1c2n6FL_pv_"
#define MANGLED_OPERATOR_NEW_ARRAY "__1c2N6FL_pv_"
#define MANGLED_OPERATOR_DELETE    "__1c2k6Fpv_v_"
#define MANGLED_OPERATOR_DELETE_ARRAY "__1c2K6Fpv_v_"
#elif defined(_AIX)
#define MANGLED_OPERATOR_NEW       "_Znwm"
#define MANGLED_OPERATOR_NEW_ARRAY "_Znam"
#define MANGLED_OPERATOR_DELETE    "_ZdlPv"
#define MANGLED_OPERATOR_DELETE_ARRAY "_ZdaPv"
#elif defined(WIN32)
#ifdef WIN64
#define MANGLED_OPERATOR_NEW       "??2@YAPEAX_K@Z"
#define MANGLED_OPERATOR_NEW_ARRAY "??_U@YAPEAX_K@Z"
#define MANGLED_OPERATOR_DELETE    "??3@YAXPEAX@Z"
#define MANGLED_OPERATOR_DELETE_ARRAY "??_V@YAXPEAX@Z"
#else
#define MANGLED_OPERATOR_NEW       "??2@YAPAXI@Z"
#define MANGLED_OPERATOR_NEW_ARRAY "??_U@YAPAXI@Z"
#define MANGLED_OPERATOR_DELETE    "??3@YAXPAX@Z"
#define MANGLED_OPERATOR_DELETE_ARRAY "??_V@YAXPAX@Z"
#endif
#else
#error Unknown platform
#endif


#define STASH_OLD_FUNC(oldFunc,funcArray,index) \
	if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_RECORD && prc == Patch_OK && (oldFunc)) \
	{\
		funcArray[index] = (oldFunc);\
	}

#define OUTPUT_PATCH_RESULT(prc,symname,modulename) \
	if(PatchDebug()) \
	{ \
		const char* module = (modulename); \
		if(strlen(module)==0 && mProgramName) \
		{ \
			module = mProgramName; \
		} \
		if( (prc) != Patch_OK ) \
		{ \
			OUTPUT_MSG2(AT_ERROR, "\tFailed to patch %s in module [%s]\n", (symname), (module)) \
		} \
		else \
		{ \
			OUTPUT_MSG2(AT_FATAL, "\t==> Patched %s in module [%s] <==\n", (symname), (module)) \
		} \
	}

#define OUTPUT_PATCH_OK(prc,symname,modulename) \
	if((prc) == Patch_OK && PatchDebug()) \
	{ \
		const char* module = (modulename); \
		if(strlen(module)==0 && mProgramName) \
		{ \
			module = mProgramName; \
		} \
		OUTPUT_MSG2(AT_FATAL, "\t==> Patched %s in module [%s] <==\n", (symname), (module)) \
	}

// Singleton object to facilitate patching
PatchManager* gPatchMgr = NULL;

// ctor
PatchManager::PatchManager(const char* iConfigFile)
	: mAlignment(sizeof(long)),
	mNumPatchAttempts(0),
	mBackTraceDepth(0),
	mCheckFrequency(-1),
	mMaxCachedBlocks(1024),
	mpOutput(stdout),
	mMinBlockSize(0),
	mMaxBlockSize(ULONG_MAX),
	mMaxDeferFreeBytes(0),
	mOutputLevel(AT_FATAL),
	mActionType(AT_ACTION_OUTPUT),
	mProgramName(NULL),
	mSelfContained(false),
	mCheckUnderrun(false),
	mCheckAccessFreedBlock(true),
	mPatchTried(false),
	mPatchFinished(true),
	mPatchDlopen(true),
	mPatchDebug(false),
	mBatchMode(false),
	mIncrementalPatch(true),
	mPatchAllUserModules(false),
	mInitUserSpace(false),
	mFloodFreeSpace(false),
	mBackTraceFree(false)
{
	GET_SYSTEM_PAGE_SIZE(mPageSize);

	mInitialized = ParseConfigFile(iConfigFile);
}

PatchManager::~PatchManager()
{
	// output stream
	if(fileno(mpOutput) > 2)
	{
		::fclose(mpOutput);
	}

	// clean patch status list
	std::list<ModulePatchStatus*>::iterator itps;
	for(itps=mPatchedList.begin(); itps!=mPatchedList.end(); itps++)
	{
		ModulePatchStatus* lpModuleStatus = *itps;
		delete lpModuleStatus;
	}
	mPatchedList.clear();

	// clean event list
	std::list<BlockEvent*>::iterator ite;
	for(ite=mEventList.begin(); ite!=mEventList.end(); ite++)
	{
		BlockEvent* lpEvent = *ite;
		delete lpEvent;
	}
	mEventList.clear();

	/*// delete default lib name
	if(mDefaultMallocModuleName)
	{
		free(mDefaultMallocModuleName);
	}*/

	if(mProgramName)
	{
		free(mProgramName);
	}

	// Object is not valid from this point on
	mInitialized = false;
}

void PatchManager::ProcessEvent(AT_EVENT iType, unsigned long iValue, unsigned long iRawBlockAddr, size_t iRawBlockSize)
{
	// This is needed when process is exiting
	if(!IsInitialized())
	{
		return;
	}

	std::list<BlockEvent*>::iterator ite;
	for(ite=mEventList.begin(); ite!=mEventList.end(); ite++)
	{
		BlockEvent* lpEvent = *ite;
		// If the transaction is malloc/free and the address matches
		if( (lpEvent->mType == iType && lpEvent->mValue == iValue)
			// If we want to find a block that contains the specified address
			|| (lpEvent->mType==AT_EVENT_MALLOC_AROUND && iType==AT_EVENT_MALLOC && lpEvent->mValue>=iRawBlockAddr && lpEvent->mValue<=iRawBlockAddr+iRawBlockSize)
			|| (lpEvent->mType==AT_EVENT_FREE_AROUND && iType==AT_EVENT_FREE && lpEvent->mValue>=iRawBlockAddr && lpEvent->mValue<=iRawBlockAddr+iRawBlockSize) )
		{
			// A string to describe the event
			const char* lEventName = NULL;
			if(lpEvent->mType == AT_EVENT_MALLOC)
			{
				lEventName = "malloc";
			}
			else if(lpEvent->mType == AT_EVENT_MALLOC_AROUND)
			{
				lEventName = "malloc_around";
			}
			else if(lpEvent->mType == AT_EVENT_FREE)
			{
				lEventName = "free";
			}
			else if(lpEvent->mType == AT_EVENT_FREE_AROUND)
			{
				lEventName = "free_around";
			}

			// Dump the message
			if(mActionType == AT_ACTION_OUTPUT)
			{
				OUTPUT_MSG2(AT_FATAL, "Event: %s at 0x%lx\n", lEventName, iValue);
			}
			// Stop until user hit a key
			else if(mActionType == AT_ACTION_PAUSE)
			{
#if defined(linux) || defined(sun) || defined(__hpux) || defined(_AIX)
				char lTerminalPath[L_ctermid + 1] = {0};
				const char* lpTerminalPath = ::ctermid(lTerminalPath);

				FILE* lpTerminal = ::fopen(lTerminalPath, "r+");
				::fprintf(lpTerminal, "Event: %s at 0x%lx, raw block start at 0x%lx size=%ld  Hit a key to continue\n",
							lEventName, iValue, iRawBlockAddr, iRawBlockSize);
				::fflush(lpTerminal);

				char lChar = ::fgetc(lpTerminal);

				::fclose(lpTerminal);
				lpTerminal = NULL;
#elif defined(WIN32)
				char lMessage[128];
				::sprintf(lMessage, "Event: %s at 0x%lx, raw block start at 0x%lx size=%ld",
					lEventName, iValue, iRawBlockAddr, iRawBlockSize);
				::MessageBox(NULL, lMessage, "Event Notifier", MB_OK);
#else
#error Unknown platform
#endif
			}
		}
	}
}

#if defined(linux) || defined(sun) || defined(__hpux) || defined(_AIX)
void* AT_dlopen(const char* iFileName, int flag)
{
	void* handle = ::dlopen(iFileName, flag);
	if(handle)
	{
		gPatchMgr->PostDlopen(iFileName, handle);
	}
	return handle;
}

int AT_dlclose(void* iHandle)
{
	int rc = ::dlclose(iHandle);

	if(rc == 0)
	{
		gPatchMgr->PostDlclose(iHandle);
	}
	return rc;
}
#endif

// Actions after a successful dlopen
void PatchManager::PostDlopen(const char* iFileName, void* iHandle)
{
	// Either there is still DSO not patched or user declare DSOs are not self coontained
	if(!IsPatchFinished() || !IsSelfContained() )
	{
		if(PatchDebug())
		{
			OUTPUT_MSG2(AT_FATAL, "Module [%s] is dynamically loaded with handle [0x%lx]\n", iFileName, iHandle);
		}
		// On AIX, we need to update cached LD_INFO buffer when new lib is loaded in
		// HP-UX too, since the link list of loaded modules are in my private copy
#if defined(_AIX) || defined(__hpux) || defined(WIN32)
		RefreshLdinfoBuffer();
#endif
		// A new DSO and its dependency is just loaded in,
		// if we still have some modules to patch, try to patch again.
		Patch();

		// Update record info file if necessary
		if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_RECORD)
		{
			UpdateRecordInfoFile();
		}
	}
}

// Actions after a successful dlclose
void PatchManager::PostDlclose(void* iHandle)
{
	if(IncrementalPatch())
	{
		ClearPatchStatus();
		if(PatchDebug())
		{
			OUTPUT_MSG1(AT_FATAL, "Module with handle [0x%lx] is dynamically closed\n", iHandle);
		}
	}
}

bool PatchManager::PatchDlopen()
{
	bool rc = true;

	if(gPatchMgr->PatchDebug())
	{
		OUTPUT_MSG0(AT_FATAL, "Patch dlopen because either there are to-be-tracked modules that are not loaded in core yet" \
								" or their malloc/free are not self-contained\n")
	}

	for(void* lpModuleHandle = GetFirstModuleHandle();
		lpModuleHandle;
		lpModuleHandle = GetNextModuleHandle(lpModuleHandle))
	{
		const char* lpModuleName = GetModuleName(lpModuleHandle);

		if(lpModuleName	&& (!mIncrementalPatch || !DlopenIsPatched(lpModuleName)) )
		{
			// Don't patch myself on any platforms.
			if(::strstr(lpModuleName, MYLIBNAME))
			{
				continue;
			}

#if defined(linux) || defined(sun) || defined(__hpux) || defined(_AIX)
			if(::strstr(lpModuleName, "/usr/")
				|| ::strstr(lpModuleName, "/lib64/")
				|| ::strstr(lpModuleName, LIBC)
				|| ::strstr(lpModuleName, LIBDL) )
			{
				// Skip these on Unix
				continue;
			}
			Patch_RC prc;
			// For AIX/XCOFF
			// We need to assign the default function before patching
			// because the patcher will use it as key in TOC to find the target
			FunctionPtr lpOldDlopen = (FunctionPtr)dlopen;
			// patch dlopen
			prc = PatchFunction(lpModuleHandle,
								lpModuleName,
								"dlopen",
								(FunctionPtr)AT_dlopen,
								&lpOldDlopen);
			OUTPUT_PATCH_OK(prc, "dlopen()", lpModuleName)

			// Track dlclose too because a module can be dlclosed and
			// be dlopen again later.
			FunctionPtr lpOldDlclose = (FunctionPtr)dlclose;
			prc = PatchFunction(lpModuleHandle,
								lpModuleName,
								"dlclose",
								(FunctionPtr)AT_dlclose,
								&lpOldDlclose);
			OUTPUT_PATCH_OK(prc, "dlclose()", lpModuleName)

#elif defined(WIN32)
			PatchLoadLibraryW((HMODULE)lpModuleHandle, lpModuleName);
#else
#error Unknown platform
#endif

			// Mark this module already been patched with dlopen
			if(mIncrementalPatch)
			{
				SetDlopenPatched(lpModuleName);
			}
		}
	}

	return rc;
}

// This is called to patch my free/operator delete/operator delete[] into all DSOs
// (excluding some sys libs)
bool PatchManager::PatchCatchAllFree()
{
	bool rc = true;

	if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_RECORD)
	{
		return true;
	}

	// Get the free routine from one of the ModuleToPatch routine
	if(mModuleConfiguration.mModules.empty())
	{
		return true;
	}
	std::list<ModuleConfig::ModuleToPatch*>::iterator it = mModuleConfiguration.mModules.begin();
	FunctionPtr lpReplacementFree    = (FunctionPtr)(*it)->mNewFree;
	FunctionPtr lpReplacementRealloc = (FunctionPtr)(*it)->mNewRealloc;
	FunctionPtr lpReplacementDelete  = (FunctionPtr)(*it)->mNewOperatorDelete;
	FunctionPtr lpReplacementDeleteArray = (FunctionPtr)(*it)->mNewOperatorDeleteArray;

	if(gPatchMgr->PatchDebug())
	{
		OUTPUT_MSG0(AT_FATAL, "Not self-contained, try to Patch free/realloc/delete/delete[]\n")
	}

	for(void* lpModuleHandle = GetFirstModuleHandle();
		lpModuleHandle;
		lpModuleHandle = GetNextModuleHandle(lpModuleHandle))
	{
		const char* lpModuleName = GetModuleName(lpModuleHandle);
		if(lpModuleName	&& (!mIncrementalPatch || !FreeIsPatched(lpModuleName)) )
		{
			// Don't patch myself on any platforms.
			if(::strstr(lpModuleName, MYLIBNAME)
#ifdef WIN32
				|| SkipDllToPatchW(lpModuleName)
#else
				|| ::strstr(lpModuleName, LIBC)
				//|| ::strstr(lpModuleName, "/usr/")
				//|| ::strstr(lpModuleName, LIBPTHREAD)
				|| ::strstr(lpModuleName, LIBSH)
#ifdef _AIX
				|| ::strstr(lpModuleName, "libc_r.a")
#elif defined(linux)
				|| ::strstr(lpModuleName, "\/lib64\/")
				|| ::strstr(lpModuleName, "ld-linux-x86-64.so")
#endif
#endif
													)
			{
				// Skip it, known libraries that could cause trouble if patched
				continue;
			}

			if(PatchDebug())
			{
				OUTPUT_MSG1(AT_FATAL, "\tCheck module [%s]\n", lpModuleName);
			}

			// For AIX/XCOFF
			// We need to assign the default allocator function before patching
			// because the patcher will use it as key in TOC to find the target
			FunctionPtr lpOldFree = (FunctionPtr)free;
			// patch free
			Patch_RC prc = PatchFunction(lpModuleHandle,
											lpModuleName,
											"free",
											lpReplacementFree,
											&lpOldFree);
			OUTPUT_PATCH_OK(prc, "free()", lpModuleName)

			// realloc
			ReallocFuncPtr lpOldRealloc = realloc;
			prc = PatchFunction(lpModuleHandle,
								lpModuleName,
								"realloc",
								lpReplacementRealloc,
								(FunctionPtr*)&lpOldRealloc);

			OUTPUT_PATCH_OK(prc, "realloc()", lpModuleName)

			// operator delete
			OperatorDeletePtr lpOldOperatorDelete = operator delete;
			prc = PatchFunction(lpModuleHandle,
								lpModuleName,
								MANGLED_OPERATOR_DELETE,
								lpReplacementDelete,
								(FunctionPtr*)&lpOldOperatorDelete);
			OUTPUT_PATCH_OK(prc, "operator delete()", lpModuleName)

			// operator delete[]
#ifdef WIN32
			OperatorDeleteArrayPtr lpOldOperatorDeleteArray = operator delete;
#else
			OperatorDeleteArrayPtr lpOldOperatorDeleteArray = operator delete[];
#endif
			prc = PatchFunction(lpModuleHandle,
								lpModuleName,
								MANGLED_OPERATOR_DELETE_ARRAY,
								lpReplacementDeleteArray,
								(FunctionPtr*)&lpOldOperatorDeleteArray);
			OUTPUT_PATCH_OK(prc, "operator delete[]()", lpModuleName)

			// Mark it
			if(mIncrementalPatch)
			{
				SetFreePatched(lpModuleName);
			}
		}
	}

	return rc;
}

// Retrun NULL if the module is NOT in core
static void* GetModuleHandleInCore(const char* ipModuleName)
{

	bool lbModuleInCore = false;
	void* lpModuleHandle;

	if(ipModuleName == NULL)
	{
		return NULL;
	}

	for(lpModuleHandle = GetFirstModuleHandle();
		lpModuleHandle;
		lpModuleHandle = GetNextModuleHandle(lpModuleHandle))
	{
		const char* lpModuleInCore = GetModuleName(lpModuleHandle);
		if(lpModuleInCore == NULL)
		{
			continue;
		}
		const char* lpModuleInCoreBaseName = GetBaseName(lpModuleInCore);
		const char* lpConfiguredModuleBaseName = GetBaseName(ipModuleName);
		if(::AT_STRCMP(lpConfiguredModuleBaseName, lpModuleInCoreBaseName) == 0)
		{
			// Find the module in core
			lbModuleInCore = true;
			break;
		}
	}
	return lbModuleInCore ? lpModuleHandle: NULL;
}

// This is the core method to patch specified modules with
// chosen allocator
// Return true if all modules are found and malloc/free are found in each of them
bool PatchManager::Patch()
{
	bool rc = true;

	mNumPatchAttempts++;
	if(PatchDebug())
	{
		OUTPUT_MSG1(AT_FATAL, "========= Patch Attempt %d =========\n", mNumPatchAttempts);
	}

	// Shall initialize private heaps before any patching.
	if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_PRIVATE_HEAP)
	{
		if(false == InitPrivateHeaps() )
		{
			OUTPUT_MSG0(AT_FATAL, "Failed to initialize private heaps\n")
			return false;
		}
	}

	bool lbDlopenInstalled = false;
	// We need to patch all modules with my free routine
	if(!mSelfContained)
	{
		PatchCatchAllFree();
		// I need also patch dlopen in anticipation of dlopened DSOs
		if(mPatchDlopen)
		{
			if(PatchDlopen())
			{
				lbDlopenInstalled = true;
			}
		}
	}

	// If this is not the first we do patch and user want to patch all
	if(mPatchTried && mPatchAllUserModules)
	{
		// If new library is dynamically loaded in, we need to update our list of modules
		// The following function will return true if new modules added to our list
		if(AddAllModulesInCore())
		{
			mPatchFinished = false;
		}
	}

	if(mPatchFinished && mIncrementalPatch)
	{
		return true;
	}

	if(PatchDebug())
	{
		OUTPUT_MSG0(AT_FATAL, "Start patching process specified by configuration file\n")
	}

	// Go through each module in the group and patch it.
	std::list<ModuleConfig::ModuleToPatch*>::iterator it2;
	for(it2=mModuleConfiguration.mModules.begin(); it2!=mModuleConfiguration.mModules.end(); it2++)
	{
		ModuleConfig::ModuleToPatch* lpModule = *it2;

		// If it is already patched, next please
		if(lpModule->mPatched && mIncrementalPatch)
		{
			continue;
		}

		// Check if the module is in core
		char* lpModuleName = lpModule->mModuleName;
		void* lpModuleHandle = GetModuleHandleInCore(lpModuleName);
		// If we didn't find the module in core at all, we may try it later
		// since they could be dynamically loaded in later by the process.
		if(lpModuleHandle == NULL)
		{
			OUTPUT_MSG1(AT_ERROR, "Failed to find module [%s] in process image\n", lpModuleName);
			OUTPUT_MSG0(AT_ERROR, "Will try to patch again when it is dynamiclly loaded\n");
			if(!lbDlopenInstalled && mPatchDlopen)
			{
				// We want to intercept dlopen so that we have a chance to try again
				// to patch the symbols that configure file specifies.
				if(PatchDlopen())
				{
					lbDlopenInstalled = true;
				}
			}
			rc = false;
			continue;
		}

		if(PatchDebug())
		{
			OUTPUT_MSG1(AT_FATAL, "\tCheck module [%s]\n", lpModuleName);
		}

		// Remember the old routines if I am doing record logging
		long lModuleIndex;
		if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_RECORD)
		{
			lModuleIndex = (long)(lpModule->mOldFree);
			if(lModuleIndex<0 || lModuleIndex>MAX_MODULES_TO_RECORD)
			{
				OUTPUT_MSG0(AT_FATAL, "Unknown module number when remembering old allocate routines\n");
				// set it to max number so that we are unlikely to use it.
				lModuleIndex = MAX_MODULES_TO_RECORD-1;
			}
		}

		Patch_RC prc;
		// patch free
		lpModule->mOldFree = free;
		prc = PatchFunction(lpModuleHandle,
							lpModuleName,
							"free",
							(FunctionPtr)lpModule->mNewFree,
							(FunctionPtr*)&lpModule->mOldFree);
		OUTPUT_PATCH_RESULT(prc, "free()", lpModuleName)
		STASH_OLD_FUNC(lpModule->mOldFree, OldFree, lModuleIndex)

		// patch realloc, it is ok if the module doesn't reference it at all.
		lpModule->mOldRealloc = realloc;
		prc = PatchFunction(lpModuleHandle,
							lpModuleName,
							"realloc",
							(FunctionPtr)lpModule->mNewRealloc,
							(FunctionPtr*)&lpModule->mOldRealloc);
		OUTPUT_PATCH_RESULT(prc, "realloc()", lpModuleName)
		STASH_OLD_FUNC(lpModule->mOldRealloc, OldRealloc, lModuleIndex)

		// patch calloc, it is ok if the module doesn't reference it at all.
		lpModule->mOldCalloc = calloc;
		prc = PatchFunction(lpModuleHandle,
							lpModuleName,
							"calloc",
							(FunctionPtr)lpModule->mNewCalloc,
							(FunctionPtr*)&lpModule->mOldCalloc);
		OUTPUT_PATCH_RESULT(prc, "calloc()", lpModuleName)
		STASH_OLD_FUNC(lpModule->mOldCalloc, OldCalloc, lModuleIndex)

		// malloc shall be patched last, in case program requests a block and try to free it,
		// but we just finished malloc patching, free is not patched yet. Then program will
		// wind up to free it by default MM. we are doomed then.
		lpModule->mOldMalloc = malloc;
		prc = PatchFunction(lpModuleHandle,
							lpModuleName,
							"malloc",
							(FunctionPtr)lpModule->mNewMalloc,
							(FunctionPtr*)&lpModule->mOldMalloc);
		OUTPUT_PATCH_RESULT(prc, "malloc()", lpModuleName)
		STASH_OLD_FUNC(lpModule->mOldMalloc, OldMalloc, lModuleIndex)

		// operator delete
		lpModule->mOldOperatorDelete = operator delete;
		prc = PatchFunction(lpModuleHandle,
							lpModuleName,
							MANGLED_OPERATOR_DELETE,
							(FunctionPtr)lpModule->mNewOperatorDelete,
							(FunctionPtr*)&lpModule->mOldOperatorDelete);
		OUTPUT_PATCH_RESULT(prc, "operator delete()", lpModuleName)
		STASH_OLD_FUNC(lpModule->mOldOperatorDelete, OldDelete, lModuleIndex)

		// operator delete[]
//#ifndef WIN32 // doesn't declare operator delete[] ??
		lpModule->mOldOperatorDeleteArray = operator delete[];
//#endif
		prc = PatchFunction(lpModuleHandle,
							lpModuleName,
							MANGLED_OPERATOR_DELETE_ARRAY,
							(FunctionPtr)lpModule->mNewOperatorDeleteArray,
							(FunctionPtr*)&lpModule->mOldOperatorDeleteArray);
		OUTPUT_PATCH_RESULT(prc, "operator delete[] ()", lpModuleName)
		STASH_OLD_FUNC(lpModule->mOldOperatorDeleteArray, OldDeleteArray, lModuleIndex)

		// operator new
		lpModule->mOldOperatorNew = operator new;
		prc = PatchFunction(lpModuleHandle,
							lpModuleName,
							MANGLED_OPERATOR_NEW,
							(FunctionPtr)lpModule->mNewOperatorNew,
							(FunctionPtr*)&lpModule->mOldOperatorNew);
		OUTPUT_PATCH_RESULT(prc, "operator new()", lpModuleName)
		STASH_OLD_FUNC(lpModule->mOldOperatorNew, OldNew, lModuleIndex)

		// operator new[]
//#ifndef WIN32 // doesn't declare operator new[] ??
		lpModule->mOldOperatorNewArray = (OperatorNewArrayPtr) operator new[];
//#endif
		prc = PatchFunction(lpModuleHandle,
							lpModuleName,
							MANGLED_OPERATOR_NEW_ARRAY,
							(FunctionPtr)lpModule->mNewOperatorNewArray,
							(FunctionPtr*)&lpModule->mOldOperatorNewArray);
		OUTPUT_PATCH_RESULT(prc, "operator new[] ()", lpModuleName)
		STASH_OLD_FUNC(lpModule->mOldOperatorNewArray, OldNewArray, lModuleIndex)

		// Done for this module
		lpModule->mPatched = true;
	}

	if(rc)
	{
		mPatchFinished = true;
		OUTPUT_MSG0(AT_FATAL, "All specified modules are patched\n");
	}
	mPatchTried = true;

	if(PatchDebug())
	{
		OUTPUT_MSG0(AT_FATAL, "\n");
	}

	return rc;
}

bool PatchManager::DlopenIsPatched(const char* ipModuleName)
{
	bool rc = false;
	std::list<ModulePatchStatus*>::iterator itps;
	for(itps=mPatchedList.begin(); itps!=mPatchedList.end(); itps++)
	{
		ModulePatchStatus* lpModuleStatus = *itps;
		// Found the module by name
		if(::AT_STRCMP(ipModuleName, lpModuleStatus->GetModuleName()) == 0)
		{
			rc = lpModuleStatus->DlopenIsPatched();
			break;
		}
	}
	return rc;
}

bool PatchManager::FreeIsPatched(const char* ipModuleName)
{
	bool rc = false;
	std::list<ModulePatchStatus*>::iterator itps;
	for(itps=mPatchedList.begin(); itps!=mPatchedList.end(); itps++)
	{
		ModulePatchStatus* lpModuleStatus = *itps;
		// Found the module by name
		if(::AT_STRCMP(ipModuleName, lpModuleStatus->GetModuleName()) == 0)
		{
			rc = lpModuleStatus->FreeIsPatched();
			break;
		}
	}
	return rc;
}

void PatchManager::SetDlopenPatched(const char* ipModuleName)
{
	ModulePatchStatus* lpModuleStatus = NULL;
	std::list<ModulePatchStatus*>::iterator itps;
	for(itps=mPatchedList.begin(); itps!=mPatchedList.end(); itps++)
	{
		// Found the module by name
		if(::AT_STRCMP(ipModuleName, (*itps)->GetModuleName()) == 0)
		{
			lpModuleStatus = *itps;
			break;
		}
	}

	// Create a new patch status for this module if not exist yet
	if(!lpModuleStatus)
	{
		lpModuleStatus = new ModulePatchStatus(ipModuleName);
		mPatchedList.push_front(lpModuleStatus);
	}

	lpModuleStatus->SetDlopenPatched();
}

void PatchManager::SetFreePatched(const char* ipModuleName)
{
	ModulePatchStatus* lpModuleStatus = NULL;
	std::list<ModulePatchStatus*>::iterator itps;
	for(itps=mPatchedList.begin(); itps!=mPatchedList.end(); itps++)
	{
		// Found the module by name
		if(::AT_STRCMP(ipModuleName, (*itps)->GetModuleName()) == 0)
		{
			lpModuleStatus = *itps;
			break;
		}
	}

	// Create a new patch status for this module if not exist yet
	if(!lpModuleStatus)
	{
		lpModuleStatus = new ModulePatchStatus(ipModuleName);
		mPatchedList.push_front(lpModuleStatus);
	}

	lpModuleStatus->SetFreePatched();
}

int PatchManager::ClearPatchStatus()
{
	int lNumModules = 0;
	std::list<ModulePatchStatus*>::iterator itps;
	for(itps=mPatchedList.begin(); itps!=mPatchedList.end(); itps++)
	{
		ModulePatchStatus* lpModuleStatus = *itps;
		// Only flag those not in the core any more.
		if(GetModuleHandleInCore(lpModuleStatus->GetModuleName()) == NULL)
		{
			lpModuleStatus->ClearPatchStatus();
			lNumModules++;
		}
	}
	return lNumModules;
}

// We want to patch all user modules currently in core
// Return true if there is new module added
bool PatchManager::AddAllModulesInCore()
{
	int lOldNumModules = mModuleConfiguration.NumModuleToPatch();

	// Collect all user modules to be patched
	for(void* lpModuleHandle = GetFirstModuleHandle();
		lpModuleHandle;
		lpModuleHandle = GetNextModuleHandle(lpModuleHandle))
	{
		const char* lpModuleName = GetModuleName(lpModuleHandle);

		// Always skip AccuTrak library itself
		// And pthread library
		if(lpModuleName==NULL || ::strstr(lpModuleName, MYLIBNAME)
			|| ::strstr(lpModuleName, "libpthread") )
		{
			continue;
		}

#ifdef WIN32
		if(SkipDllToPatchW(lpModuleName))
		{
			continue;
		}
#endif

		// If mode is record, all modules are added including system libs
		// Otherwise, all user modules
#if defined(linux) || defined(sun) || defined(__hpux) || defined(_AIX)
		if( ::strncmp(lpModuleName, "/usr/", 5)	// Nothing from /usr (assume they are system libs)
			&& ::strncmp(lpModuleName, "/lib", 4)	// or either /lib or lib64
			&& !::strstr(lpModuleName, "libc.")
			&& !::strstr(lpModuleName, "libpthread")
			&& !::strstr(lpModuleName, LIBSH)
#ifdef AIX
			&& !::strstr(lpModuleName, "libc_r.a")
#elif defined(sun)
			&& !::strstr(lpModuleName, "libthread.so")
#endif
			)
#endif
		{
			const char* lpModuleBaseName = GetBaseName(lpModuleName);
			mModuleConfiguration.AddModule(lpModuleBaseName);
		}
	}
	int lNewNumModules = mModuleConfiguration.NumModuleToPatch();
	return (lNewNumModules-lOldNumModules > 0);
}

// Patch mgr shall decide how often to do the grand check
void PatchManager::CheckAll(AT_EVENT iType)
{
	static int gCount = 0;

	if(mCheckFrequency>0 && ++gCount%mCheckFrequency==0)
	{
		gCount = 0;
		AT_CheckAllBlocks();
	}
}

// Patch mgr shall know how to deal with errors
void PatchManager::ProcessMemError(void* lpUser, AT_MEM_ERROR iError)
{
	OUTPUT_MSG2(AT_FATAL, "FATAL memory error: %d at 0x%lx\n", iError, lpUser);
	::abort();
}

void PatchManager::DumpInUseBlocks()
{
	if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_PADDING)
	{
		// Patch mgr knows where to dump this information
		const char* lpFileName = "objdump.txt";

		FILE* lpFile = ::fopen(lpFileName, "a");
		if(lpFile)
		{
			AT_DumpInUseBlocks(lpFile);
			::fclose(lpFile);
		}
		else
		{
			OUTPUT_MSG1(AT_FATAL, "Failed to open file [%s] to dump out in-use memory blocks\n", lpFileName);
		}
	}
}

PatchManager::ModulePatchStatus::ModulePatchStatus(const char* ipModuleName)
	: mStatus(PatchedNone)
{
	mModuleName = (char*) malloc(strlen(ipModuleName)+1);
	::strcpy(mModuleName, ipModuleName);
}

PatchManager::ModulePatchStatus::~ModulePatchStatus()
{
	::free(mModuleName);
	mStatus = -1;
}
