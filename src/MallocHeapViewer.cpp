/************************************************************************
** FILE NAME..... MallocHeapViewer.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Display and analyze heap profile/fragmentation through
**                   logged file
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
#pragma warning(disable:4786) // debug symbol is longer than 255
#else
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <set>

#include "MmapFile.h"
#include "MallocHeap.h"
//#include "HeapImage.h"

#define LINE_DIVIDER \
"=============================="

#define KB 1024
#define MB 1048576


static const char PageInfoSignature[] = "heap";
#define SET_HEAPINFO_SIGNATURE(p) (*(int*)(p)->mSignature=*(const int*)PageInfoSignature)

#define VERIFY_HEAPINFO_SIGNATURE(p) (*(int*)(p)->mSignature==*(const int*)PageInfoSignature)

enum BLOCK_SIZE_CLASS
{
	SMALL_BLOCK,
	MEDIUM_BLOCK,
	EXTERNAL_BLOCK,
	LARGE_BLOCK
};

/////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////
//bool gGenHeapImages = false;
static bool gbPrintDetail = false;

/////////////////////////////////////////////////////////////////////
// Utility functions
/////////////////////////////////////////////////////////////////////
static bool PrintHeapHeader(HeapHeader* ipHeader)
{
	if(!VERIFY_HEAPINFO_SIGNATURE(ipHeader))
	{
		fprintf(stderr, "Invalid heap info signature\n");
		return false;
	}
	printf("Version = %d.%d\n", MEM_MAJOR_VERSION(ipHeader->mVersion),
								MEM_MINOR_VERSION(ipHeader->mVersion));
	printf("SubPool_Limit = %d\n", ipHeader->mSubpoolLimit);
	//printf("Small_Block_Threshold = %d\n", ipHeader->mSmallBlockThreshold);
	//printf("Medium_Block_Threshold = %ld\n", ipHeader->mMaxSubAlloc);
	//printf("Large_Block_Threshold = %ld\n", ipHeader->mLargeBlockThreshold);
	printf("0 ... Small(bucket) ... %d ... Medium(var) ... %ld ... Extern(region) ... %ld ... Large(mmap) ...\n",
		ipHeader->mSmallBlockThreshold,
		ipHeader->mMaxSubAlloc, ipHeader->mLargeBlockThreshold);
	printf("Max_Free_Bytes = %ld\n", ipHeader->mMaxProcFree);
	printf("Heap_Grow_Increment = %ld\n", ipHeader->mHeapGrowIncrement);
	printf("\n");

	return true;
}

static bool PrintHeapInfo(HeapInfo* ipHeapInfo, unsigned long iNumHeapSnapShot)
{
	if (gbPrintDetail)
	{
		unsigned int lTotalSplit = ipHeapInfo->mRegionSplitCountForSmall + ipHeapInfo->mRegionSplitCountForMedium
			+ ipHeapInfo->mRegionSplitCountForBig;

		printf("%s [%ld] %s\n", LINE_DIVIDER, iNumHeapSnapShot, LINE_DIVIDER);
		printf("Pool_Context[%d,%d] RequestSize=%ld HeapTop=0x%lx HeapCommitted=%ld KB HeapFree=%ld KB\n", 
			ipHeapInfo->mPool, ipHeapInfo->mSubPool, ipHeapInfo->mUserRequestSize, ipHeapInfo->mHeapTop,
			ipHeapInfo->mHeapTotal/KB, ipHeapInfo->mHeapTotalFree/KB);
		printf("Region Fragmentation: split_cnt(small %d, medium %d, big %d)=%d coalesce_cnt=%d\n",
			ipHeapInfo->mRegionSplitCountForSmall, ipHeapInfo->mRegionSplitCountForMedium,
			ipHeapInfo->mRegionSplitCountForBig, lTotalSplit,
			ipHeapInfo->mRegionCoalesceCnt);
	}
	else
		printf("[%ld] ", iNumHeapSnapShot);

	return true;
}

static bool PrintPageInfo(PageInfo* ipPageInfo, HeapHeader* ipHeader)
{
	if(ipPageInfo->mPageType >= PAGE_TYPES)
	{
		fprintf(stderr, "[Error] unknown page type %d\n", ipPageInfo->mPageType);
		return false;
	}

	if (gbPrintDetail)
	{
		unsigned long lPageSz = ipPageInfo->mPageEnd - ipPageInfo->mPageStart;

		printf("[%d,%d] %s%s(%ld KB) [0x%lx - 0x%lx] InUse=%d Free=%d MaxBlockSize=%d\n", 
			ipPageInfo->mPool, ipPageInfo->mSubPool,
			PageTypeNames[ipPageInfo->mPageType],
			(ipPageInfo->mPageType==PT_External && lPageSz>=ipHeader->mLargeBlockThreshold)?"_Large":"",
			lPageSz/KB,
			ipPageInfo->mPageStart, ipPageInfo->mPageEnd,
			ipPageInfo->mInUseBytes, ipPageInfo->mFreeBytes,
			ipPageInfo->mMaxFreeBlockSize);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////
// The core functions
/////////////////////////////////////////////////////////////////////
static void PrintHeapProfile(const char* ipLogFileName)
{
	// Open file and mmap it into process
	MmapFile lMmapHeapFile(ipLogFileName);

	if(lMmapHeapFile.InitSucceed() == false)
		return;

	// Process log file header
	HeapHeader* lpHeader = (HeapHeader*)lMmapHeapFile.GetStartAddr();
	if (false == PrintHeapHeader(lpHeader))
		return;

	HeapInfo* lpHeapInfo = (HeapInfo*)(lpHeader+1);

	//if (!gbPrintDetail)
	//{
	//	printf("UserSize\tMaxFreeBlock\tTotalFree\tFragmentation\n");
	//}

	unsigned long lNumHeapSnapShot = 0;

	unsigned long lTotalPage = 0;
	unsigned long lDupPage   = 0;
	unsigned long lEndPage   = 0;
	// For each snap shot of heap
	while((char*)lpHeapInfo < lMmapHeapFile.GetEndAddr())
	{
		// First structure is HeapInfo
		if(PrintHeapInfo(lpHeapInfo, lNumHeapSnapShot) == false)
		{
			break;
		}

		// Catagorize request by size
		BLOCK_SIZE_CLASS lRequestBlockClass;
		if (lpHeapInfo->mUserRequestSize <= lpHeader->mSmallBlockThreshold)
			lRequestBlockClass = SMALL_BLOCK;
		else if (lpHeapInfo->mUserRequestSize <= lpHeader->mMaxSubAlloc)
			lRequestBlockClass = MEDIUM_BLOCK;
		else if (lpHeapInfo->mUserRequestSize < lpHeader->mLargeBlockThreshold)
			lRequestBlockClass = EXTERNAL_BLOCK;
		else
			lRequestBlockClass = LARGE_BLOCK;

		// Then there are variable number of page info structures
		PAGESET lPageSet;
		PageInfo* lpPageInfo;
		int lMaxPoolNo    = 0;
		int lMaxSubPoolNo = 0;
		unsigned int lMaxFreeBlock = 0;
		for (lpPageInfo = (PageInfo*)(lpHeapInfo+1);
			 (char*)lpPageInfo < lMmapHeapFile.GetEndAddr();
			 lpPageInfo++)
		{
			lTotalPage++;
			// Check if page type is valid, and beware of end page mark
			if(lpPageInfo->mPageType == PAGE_TYPES)
			{
				lEndPage++;
				break;
			}
			else if(lpPageInfo->mPageType > PAGE_TYPES)
			{
				fprintf(stderr, "Invalid page type %d\n", lpPageInfo->mPageType);
				return;
			}

			//PrintPageInfo(lpPageInfo, lpHeader);

			// Find the range of all pools and their subpools
			if(lMaxPoolNo < lpPageInfo->mPool)
			{
				lMaxPoolNo = lpPageInfo->mPool;
			}
			if(lMaxSubPoolNo < lpPageInfo->mSubPool)
			{
				lMaxSubPoolNo = lpPageInfo->mSubPool;
			}

			// Sort the pages in a set container
			if((lPageSet.insert(lpPageInfo)).second == false)
			{
				lDupPage++;
				// there is a page that has the same address. something is not right.
				// The heap logger can't fully lock all pages simultaneously due to cost 
				/*
				PAGESET::iterator it2 = lPageSet.find(lpPageInfo);
				PageInfo* lpPrevPage = *it2;
				fprintf(stderr, "Failed to insert into page info set\n");
				fprintf(stderr, "PageInfo1 [0x%lx - 0x%lx] type=%s pool=%d,%d inUse=%d free=%d maxFree=%d\n", 
					lpPrevPage->mPageStart, lpPrevPage->mPageEnd, 
					PageTypeNames[lpPrevPage->mPageType], lpPrevPage->mPool, lpPrevPage->mSubPool,
					lpPrevPage->mInUseBytes, lpPrevPage->mFreeBytes, lpPrevPage->mMaxFreeBlockSize);
				fprintf(stderr, "PageInfo2 [0x%lx - 0x%lx] type=%s pool=%d,%d inUse=%d free=%d maxFree=%d\n",
					lpPageInfo->mPageStart, lpPageInfo->mPageEnd, 
					PageTypeNames[lpPageInfo->mPageType], lpPageInfo->mPool, lpPageInfo->mSubPool,
					lpPageInfo->mInUseBytes, lpPageInfo->mFreeBytes, lpPageInfo->mMaxFreeBlockSize);
				*/
				continue;
			}

			if (lMaxFreeBlock < lpPageInfo->mMaxFreeBlockSize)
			{
				lMaxFreeBlock = lpPageInfo->mMaxFreeBlockSize;
			}
		}
		lNumHeapSnapShot++;

		// We get a complete set of pages sorted by address for one snap shot of heap, output it now
		// Sample various measures to reflect different views of the heap profile
		size_t lTotalFromOS = 0;
		size_t lTotalFree   = 0;

		size_t lLargeBlockBytes = 0;
		size_t lSmallPageBytes = 0;
		size_t lFixedPageBytes = 0;
		size_t lExternalPageBytes = 0;
		size_t lFreeRegionBytes = 0;

		size_t* lBytesMainHeapSubPool = new size_t[(lMaxPoolNo+1)*(lMaxSubPoolNo+1)];
		::memset(lBytesMainHeapSubPool, 0, sizeof(size_t)*(lMaxPoolNo+1)*(lMaxSubPoolNo+1));

		// Find the cause of heap growth
		unsigned int lBucketInMyContext = 0;
		unsigned int lBucketInOtherContext = 0;

		unsigned int lMaxFreeBlockInMyContext = 0;
		unsigned int lMaxFreeBlockInOtherContext = 0;

		unsigned int lMaxFreeRegion = 0;

		size_t lGapTotalKBytes = 0;

		// Print out addr-sorted page info structures
		PAGESET::iterator it;
		char* lpPrevPageEndAddr = NULL;
		for(it=lPageSet.begin(); it!=lPageSet.end(); it++)
		{
			PageInfo* lpPage = *it;

			size_t lPageSize = (lpPage->mPageEnd - lpPage->mPageStart);

			///////////////////////////////////////////////////////////////////
			// Statistics
			///////////////////////////////////////////////////////////////////
			lTotalFree += lpPage->mFreeBytes;
			lTotalFromOS += lPageSize;

			// Large_block is directly allocated from OS mmap/VirtualAlloc
			if(lpPage->mPageType == PT_External && lPageSize >= lpHeader->mLargeBlockThreshold)
			{
				lLargeBlockBytes += lPageSize;
			}
			// Others are from main heap (except Windows) sbrk/VirtualAlloc
			else
			{
				int lIndex = lpPage->mPool*(lMaxSubPoolNo+1) + lpPage->mSubPool;
				lBytesMainHeapSubPool[lIndex] += lPageSize;
				// Bytes beloings to different pages on main heap
				if(lpPage->mPageType == PT_Small2 || lpPage->mPageType == PT_Free)
				{
					lSmallPageBytes += lPageSize;
				}
				else if(lpPage->mPageType == PT_Fixed)
				{
					lFixedPageBytes += lPageSize;
				}
				else if(lpPage->mPageType == PT_External)
				{
					lExternalPageBytes += lPageSize;
				}
				else if (lpPage->mPageType == PT_FreeRegion)
				{
					lFreeRegionBytes += lPageSize;
				}
			}

			///////////////////////////////////////////////////////////////////
			// Seek the cause of heap growth
			///////////////////////////////////////////////////////////////////
			if (lRequestBlockClass == SMALL_BLOCK && lpPage->mPageType == PT_Small2)
			{
				if (lpHeapInfo->mUserRequestSize <= lpPage->mMaxFreeBlockSize
					&& lpHeapInfo->mUserRequestSize > lpPage->mMaxFreeBlockSize - 8)
				{
					if (lpHeapInfo->mPool == lpPage->mPool && lpHeapInfo->mSubPool == lpPage->mSubPool)
						lBucketInMyContext++;
					else
						lBucketInOtherContext++;
				}
			}
			else if (lRequestBlockClass == MEDIUM_BLOCK && lpPage->mPageType == PT_Fixed)
			{
				if (lpHeapInfo->mPool == lpPage->mPool && lpHeapInfo->mSubPool == lpPage->mSubPool)
				{
					if (lMaxFreeBlockInMyContext < lpPage->mMaxFreeBlockSize)
						lMaxFreeBlockInMyContext = lpPage->mMaxFreeBlockSize;
				}
				else
				{
					if (lMaxFreeBlockInOtherContext < lpPage->mMaxFreeBlockSize)
						lMaxFreeBlockInOtherContext = lpPage->mMaxFreeBlockSize;
				}
			}
			else if (lRequestBlockClass == EXTERNAL_BLOCK && lpPage->mPageType == PT_FreeRegion)
			{
				if (lMaxFreeRegion < lpPage->mMaxFreeBlockSize)
					lMaxFreeRegion = lpPage->mMaxFreeBlockSize;
			}

			// check contiguousness of this page with the previous one
			if(gbPrintDetail && lpPrevPageEndAddr && lpPage->mPageStart != lpPrevPageEndAddr)
			{
				size_t lGapKBytes = ((long)lpPage->mPageStart-(long)lpPrevPageEndAddr)/1024;
				// Ignore gap between main heap and mmap arena
				if (lpPrevPageEndAddr <= lpHeapInfo->mHeapTop)
				{
					if (lGapKBytes < 4 * 1024 * 1024)
					{
						lGapTotalKBytes += lGapKBytes;
						printf(" ==> Gap(%ld KB) [0x%lx = 0x%lx]\n",
								lGapKBytes, lpPrevPageEndAddr, lpPage->mPageStart);
					}
					else
					{
						printf(" ==> Large Gap <== [0x%lx = 0x%lx]\n",
								lpPrevPageEndAddr, lpPage->mPageStart);
					}
				}
			}
			lpPrevPageEndAddr = lpPage->mPageEnd;

			if(PrintPageInfo(*it, lpHeader) == false)
			{
				return;
			}
		}
		// win32 can't fit big number like lTotalFree*100
		int lFragPercent = (int) ((double)lTotalFree*100/lTotalFromOS);

		if (gbPrintDetail)
		{
			// overall statistics
			printf("TOTAL_FROM_OS[%ld MB]=MainHeap[%ld MB]+mmap[%ld MB], TOTAL_FREE=[%ld MB], FRAGMENTATION=[%ld%%]\n", 
					lTotalFromOS/MB, (lTotalFromOS-lLargeBlockBytes)/MB, lLargeBlockBytes/MB,
					lTotalFree/MB, lFragPercent);
			// temperary debug info
			printf("Total_Gap: %ld MB\n", lGapTotalKBytes/1024);
			// memory distribution in various types
			printf("FreeRegion=[%ld MB], SmallPage=[%ld MB], FixedPage=[%ld MB], ExternalPage(excluding Large)=[%ld MB], LargePage=[%ld MB]\n",
				lFreeRegionBytes/MB, lSmallPageBytes/MB, lFixedPageBytes/MB,
				lExternalPageBytes/MB, lLargeBlockBytes/MB);
			for(int i=0; i<=lMaxPoolNo; i++)
			{
				for(int j=0; j<=lMaxSubPoolNo; j++)
				{
					size_t lSubPoolSize = lBytesMainHeapSubPool[i*(lMaxSubPoolNo+1)+j];
					if(lSubPoolSize > 0)
					{
						printf("\t[%d,%d]: %ld KB\n", i, j, lSubPoolSize/KB);
					}
				}
			}
		}

		// Print the fragmentation reaosn
		if (lRequestBlockClass == SMALL_BLOCK)
		{
			printf("[Small_Block(Request=%d) %d buckets in chosen context %d buckets in other contexts]",
				lpHeapInfo->mUserRequestSize,
				lBucketInMyContext, lBucketInOtherContext);
		}
		else if (lRequestBlockClass == MEDIUM_BLOCK)
		{
			printf("[Medium_Block(Request=%d) max_free_block %d in chosen context %d in other contexts]",
				lpHeapInfo->mUserRequestSize, 
				lMaxFreeBlockInMyContext, lMaxFreeBlockInOtherContext);
		}
		else if (lRequestBlockClass == EXTERNAL_BLOCK)
		{
			printf("[External_Block(Request=%d) max_free_region %d]",
				lpHeapInfo->mUserRequestSize, lMaxFreeRegion);
		}
		else
			printf("[Large_Block(Request=%d)]", lpHeapInfo->mUserRequestSize);

		printf(" MaxFreeBlock=%d\tTotalFree=%ld\tTotalFromOS=%ld\tFragmentation=%d%%%s\n", 
			lMaxFreeBlock, lTotalFree, lTotalFromOS, lFragPercent,
			(lpHeapInfo->mUserRequestSize <= lMaxFreeBlock) ? " <===" : "");
		printf("\n");

		// Generate a image file
		/*if(gGenHeapImages)
		{
			char lImageName[64];
			sprintf(lImageName, "t%03ld", lNumHeapSnapShot);
			if(lPageSet.size()>0 && false == GenHeapBar(lImageName, lpHeapInfo, &lPageSet))
			{
				// something wrong, abort the mission
				break;
			}
		}*/

		delete lBytesMainHeapSubPool;
		// Move to next heap snap shot
		lpHeapInfo = (HeapInfo*)(lpPageInfo+1);
	}

	// Debug only
#ifdef _DEBUG
	printf("TotalPage=%ld EndPage=%ld DupPage=%ld\n", lTotalPage, lEndPage, lDupPage);
#endif
}

static void PrintUsage(const char* ExecName)
{
	printf("Usage: %s HeapLogFile\n", ExecName);
	printf("\t[-h] This help message.\n");
	printf("\t[-l] Print out detail of heap profile.\n");
	printf("\tEnvironment Variables:\n");
	printf("\t\tAT_HEAP_DETAIL=1 same as [-l] option\n");
	//printf("\t\tAT_HEAP_IMAGE=1 to generate snapshot images of heap profile\n");
	exit(0);
}

////////////////////////////////////////////////////////////////////
// Fun starts here.
////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
	if(argc < 2)
	{
		PrintUsage(argv[0]);
	}

	const char* LogFileName = NULL;

	// Pick up command line options
	for (int i=1; i<argc; i++)
	{
		if (*(argv[i]) == '-')
		{
			if (0 == ::strcmp(argv[i], "-l"))
				gbPrintDetail = true;
			else if (0 == ::strcmp(argv[i], "-h"))
				PrintUsage(argv[0]);
			else
			{
				printf("[Error] Unkonwn option [%s]\n", argv[i]);
				exit(-1);
			}
		}
		else if (LogFileName == NULL)
		{
			LogFileName = argv[i];
		}
		else
		{
			printf("[Error] See %s when there is already input file [%s]\n", argv[i], LogFileName);
			exit(-1);
		}
	}

	// more setting from env var
	const char* lEnvStr = ::getenv("AT_HEAP_DETAIL");
	if(lEnvStr)
	{
		gbPrintDetail = true;
	}

	/*lEnvStr = ::getenv("AT_HEAP_IMAGE");
	if(lEnvStr)
	{
		gGenHeapImages = true;
		InitHeapImage();
	}*/


	PrintHeapProfile(LogFileName);

	/*if(gGenHeapImages)
	{
		FiniHeapIMage();
	}*/

	return 0;
}

