/************************************************************************
** FILE NAME..... PatchElf_HP-UX_IA64.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Patch symbols in 64bit elf executable or shared lib.
**
** NOTES............  
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS..... HP-UX IA64 only
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
#include <elf.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "PatchSym.h"
#include "PatchManager.h"


static load_module_desc* gpModuleDescs = NULL;
static int gTotalModules = 0;
static int gBufferSz = 32;

static load_module_desc* GetModuleDesc()
{
	if(gpModuleDescs)
	{
		return gpModuleDescs;
	}
	else
	{
		bool bDone = false;
		gpModuleDescs = (load_module_desc*)malloc(sizeof(load_module_desc)*gBufferSz);
		for(int i=0; i<gBufferSz; i++)
		{
			void* handle = ::dlget(i, &gpModuleDescs[i], sizeof(load_module_desc));
			if(handle)
			{
				gTotalModules++;
			}
			else
			{
				// We get to the last module index ?
				bDone = true;
				break;
			}
		}
		// We get to this point because there are not enough buffer to fit all the loaded modules
		if(!bDone)
		{
			::free(gpModuleDescs);
			gpModuleDescs = NULL;
			gTotalModules = 0;
			gBufferSz *= 2;
			return GetModuleDesc();
		}
	}
	return gpModuleDescs;
}

// find symbol in DT_SYMTAB
static Patch_RC FindSymbol(struct load_module_desc* ipModuleDescs,
							const char* iModuleName,
							const char* iSymbolToFind,
							Elf64_Addr* oSymbolAddr)
{
	Patch_RC rc = Patch_Function_Name_Not_Found;
	
	if(!ipModuleDescs || !iModuleName || !iSymbolToFind || !oSymbolAddr)
	{
		return Patch_Invalid_Params;
	}

	*oSymbolAddr = NULL;
	// Hopefuly 512 bytes is enough to hold the module name
	// We need a private copy because the dlgetname return a pointer to an internal static string
	char lpModuleToFind[512];

	// HP-UX: All virtual address in the data structure is the link-time virtual addr
	//        We have to adjust for run-time loading base by ourself. 
	// !!Ugh!!
	Elf64_Phdr* phdr = NULL;
	Elf64_Addr  lModuleRunTimeBase = 0;
	Elf64_Addr  lModuleLinkTimeBase = 0;
	Elf64_Addr  lModuleRunTimeDataBase = 0;

	::strncpy(lpModuleToFind, iModuleName, 512);
	for(int i=0; i<gTotalModules; i++)
	{
		char* lpModuleName = ::dlgetname(&ipModuleDescs[i],	sizeof(load_module_desc),
										NULL, 0, NULL);
		//printf("see module[%d] name is [%s]\n", i, lpModuleName);
		if(::strstr(lpModuleName, lpModuleToFind) )
		{
			lModuleRunTimeBase = ipModuleDescs[i].text_base;
			phdr = (Elf64_Phdr *)ipModuleDescs[i].phdr_base;
			lModuleRunTimeDataBase = ipModuleDescs[i].data_base;
			break;
		}
	}

	if(!phdr)
	{
		return Patch_Module_Not_Found;
	}
	lModuleLinkTimeBase = phdr->p_vaddr - phdr->p_offset;

	if(gPatchMgr->PatchDebug())
	{
		OUTPUT_MSG2(AT_FATAL, "\tTry to patch [%s] in Module [%s]\n",iSymbolToFind, iModuleName);
	}
	// Go through the program header array to find .dynamic setion
	while (phdr->p_type != PT_DYNAMIC) {
		phdr++;
	}

	if(!phdr)
	{
		OUTPUT_MSG1(AT_FATAL, "Failed to .dynmic segment in module [%s]\n", lpModuleToFind);
		return rc;
	}
	// Which is also global symbol _DYNAMIC. (unfortunately we can only get exec's)
	Elf64_Dyn* dyn = (Elf64_Dyn*)(lModuleRunTimeBase + (phdr->p_vaddr - lModuleLinkTimeBase) );

	// Search locations of DT_SYMTAB and DT_STRTAB, etc.
	Elf64_Sym* symtab  = NULL;
	const char* strtab = NULL;
	void* jmprel       = NULL;

	Elf64_Xword pltrelsz = 0;
	Elf64_Sword reltype = -1;
	Elf64_Xword relentsz = 0;
	Elf64_Xword relaentsz = 0;

	while (dyn->d_tag) {
		switch (dyn->d_tag) {
		case DT_STRTAB:
			strtab = (const char*)(lModuleRunTimeBase + (dyn->d_un.d_ptr - lModuleLinkTimeBase) );
			break;
		case DT_SYMTAB:
			symtab = (Elf64_Sym*)(lModuleRunTimeBase + (dyn->d_un.d_ptr - lModuleLinkTimeBase) );
			break;
		case DT_JMPREL:
			// This points to the plt relocation entries only.
			jmprel = (void*)(lModuleRunTimeBase + (dyn->d_un.d_ptr - lModuleLinkTimeBase) );
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
			// relocation type is rela
			Elf64_Rel* rel = (Elf64_Rel*)jmprel;
			for(i=0; i<pltrelsz/relentsz; i++, rel=(Elf64_Rel* )((char*)rel+relentsz))
			{
				symindex = ELF64_R_SYM(rel->r_info);
				sym = symtab + symindex;
				// get symbol name from the string table
				const char* str = strtab + sym->st_name;

				//if(gPatchMgr->PatchDebug())
				//{
				//	OUTPUT_MSG1(AT_FATAL, "\t\tSee symbol [%s]\n", str);
				//}

				// compare it with our symbol
				if(::strcmp(str, iSymbolToFind) == 0)
				{
					// HP-UX IA64 always start data seg @0x6000000000000000
					// Adjust to run time data seg address
					Elf64_Addr lOffset = rel->r_offset & 0xffffffff;
					*oSymbolAddr = lModuleRunTimeDataBase + lOffset;
					rc = Patch_OK;
					break;
				}
			}
		}
		else
		{
			// relocation type is rela
			Elf64_Rela* rela = (Elf64_Rela*)jmprel;
			for(i=0; i<pltrelsz/relaentsz; i++, rela=(Elf64_Rela* )((char*)rela+relaentsz))
			{
				symindex = ELF64_R_SYM(rela->r_info);
				sym = symtab + symindex;
				// get symbol name from the string table
				const char* str = strtab + sym->st_name;

				//if(gPatchMgr->PatchDebug())
				//{
				//	OUTPUT_MSG(AT_FATAL, "\t\tSee symbol [%s]\n", str);
				//}

				// compare it with our symbol
				if(::strcmp(str, iSymbolToFind) == 0)
				{
					Elf64_Sword lRelocType = ELF64_R_TYPE(rela->r_info);
					// HP-UX IA64 always start data seg @0x6000000000000000
					// Adjust to run time data seg address
					Elf64_Addr lOffset = rela->r_offset & 0xffffffff;
					*oSymbolAddr = lModuleRunTimeDataBase + lOffset;
					//printf("Module=%s Symbol=%s Reloca Type=0x%x, rela->r_offset=0x%lx\n", lpModuleToFind, iSymbolToFind, lRelocType, rela->r_offset);
					rc = Patch_OK;
					break;
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
	//*oOldFuncPtr = NULL;

	// Link map is not implemented by HP !!Ugh!!
	// We use module descritor to get all loaded module
	// Be aware to update it when necessary becuase struct load_module_desc
	// is a private copy I made
	load_module_desc* lpModuleDesc = GetModuleDesc();
	if(!lpModuleDesc)
		return Patch_Unknown_Platform;

	Elf64_Addr lSymbolAddr = 0;
	Patch_RC rc = FindSymbol(lpModuleDesc, iModuleName, iFunctionName, &lSymbolAddr);
	if(rc != Patch_OK)
	{
		return rc;
	}
	else
	{
		// Stitch in the replacement function

		// Don't know how to retrieve the old function pointer yet
		//*oOldFuncPtr = (FunctionPtr)lSymbolAddr;

		// IA64 use function descriptor structure to do function call
		// The struct has two part: (1)address of function (2)its gp
		// A function pointer is a pointer to the function descriptor for the function
		((FunctionPtr*)lSymbolAddr)[0] = ((FunctionPtr*)iFuncPtr)[0];
		((FunctionPtr*)lSymbolAddr)[1] = ((FunctionPtr*)iFuncPtr)[1];
	}

	return Patch_OK;
}

//////////////////////////////////////////////////////////////
// Provide a platform independant API to iterate all modules
//////////////////////////////////////////////////////////////
void* GetFirstModuleHandle()
{
	return GetModuleDesc();
}

void* GetNextModuleHandle(void* ipModuleHandle)
{
	for(int i=0; i<gTotalModules; i++)
	{
		if(&gpModuleDescs[i]==ipModuleHandle && i+1<gTotalModules)
		{
			return &gpModuleDescs[i+1];
		}
	}
	return NULL;
}

const char* GetModuleName(void* ipModuleHandle)
{
	if(ipModuleHandle)
	{
		return ::dlgetname((load_module_desc*)ipModuleHandle, sizeof(load_module_desc),	NULL, 0, NULL);
	}
	return NULL;
}

void RefreshLdinfoBuffer()
{
	// Free the buffer which contains staled module description
	if(gpModuleDescs)
	{
		::free(gpModuleDescs);
	}
	gpModuleDescs = NULL;
	gTotalModules = 0;
	GetModuleDesc();
}
