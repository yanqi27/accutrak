
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
	const int n = 8;

	int i;
	void* pa[128];

	// allocate a set of blocks
	printf("\tAllocate 8 small blocks...\n");
	for(i=0; i<n; i++)
	{
		size_t sz = i*8+1;
		pa[i] = malloc(sz);
		printf("\t\tAllocate %ld byte at 0x%lx\n", sz, pa[i]);
	}

	// Free them all
	printf("\tFree them all ...\n");
	for(i=0; i<8; i++)
	{
		free(pa[i]);
	}

	// Free them all
	printf("\tFree them twice ...\n");
	for(i=0; i<8; i++)
	{
		free(pa[i]);
	}

	return rc;
}
