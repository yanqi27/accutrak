#include <ucontext.h>
#include <sys/frame.h>
#include <sys/procfs_isa.h>
#include <dlfcn.h>
#include <stdio.h>
#include <demangle.h>

#include "BackTrace.h"

#if defined(sparc) || defined(__sparc)
#define FRAME_PTR_REGISTER REG_SP
#endif

struct frame * cs_getprgregframeptr(prgregset_t * ptr);
struct frame * cs_getmyframeptr();


 /*
 * set of routines to walk calling thread's call stack...
 * returns a pointer to the current stack pointer in the caller of this
 * routine
 */

// This is the offset for 64bit sparc. It is 0 for 32bit.
static const long BIAS = 2047;

struct frame * cs_getmyframeptr()
{
        ucontext_t u;
        struct frame * sp;

    (void) getcontext(&u);

    sp = (struct frame *) u.uc_mcontext.gregs[FRAME_PTR_REGISTER];

    return(((frame *)((char*)sp+BIAS))->fr_savfp);
}

void GetBackTrace(address_t* ipStackBuffer, int iTotalDepth, int iSkipLevel)
{

    long          savpc = 0;
    struct frame *  savfp;
    struct frame *  fp;
    struct frame buff;

    int index =0;
    int nPos =0;

    fp = cs_getmyframeptr();

    while (fp &&
       	nPos++<(iSkipLevel +iTotalDepth)
          )
    {
		savpc = ((frame *)((char *)fp+BIAS))->fr_savpc;
        if(index < iTotalDepth && nPos>iSkipLevel)
		{
			ipStackBuffer[index++] = (address_t)savpc;
		}
		fp = ((frame *)((char *)fp+BIAS))->fr_savfp;
    }
	// seal the array
	if(index < iTotalDepth)
	{
		ipStackBuffer[index] = 0;
	}
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
	
	Dl_info info;

	if(dladdr((void *)iAddr, &info)!= 0)
 	{
 		if(info.dli_fname && info.dli_sname) 
		{
			char sUnMangledName[4096];
			int nError =  cplus_demangle(info.dli_sname,sUnMangledName,4096);
			if(!nError)
			{
				if(iPrintAddr)
					fprintf(fp, "0x%lx: %s:%s - offset:0x%x\n", iAddr, info.dli_fname, sUnMangledName, (unsigned long)iAddr-(unsigned long)info.dli_saddr);
				else
					fprintf(fp," %s:%s - offset:0x%x\n", info.dli_fname, sUnMangledName, (unsigned long)iAddr-(unsigned long)info.dli_saddr);
			}
			else 
			{
				if(iPrintAddr)
					fprintf(fp,"0x%lx: %s:%s - offset:0x%x\n", iAddr, info.dli_fname, info.dli_sname, (unsigned long)iAddr-(unsigned long)info.dli_saddr);
				else
					fprintf(fp," %s:%s - offset:0x%x\n", info.dli_fname, info.dli_sname, (unsigned long)iAddr-(unsigned long)info.dli_saddr);
			}
		}
	}
	else
		return false;

	return true;
}

void PrintCallStack()
{
    long          savpc = 0;
    struct frame *  savfp;
    struct frame *  fp;
    struct frame buff;

    int index =0;
    int nPos =0;

    fp = cs_getmyframeptr();

	//printf("fp = 0x%x, savpc = 0x%x, savfp = 0x%x\n", fp,((frame *)((char *)fp+BIAS))->fr_savpc, ((frame *)((char *)fp+BIAS))->fr_savfp);

    while (fp &&
       	nPos++<MAX_STACK_DEPTH
          )
    {
		savpc = ((frame *)((char *)fp+BIAS))->fr_savpc;
		//printf("fp= 0x%x, savpc= 0x%x\n",fp,savpc);
 		Dl_info info;
		if(dladdr((void *)savpc, &info)!= 0)
 		{
 			if(info.dli_fname && info.dli_sname) {
				char sUnMangledName[256];
				if(!cplus_demangle(info.dli_sname,sUnMangledName,256))
 					printf(" %s:%s - offset:0x%x \n", info.dli_fname,sUnMangledName,(unsigned long)savpc-(unsigned long)info.dli_saddr);
				else
	 				printf(" %s:%s - offset:0x%x \n", info.dli_fname,info.dli_sname,(unsigned long)savpc-(unsigned long)info.dli_saddr);
			}
		}
		fp = ((frame *)((char *)fp+BIAS))->fr_savfp;
    }
}
*/
