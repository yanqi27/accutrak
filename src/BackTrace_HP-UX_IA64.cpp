#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <acxx_demangle.h>
#include <dlfcn.h>
#include <unwind.h>

extern "C" {
#include <uwx.h>
#include <uwx_self.h>
}

#include "BackTrace.h"

//extern int hk_printf(const char *fmt, ...);

// We put these two structures here so that they are initialized once
// for each thread and can be used throughout the whole life span of it.
// This avoids init overhead in each DoStackWalk() call.
__thread static struct uwx_env *env;
__thread static struct uwx_self_info *info;

// FIXME: There should be a destructor for this function to free env and info
void GetBackTrace(address_t* ipStackBuffer, int iTotalDepth, int iSkipLevel)
{
	uint64_t ip;
	int status;
	int index = 0;
	int nPos = 0;

	if (!env) {
		env = uwx_init();
		info = uwx_self_init_info(env);
		status = uwx_register_callbacks(env, (intptr_t)info, uwx_self_copyin,
				uwx_self_lookupip);
		if (status != UWX_OK) {
			env = NULL;
			return ;
		}
	}

	status = uwx_self_init_context(env);
	if (status != UWX_OK) {
		//hk_printf("uwx_self_init_context() error\n");
		return ;
	}

	for(;;) {
		if (index >= iTotalDepth)
			break;

		if (nPos++ >= iSkipLevel + iTotalDepth)
			break;

		status = uwx_get_reg(env, UWX_REG_IP, &ip);
		if (status != UWX_OK)
			break;

		if (nPos > iSkipLevel)
			ipStackBuffer[index++] = (address_t)ip;

		status = uwx_step(env);
		if (status != UWX_OK)
			break;
	}
	// seal the array
	if(index < iTotalDepth)
	{
		ipStackBuffer[index] = 0;
	}
	return ;
}

void PrintFuncName(FILE* ipFilePtr, address_t iInstrAddress)
{
	if(!ipFilePtr || !iInstrAddress)
	{
		return;
	}

	Dl_info dlInfo;
	if(::dladdr(iInstrAddress, &dlInfo) )
	{
		::fprintf(ipFilePtr, "0x%lx %s:%s\n", iInstrAddress, dlInfo.dli_fname, dlInfo.dli_sname);
	}
	else
	{
		::fprintf(ipFilePtr, "0x%lx unknown\n", iInstrAddress);
	}
}

/*
bool DumpStackFrame(FILE* fp, address_t iAddr, bool iPrintAddr)
{
	// protection
	if(!fp)
		return false;

	Dl_info dlInfo;
	char buf[4096];
	
	if(dladdr((void *)iAddr, &dlInfo) == 0)
		return false;

	if(dlInfo.dli_fname && dlInfo.dli_sname)
	{
		char sUnMangledName[2048];
		memset(sUnMangledName, 0, 2048);
		size_t lSize = 2048;
		int lStatus;
		__cxa_demangle(dlInfo.dli_sname, sUnMangledName, &lSize, &lStatus); 

		// Note: On IA64 dlInfo.dli_saddr is the address of the function descriptor
		// The real function address is the address it points to
		if (lStatus == 0) // means ok
		{
			if(iPrintAddr)
				snprintf(buf, 4096, "0x%lx: %s:%s - offset:0x%lx\n", iAddr, dlInfo.dli_fname, sUnMangledName, (unsigned long)iAddr - (unsigned long)((uint64_t *)dlInfo.dli_saddr)[0]);
			else
				snprintf(buf, 4096, " %s:%s - offset:0x%lx\n", dlInfo.dli_fname, sUnMangledName, (unsigned long)iAddr - (unsigned long)((uint64_t *)dlInfo.dli_saddr)[0]);
		}
		else 
		{
			if(iPrintAddr)
				snprintf(buf, 4096, "0x%lx: %s:%s\n", iAddr, dlInfo.dli_fname, dlInfo.dli_sname);
			else
				snprintf(buf, 4096, " %s:%s\n", dlInfo.dli_fname, dlInfo.dli_sname);
		}
		fprintf(fp, buf);
	}
	return true;
}

void PrintCallStack()
{
	uint64_t ip;
	int status;
	int nPos = 0;

	if (!env) {
		env = uwx_init();
		info = uwx_self_init_info(env);
		status = uwx_register_callbacks(env, (intptr_t)info, uwx_self_copyin,
				uwx_self_lookupip);
		if (status != UWX_OK) {
			env = NULL;
			return;
		}
	}

	status = uwx_self_init_context(env);
	if (status != UWX_OK) {
		hk_printf("uwx_self_init_context() error\n");
		return;
	}

	for(;;) {
		Dl_info dlInfo;

		if (nPos++ >= MAX_STACK_DEPTH)
			break;

		status = uwx_get_reg(env, UWX_REG_IP, &ip);
		if (status != UWX_OK)
			break;

		if (dladdr((void *)ip, &dlInfo) != 0) {
			if (dlInfo.dli_fname && dlInfo.dli_sname) {
				char sUnMangledName[1024];
				size_t lSize = 1024;
				int lStatus;
				__cxa_demangle(dlInfo.dli_sname, sUnMangledName, &lSize, &lStatus);
				if (lStatus == 0)
					hk_printf(" %s:%s - offset:0x%lx \n", dlInfo.dli_fname, sUnMangledName,(unsigned long)ip - (unsigned long)((uint64_t *)dlInfo.dli_saddr)[0]);
				else
					hk_printf(" %s:%s - offset:0x%lx \n", dlInfo.dli_fname, dlInfo.dli_sname,(unsigned long)ip - (unsigned long)((uint64_t *)dlInfo.dli_saddr)[0]);

			}
		}

		status = uwx_step(env);
		if (status != UWX_OK)
			break;
	}
}
*/
