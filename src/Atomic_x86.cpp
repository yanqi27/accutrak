/************************************************************************
** FILE NAME..... Atomic_x86.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Common routines
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
#include "Atomic_op.h"

#if defined(linux) && defined(__x86_64)

extern "C"
long InterlockedCompareOneSetZero(volatile long *p)
{
	long result;

	__asm__ __volatile__ ("lock; cmpxchgq %1, %2"
							: "=a" (result)
							: "r" ((long)0), "m" (*p), "a" (1));

	return result;
}

extern "C"
long InterlockedExchange(volatile long * ipLong, long iNewVal)
{
	register long result;
	__asm__ __volatile__ ("lock; xchgq %0,%2"
							: "=r" (result)
							: "0" (iNewVal), "m" (*ipLong)
							: "memory");
	return result;
}

extern "C"
long InterlockedCompareExchange(long volatile * oDest, /* dest      */
							   long iNewVal,           /* new value */
							   long iOldVal)           /* old value */
{
	register long result;
	__asm__ __volatile__ ("lock; cmpxchgq %1, %2"
							: "=a" (result)
							: "r" (iNewVal), "m" (*oDest), "a" (iOldVal));

	return result;
}

extern "C"
long InterlockedAdd(volatile long *ipLong, long iValue)
{
	register long __result;
	__asm__ __volatile__ ("lock xaddq %0, %2;"
							: "=r" (__result)
							: "0" (iValue), "m" (*ipLong)
							: "memory");
	return __result;
}


extern "C"
long InterlockedDecrement(long* ipLong)
{
	register long __result;
	__asm__ __volatile__ ("lock xaddq %%rbx, (%%rax); decq %%rbx"
							: "=b" (__result)
							: "a" (ipLong), "b" ( (long)-1)
							: "cc", "memory");
	return __result;
}

#endif
