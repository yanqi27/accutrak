
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
	const int n = 32;

	int i;
	void** pa = new void*[n];
	// allocate a set a blocks with wide range of sizes
	printf("Configuration file set size range to [4096, 10240]\n");
	for(i=0; i<n; i++)
	{
		size_t sz = i*512;
		pa[i] = malloc(sz);
		printf("Allocate %ld byte at 0x%lx\n", sz, pa[i]);
	}

	// Free them all
	printf("Free them all ...");
	for(i=0; i<n; i++)
	{
		free(pa[i]);
	}
	printf("OK\n");

	return rc;
}

