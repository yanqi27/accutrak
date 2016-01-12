/************************************************************************
** FILE NAME..... atomic.h
**
** (c) COPYRIGHT
**
** FUNCTION......... Low level data synchronization
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
#ifndef _ATOMIC_H
#define _ATOMIC_H

#if defined(_AIX)
#include <sys/atomic_op.h>
#endif

/************************************************************************
**
** simple SMP trylock, spinlock implementations
**
************************************************************************/


/************************************************************************
** Atomic primitive implemented in hand-written assembly
**
** Window has native support, AIX needs an lock protection
************************************************************************/
#if defined(__linux) || (defined(__sun) && defined(sparc)) || (defined(__hpux) && defined(__ia64))

/*
** The function atoimcally sets *p = q, and returns its prior value.
** It will retry in case of contention.
*/
extern "C" long InterlockedExchange(volatile long *p, long q);

/*
** This function atomically compares value at address p with old_val
** If *p==old_val, new_val is set at (*p), otherwise no operation
** @Return: initial value of the Destination parameter (*p)
*/
extern "C" long InterlockedCompareExchange(long volatile *p, long new_val, long old_val);

/*
** Atomic increment/decrement
*/
/* @Return  previous contents */
extern "C" long InterlockedAdd(volatile long *p, long iValue);
extern "C" long InterlockedDecrement(long* ipLong);

#endif

#if defined(WIN32) || defined(__linux) || (defined(__sun) && defined(sparc)) || (defined(__hpux) && defined(__ia64))

#define SYS_INTERLOCKED_EXCHANGE(p, q)   InterlockedExchange(p,q)
#define SYS_INTERLOCKED_COMPARE_EXCHANGE(p, new_val, old_val) \
                                         InterlockedCompareExchange(p, new_val, old_val)
#define SYS_INTERLOCKED_INCREMENT(p)     InterlockedAdd((p),1)
#define SYS_INTERLOCKED_DECREMENT(p)     InterlockedDecrement(p)

#elif defined(_AIX)

#define SYS_INTERLOCKED_DECREMENT(p) (fetch_and_addlp((p), (-1))-1)

#endif

/************************************************************************
** Lock states
** NOTE: Some platforms hard-code these values in their assemly implementation
************************************************************************/
#define SPINLOCK_UNLOCKED 0x1
#define SPINLOCK_LOCKED   0x0

/************************************************************************
** type  spinlock_t
************************************************************************/
#if defined(_AIX)
typedef int spinlock_t;
#else
typedef volatile long spinlock_t;
#endif

/************************************************************************
** Initializer
************************************************************************/
#if defined(_AIX)
#define spinlock_init(lock) _clear_lock(&(lock), SPINLOCK_UNLOCKED)
#else
#define spinlock_init(lock) ((lock) = SPINLOCK_UNLOCKED)
#endif

/************************************************************************
** Release lock spinlock_unlock(x)
************************************************************************/
#define spinlock_unlock(lock) spinlock_init(lock)

/************************************************************************
** Test if lock is held spinlock_test(x)
** Return true if unlocked
************************************************************************/
#define spinlock_test(lock) (lock)

/************************************************************************
** Grab the lock if free  MB_COMPARE_AND_LOCK(x)
** Non-block function
** @Return non-zero if succees
** some implementation depends on the fact SPINLOCK_UNLOCKED is non-zero
************************************************************************/
#if defined(_AIX)

#define MB_COMPARE_AND_LOCK(x) (!_check_lock(x, SPINLOCK_UNLOCKED, SPINLOCK_LOCKED))

#elif defined(WIN32)

#ifdef WIN64
#define MB_COMPARE_AND_LOCK(x) InterlockedCompareExchange((x),0,1)
#else
#define MB_COMPARE_AND_LOCK(x) InterlockedCompareExchange((PVOID *)(x), (PVOID)0, (PVOID)1)
#endif

#elif defined(__sun) || defined(__linux__) || (defined(__hpux) && defined(__ia64))
/************************************************************************
** long InterlockedCompareOneSetZero(volatile long *p)
** Test if *p is one, if true, clear it and return one
** This is equivalent to InterlockedCompareExchange(p, new_val=0, old_val=1);
**
** @Return 0 if fail (lock is held by other).
************************************************************************/
extern "C" long InterlockedCompareOneSetZero(volatile long *);
#define MB_COMPARE_AND_LOCK(x) InterlockedCompareOneSetZero(x)

#else

/*#define MB_COMPARE_AND_LOCK(lock) SYS_INTERLOCKED_EXCHANGE(lock, SPINLOCK_LOCKED)*/

#endif

/************************************************************************
** Try to hold a lock spinlock_trylock(x)
** Return true if got the lock
************************************************************************/
#define spinlock_trylock(x) \
	((spinlock_test(x) && MB_COMPARE_AND_LOCK(&(x))))

/************************************************************************
** Repeatedly(spin) try locking it until got it
** yield the scheduled time slice after fixed times of spinning.
************************************************************************/
#define SYS_SMP_SPIN_QUANTUM 50

//#ifdef sun
//#define YIELD() thr_yield()
//#else
//#define YIELD() sched_yield()
//#endif

#define spinlock_lock(x) \
{ int i = SYS_SMP_SPIN_QUANTUM; \
  while (!spinlock_trylock(x)) \
  { if (i-- == 0) { YIELD(); i = SYS_SMP_SPIN_QUANTUM; } } }

#endif /* _ATOMIC_H */
