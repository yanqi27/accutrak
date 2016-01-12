#ifndef _MALLOC_HEAP_H
#define _MALLOC_HEAP_H
/************************************************************************
** FILE NAME..... MallocHeap.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Declaration of heap structures.
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

#ifdef _SMARTHEAP
#include "smrtheap.h"

#include <set>

// STL Set to store pages sorted by address.
struct lt_PageInfo
{
  bool operator()(const PageInfo* p1, const PageInfo* p2) const
  {
    return (p1->mPageStart < p2->mPageStart);
  }
};

typedef std::set<PageInfo*, lt_PageInfo> PAGESET;

/* A hack of memory manager */
#define PT_Fixed            (0)
#define PT_FS               (1)
#define PT_Moveable         (2)
#define PT_Overhead         (2)
#define PT_External         (3)
#define PT_ThreadSmall      (4)
#define PT_Small2           (5)
#define PT_Free             (6)
#define PT_FreeRegion       (7)
#define PAGE_TYPES          (8)

static const char* PageTypeNames[] = 
{
	"Fixed_Page ",
	"FS_Page",
	"Overhead   ",
	"External_Page",
	"ThreadSmall_Page",
	"Small2_Page",
	"Free_Page",
	"FreeRegion ",
	"Invalid_Page"
};

#endif // _SMARTHEAP

#endif // _MALLOC_HEAP_H
