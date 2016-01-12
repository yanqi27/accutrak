/************************************************************************
** FILE NAME..... PerfCounter.h
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
#ifndef _PERF_COUNTER_H
#define _PERF_COUNTER_H

extern bool GetProcessMemory(size_t* opVSS, size_t* opRSS);

#ifdef WIN32
extern bool InitPerfCounter();
extern bool FiniPerfCounter();
#endif

#endif // _PERF_COUNTER_H
