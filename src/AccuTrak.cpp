/************************************************************************
** FILE NAME..... AccuTrak.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... API implementation
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
#define AT_EXPORTS
#endif

#include "AccuTrak.h"
#include "PatchManager.h"
#include "PatchSym.h"

// Some platforms may need to reference this dummy symbol in order to link
// in accutrak library
DLL_AT_EXIM int gAccuTrakDummy = 0;

// Patch malloc/free incrementally
// Application can call this after modules are dynamically loaded in
// by dlopen or the like.
bool AT_PatchMemRoutines()
{
	bool rc = false;

	if(gPatchMgr)
	{
		if(gPatchMgr->PatchDebug())
		{
			OUTPUT_MSG0(AT_FATAL, "AT_PatchMemRoutines is called\n");
		}
		// On AIX, we need to update cached LD_INFO buffer when new lib is loaded in
		// HP-UX too, since the link list of loaded modules are in my private copy
#if defined(_AIX) || defined(__hpux) || defined(WIN32)
		RefreshLdinfoBuffer();
#endif
		rc = gPatchMgr->Patch();
	}
	else
	{
		rc = AT_StartAccuTrak();
	}

	return rc;
}

// Application calls this function to initialize accutrak
// After return, based on configuration file specification,
// some modules will be patched with replacement memory management
// routines, hence behave differently
//
// On success, it returns true, otherwise false.
bool AT_StartAccuTrak()
{
	const char* lpConfigEnvVar = "AT_CONFIG_FILE";
	bool rc = false;

	// We can only start AccuTrak once
	if(gPatchMgr)
	{
		OUTPUT_MSG0(AT_ERROR, "AccuTrak has already been started\n");
		return false;
	}

	// The envrionment variable tells where the configuration file is
	char* lpConfigFilePath = ::getenv(lpConfigEnvVar);
	if(lpConfigFilePath)
	{
		gPatchMgr = new PatchManager(lpConfigFilePath);
		if(gPatchMgr && gPatchMgr->IsInitialized())
		{
			gPatchMgr->PrintConfigDefinition();
			rc = gPatchMgr->Patch();
		}
		else
		{
			delete gPatchMgr;
			gPatchMgr = NULL;
			::fprintf(stderr, "Failed to start AccuTrak\n");
			return false;
		}
	}
	else
	{
		::fprintf(stderr, "Pleae set envrionment variable [%s] to use AccuTrak features\n", lpConfigEnvVar);
		return false;
	}

	return rc;
}

// Dismantle the data structure of patch manager and its dependency
// Also unpatch malloc/calloc/realoc, but we have to leave free stay until all blocks allocated by
// my allocator have been released.
bool AT_StopAccuTrak()
{
	if(gPatchMgr)
	{
		gPatchMgr->DumpInUseBlocks();
		// Don't release the object because there are still some blocks to be freed
		// maybe even malloced after AT's finalizer is called
		//delete gPatchMgr;
		//gPatchMgr = NULL;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////
// The following starts up AccuTrak at bootup
// and report/clean at shutdown.
///////////////////////////////////////////////////////////////////////////
static void AT_Initializer()
{
	const char* StartupEnvName = "AT_DELAY_START";
	if(::getenv(StartupEnvName))
	{
		return;
	}
	else if(AT_StartAccuTrak())
	{
		OUTPUT_MSG0(AT_INFO, "Start AccuTrak now\n");
	}
}

static void AT_Finalizer()
{
	AT_StopAccuTrak();
}

#ifndef WIN32

// A dummy class to do some initializing work
class AT_Init_Fini
{
public:
	AT_Init_Fini();
	~AT_Init_Fini();
};

AT_Init_Fini::AT_Init_Fini()
{
	AT_Initializer();
}

AT_Init_Fini::~AT_Init_Fini()
{
	AT_Finalizer();
}

static AT_Init_Fini gInitFini;


// The global objects maybe destructed too early, maybe fini function arranged by linker
// will be later.

#else

extern void AT_Init_MallocPadding_cpp();
extern void AT_Fini_MallocPadding_cpp();
extern void AT_Init_ReplacementMalloc_cpp();
extern void AT_Fini_ReplacementMalloc_cpp();
extern void AT_Init_SysAlloc_cpp();
extern void AT_Fini_SysAlloc_cpp();

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		AT_Init_MallocPadding_cpp();
		AT_Init_ReplacementMalloc_cpp();
		AT_Init_SysAlloc_cpp();
		AT_Initializer();
	}
	else if(fdwReason == DLL_PROCESS_DETACH)
	{
		AT_Fini_MallocPadding_cpp();
		AT_Fini_ReplacementMalloc_cpp();
		AT_Fini_SysAlloc_cpp();
		AT_Finalizer();
	}

	return TRUE;
}

#endif
