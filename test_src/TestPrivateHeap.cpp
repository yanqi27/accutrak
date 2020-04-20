
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>

#ifdef _AIX
#include "AccuTrak.h"
int gDummy = gAccuTrakDummy;
#endif

#include "smrtheap.h"

extern "C"	void* AllocMem(size_t);

int main(int argc, char* argv[])
{
	int rc=0;

	MemRegisterTask();

	// This shall be the default heap
	void* lpData = ::malloc(15);
	if(lpData)
	{
		printf("Allocated 15 bytes at 0x%lx\n", lpData);
		::free(lpData);
	}

	// This one is supposed to be the private heap block
	lpData = AllocMem(12);
	free(lpData);

	return rc;
}

