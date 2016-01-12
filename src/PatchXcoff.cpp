/************************************************************************
** FILE NAME..... PatchXcoff.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Patch symbols in xcoff executable or shared library.
**
** NOTES............  
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS.....  Xcoff is used by AIX only
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
#ifdef __64BIT__
#define __XCOFF64__
#else
#define __XCOFF32__
#endif

#include <stdlib.h>
#include <stdio.h>
#include <xcoff.h>
#include <sys/ldr.h>
#include <errno.h>

#include "PatchSym.h"
#include "PatchManager.h"

typedef void* address_t;

// cached ld_info buffer
static ld_info* lpBuffer = NULL;

static ld_info* GetLdInfoList(bool ibRefresh=false)
{
	// Heuristic size for the buffer
	size_t lBufferSz = (sizeof(struct ld_info) + 64) * 32;

	// Use cached info or create new buffer
	if(lpBuffer && !ibRefresh)
	{
		return lpBuffer;
	}
	// Get the buffer from heap
	else
	{
		if(lpBuffer)
		{
			::free(lpBuffer);
		}

		if(! (lpBuffer = (struct ld_info *)malloc(lBufferSz) ) )
		{
			OUTPUT_MSG0(AT_FATAL, "Failed to get buffer for LD_INFO\n");
			return NULL;
		}
	}

	do
	{
		errno = 0;
		// Returns a list of all object files loaded for the current process
		if(-1 == ::loadquery(L_GETINFO, lpBuffer, lBufferSz) )
		{
			if (errno == ENOMEM)
			{
				// buffer size too small
				lBufferSz *= 2;
				::free(lpBuffer);
				lpBuffer = (struct ld_info *)malloc(lBufferSz);
			}
			else
			{
				OUTPUT_MSG0(AT_FATAL, "Failed to retrieve LD_INFO\n");
				return NULL;
			}
		}
		else
		{
			break;
		}
	} while (1);

	return lpBuffer;
}

/* find symbol in TOC */
static Patch_RC FindSymbol(struct ld_info* ipLdInfo,
							const char* iModuleName,
							FunctionPtr iOldFuncionPtr,
							FunctionPtr iFuncPtr)
{
	Patch_RC prc = Patch_Function_Name_Not_Found;

	if(!ipLdInfo || !iModuleName || !iOldFuncionPtr)
	{
		return Patch_Invalid_Params;
	}

	bool bFound = false;
	do
	{
		// Find the module by name
		// For Xcoff, there maybe multiple ldinfo that have the same module name, like archive libC.a
		if( ::strstr(ipLdInfo->ldinfo_filename, iModuleName) )
		{
			bFound = true;

			if(gPatchMgr->PatchDebug())
			{
				OUTPUT_MSG2(AT_FATAL, "\tTry to patch symbol with old value [0x%lx] in Module [%s]\n", iOldFuncionPtr, iModuleName);
			}

			// We just found the ld_info that matches module name
			FILHDR*  lpFileHeader = (FILHDR *) ipLdInfo->ldinfo_textorg;
			AOUTHDR* lpOptHeader = (AOUTHDR *) ((char *)lpFileHeader + sizeof(FILHDR));
			void** lpTOC;
			int    lTOCSz;

			if(lpFileHeader->f_opthdr)
			{
				if(!lpOptHeader->o_sntoc)
				{
					if(gPatchMgr->PatchDebug())
					{
						OUTPUT_MSG1(AT_FATAL, "\tNo TOC in Module [%s]\n", iModuleName);
					}
					// there is no TOC in this module
					continue;
				}

				lpTOC = (void **)((unsigned long)ipLdInfo->ldinfo_dataorg
						+ lpOptHeader->o_toc - lpOptHeader->data_start);

				// TOC is located at the end of the data section
				lTOCSz = lpOptHeader->dsize - (lpOptHeader->o_toc - lpOptHeader->data_start);
				int NumEntries = lTOCSz / sizeof(void *);
				// traverse TOC entries
				void** lptoc = lpTOC;
				// search TOC for fn descriptor of given symbol
				while(NumEntries--)
				{
					//if(gPatchMgr->PatchDebug())
					//{
					//	OUTPUT_MSG1(AT_FATAL, "\t\tSee symbol with value [0x%lx]\n", *lptoc);
					//}

					// Compare the value of symbol
					// This value is a pointer to a structure somewhat like (beware when read in dbx)
					//		struct FunctionDescriptor
					//		{
					//			address_t mFunctionAddress;
					//			?? (module's TOC ?)
					//		}
					if(*lptoc == iOldFuncionPtr)
					{
						if(gPatchMgr->PatchDebug())
						{
							OUTPUT_MSG2(AT_FATAL, "\t\tSee symbol with same value at [0x%lx], replace it with [0x%lx]\n", lptoc, iFuncPtr);
						}
						*(FunctionPtr*)lptoc = iFuncPtr;
						prc = Patch_OK;
						break;
					}
					lptoc++;
				}
			}
		}
	}
	while(ipLdInfo->ldinfo_next
			&& (ipLdInfo = (struct ld_info *)((char *)ipLdInfo + ipLdInfo->ldinfo_next)));

	if(!bFound)
	{
		return Patch_Module_Not_Found;
	}

	return prc;
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
						const char* iFunctionName,
						FunctionPtr iFuncPtr,
						FunctionPtr* oOldFuncPtr)
{
	// LD_INFO is a list of modules, each node represents one loaded library
	ld_info *lpLdInfo = GetLdInfoList();
	if(!lpLdInfo)
	{
		return Patch_Unknown_Platform;
	}

	Patch_RC prc = FindSymbol(lpLdInfo, iModuleName, *oOldFuncPtr, iFuncPtr);

	return prc;
}

// Provide a platform independant API to iterate all modules
void* GetFirstModuleHandle()
{
	return GetLdInfoList();
}

void* GetNextModuleHandle(void* ipModuleHandle)
{
	ld_info *lpLdInfo = (ld_info *)ipModuleHandle;
	if(lpLdInfo && lpLdInfo->ldinfo_next>0)
	{
		return (char*)lpLdInfo+lpLdInfo->ldinfo_next;
	}
	return NULL;
}

const char* GetModuleName(void* ipModuleHandle)
{
	ld_info *lpLdInfo = (ld_info *)ipModuleHandle;
	return (lpLdInfo ? lpLdInfo->ldinfo_filename : NULL);
}

void RefreshLdinfoBuffer()
{
	GetLdInfoList(true);
}
