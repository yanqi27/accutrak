
#ifndef _ACCUTRAK_H
#define _ACCUTRAK_H
/************************************************************************
** FILE NAME..... AccuTrak.h
**
** (c) COPYRIGHT
**
** FUNCTION......... Interface for AccuTrac utility program
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
** RETURN VALUES.... true  - successful
**                   false - error
**
** AUTHOR(S)........ Michael Q Yan
**
** CHANGES:
**
************************************************************************/
#ifdef WIN32
	#ifdef AT_EXPORTS
		#define	DLL_AT_EXIM	_declspec(dllexport)
	#else
		#define	DLL_AT_EXIM	_declspec(dllimport)
	#endif
#else
	#define	DLL_AT_EXIM
#endif

// Some platforms may need to reference this dummy symbol in order to link
// in accutrak library
extern "C" DLL_AT_EXIM int gAccuTrakDummy;

// Application calls this function to initialize accutrak
// After return, based on configuration file specification,
// some modules will be patched with replacement memory management
// routines, hence behave differently
extern "C" DLL_AT_EXIM bool AT_StartAccuTrak();

// Patch malloc/free incrementally
// Application can call this after modules are dynamically loaded in
// by dlopen or the like.
extern "C" DLL_AT_EXIM bool AT_PatchMemRoutines();

// Unpatch malloc/calloc/realoc, but we have to leave free stay until all blocks allocated by
// my allocator have been released.
extern "C" DLL_AT_EXIM bool AT_StopAccuTrak();

// Version number
#define VER_MAJOR 2
#define VER_MINOR 0

#endif //_ACCUTRAK_H
