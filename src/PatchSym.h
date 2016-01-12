
#ifndef _PATCH_SYM_H
#define _PATCH_SYM_H
/************************************************************************
** FILE NAME..... PatchSym.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION.........
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
typedef void* FunctionPtr;
typedef void* ModuleHandle;

enum Patch_RC
{
	Patch_OK,
	Patch_Unknown_Platform,
	Patch_Invalid_Params,
	Patch_Module_Not_Found,
	Patch_Function_Name_Not_Found
};

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
						FunctionPtr* oOldFuncPtr);

// Provide a platform independant API to iterate all modules
ModuleHandle GetFirstModuleHandle();
ModuleHandle GetNextModuleHandle(ModuleHandle ipModuleHandle);
const char* GetModuleName(ModuleHandle ipModuleHandle);

/*// Give a function address, find the module that contains this func
const char* GetModuleNameByFuncAddr(FunctionPtr iFuncPtr);*/

// These two keep link map in transit buffer
// Others have static list
#if defined(_AIX) || defined(__hpux) || defined(WIN32)
void RefreshLdinfoBuffer();
#endif

#if defined(WIN32)
const char* GetExecName();
#endif

#endif //_PATCH_SYM_H
