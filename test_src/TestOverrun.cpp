
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <stdlib.h>
#endif

#include <stdio.h>

#if defined(_AIX) || defined(WIN32)
#include "AccuTrak.h"
int gDummy = gAccuTrakDummy;
#endif

int main(int argc, char* argv[])
{
	int rc=0;


#ifdef WIN32
	// Get system page size, 4K on Win32, 8K on Win64/IA64
	SYSTEM_INFO lSysInfo;
	GetSystemInfo(&lSysInfo); 
	size_t lPageSize = lSysInfo.dwPageSize;
#else
	size_t lPageSize = ::sysconf(_SC_PAGE_SIZE);
#endif
	// A close to page size block will trigger out of page header
	void* ltmp = malloc(lPageSize-5);
	printf("testing block size close to page size [size=0x%lx] [addr=0x%lx] ok\n", 
				lPageSize-5, ltmp);
	free(ltmp);

	ltmp = malloc(lPageSize+5);
	printf("testing regular block size [size=0x%lx] [addr=0x%lx] ok\n", 
				lPageSize+5, ltmp);
	free(ltmp);

	// Test malloc
	const int sz = 3;
	char* lStr = (char*)malloc(sz);
	printf("testing access. Allocated %d bytes at [0x%lx]\n", sz, lStr);
	// Test access permission in user space
	// as well as overrun/underrun
	for(int i=-2; i<sz+7; i++)
	{
		printf("Access %dth byte at 0x%lx\n", i, &lStr[i]);
		// comiler optimizer can be aggressive. The following line maybe opted out
		// if there is no side effect.
		rc += lStr[i];
	}
	free(lStr);

	return rc;
}

