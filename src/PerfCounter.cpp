/************************************************************************
** FILE NAME..... PerfCounter.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Perfromance counter
**
** NOTES............  
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS..... 
**
** LIMITATIONS...... 
** 
** DEVIATIONS....... 
** 
** RETURN VALUES.... 0  - successful
**                   !0 - error
** 
** AUTHOR(S)........ Michael Q Yan
** 
** CHANGES:
** 
************************************************************************/
#ifdef WIN32
#include <windows.h>
#include <pdh.h>
#include <pdhMsg.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "PerfCounter.h"

#ifdef sun

#include <procfs.h>

// Read psinfo from /proc file system
bool ReadProcPsinfo(const char* iFileName, void* ipBuffer, unsigned int iBufferSize)
{
	int lFileDescriptor = ::open(iFileName, O_RDONLY);
	if(lFileDescriptor == -1)
	{
		return false;
	}
	int lBytes = ::read(lFileDescriptor, ipBuffer, iBufferSize);
	if(lBytes == -1)
	{
		::close(lFileDescriptor);
		return false;
	}
	::close(lFileDescriptor);
	return true;
}

bool GetProcessMemory(size_t* opVSS, size_t* opRSS)
{
	// construct the file name of psinfo under /proc file system
	static char lPsinfoFileName[64];
	static bool gOnce = true;

	if(gOnce)
	{
		sprintf(lPsinfoFileName, "/proc/%d/psinfo", ::getpid());
		gOnce = false;
	}

	::psinfo_t lPsinfoBuffer;
	if(ReadProcPsinfo(lPsinfoFileName, &lPsinfoBuffer, sizeof(::psinfo_t)) )
	{
		*opVSS = lPsinfoBuffer.pr_size;
		*opRSS = lPsinfoBuffer.pr_rssize;
	}
	else
	{
		*opVSS = 0;
		*opRSS = 0;
		return false;
	}
	return true;
}

#elif defined(WIN32)

static void* hQuery = NULL;
static void* hVB  = NULL; 
static void* hPB  = NULL;
static char* gInstName = NULL;

static char* GetProcInstanceName(const char* ipProcName)
{
	char* lCntBuf=NULL;
	char* lInstBuf=NULL;
	DWORD lCntBufSz = 0;
	DWORD lInstBufSz = 0;
	PDH_STATUS rc;

	// Find out the buffer size needed.
	rc = PdhEnumObjectItems(NULL,
							NULL,
							"Process",
							lCntBuf,
							&lCntBufSz,
							lInstBuf,
							&lInstBufSz,
							PERF_DETAIL_WIZARD,
							0);
	if(rc != ERROR_SUCCESS)
	{
		::fprintf(stderr, "Failed to query performance data help library on instance/counter.\n");
		return NULL;
	}

	if(lCntBufSz > 0)
	{
		//lCntBuf = new char[lCntBufSz];
		lCntBufSz = 0;
	}

	if(lInstBufSz > 0)
	{
		lInstBuf = new char[lInstBufSz];
	}

	//Query PDH on instances/counters
	rc = PdhEnumObjectItems(NULL,
							NULL,
							"Process",
							lCntBuf,
							&lCntBufSz,
							lInstBuf,
							&lInstBufSz,
							PERF_DETAIL_WIZARD,
							0);
	if(rc != ERROR_SUCCESS)
	{
		::fprintf(stderr, "Failed to query performance data help library on instance/counter.\n");
		//delete lCntBuf;
		delete lInstBuf;
		return NULL;
	}

	// Find the instance name (my process name)
	const char* cursor = lInstBuf;
	char* lResult = NULL;
	while(*cursor)
	{
		if(::strstr(cursor, ipProcName))
		{
			lResult = ::strdup(cursor);
			break;
		}
		cursor += ::strlen(cursor)+1;
	}
	//delete lCntBuf;
	delete lInstBuf;

	return lResult;
}

// Create the context in pdh library
bool InitPerfCounter()
{
	//Open query of PDH
	if(ERROR_SUCCESS != ::PdhOpenQuery(NULL, 0, &hQuery)) 
	{
		hQuery = NULL;
		::fprintf(stderr, "Failed to open performance data help library.\n");
		return false;
	}

	const char* lProcName = "MallocReplay";
	if( (gInstName = GetProcInstanceName(lProcName)) == NULL)
	{
		::fprintf(stderr, "Failed to find process instace %s\n", lProcName);
		return false;
	}

	char lVBPath[128], lPBPath[128];
	::sprintf(lVBPath, "\\Process(%s)\\Virtual Bytes", gInstName);
	::sprintf(lPBPath, "\\Process(%s)\\Private Bytes", gInstName);

	// check paths
	if(ERROR_SUCCESS != PdhValidatePath(lVBPath))
	{
		::fprintf(stderr, "Path [%s] is invalid\n", lVBPath);
		return false;
	}

	//Add counter into query list
	if(ERROR_SUCCESS != PdhAddCounter(hQuery, lVBPath, 0, &hVB)
		|| ERROR_SUCCESS != PdhAddCounter(hQuery, lPBPath, 0, &hPB) )
	{
		::fprintf(stderr, "Failed to add a counter to performance data help library.\n");
		return false;
	}

	return true;
}

// Release the handle now
bool FiniPerfCounter()
{
	if(hQuery) 
	{
		::PdhCloseQuery(hQuery);
	}
	if(gInstName)
	{
		free(gInstName);
	}
	return true;
}

bool GetProcessMemory(size_t* opVSS, size_t* opRSS)
{
	if(hQuery == NULL)
		return false;

	if(ERROR_SUCCESS != ::PdhCollectQueryData(hQuery)) 
	{
		::fprintf(stderr, "PdhCollectQueryData failed\n");
		return false;
	}

	//Get the formatted value
	PDH_FMT_COUNTERVALUE pdhFMTVal;
	DWORD dwType;
	// VB
	PDH_STATUS rc = ::PdhGetFormattedCounterValue(hVB, PDH_FMT_LONG, &dwType, &pdhFMTVal);
	if(ERROR_SUCCESS != rc || pdhFMTVal.CStatus != ERROR_SUCCESS)
	{
		::fprintf(stderr, "Failed to get counter value of VB.\n");
		::fprintf(stderr, "RC=0x%lx, PDH_CSTATUS=0x%lx\n", rc, pdhFMTVal.CStatus);
		return false;
	}
	else
	{
		*opVSS = pdhFMTVal.longValue / 1024;
	}
	// PB
	if(ERROR_SUCCESS != PdhGetFormattedCounterValue(hPB, PDH_FMT_LONG, NULL, &pdhFMTVal)
		|| pdhFMTVal.CStatus != ERROR_SUCCESS) 
	{
		::fprintf(stderr, "Failed to format counter value of PB.\n");
		return false;
	}
	else
	{
		*opRSS = pdhFMTVal.longValue / 1024;
	}

	return true;
}


#elif defined(linux)

#define LINEBUFFLEN 1024

static size_t GetValueFromString(char* ipString)
{
	char* cursor = ipString;
	// Get rid of preceding blanks
	while (!isdigit(*cursor))
	{
		cursor++;
	}
	// Get rid of following blanks
	char* numString = cursor;
	while (isdigit(*cursor))
	{
		cursor++;
	}
	*cursor = '\0';
	size_t result = ::atol(numString);
	return result;
}

bool GetProcessMemory(size_t* opVSS, size_t* opRSS)
{
	*opVSS = 0;
	*opRSS = 0;

	// There are a few places under /proc/pid to retrieve memory usages
	// /proc/pid/stat   -- RedHat has a bug with it
	// /proc/pid/statm  -- Undervalue if mmap is heavily used.
	// /proc/pid/status -- Generally accurate, but may over estimate after 
	//                     server runs a while, don't know why
	// /proc/pid/maps   -- The most accurate one. maybe ?

	char lFileName[256];
	::sprintf(lFileName, "/proc/%d/status", ::getpid());
	FILE* lpFile = ::fopen(lFileName, "r");
	if(lpFile)
	{
		char lLineBuf[LINEBUFFLEN];

		// The /proc/pid/stat file has only one line
		/*if(!::fgets(lLineBuf, LINEBUFFLEN, lpFile))
		{
			// warning
		}
		else
		{
			unsigned long lsize, lresident, lshare, ltrs, llrs, ldrs, ldt;

			if(::sscanf(lLineBuf, "%lu %lu %lu %lu %lu %lu %lu",
				&lsize, &lresident, &lshare, &ltrs, &llrs, &ldrs, &ldt) == 7)
			{
				long lPageSize = ::sysconf(_SC_PAGESIZE);
				*opVSS = lsize*lPageSize / 1024;
				*opRSS = lresident*lPageSize / 1024;
			}
		}*/

		// The /proc/pid/status file has multiple lines
		while(::fgets(lLineBuf, LINEBUFFLEN, lpFile))
		{
			if (0 == strncmp(lLineBuf, "VmSize:", 7) )
			{
				*opVSS = GetValueFromString(&lLineBuf[7]);
			}
			else if (0 == strncmp(lLineBuf, "VmRSS:", 6) )
			{
				*opRSS = GetValueFromString(&lLineBuf[6]);
			}

			// exit loop if we get all we want
			if (*opVSS && *opRSS)
			{
				break;
			}
		}

		// done with the file
		::fclose(lpFile);
	}
	else
	{
		// warning
	}
	return true;
}

#elif defined(_AIX) || defined(__hpux)

bool GetProcessMemory(size_t* opVSS, size_t* opRSS)
{
	*opVSS = 0;
	*opRSS = 0;
	return true;
}

#endif
