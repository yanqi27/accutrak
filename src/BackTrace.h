
#ifndef _BACK_TRACE_H
#define _BACK_TRACE_H
/************************************************************************
** FILE NAME..... BackTrace.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Interface to retrieve in-process back trace.
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

#define MAX_STACK_DEPTH 32 // 16 is enough for normal case

typedef void* address_t;

void GetBackTrace(address_t* ipBuffer, int iLevel, int iSkip);

void PrintFuncName(FILE* ipFilePtr, address_t iInstrAddress);

#endif // _BACK_TRACE_H
