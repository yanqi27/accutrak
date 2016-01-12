/************************************************************************
** FILE NAME..... StlAllocator.h
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... This Allocator is to be used in stl container 
**                   that get memory from static storage instead of heap
** 
** NOTES............ 
** 
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS..... 
** 
** LIMITATIONS...... This implementation is NOT thread-safe
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
#ifndef _STL_ALLOCATOR_H
#define _STL_ALLOCATOR_H

#include "StaticBufferMgr.h"

// the allocator here is a standard allocator, not an RW alternative allocator interface, need to define _RWSTD_ALLOCATOR for sun
#if defined(sun) && !defined(_RWSTD_ALLOCATOR)
#error _RWSTD_ALLOCATOR not defined
#endif

/**
 * Allocator allows stl collections to use our Buffer. When creating a collection
 * that must be in the buffer, create an Allocator and initializing it with the 
 * desired Buffer. Then create the collection initializing with that Allocator
 * instance.
 * The interface of Allocator is as defined by std::allocator.
 */
template<class _Ty>
class Allocator
{
public:
	typedef size_t		size_type;
	typedef ptrdiff_t	difference_type;
	typedef _Ty*		pointer;
	typedef const _Ty*	const_pointer;
	typedef _Ty&		reference;
	typedef const _Ty&	const_reference;
	typedef _Ty			value_type;

	Allocator()	{}

	//~Allocator() {}

	template<class _UT>
	Allocator(const Allocator<_UT>& irAllocator) 
	{
	}

	template<class _UT>
	struct rebind
	{
			typedef Allocator<_UT> other;
	};

	pointer address(reference irX) const throw()
	{
		return (&irX); 
	}

	const_pointer address(const_reference irX) const throw()
	{
		return (&irX); 
	}

	pointer allocate(size_type inSize, const void *)
	{
		return reinterpret_cast<pointer>(GetBlock(sizeof(value_type) *inSize));
	}

	pointer allocate(size_type inSize)
	{
		return allocate(inSize, NULL);
	}

	void deallocate(pointer ipPtr, size_type iSize)
	{
		FreeBlock(ipPtr);
	}

#ifdef WIN32
	char*_Charalloc(size_type inSize)
	{
		// buffer pointer is NULL, do allocation
		// from process heap
		return reinterpret_cast<char*>(GetBlock(inSize));
	}

	void deallocate(void* ipPtr, size_type) throw()
	{
		// If allocation is being done from process heap, the
		// deallocate
	   FreeBlock(ipPtr);
	}
#endif

	void construct(pointer ipPtr, const_reference _V)
	{
#ifdef WIN32
		std::_Construct(ipPtr, _V);
#else
		new ((void *)ipPtr) (_Ty)(_V);
#endif
	}

	void destroy(pointer ipPtr) throw()
	{
#ifdef WIN32
			std::_Destroy(ipPtr);
#else
			ipPtr->~_Ty(); 
#endif
	}


	size_type max_size() const throw()
	{
		size_type lnSize = static_cast<size_type>(UINT_MAX / sizeof(value_type));
		return (0 < lnSize ? lnSize : 1); 
	}
};

template<class _Ty, class _UT> inline
bool operator==(const Allocator<_Ty>&, const Allocator<_UT>&)
{
	return true; 
}

template<class _Ty, class _UT> inline
bool operator!=(const Allocator<_Ty>&, const Allocator<_UT>&)
{
	return false; 
}


// the following is only for Unix
#ifndef WIN32
	// CLASS allocator<void>
template<> class Allocator<void> 
{
public:
	typedef void _Ty;
	typedef _Ty  *pointer;
	typedef const _Ty  *const_pointer;
	typedef _Ty value_type;

	template<class _UT>
	struct rebind 
	{
		typedef Allocator<_UT> other;
	};

	Allocator()
	{
	}

	Allocator(const Allocator<_Ty>& /*irAlloc*/)
	{
	}

	template<class _UT>
	Allocator(const Allocator<_UT>& /*irAlloc*/)
	{
	}

	template<class _UT>
	Allocator<_Ty>& operator=(const Allocator<_UT>& /*irAlloc*/)
	{
		return (*this);
	}
};
#endif // WIN32

#endif // _STL_ALLOCATOR_H
