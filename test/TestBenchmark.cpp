
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#endif

#include <stdio.h>
#include <vector>

/////////////////////////////////////////////////////////////////////
// Platform dependent types/api
/////////////////////////////////////////////////////////////////////
#ifdef WIN32
typedef HANDLE		threadID_t;
#else
typedef pthread_t	threadID_t;
#endif

#if defined(_AIX) || defined(WIN32)
#include "AccuTrak.h"
int gDummy = gAccuTrakDummy;
#endif

const int gTotalThread = 4;
const size_t gBlockSz = 512;
const size_t gCachedBlocks = 4096;
const size_t gTotalAllocs = 1024 * 8;

// Thread startup func
#ifdef WIN32
DWORD WINAPI threadCycle(LPVOID arg)
#else
static void *threadCycle(void *arg)
#endif
{
	// Simply allocate memory and cache them in a container
	// release the old one if the cache is full
	std::vector<char *> lpBlocks(gCachedBlocks);
	size_t allocCnt = 0;

	while (allocCnt < gTotalAllocs) {
		size_t index = allocCnt % gCachedBlocks;
		if (lpBlocks[index])
			delete [] lpBlocks[index];
		lpBlocks[index] = new char[gBlockSz];
		allocCnt++;
	}
	// Don't bother to release the blocks in cache

	return 0;
}

// Main starts here
int main(int argc, char* argv[])
{
	int rc=0;
	int i, numThreads = gTotalThread;
	threadID_t ptids[gTotalThread];

	// Test malloc/free here. Should be patched.
	void* p = ::malloc(9);
	p = ::realloc(p, 10);
	::free(p);

	// Spawn threads to do memory ops
	for(i=0; i<numThreads; i++) {
		// create threads
#ifdef WIN32
		ptids[i] = ::CreateThread(0, 0,
									LPTHREAD_START_ROUTINE(threadCycle),
									(LPVOID)&args[i], 0, NULL);
		if(ptids[i] == NULL)
#else
		if(rc = pthread_create(&ptids[i], NULL, threadCycle, NULL))
#endif
		{
			fprintf(stderr, "Can't create thread %d\n", i);
			exit(rc);
		}
	}
	// join the threads
	for(i=0; i<numThreads; i++) {

#ifdef WIN32
		DWORD lpBlockAllocatedByOtherThread;
		const DWORD lResult = ::WaitForSingleObject(ptids[i], INFINITE);
		if(lResult != 0) {
			rc = ::GetLastError();
			fprintf(stderr, "Error join thread %d, tid[%ld], LastError=%d\n", i, ptids[i], rc);
			exit(0);
		}
		else {
			GetExitCodeThread(ptids[i], &lpBlockAllocatedByOtherThread);
		}
#else
		if(rc=pthread_join(ptids[i], NULL)) {
			fprintf(stderr, "Error join thread %d, id %ld, rc=%d\n", i, ptids[i], rc);
			exit(rc);
		}
#endif
	}

	return rc;
}

