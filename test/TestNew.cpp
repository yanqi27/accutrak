
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

class MyStruct
{
public:
	MyStruct() : mNumber(7) {}
	~MyStruct() { mNumber=-1; }
private:
	int mNumber;
};

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

	void* lpTmp = malloc(3);

	// Test patch of new/new[]/delete/delete[]
	MyStruct* lpMyStruct = new MyStruct;
	printf("Allocated struct MyStruct at [0x%lx]\n", lpMyStruct);
	delete lpMyStruct;

	int* lNewIntVector = new int[4];
	printf("Allocated int[4] at [0x%lx]\n", lNewIntVector);
	delete[] lNewIntVector;

	// Test overrun of block allocated by operator new
	const int sz = 3;
	char* lStr = new char[sz];
	printf("testing access. Allocated %d bytes at [0x%lx]\n", sz, lStr);
	// Test access permission in user space
	// as well as overrun/underrun
	for(int i=-2; i<sz+8; i++)
	{
		printf("Access %dth byte at 0x%lx\n", i, &lStr[i]);
		// comiler optimizer can be aggressive. The following line maybe opted out
		// if there is no side effect.
		rc |= lStr[i];
	}
	delete[] lStr;


	return rc;
}

