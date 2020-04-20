
#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#define EXPORT _declspec(dllexport)
#else
#define EXPORT
#endif

#ifdef WIN32
	const char* libname = "foo.dll";
#elif defined(_AIX)
	const char* libname = "libfoo.a";
#else
	const char* libname = "libfoo.so";
#endif

extern "C" EXPORT void* AllocMem(size_t iSize)
{
	void* p2 = ::malloc(3);
	::free(p2);

	void* p = ::malloc(iSize);

	if(p)
	{
		printf("Allocated %d bytes at 0x%lx in module %s\n", iSize, p, libname);
	}
	return p;
}
