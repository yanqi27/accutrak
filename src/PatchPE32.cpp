/************************************************************************
** FILE NAME..... PatchPE32.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Patch symbols in PE executable or DLL.
**
** NOTES............  
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS.....  Portable Executable Win32 only
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
#include <windows.h>
#include <DbgHelp.h>
#include "PSAPI.h"

#include "PatchSym.h"
#include "PatchManager.h"
#include "ReplacementMalloc.h"

// A simple structure to pair a symobl to patch and the replacement procedure
struct PatchSymbol
{
	const char* mName;
	PROC        mNewProc;
};

// Don't ever try to touch these dlls
static const char * gIgnoreSystemDLLs[] =
{
	// Including me
	MYLIBNAME,
	"msvcrt.dll",
	"msvcrtd.dll",
	"msvcr80.dll",
	"msvcr80d.dll",
	// Do following dll have debug version??
	"ntdll.dll",
	"kernel32.dll",
	"user32.dll",
	"gdi32.dll",
	"advapi32.dll",
	"comdlg32.dll",
	"comctl32.dll",
	"ctl3d32.dll",
	"shell32.dll",
	"rpcrt4.dll",
	"rpcns4.dll",
	"rpcdce4.dll",
	"winspool.drv",
	"lz32.dll",
	"version.dll",
	"winmm.dll",
	"dsound.dll",
	"ddraw.dll",
	"mpr.dll",
	"glu32.dll",
	"opengl32.dll",
	"dlcapi.dll",
	"msacm32.dll",
	"ole2.dll",
	"oleaut32.dll",
	"olecli32.dll",
	"oledlg.dll",
	"ole32.dll",
	"olesvr32.dll",
	"nddeapi.dll",
	"netapi32.dll",
	"netrap.dll",
	"samlib.dll",
	"rasapi32.dll",
	"rasfil32.dll",
	"rasppp.dll",
	"raspppen.dll",
	"rasman.dll",
	"rasser.dll",
	"rascauth.dll",
	"mfcuia32.dll",
	"mfcuiw32.dll",
	"mfcans32.dll",
	"vdmdbg.dll",
	"msvfw32.dll",
	"win32spl.dll",
	"spoolss.dll",
	"winstrm.dll",
	"wsock32.dll",
	"avifil32.dll",
	"avicap32.dll",
	"msidle.dll",
	"oleacc.dll",
	"shlwapi.dll",
	"spyhk40.dll",
	"ha20load.dll",
	"ha30load.dll",
	"haloader.dll",
	"haloader.exe",
	"sxloader.dll",
	"sxloader.exe",
	"dbi.dll",
	"mspdb40.dll",
	"mspdb41.dll",
	"mspdb50.dll",
	"mspdb60.dll",
	"mspdb70.dll",
	"msdia20.dll",
	"mso97.dll",
	"mso97rt.dll",
	"mso9.dll",
	NULL
};

HMODULE WINAPI AT_LoadLibraryA(LPCTSTR ipFileName)
{
	HMODULE handle = ::LoadLibraryA(ipFileName);
	if(handle != NULL)
	{
		gPatchMgr->PostDlopen(ipFileName, handle);
	}
	return handle;
}

HMODULE WINAPI AT_LoadLibraryW(LPCWSTR ipFileName)
{
	HMODULE handle = ::LoadLibraryW(ipFileName);
	if(handle != NULL)
	{
		//const size_t lResultLength = (::wcslen(ipFileName)+1) * MB_CUR_MAX;
		//char* lpFileNameA = new char[lResultLength];
		char lpFileNameA[MAX_PATH];
		if(::wcstombs(lpFileNameA, ipFileName, MAX_PATH) > 0)
		{
			gPatchMgr->PostDlopen(lpFileNameA, handle);
		}
		//delete[] lpFileNameA;
	}
	return handle;
}

bool WINAPI AT_FreeLibrary(HMODULE iHandle)
{
	int rc = ::FreeLibrary(iHandle);

	// succeed
	if(rc != 0)
	{
		gPatchMgr->PostDlclose(iHandle);
	}
	return rc;
}

void WINAPI AT_FreeLibraryAndExitThread(HMODULE iHandle, DWORD dwExitCode)
{
	int rc = ::FreeLibrary(iHandle);

	// succeed
	if(rc != 0)
	{
		gPatchMgr->PostDlclose(iHandle);
	}
	
	// Exit thread
	::ExitThread(dwExitCode);
}

/*// CRT heap APIs to patch
static PatchSymbol gMallocWithDeadPageSyms[] =
{
	{"free",			(PROC)FreeBlockWithDeadPage},
	// operator delete
	{"??3@YAXPAX@Z",	(PROC)FreeBlockWithDeadPage},
	// operator delete[]
	{"??_V@YAXPAX@Z",	(PROC)FreeBlockWithDeadPage},

	{"malloc",			(PROC)MallocBlockWithDeadPage},
	{"calloc",			(PROC)CallocBlockWithDeadPage},
	{"realloc",			(PROC)ReallocBlockWithDeadPage},
	// operator new
	{"??2@YAPAXI@Z",	(PROC)NewBlockWithDeadPage},
	// operator new[]
	{"??_U@YAPAXI@Z",	(PROC)NewBlockWithDeadPage},

	{NULL,				NULL},
};

static PatchSymbol gMallocWithPaddingSyms[] =
{
	{"free",			(PROC)FreeBlockWithPadding},
	// operator delete
	{"??3@YAXPAX@Z",	(PROC)FreeBlockWithPadding},
	// operator delete[]
	{"??_V@YAXPAX@Z",	(PROC)FreeBlockWithPadding},

	{"malloc",			(PROC)MallocBlockWithPadding},
	{"calloc",			(PROC)CallocBlockWithPadding},
	{"realloc",			(PROC)ReallocBlockWithPadding},
	// operator new
	{"??2@YAPAXI@Z",	(PROC)NewBlockWithPadding},
	// operator new[]
	{"??_U@YAPAXI@Z",	(PROC)NewBlockWithPadding},

	{NULL,				NULL},
};

// Free routines only
static PatchSymbol gFreeWithDeadPageSyms[] =
{
	{"free",			(PROC)FreeBlockWithDeadPage},
	// operator delete
	{"??3@YAXPAX@Z",	(PROC)FreeBlockWithDeadPage},
	// operator delete[]
	{"??_V@YAXPAX@Z",	(PROC)FreeBlockWithDeadPage},
	
	{NULL,				NULL},
};

static PatchSymbol gFreeWithPaddingSyms[] =
{
	{"free",			(PROC)FreeBlockWithPadding},
	// operator delete
	{"??3@YAXPAX@Z",	(PROC)FreeBlockWithPadding},
	// operator delete[]
	{"??_V@YAXPAX@Z",	(PROC)FreeBlockWithPadding},
	
	{NULL,				NULL},
};*/

////////////////////////////////////////////////////////////////////////////
// Helper routines to iterate modules in core.
////////////////////////////////////////////////////////////////////////////

// Cached all modules in process
static HMODULE* gHModules = NULL;
static int gNumModulesInCore = 0;

#define DEFAULT_MODULE_BUFFER_ENTRY 1024
static HMODULE  gDefaultModuleHandleBuffer[DEFAULT_MODULE_BUFFER_ENTRY];
static unsigned int gHModuleBufferEntries = DEFAULT_MODULE_BUFFER_ENTRY;

static HMODULE* GetAllModuleHandles(bool ibRefresh=false)
{
	// First time here, point to the static buffer 
	if(gHModules == NULL)
	{
		gHModules = gDefaultModuleHandleBuffer;
	}
	else if(ibRefresh == false)
	{
		return gHModules;
	}

	DWORD lNeedBytes;
	// PSAPI to retrieve all module handles in core
	if( EnumProcessModules(GetCurrentProcess(), gHModules, gHModuleBufferEntries*sizeof(HMODULE), &lNeedBytes))
	{
		gNumModulesInCore = lNeedBytes / sizeof(HMODULE);
	}
	else if(gHModuleBufferEntries < lNeedBytes/sizeof(HMODULE) )
	{
		// Buffer is too small, this should be very rarely, more than 1024 modules
		// Double the buffer size and allocate from heap
		do
		{
			gHModuleBufferEntries <<= 1;
		}
		while(gHModuleBufferEntries < lNeedBytes / sizeof(HMODULE));

		gHModules = new HMODULE[gHModuleBufferEntries];
		// Try the call again.
		if( EnumProcessModules(GetCurrentProcess(), gHModules, gHModuleBufferEntries*sizeof(HMODULE), &lNeedBytes))
		{
			gNumModulesInCore = lNeedBytes / sizeof(HMODULE);
		}
		else
		{
			delete[] gHModules;
			gHModules = NULL;
		}
	}
	return gHModules;
}

// Provide a platform independant API to iterate all modules
void* GetFirstModuleHandle()
{
	HMODULE* lpAllModules = GetAllModuleHandles();
	return lpAllModules[0];
}

void* GetNextModuleHandle(void* ipModuleHandle)
{
	static int gPrevIndex = 0;
	// Most of the case, this function is called successively
	// Use this static var to speed up the search
	if(ipModuleHandle == gHModules[gPrevIndex])
	{
		if(++gPrevIndex < gNumModulesInCore)
		{
			return gHModules[gPrevIndex];
		}
	}

	for(int i=0; i<gNumModulesInCore; i++)
	{
		if(ipModuleHandle == gHModules[i] && i+1 < gNumModulesInCore)
		{
			gPrevIndex = i+1;
			return gHModules[i+1];
		}
	}
	return NULL;
}

const char* GetModuleName(void* ipModuleHandle)
{
	if(!ipModuleHandle)
	{
		return NULL;
	}

	static char lModuleName[MAX_PATH];
	if(::GetModuleBaseNameA(GetCurrentProcess(), 
							(HMODULE)ipModuleHandle, 
							lModuleName, sizeof(lModuleName)/sizeof(char) ) )
	{
		return lModuleName;
	}
	return NULL;
}

void RefreshLdinfoBuffer()
{
	GetAllModuleHandles(true);
}

const char* GetExecName()
{
	static char lModuleName[MAX_PATH];
	if(::GetModuleBaseName(GetCurrentProcess(), NULL, lModuleName, sizeof(lModuleName)/sizeof(char) ) )
	{
		return lModuleName;
	}
	return NULL;
}

// Helper function to iterate an array of names which is ended by NULL
// In order to dertermine if a particular name is on the list
// Return true if find it.
static bool IsThisNameOnList(const char* ipOneName, const char** ipAllNames)
{
	for(int i=0; ipAllNames[i]; i++)
	{
		if(::strcmpi(ipOneName, ipAllNames[i]) == 0)
		{
			return true;
		}
	}
	return false;
}

/*// Helper function to find out if the symbol is to be patched
// Return the matching structure, otherwise NULL
static PatchSymbol* IsSymbolOnList(const char* ipOneName, PatchSymbol* ipPatchSymbols)
{
	PatchSymbol* lResult = NULL;
	while(ipPatchSymbols && ipPatchSymbols->mName)
	{
		if(::strcmp(ipOneName, ipPatchSymbols->mName) == 0)
		{
			lResult = ipPatchSymbols;
			break;
		}
		ipPatchSymbols++;
	}
	return lResult;
}*/

////////////////////////////////////////////////////////////////////////////////
// This is the core of window patching funciton
//
// PE binary file format says.
//		Module has a list of imported DLLs (dependant DLL)
//		Each of these imported DLL has an IAT (Import Address Table)
//		Each table entry is initially a name (or an ordinal),
//		after binding, it is procedure addr
//
// Return true if symbol is found and patched.
////////////////////////////////////////////////////////////////////////////////
static bool PatchModule(HMODULE      ihModuleToPatch,
						const char*  ipSymbolName,
						PROC         ipNewFuncPtr,
						PROC*        opOldFuncPtr,
						//PatchSymbol* ipSymbolToPatch,
						const char** ipCandidateImportModules)
{
	bool rc = false;
	// The module handle is the loading_base, and it starts with a DOS header
	char* lModBase = (char*)ihModuleToPatch;
	IMAGE_DOS_HEADER* lpDOSHeader = (PIMAGE_DOS_HEADER)ihModuleToPatch;

	if(IsBadReadPtr(lpDOSHeader, sizeof(IMAGE_DOS_HEADER))
		|| IMAGE_DOS_SIGNATURE != lpDOSHeader->e_magic )
	{
		// Signature doesn't match, wonder why ??
		OUTPUT_MSG0(AT_FATAL, "DLL has wrong DOS signature\n");
		return false;
	}

	IMAGE_NT_HEADERS* lpNTHeader = (IMAGE_NT_HEADERS*)(lModBase + lpDOSHeader->e_lfanew);
	if(IsBadReadPtr(lpNTHeader, sizeof(IMAGE_NT_HEADERS))
		|| IMAGE_NT_SIGNATURE != lpNTHeader->Signature )
	{
		// Signature doesn't match, wonder why ??
		OUTPUT_MSG0(AT_FATAL, "DLL has wrong NT signature\n");
		return false;
	}

	if (0 == lpNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
    {
		// If there is no imports section, leave now.
		return false;
	}

	IMAGE_IMPORT_DESCRIPTOR* lpImportDesc = (IMAGE_IMPORT_DESCRIPTOR*)
		(lModBase + lpNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	MEMORY_BASIC_INFORMATION mbi;
	// Check all imported dll by name
	while(lpImportDesc->Name)
	{
		const char* lpImportDllName = lModBase + lpImportDesc->Name;
		if(IsThisNameOnList(lpImportDllName, ipCandidateImportModules))
		{
            // Find the imported DLL, 
			// Check what among its exported symbols are referenced and bound by the patched module
			// PE keeps two thunk table in parallel. They are the same when loaded in core at the beginning
			// rtlk binds the 2nd one which is the IAT
			IMAGE_THUNK_DATA* lpOrigThunk = (IMAGE_THUNK_DATA*)(lModBase + lpImportDesc->OriginalFirstThunk);
			IMAGE_THUNK_DATA* lpBoundThunk = (IMAGE_THUNK_DATA*)(lModBase + lpImportDesc->FirstThunk);
			// Check each entry in the thunk table
			while(lpOrigThunk->u1.Function)
			{
				// What if it is an ordinal ?
				if(IMAGE_ORDINAL_FLAG != (lpOrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG ))
				{
					// This entry is the RVA of symbol name
					IMAGE_IMPORT_BY_NAME* lpByName =
											(IMAGE_IMPORT_BY_NAME*)(lModBase + lpOrigThunk->u1.AddressOfData);
					// Skip NULL name
					if('\0' == lpByName->Name[0])
					{
						continue;
					}
					// Is this the function I want to patch.
					//PatchSymbol* lpPatchSym = IsSymbolOnList((const char*)lpByName->Name, ipSymbolToPatch);
					//if(lpPatchSym)
					if(ipSymbolName[0] == lpByName->Name[0] && 
						0 == ::strcmp(ipSymbolName, (const char*)lpByName->Name))
					{
						bool lbChangeProtect = false;
						// Because the IAT is in .text segment, we have to make it writable first
						::VirtualQuery(lpBoundThunk, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
						if( (mbi.Protect & (PAGE_EXECUTE_READWRITE | PAGE_READWRITE)) == 0)
						{
							if (0 == ::VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect))
							{
								// wonder why it fails
								OUTPUT_MSG0(AT_FATAL, "Failed to VirtualProtect\n");
								return false;
							}
							lbChangeProtect = true;
							//fprintf(stderr, "VirtualProtect base=0x%lx size=0x%lx, prot=%d, prev_prot=%d\n",
							//	mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, mbi.Protect);
						}
						// Patch the function 
						PROC * pTemp = (PROC*)&lpBoundThunk->u1.Function;
						//if(gPatchMgr->PatchDebug())
						//{
						//	OUTPUT_MSG5(AT_FATAL, "\t\tFound symbol [%s] at IAT[0x%lx] in module [0x%lx], replace old function [0x%lx] with new one [0x%lx]\n",
						//					ipSymbolName, pTemp, ihModuleToPatch, *pTemp, ipNewFuncPtr);
						//}
						if(opOldFuncPtr)
						{
							*opOldFuncPtr = (PROC) *pTemp;
						}
						*pTemp = ipNewFuncPtr;
						// Change page protect back to its original
						if(lbChangeProtect)
						{
							DWORD lOldProtect;
							if(0 == :: VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &lOldProtect) )
							{
								// wonder why it fails
								OUTPUT_MSG0(AT_FATAL, "Failed to VirtualProtect\n");
								return false;
							}
							//fprintf(stderr, "VirtualProtect base=0x%lx size=0x%lx, prot=%d, prev_prot=%d\n\n",
							//				mbi.BaseAddress, mbi.RegionSize, mbi.Protect, lOldProtect);
						}
						rc = true;
						break;
					}
				}
				lpOrigThunk++;
				lpBoundThunk++;
			}

		}
		lpImportDesc++ ;
	}
	return rc;
}

void PatchManager::PatchLoadLibraryW(HMODULE ihModule, const char* ipModuleName)
{
	// LoadLibrary routines
	static PatchSymbol gLoadLibrarySyms[] =
	{
		{"LoadLibrary",  (PROC)AT_LoadLibraryA},
		{"LoadLibraryA", (PROC)AT_LoadLibraryA},
		{"LoadLibraryW", (PROC)AT_LoadLibraryW},
		{"FreeLibrary",  (PROC)AT_FreeLibrary},
		{"FreeLibraryAndExitThread", (PROC)AT_FreeLibraryAndExitThread},
		{NULL, NULL}
		//"LoadLibraryExA",
		//"LoadLibraryExW",
	};

	static const char* lModulesExportLoadLibrarySymbols[] = 
	{
		"KERNEL32.dll",
		NULL
	};

	if(!IsThisNameOnList(ipModuleName, gIgnoreSystemDLLs))
	{
		if(PatchDebug())
		{
			OUTPUT_MSG1(AT_FATAL, "\tCheck Module [%s]\n", ipModuleName);
		}
		// Try to patch this module with LoadLibrary routines
		for(int i=0; gLoadLibrarySyms[i].mName; i++)
		{
			PROC lOldFunc;
			if(true == PatchModule(ihModule, gLoadLibrarySyms[i].mName, gLoadLibrarySyms[i].mNewProc, &lOldFunc, lModulesExportLoadLibrarySymbols))
			{
				if(PatchDebug())
				{
					OUTPUT_MSG2(AT_FATAL, "\t==> Patched [%s] in module [%s] <==\n", 
						gLoadLibrarySyms[i].mName, ipModuleName);
				}
			}
		}
	}
}

/*void PatchManager::PatchFreeW(HMODULE ihModule, const char* ipModuleName)
{
	static const char* lModulesExportFreeSymbols[] = 
	{
		"MSVCRT.dll",
		"MSVCRTD.dll",
		NULL
	};

	if(!IsThisNameOnList(ipModuleName, gIgnoreSystemDLLs))
	{
		if(PatchDebug())
		{
			OUTPUT_MSG1(AT_FATAL, "\tTry to patch Free/Delete routines in Module [%s]\n", ipModuleName);
		}
		// Try to patch this module with free routines
		if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_DEAD_PAGE)
		{
			PatchModule(ihModule, gFreeWithDeadPageSyms, lModulesExportFreeSymbols);
		}
		else if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_PADDING)
		{
			PatchModule(ihModule, gFreeWithPaddingSyms, lModulesExportFreeSymbols);
		}
		else if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_RECORD)
		{
		}
	}
}

void PatchManager::PatchMallocW(HMODULE ihModule, const char* ipModuleName)
{
	static const char* lModulesExportMallocSymbols[] = 
	{
		"MSVCRT.dll",
		"MSVCRTD.dll",
		NULL
	};

	if(!IsThisNameOnList(ipModuleName, gIgnoreSystemDLLs))
	{
		if(PatchDebug())
		{
			OUTPUT_MSG1(AT_FATAL, "\tTry to patch Malloc/New/Free/Delete routines in Module [%s]\n", ipModuleName);
		}
		// Try to patch this module with free routines
		if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_DEAD_PAGE)
		{
			PatchModule(ihModule, gMallocWithDeadPageSyms, lModulesExportMallocSymbols);
		}
		else if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_PADDING)
		{
			PatchModule(ihModule, gMallocWithPaddingSyms, lModulesExportMallocSymbols);
		}
		else if(mModuleConfiguration.mMode == ModuleConfig::AT_MODE_ADD_RECORD)
		{
		}
	}
}*/

// Return true if we want to skip this dll for any patching
bool PatchManager::SkipDllToPatchW(const char* ipModuleName)
{
	bool bSkipIt = false;
	if(IsThisNameOnList(ipModuleName, gIgnoreSystemDLLs))
	{
		bSkipIt = true;
	}
	return bSkipIt;
}

// Replace the function in the process
//
// Param: iModuleName, if not NULL, patch this module only
//        iFunctionName, the function to patch
//        iFuncPtr, new function
//        oOldFuncptr, the replaced function pointer
//
// Return: Patch_OK means success, otherwise fail.
Patch_RC PatchFunction(ModuleHandle iModuleHandle,
						const char* iModuleName,
						const char* iSymbolToFind,
						FunctionPtr iNewFuncPtr,
						FunctionPtr* opOldFuncPtr)
{
	static const char* gModulesExportMallocSymbols[] = 
	{
		"MSVCRT.dll",
		"MSVCRTD.dll",
		"msvcr80.dll",
		"msvcr80d.dll",
		LIBSH,
		NULL
	};

	// Ground old function pointer
	if(opOldFuncPtr)
	{
		*opOldFuncPtr = NULL;
	}

	if(false == PatchModule((HMODULE)iModuleHandle, iSymbolToFind, (PROC)iNewFuncPtr, (PROC*)opOldFuncPtr, gModulesExportMallocSymbols))
	{
		return Patch_Function_Name_Not_Found;
	}

	return Patch_OK;
}

/*
// Callback for StackWalk
static DWORD __stdcall GetModBase(HANDLE hProcess, DWORD dwAddr)
{
	static CSymbolEngine g_cSym;
    // Check inside the symbol engine first:
    IMAGEHLP_MODULE stIHM;
    stIHM.SizeOfStruct = sizeof( IMAGEHLP_MODULE );
    if( (dwAddr != 0xFFFFFFFF)  &&  g_cSym.SymGetModuleInfo( dwAddr, &stIHM ))
    {
        return( (DWORD) stIHM.BaseOfImage );
    }

    // Otherwise, query the virtual memory management:
    MEMORY_BASIC_INFORMATION stMBI ;
    if ( 0 != VirtualQueryEx( hProcess, (LPCVOID) dwAddr, &stMBI, sizeof(stMBI)) )
    {
        return( (DWORD) stMBI.AllocationBase );
    }
    return ( 0 ) ;
}

void DoStackWalk(DWORD  *pdwAddrBuffer,	// Destination address buffer
				 DWORD	dwLevels,		// Size of address buffer in entries
				 DWORD	dwNumSkip		// Number of stack levels to skip
				)
{
    HANDLE hProcess = GetCurrentProcess();

	// Is this necessary?
	// SmartCS lCS( &g_HookCS_ST.theCS );

    // The thread information
    CONTEXT stCtx;
    stCtx.ContextFlags = CONTEXT_FULL;

    if( GetThreadContext( GetCurrentThread(), &stCtx ))
    {
        STACKFRAME64 stFrame;
        DWORD dwMachine;

		ZeroMemory( &stFrame, sizeof(STACKFRAME) );

        stFrame.AddrPC.Mode = AddrModeFlat;

        dwMachine                = IMAGE_FILE_MACHINE_I386 ;
		stFrame.AddrPC.Offset    = stCtx.Eip    ;
        stFrame.AddrStack.Offset = stCtx.Esp    ;
        stFrame.AddrFrame.Offset = stCtx.Ebp    ;
        stFrame.AddrStack.Mode   = AddrModeFlat ;
        stFrame.AddrFrame.Mode   = AddrModeFlat ;

        //  Loop for dwLevels stack frames, after
		//  skipping the first dwNumSkip levels:
        for( DWORD i=0; i < dwNumSkip+dwLevels; i++ )
        {
            if( FALSE == StackWalk( dwMachine              ,
                                    hProcess               ,
                                    hProcess               ,
                                    &stFrame               ,
                                    &stCtx                 ,
                                    NULL                   ,
                                    SymFunctionTableAccess ,
                                    GetModBase             ,
                                    NULL                    )
			  )
            {
                break;
            }
            if( i >= dwNumSkip )
            {
                // Also check that the address is not zero.
                // Sometimes StackWalk returns TRUE with a frame of zero.
                if( 0 != stFrame.AddrPC.Offset )
                {
					*pdwAddrBuffer = stFrame.AddrPC.Offset;
					pdwAddrBuffer++;
                }
            }
        }
    }
} // DoStackWalk
*/
typedef void* address_t;
void GetBackTrace(address_t *pdwAddrBuffer,		// Destination address buffer
					int	dwLevels,       // Size of address buffer in entries
					int	dwNumSkip)      // Number of stack levels to skip
{
	// Get the first stack pointer from EBP
    address_t* lnESP = NULL;
#ifdef WIN64
	// !FIX!
	return;
#elif defined(WIN32)
    __asm
   {
		mov     lnESP, ebp
	}
#endif

	const address_t* lBeginningOfCallStack = lnESP;
    try
    {
        for(int i=0; i < dwNumSkip+dwLevels; ++i )
        {
			// jmuraira:040902 - Enhancement to reduce the number of bad pointer AVs:
            // (These are caught anyway, but they are a hindrance during tests in
            //  which "stop always" for AV exceptions is enabled in the debugger.)
            // The new stack frame should be in the vecinity of the first stack frame
			// from the first stack frame, to the first stack frame + 1meg / sizeof(ptr)
            if( lnESP &&
				(lnESP < lBeginningOfCallStack + 0x40000) &&
				(lnESP >= lBeginningOfCallStack) )
            {
                if( i >= dwNumSkip )
                {
                    // The return address is the value of the DWORD
                    // immediately after ESP
                    *pdwAddrBuffer = (address_t) *(lnESP+1);
                    ++pdwAddrBuffer;
                }

                // Advance the stack pointer. The next stack frame is
                // where ESP points to
                lnESP = (address_t*) (*lnESP);
            }
            else
            {
                return;
            }
        }
    }
    catch(...)
    {
        return;
    }

}

void PrintFuncName(FILE* ipFilePtr, address_t iInstrAddress)
{
	if(!ipFilePtr || !iInstrAddress)
	{
		return;
	}

	//if(lpFunctionNames != NULL)
	//{
	//	::fprintf(ipFilePtr, "0x%lx %s\n", iInstrAddress, lpFunctionNames[0]);
	//}
	//else
	//{
		::fprintf(ipFilePtr, "0x%lx unknown\n", iInstrAddress);
	//}
}

