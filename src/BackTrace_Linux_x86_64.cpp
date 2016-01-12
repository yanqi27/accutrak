
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <execinfo.h>
#include <pthread.h>

#include "BackTrace.h"

// Lock to serialize backtrace
//static pthread_mutex_t gBTLock = PTHREAD_MUTEX_INITIALIZER;

// The following function is implemented in libiberty.a which header file is not in /usr/include
//extern "C" char *cplus_demangle(const char *mangled, int options);
//#define DMGL_PARAMS	 (1 << 0)	// Include function args

//bool GetModuleAndFuncName(void* dwAddr, char* &pModuleName,char* &pFuncName);

//bool _bCalledByBackTrace = false;

void GetBackTrace(address_t* ipStackBuffer, int iTotalDepth, int iSkipLevel)
{
	int max_stack_depth = iSkipLevel +iTotalDepth;
	void* lTrace[MAX_STACK_DEPTH];

	// Too many level of call stacks
	if(max_stack_depth > MAX_STACK_DEPTH-1)
	{
		max_stack_depth = MAX_STACK_DEPTH-1;
	}

	// Is it serialize by upper level callers?
	//::pthread_mutex_lock(&gBTLock);
	//_bCalledByBackTrace = true;

	int lTraceSize = ::backtrace(lTrace, max_stack_depth);

 	//_bCalledByBackTrace = false;
	//::pthread_mutex_unlock(&gBTLock);

	int index = 0;
	for(; index<iTotalDepth && (index+iSkipLevel)<max_stack_depth; index++)
	{
		ipStackBuffer[index]= lTrace[index+iSkipLevel];
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

	char** lpFunctionNames = ::backtrace_symbols(&iInstrAddress, 1);

	if(lpFunctionNames != NULL)
	{
		::fprintf(ipFilePtr, "0x%lx %s\n", iInstrAddress, lpFunctionNames[0]);
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

	char * funcName =NULL;
	char * moduleName = NULL;
	if(GetModuleAndFuncName((void*)iAddr, moduleName, funcName))
	{
		if(funcName && moduleName)
		{
			if(iPrintAddr)
				fprintf(fp,"0x%lx: %s:%s\n", iAddr, moduleName, funcName);
			else
				fprintf(fp," %s:%s\n", moduleName, funcName);
		}
		else
		{
			if(iPrintAddr)
				fprintf(fp,"0x%lx: %s:%s\n", iAddr, "UnknownModule", "UnKnownFunc");
			else
				fprintf(fp," %s:%s\n", "UnknownModule", "UnKnownFunc");
		}
	}
	else
		return false;

	return true;
}

// printCallStack could trigger printCallStack->::backtrace_symbols->malloc->printCallStack stack overflow
void PrintCallStack()
{
	const int max_stack_depth = 128;
	void* lTrace[max_stack_depth];

// 1/15/2005 yuwen, add a global variable to toggle system malloc of li_malloc because
// backtrace in linux will call malloc hence falls in a recursion to malloc call.
	//::pthread_mutex_lock(&gBTLock);
 	//_bCalledByBackTrace = true;
	int lTraceSize = ::backtrace(lTrace, max_stack_depth);
 	//_bCalledByBackTrace = false;
	//::pthread_mutex_unlock(&gBTLock);

 	if(lTraceSize > 0){

		//::pthread_mutex_lock(&gBTLock);
		//_bCalledByBackTrace = true;
 		char** lpFunctionNames = ::backtrace_symbols(lTrace, lTraceSize);
	 	//_bCalledByBackTrace = false;
		//::pthread_mutex_unlock(&gBTLock);

		if(lpFunctionNames != NULL){
			printf(" lpFunctionNames = 0x%x",lpFunctionNames);
 			for(int i=2; i<lTraceSize; i++)
			{
				if(lpFunctionNames[i])
					printf("Func: %s\n", lpFunctionNames[i]);
				else
					printf("NULL function name\n");
			}
		}
		else
			printf("NULL function trace\n");
	}
	else
	{
		printf("TraceSize not correct: %d\n",lTraceSize);
	}
}


bool GetModuleAndFuncName(void* dwAddr, char* &pModuleName,char* &pFuncName)
{
 	void *tmp[32];
	tmp[0] = (void *)dwAddr;
	char** lpStackFrames = ::backtrace_symbols(tmp, 1);

//	_ASSERT(lpStackFrames[0] != NULL);
	// Parse each stack frame for Module/Function/Address
	// Buffer looks like this: "./libStackWalker.so(_Z11DoStackWalkv+0x1c) [0x2a95671744]"
	char* lCursor = lpStackFrames[0];

	char* lpFunctionName = NULL;
	char* lpDemangledFuncName = NULL;

	if(::strchr(lCursor, '(') && ::strchr(lCursor, '+') && ::strchr(lCursor, '[') && ::strchr(lCursor, ']'))
	{
		// First comes module name
		pModuleName = lCursor;
		while(*lCursor != '(')
		{
			lCursor++;
		}
		*lCursor = '\0';
		// Second is function name
		lpFunctionName = ++lCursor;
		while(*lCursor != '+')
		{
			lCursor++;
		}
		*lCursor = '\0';
		// Third is address in hex
		while(*lCursor != '[')
		{
			lCursor++;
		}
		*lCursor = '\0';
		char* lpAddrStr = ++lCursor;
		while(*lCursor != ']')
		{
			lCursor++;
		}
		*lCursor = '\0';
		void* lpAddr = (void*)::strtoul(lpAddrStr, (char**)NULL, 16);

		#ifdef __64BIT__
			lpDemangledFuncName = cplus_demangle(lpFunctionName, DMGL_PARAMS);
			if(lpDemangledFuncName == NULL)
			{
				lpDemangledFuncName = lpFunctionName;
			}
		#else
			char* lpDemangledFuncName = lpFunctionName;
		#endif
			pFuncName = lpDemangledFuncName;

		return true;
	}

	return false;
}
*/
