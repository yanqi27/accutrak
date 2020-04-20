
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

	char* lpChar = new char[2];

	// Test malloc
	const int sz = 3;
	char* lStr = (char*)malloc(sz);
	printf("Allocated %d bytes at 0x%lx\n", sz, lStr);
	free(lStr);
	fprintf(stdout, "Freed ok.\n");

	delete[] lpChar;

	return rc;
}

