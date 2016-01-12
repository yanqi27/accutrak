/************************************************************************
** FILE NAME..... PatchElf64.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Patch symbols in 64bit elf executable or shared lib.
**
** NOTES............  
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS.....  Linux x86_64 & Solaris sparcv9 only
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <elf.h>
#include <link.h>

#include "PatchSym.h"
#include "PatchManager.h"

#if defined(linux)

// locate link-map of my process
struct link_map *GetLinkMapList()
{
	static link_map *gLinkMap = NULL;

	if(gLinkMap)
	{
		return gLinkMap;
	}

	// Use the existing _DYNAMIC seems to be more simple and reliable
	Elf64_Dyn* dyn = _DYNAMIC;
	while (dyn->d_tag != DT_PLTGOT) {
		dyn++;
	}

	// The GOT section holds the address of GOT
	Elf64_Xword got = (Elf64_Xword) dyn->d_un.d_ptr;
	// second GOT entry holds the linking map
	got += sizeof(got);
	// now just read first link_map item and return it
	gLinkMap = *(struct link_map**)got;
	// On Linux, if LD_PRELOAD is set, above pointer is not the head of link list
	// It actually points to the preloaded lib.
	// The real head(the exec file) is before it.
	while(gLinkMap->l_prev)
	{
		gLinkMap = gLinkMap->l_prev;
	}
	return gLinkMap;
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
						FunctionPtr iFuncPtr,
						FunctionPtr* oOldFuncPtr)
{
	*oOldFuncPtr = NULL;
	// Link map is a linked list, each node represents one loaded library
	// It has a pointer to the base address of the shared object, its name,
	// and pointer to its dynamic section.
	link_map *map = GetLinkMapList();
	if(!map)
	{
		return Patch_Unknown_Platform;
	}

	Patch_RC rc = Patch_Function_Name_Not_Found;

	if(!iModuleName || !iSymbolToFind)
	{
		return Patch_Invalid_Params;
	}

	// Find the specified module
	while(map && !::strstr(map->l_name, iModuleName))
	{
		map = map->l_next;
	}
	// Module name doesn't match
	if(!map)
	{
		return Patch_Module_Not_Found;
	}

	if(gPatchMgr->PatchDebug())
	{
		if(strlen(map->l_name)==0 && gPatchMgr->GetProgramName())
		{
			OUTPUT_MSG2(AT_FATAL, "\tTry to patch [%s] in Module [%s]\n",iSymbolToFind, gPatchMgr->GetProgramName())
		}
		else
		{
			OUTPUT_MSG2(AT_FATAL, "\tTry to patch [%s] in Module [%s]\n",iSymbolToFind, map->l_name)
		}
	}
	// Search locations of DT_SYMTAB and DT_STRTAB
	// Also save the nchains from hash table.
	void* symtab = NULL;
	void* strtab = NULL;
	void* jmprel = NULL;
	Elf64_Xword pltrelsz = 0;
	Elf64_Sword reltype = -1;
	Elf64_Xword relentsz = 0;
	Elf64_Xword relaentsz = 0;

	Elf64_Dyn *dyn = (Elf64_Dyn *)map->l_ld;
	while (dyn->d_tag) 
	{
		switch (dyn->d_tag) 
		{
		case DT_STRTAB:
			strtab = (void*)dyn->d_un.d_ptr;
			break;
		case DT_SYMTAB:
			symtab = (void*)dyn->d_un.d_ptr;
			break;
		case DT_JMPREL:
			// This points to the plt relocation entries only.
			jmprel = (void*)dyn->d_un.d_ptr;
			break;
		case DT_PLTRELSZ:
			pltrelsz = dyn->d_un.d_val;
			break;
		case DT_PLTREL:
			// this entry shows if PLT uses REL or RELA of relocation table type
			reltype = dyn->d_un.d_val;
			break;
		case DT_RELENT:
			relentsz = dyn->d_un.d_val;
			break;
		case DT_RELAENT:
			relaentsz = dyn->d_un.d_val;
			break;
		default:
			break;
		}
		dyn++;
	}

	// Search relocation section for the function
	if(jmprel && pltrelsz && (reltype==DT_REL || reltype==DT_RELA)
		&& (relentsz || relaentsz) && symtab && strtab)
	{
		int i;
		int symindex;
		Elf64_Sym* sym;

		if(reltype==DT_REL)
		{
			// relocation type is rel
			// 
			// i386 uses this reloc
			Elf64_Rel* rel = (Elf64_Rel*)jmprel;
			for(i=0; i<pltrelsz/relentsz; i++, rel=(Elf64_Rel* )((char*)rel+relentsz))
			{
				symindex = ELF64_R_SYM(rel->r_info);
				sym = (Elf64_Sym*)symtab + symindex;
				// get symbol name from the string table
				const char* str = (const char*)strtab + sym->st_name;
				// compare it with our symbol
				if(::strcmp(str, iSymbolToFind) == 0)
				{
					Elf64_Addr lSymbolAddr = map->l_addr + rel->r_offset;
					// retrieve the old function pointer
					// BE AWARE, this could be the pointer to dynamic linker
					// if lazy binding is effective and the function is never called before this point
					*oOldFuncPtr = *(FunctionPtr*)lSymbolAddr;
					// Stitch in the replacement function
					*(FunctionPtr*)lSymbolAddr = iFuncPtr;
					rc = Patch_OK;
					break;
				}
			}
		}
		else
		{
			// relocation type is rela
			//
			// AMD64/x86_64 uses this reloc only.
			// For function hook ups, the following shall hold:
			// ELF64_R_TYPE(rela->r_info) == R_X86_64_JUMP_SLOT
			// rela->r_addend==0 because R_X86_64_JUMP_SLOT reloc entry doesn't use addend.
			Elf64_Rela* rela = (Elf64_Rela*)jmprel;
			for(i=0; i<pltrelsz/relaentsz; i++, rela=(Elf64_Rela* )((char*)rela+relaentsz))
			{
				symindex = ELF64_R_SYM(rela->r_info);
				sym = (Elf64_Sym*)symtab + symindex;
				// get symbol name from the string table
				const char* str = (const char*)strtab + sym->st_name;
				// compare it with our symbol
				if(::strcmp(str, iSymbolToFind) == 0)
				{
					// r_offset has the offset (relative to module loading base)
					// to the GOT entry which contains address of function pointer
					Elf64_Addr lSymbolAddr = map->l_addr + rela->r_offset;
					// retrieve the old function pointer
					// BE AWARE, this could be the pointer to dynamic linker
					// if lazy binding is effective and the function is never called before this point
					*oOldFuncPtr = *(FunctionPtr*)lSymbolAddr;
					// Stitch in the replacement function
					*(FunctionPtr*)lSymbolAddr = iFuncPtr;
					rc = Patch_OK;
					break;
				}
			}
		}
	}
	else
	{
		// It maybe normal for some DSOs which have no text segment at all
		//OUTPUT_MSG0(AT_INFO, "Failed to find all necessary sections in .dynmic segment\n");
	}

	return rc;
}

#elif defined(sun)

#include <sys/elf_SPARC.h>

#define	M64_PLT_ENTSIZE		32	// plt entry size in bytes

#include "sparcv9_plt.cpp"

extern Elf64_Dyn _DYNAMIC[];

// locate link-map of my process
struct link_map *GetLinkMapList()
{
	static link_map *gLinkMap = NULL;

	if(gLinkMap)
	{
		return gLinkMap;
	}

	// Use the exported symbol _DYNAMIC to get dynamic section
	Elf64_Dyn* dyn = _DYNAMIC;
	while (dyn->d_tag != DT_PLTGOT) {
		dyn++;
	}

	// The V9 ABI assigns the link map to the start of .PLT2.
	Elf64_Xword plt = (Elf64_Xword) dyn->d_un.d_ptr;
	plt += M64_PLT_ENTSIZE*2;
	gLinkMap = *(struct link_map**)plt;
	return gLinkMap;
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
						FunctionPtr iFuncPtr,
						FunctionPtr* oOldFuncPtr)
{
	*oOldFuncPtr = NULL;
	// Link map is a linked list, each node represents one loaded library
	// It has a pointer to the base address of the shared object, its name,
	// and pointer to its dynamic section.
	link_map *map = GetLinkMapList();
	if(!map)
	{
		return Patch_Unknown_Platform;
	}

	Patch_RC rc = Patch_Function_Name_Not_Found;
	unsigned long lObjectBase = 0;

	if(!map || !iModuleName || !iSymbolToFind)
	{
		return Patch_Invalid_Params;
	}

	// Find the specified module
	while(map && !::strstr(map->l_name, iModuleName))
	{
		map = map->l_next;
		if(map)
		{
			// This address is needed for shared library to calulate following sections
			// Executable module doesn't need this though map->l_addr is non-zero. 
			lObjectBase = map->l_addr;
		}
	}
	// Module name doesn't match
	if(!map)
	{
		return Patch_Module_Not_Found;
	}

	if(gPatchMgr->PatchDebug())
	{
		OUTPUT_MSG2(AT_FATAL, "\tTry to patch [%s] in Module [%s]\n",iSymbolToFind, map->l_name);
	}

	// Search locations of DT_SYMTAB and DT_STRTAB
	// Also save the nchains from hash table.
	void* symtab = NULL;
	void* strtab = NULL;
	void* jmprel = NULL;
	void* rela   = NULL;
	void* pltpad = NULL;
	Elf64_Xword pltrelsz = 0;
	Elf64_Sword reltype = -1;
	Elf64_Xword relasz = 0;
	Elf64_Xword relaentsz = 0;

	Elf64_Dyn *dyn = (Elf64_Dyn *)map->l_ld;
	while (dyn->d_tag) 
	{
		switch (dyn->d_tag) 
		{
		case DT_STRTAB:
			// string table
			strtab = (void*)(lObjectBase + dyn->d_un.d_ptr);
			break;
		case DT_SYMTAB:
			// symbol table
			symtab = (void*)(lObjectBase + dyn->d_un.d_ptr);
			break;
		case DT_JMPREL:
			// Relocation entries for plt only.
			jmprel = (void*)(lObjectBase + dyn->d_un.d_ptr);
			break;
		case DT_RELA:
			// Relocation entries
			// Solaris, these entries includes entries pointed by DT_JMPREL
			// On other ELF implementation, they don't
			rela = (void*)(lObjectBase + dyn->d_un.d_ptr);
			break;
		case DT_PLTPAD:
			// This is for sparcv9 only
			// The pltpad is array of plt entry just like regular plt
			// But the code refer the pltpad is determined at run-time, relocation time
			// It is like pltpad is used as scratch pad.
			// why this ??
			pltpad = (void*)(lObjectBase + dyn->d_un.d_ptr);
			break;
		case DT_RELASZ:
			relasz = dyn->d_un.d_val;
			break;
		case DT_PLTRELSZ:
			pltrelsz = dyn->d_un.d_val;
			break;
		case DT_PLTREL:
			// this entry shows if PLT uses REL or RELA of relocation table type
			reltype = dyn->d_un.d_val;
			break;
		case DT_RELAENT:
			relaentsz = dyn->d_un.d_val;
			break;
		default:
			break;
		}
		dyn++;
	}

	// Search relocation section for the function
	if(rela && jmprel && pltrelsz && reltype==DT_RELA
		&& relaentsz && symtab && strtab)
	{
		int i;
		int symindex;
		Elf64_Sym* sym;

		if(reltype==DT_REL)
		{
			// relocation type is rel
			OUTPUT_MSG0(AT_FATAL, "sparc doesn't use REL relocation type\n");
		}
		else
		{
			int lWarningNum = 0;
			// relocation type is rela
			//Elf64_Rela* pltrela = (Elf64_Rela*)jmprel;
			//for(i=0; i<pltrelsz/relaentsz; i++, pltrela=(Elf64_Rela* )((char*)pltrela+relaentsz))
			Elf64_Rela* lpRelocationEntry = (Elf64_Rela*)rela;
			for(i=0; i<relasz/relaentsz; i++, lpRelocationEntry=(Elf64_Rela* )((char*)lpRelocationEntry+relaentsz))
			{
				Elf64_Sword rtype = ELF64_R_TYPE(lpRelocationEntry->r_info);
				if(rtype == R_SPARC_JMP_SLOT || rtype == R_SPARC_WDISP30)
				{
					symindex = ELF64_R_SYM(lpRelocationEntry->r_info);
					sym = (Elf64_Sym*)symtab + symindex;
					// get symbol name from the string table
					const char* str = (const char*)strtab + sym->st_name;
					// compare it with our symbol
					if(::strcmp(str, iSymbolToFind) == 0)
					{
						if(rtype == R_SPARC_JMP_SLOT)
						{
							// r_offset has the offset (relative to module loading base)
							// to the PLT entry which contains instructions to jump to target function
							// For sparc, unlike x86_64, PLT is in data segment and private to process
							Elf64_Addr lSymbolAddr = lObjectBase + lpRelocationEntry->r_offset;
							InterceptFunciton(lSymbolAddr, iFuncPtr, oOldFuncPtr);
							//DEBUG
							if(gPatchMgr->PatchDebug())
							{
								OUTPUT_MSG5(AT_FATAL, "\t\tFound symbol [%s] at plt[0x%lx] in module [%s], replace old function [0x%lx] with new one [0x%lx]\n",
											iSymbolToFind, lSymbolAddr, iModuleName, *oOldFuncPtr, iFuncPtr);
							}
							rc = Patch_OK;
							//break;
						}
						// Only consider this for sparcv9, which I still don't understand the design yet.
						else if(rtype == R_SPARC_WDISP30 && pltpad)
						{
							// The symbol address is in code .text section. there maybe multi-instances of this
							// which calls into old function ?? not pltpad ??
							Elf64_Addr lSymbolAddr = lObjectBase + lpRelocationEntry->r_offset;
							if(lWarningNum == 0)
							{
								lWarningNum++;
								OUTPUT_MSG3(AT_FATAL, "Symbol [%s] at [0x%lx] in module [%s] is in .text segment, can't be patched\n",
										iSymbolToFind, lSymbolAddr, iModuleName);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		// It maybe normal for some DSOs which have no text segment at all
		//OUTPUT_MSG1(AT_INFO, "Failed to find all necessary sections in .dynmic segment in module [%s]\n", iModuleName);
	}

	return rc;
}

#else
#error unknown platform
#endif

/////////////////////////////////////////////////////////////
// Provide a platform independant API to iterate all modules
/////////////////////////////////////////////////////////////
void* GetFirstModuleHandle()
{
	return GetLinkMapList();
}

void* GetNextModuleHandle(void* ipModuleHandle)
{
	link_map *lpLinkMap = (link_map *)ipModuleHandle;
	return lpLinkMap ? lpLinkMap->l_next : NULL;
}

const char* GetModuleName(void* ipModuleHandle)
{
	link_map *lpLinkMap = (link_map *)ipModuleHandle;
	return lpLinkMap ? lpLinkMap->l_name : NULL;
}

void* GetModuleHandleByName(const char* iModuleName)
{
	link_map *lpLinkMap = GetLinkMapList();
	// Find the specified module
	while(lpLinkMap && !::strstr(lpLinkMap->l_name, iModuleName))
	{
		lpLinkMap = lpLinkMap->l_next;
	}

	return lpLinkMap;
}

/*
// Give a function address, find the module that contains this func
const char* GetModuleNameByFuncAddr(FunctionPtr iFuncPtr)
{
	link_map *lpLinkMap = GetLinkMapList();
	// Find the specified module
	while(lpLinkMap)
	{
		if(iFuncPtr>lpLinkMap->l_addr && iFuncPtr<lpLinkMap->l_addr+)
		{
			return lpLinkMap->l_name;
		}
		lpLinkMap = lpLinkMap->l_next;
	}
	return NULL;
}*/
