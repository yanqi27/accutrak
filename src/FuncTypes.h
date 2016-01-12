
#ifndef _FUNC_TYPES_H
#define _FUNC_TYPES_H
/************************************************************************
** FILE NAME..... FuncTypes.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION.........
** Prototypes of various malloc/free routines
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
#include <malloc.h>

typedef void* (*MallocFuncPtr)(size_t);
typedef void* (*CallocFuncPtr)(size_t,size_t);
typedef void  (*FreeFuncPtr)(void*);
typedef void* (*ReallocFuncPtr)(void*,size_t);

typedef void* (*OperatorNewPtr)(size_t);
typedef void* (*OperatorNewArrayPtr)(size_t);
typedef void  (*OperatorDeletePtr)(void*);
typedef void  (*OperatorDeleteArrayPtr)(void*);

#endif //_FUNC_TYPES_H
