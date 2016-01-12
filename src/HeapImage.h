#ifndef _HEAP_IMAGE_H
#define _HEAP_IMAGE_H
/************************************************************************
** FILE NAME..... HeapImage.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Inteface to generate image(jpeg etc.) of heap profile
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

// Init before any other API calls
extern void InitHeapImage();
extern void FiniHeapIMage();

extern bool GenHeapBar(const char* ipImageBaseName, HeapInfo* ipHeapInfo, PAGESET* ipPageSet);

#endif // _HEAP_IMAGE_H
