
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>

extern "C"	void* AllocMem(size_t);

int main(int argc, char* argv[])
{
	int rc=0;

	void* lpData = ::malloc(15);
	if(lpData)
	{
		printf("Allocated 15 bytes at 0x%lx in module %s\n", lpData, argv[0]);
		::free(lpData);
	}

	
	lpData = AllocMem(12);
	free(lpData);

	return rc;
}

