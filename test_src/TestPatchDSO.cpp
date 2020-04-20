
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#if defined(_AIX) || defined(WIN32)
#include "AccuTrak.h"
int gDummy = gAccuTrakDummy;
#endif

typedef void* (*MallocFuncPtr)(size_t);


int main(int argc, char* argv[])
{
	int rc=0;

	void* p = ::malloc(9);
	p = ::malloc(8);


	// Test dlopened lib
#ifdef WIN32
	const char* libname = "foo.dll";
#elif defined(_AIX)
	const char* libname = "libfoo.a";
#else
	const char* libname = "libfoo.so";
#endif

#ifdef WIN32
	HMODULE lHandle = ::LoadLibrary(libname);
	if(!lHandle)
	{
		fprintf(stderr, "Failed to dynamically load library [%s]\n", libname);
		fprintf(stderr, "LastError=%d\n", ::GetLastError());
		exit(0);
	}
	MallocFuncPtr AllocMem = (MallocFuncPtr)
		::GetProcAddress(reinterpret_cast<HINSTANCE>(lHandle), "AllocMem");
	if(!AllocMem)
	{
		fprintf(stderr, "Failed to dlsym [AllocMem] in library [%s]\n", libname);
		fprintf(stderr, "LastError=%d\n", ::GetLastError());
		exit(0);
	}
#else
	void* lHandle = ::dlopen(libname, RTLD_LAZY);
	if(!lHandle)
	{
		fprintf(stderr, "Failed to dynamically load library [%s]\n", libname);
		fprintf(stderr, "%s\n", ::dlerror());
		exit(0);
	}
	MallocFuncPtr AllocMem = (MallocFuncPtr)::dlsym(lHandle, "AllocMem");
	if(!AllocMem)
	{
		fprintf(stderr, "Failed to dlsym [AllocMem] in library [%s]\n", libname);
		exit(0);
	}
#endif

	void* lpData = AllocMem(12);
	::free(lpData);

	return rc;
}

