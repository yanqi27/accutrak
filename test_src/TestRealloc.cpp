
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#if defined(_AIX) || defined(WIN32)
#include "AccuTrak.h"
int gDummy = gAccuTrakDummy;
#endif

int main(int argc, char* argv[])
{
	int rc=0;

	void* lpData;
	void* lpOldData;

	// This time realloc shall go in my allocator
	lpOldData = lpData = ::malloc(sizeof(long));
	*(long*)lpData = 0x12345678;
	lpData = ::realloc(lpData, sizeof(long)*2);

	printf("Allocated %d bytes and realloc to %d bytes\n", sizeof(long), sizeof(long)*2);
	printf("Old data is 0x12345678 @ 0x%lx and New data is 0x%lx @0x%lx\n", 
			lpOldData, *(long*)lpData, lpData);
	::free(lpData);

	return rc;
}

