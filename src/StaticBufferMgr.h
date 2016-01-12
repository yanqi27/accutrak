/************************************************************************
** FILE NAME..... StaticBufferMgr.h
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
#ifndef _STATIC_BUFFER_MGR_H
#define _STATIC_BUFFER_MGR_H

/*extern void* GetChunk(unsigned int& orChunkSize);
extern void  ReleaseChunk(void* ipChunk);

struct SINGLE_LINK_LIST_NODE
{
	SINGLE_LINK_LIST_NODE* mpNextNode;
};*/

extern void  InitStaticBuffer();
extern void  ReleaseStaticBuffer();

extern void* GetBlock(size_t size);
extern void  FreeBlock(void* ipBlock);

#endif // _STATIC_BUFFER_MGR_H
