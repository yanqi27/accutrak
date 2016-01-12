
#ifndef _REPLACEMENT_MALLOC_H
#define _REPLACEMENT_MALLOC_H
/************************************************************************
** FILE NAME..... ReplacementMalloc.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION.........
** Prototype of available replacement routines.
** 
** NOTES............ 
** 
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
#endif

#include "FuncTypes.h"

// Routines to allocate/deallocate blocks with deadpage
extern void*  MallocBlockWithDeadPage(size_t);
extern void   FreeBlockWithDeadPage(void*);
extern void*  ReallocBlockWithDeadPage(void*, size_t);
extern void*  CallocBlockWithDeadPage(size_t, size_t);
extern void*  NewBlockWithDeadPage(size_t);
//extern void   DeleteBlockWithDeadPage(void*);


// Routines to allocate/deallocate blocks with extra padding
extern void*  MallocBlockWithPadding(size_t);
extern void   FreeBlockWithPadding(void*);
extern void*  ReallocBlockWithPadding(void*, size_t);
extern void*  CallocBlockWithPadding(size_t, size_t);
extern void*  NewBlockWithPadding(size_t);
//extern void   DeleteBlockWithPadding(void*);

// Routines to allocate/deallocate blocks from private heap
extern void*  MallocBlockFromPrivateHeap(size_t);
extern void   FreeBlockFromPrivateHeap(void*);
extern void*  ReallocBlockFromPrivateHeap(void*, size_t);
extern void*  CallocBlockFromPrivateHeap(size_t, size_t);
extern void*  NewBlockFromPrivateHeap(size_t);
bool InitPrivateHeaps();

#define MAX_MODULES_TO_RECORD 256
// Routines to allocate/deallocate blocks and log all transactions to disk files
extern void* (* MallocBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t);
extern void  (* FreeBlockWithLog[MAX_MODULES_TO_RECORD]) (void*);
extern void* (* ReallocBlockWithLog[MAX_MODULES_TO_RECORD]) (void*, size_t);
extern void* (* CallocBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t, size_t);
extern void* (* NewBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t);
extern void  (* DeleteBlockWithLog[MAX_MODULES_TO_RECORD]) (void*);
extern void* (* NewArrayBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t);
extern void  (* DeleteArrayBlockWithLog[MAX_MODULES_TO_RECORD]) (void*);

extern void* (* OldMalloc[MAX_MODULES_TO_RECORD]) (size_t);
extern void  (* OldFree[MAX_MODULES_TO_RECORD]) (void*);
extern void* (* OldRealloc[MAX_MODULES_TO_RECORD]) (void*, size_t);
extern void* (* OldCalloc[MAX_MODULES_TO_RECORD]) (size_t, size_t);
extern void* (* OldNew[MAX_MODULES_TO_RECORD]) (size_t);
extern void  (* OldDelete[MAX_MODULES_TO_RECORD]) (void*);
extern void* (* OldNewArray[MAX_MODULES_TO_RECORD]) (size_t);
extern void  (* OldDeleteArray[MAX_MODULES_TO_RECORD]) (void*);

#include <stdio.h>
// expose routines to leverage managed blocks allocated from patched modules
void AT_CheckAllBlocks();
void AT_DumpInUseBlocks(FILE*);

#endif // _REPLACEMENT_MALLOC_H
