
#ifndef _SYS_ALLOC_H
#define _SYS_ALLOC_H
/************************************************************************
** FILE NAME..... SysAlloc.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION.........
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
void* GetPages(size_t iBytes, bool ibSetFirstPageDead);

void  ReleasePages(void* ipRawBlock,
				   size_t iBlockSize,
				   BlockWithDeadPageHeader* ipHeader,
				   void* ipDeadPageAddr);

// Return true if success
bool SetPageDead(void* ipDeadPageAddr, size_t iPageSize);
bool SetPageLive(void* ipDeadPageAddr, size_t iPageSize);

#endif // _SYS_ALLOC_H
