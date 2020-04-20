
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
	const int n = 100;

	int i;
	void* pa[128];

	// allocate a set of blocks
	printf("\tConfiguration file set max_cached_blocks to 64\n");
	printf("\tAllocate 64 small blocks...\n");
	for(i=0; i<n; i++)
	{
		size_t sz = i*8;
		pa[i] = malloc(sz);
		printf("\t\tAllocate %ld byte at 0x%lx\n", sz, pa[i]);
	}

	// Free them all
	printf("\tFree them all ...\n");
	for(i=0; i<n; i++)
	{
		free(pa[i]);
	}

	// allocate one block which should reuse the first freed pa[0]
	char* p = new char[(100-64)*8];
	printf("\tAllocate one small block at 0x%lx, it should be 0x%lx\n", p, pa[100-64]);

	delete[] p;
	rc = p[0];

	printf("\tRead freed block at 0x%lx...\n", pa[n-1]);

	rc = *(int*)(pa[n-1]);
	return rc;
}

