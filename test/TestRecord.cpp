
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#endif

#include <stdio.h>
/////////////////////////////////////////////////////////////////////
// Platform dependent types/api
/////////////////////////////////////////////////////////////////////
#ifdef WIN32
typedef HANDLE		threadID_t;
#else
typedef pthread_t	threadID_t;
#endif

#if defined(_AIX) || defined(WIN32)
#include "AccuTrak.h"
int gDummy = gAccuTrakDummy;
#endif

typedef void* (*MallocFuncPtr)(size_t);
struct ARGS
{
	MallocFuncPtr malloc_func;
	void*         block;
};

// Thread startup func
#ifdef WIN32
DWORD WINAPI threadCycle(LPVOID arg)
#else
static void *threadCycle(void *arg)
#endif
{
	// Use the malloc in libfoo to allocate a block
	// But free it in this module.
	ARGS* args = (ARGS*) arg;
	MallocFuncPtr lpMalloc = args->malloc_func;
	void* lpData = lpMalloc(12);
	::free(lpData);

	args->block = ::malloc(7);

	return 0;
}

#define NUM_THREADS 2
// Main starts here
int main(int argc, char* argv[])
{
	int rc=0;
	int i, numThreads = NUM_THREADS;
	threadID_t ptids[NUM_THREADS];

	// Test malloc/free here. Should be patched.
	void* p = ::malloc(9);
	p = ::realloc(p, 10);
	::free(p);

	// Dynamically load-in a shared module
#ifdef _AIX
	const char* libname = "libfoo.a";
#elif defined(WIN32)
	const char* libname = "foo.dll";
#else
	const char* libname = "libfoo.so";
#endif

#ifdef WIN32
	HMODULE lHandle = ::LoadLibrary(libname);
#else
	void* lHandle = ::dlopen(libname, RTLD_LAZY);
#endif
	if(!lHandle)
	{
		fprintf(stderr, "Failed to dlopen library [%s]\n", libname);
#ifdef WIN32
		fprintf(stderr, "rc=%d\n", ::GetLastError());
#else
		fprintf(stderr, "%s\n", ::dlerror());
#endif
		exit(-1);
	}

#ifdef WIN32
	MallocFuncPtr AllocMem = (MallocFuncPtr)::GetProcAddress(reinterpret_cast<HINSTANCE>(lHandle),
												"AllocMem");
#else
	MallocFuncPtr AllocMem = (MallocFuncPtr)::dlsym(lHandle, "AllocMem");
#endif
	if(!AllocMem)
	{
		fprintf(stderr, "Failed to dlsym [AllocMem] in library [%s]\n", libname);
		exit(-1);
	}

	ARGS args[NUM_THREADS];
	// Spawn threads to do memory ops
	for(i=0; i<numThreads; i++)
	{
		args[i].malloc_func = AllocMem;
		args[i].block       = (void*)0xdeadbeef;
		// create threads
#ifdef WIN32
		ptids[i] = ::CreateThread(0, 0,
									LPTHREAD_START_ROUTINE(threadCycle),
									(LPVOID)&args[i], 0, NULL);
		if(ptids[i] == NULL)
#else
		if(rc = pthread_create(&ptids[i], NULL, threadCycle, (void*)&args[i]))
#endif
		{
			fprintf(stderr, "Can't create thread %d\n", i);
			exit(rc);
		}
	}
	// join the threads
	for(i=0; i<numThreads; i++)
	{

#ifdef WIN32
		DWORD lpBlockAllocatedByOtherThread;
		const DWORD lResult = ::WaitForSingleObject(ptids[i], INFINITE);
		if(lResult != 0)
		{
			rc = ::GetLastError();
			fprintf(stderr, "Error join thread %d, tid[%ld], LastError=%d\n", i, ptids[i], rc);
			exit(0);
		}
		else
		{
			GetExitCodeThread(ptids[i], &lpBlockAllocatedByOtherThread);
		}
#else
		void* lpBlockAllocatedByOtherThread;
		if(rc=pthread_join(ptids[i], &lpBlockAllocatedByOtherThread))
		{
			fprintf(stderr, "Error join thread %d, id %ld, rc=%d\n", i, ptids[i], rc);
			exit(rc);
		}
#endif

		::free(args[i].block);
	}

	char* cp = new char[32];
	delete[] cp;

#ifdef WIN32
	::FreeLibrary(lHandle);
#endif

	return rc;
}

