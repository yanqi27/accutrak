/************************************************************************
** FILE NAME..... CrossPlatform.h
**
** (c) COPYRIGHT
**
** FUNCTION......... Collection of platform dependant macros/methods
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
#ifndef _CROSS_PLATFORM_H
#define _CROSS_PLATFORM_H

#if defined(WIN32) || defined(WIN64)

#pragma warning (disable:4267)
#pragma warning (disable:4996)

// Windows
#define UNSIGNED_EIGHT_BYTE_TYPE unsigned __int64
#define	PATHDELIMINATOR '\\'

typedef HANDLE ThreadID_t;
#define	GET_THREADID()      ::GetCurrentThreadId()
#define THREAD_JOIN(h)      ::WaitForSingleObject((h), INFINITE)

typedef DWORD TLS_key_t;
#define TLS_SET_VALUE(key,value) (0==TlsSetValue((key),(value)))
#define TLS_GET_VALUE(key)       TlsGetValue((key))
#define TLS_CREATE_KEY(key,dtor) (key = TlsAlloc())

#define GET_SYSTEM_PAGE_SIZE(ps) SYSTEM_INFO lSysInfo; \
	GetSystemInfo(&lSysInfo); \
	ps = lSysInfo.dwPageSize
#define AT_STRCMP strcmpi

#define MUTEX_t CRITICAL_SECTION
#define MUTEX_INIT(alock)   ::InitializeCriticalSection(alock)
#define LOCK_MUTEX(alock)	::EnterCriticalSection(alock)
#define UNLOCK_MUTEX(alock)	::LeaveCriticalSection(alock)

#define YIELD()	Sleep(0)

#else // Unix

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#define UNSIGNED_EIGHT_BYTE_TYPE unsigned long long
#define	PATHDELIMINATOR '/'

typedef pthread_t     ThreadID_t;
#define	GET_THREADID()	          pthread_self()
#define THREAD_JOIN(h)      ::pthread_join((h), NULL)

typedef pthread_key_t TLS_key_t;
#define TLS_SET_VALUE(key,value)  (0!=::pthread_setspecific((key),(value)))
#define TLS_GET_VALUE(key)        pthread_getspecific((key))
#define TLS_CREATE_KEY(key,dtor)  pthread_key_create(&(key),(dtor))

#define GET_SYSTEM_PAGE_SIZE(ps) ps=sysconf(_SC_PAGE_SIZE)

#define AT_STRCMP strcmp

#define MUTEX_t pthread_mutex_t
#define MUTEX_INIT(alock)   ::pthread_mutex_init((alock), NULL)
#define LOCK_MUTEX(alock)	::pthread_mutex_lock((alock))
#define UNLOCK_MUTEX(alock)	::pthread_mutex_unlock((alock))

#if defined(__SVR4)
#define YIELD()	thr_yield()
#else
#define YIELD()	sched_yield()
#endif

#endif

#endif // _CROSS_PLATFORM_H
