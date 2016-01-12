/************************************************************************
** FILE NAME..... CustomeMalloc.h
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
#ifndef _CUSTOM_MALLOC_H
#define _CUSTOM_MALLOC_H

#include "ReplacementMalloc.h"

extern void* (* CustomMallocs[MAX_MODULES_TO_RECORD]) (size_t);
extern void  (* CustomFrees[MAX_MODULES_TO_RECORD]) (void*);
extern void* (* CustomReallocs[MAX_MODULES_TO_RECORD]) (void*, size_t);

#endif // _CUSTOM_MALLOC_H
