/************************************************************************
** FILE NAME..... ReploacementMallocImpl.h
**
** (c) COPYRIGHT
**
** FUNCTION......... Internal data structure for replacement malloc
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
#ifndef _REPLACEMENT_MALLOC_IMPL_H
#define _REPLACEMENT_MALLOC_IMPL_H

#include "CrossPlatform.h"
#include "ReplacementMalloc.h"

// Keep this struct size is multiple of 8 bytes for 64bit
// It is necessary for underrun protection memory block, in which case
// header is grow back(lower address) from the bottom of last page
struct BlockWithDeadPageHeader
{
	union {
		void* mUserAddr;
		// when it is free and will be used for out-of-page header
		BlockWithDeadPageHeader* mNextHeader;
	} p;
	void*  mRawBlockAddr;
	size_t mUserSize;
	// We rely on this signature to distinguish my block or system's.
	char   mSignature[8];
};

// This struct is attached to the beginning of each traced memory block
// There are 0..MAX_STACK_DEPTH level of stack frames before this header
// which is tunnable by user
//
// Keep this struct size multiple of sizeof(long), 8 bytes for 64bit
// So that the user space, which is immediately after this header,
// is aligned on sizeof(long) boundary.
struct BlockWithPaddingHeader
{
	size_t                  mUserSize;
	//unsigned int            mCallStackDepth;
	// 64bit compiler shall put 4bytes padding here.
	BlockWithPaddingHeader* mNextHeader;
	BlockWithPaddingHeader* mPrevHeader;
	// We rely on this signature to distinguish my block or system's.
	char   mSignature[8];
};

// sun linker might put global var of char[8]on an address not aligned on 8 byte
// force it by the following declaration.
typedef union _AT_SIGNATURE_BYTES
{
	char                     mSignature[8];
	UNSIGNED_EIGHT_BYTE_TYPE mDumb;
} AT_SIGNATURE_BYTES;

const AT_SIGNATURE_BYTES gSignature  = { 'a', 'c', 'c', 'u', 't', 'r', 'a', 'k' };
const AT_SIGNATURE_BYTES gScratchSig = { 'F', 'R', 'E', 'E', 'B', 'L', 'K', '!' };
const AT_SIGNATURE_BYTES gDeferSig = { 'D', 'E', 'F', 'E', 'R', 'B', 'L', 'K' };

#define TAIL_MAGIC_SZ 8
#define TAIL_MAGIC_BYTE 0x55
#define USER_INIT_BYTE  0xcd
#define USER_FREE_BYTE  0xfd

// Here I assume pHeader->mSignature and gSignature is aligned on sizeof(long)
#define SET_SIGNATURE(pHeader) *(UNSIGNED_EIGHT_BYTE_TYPE*)((pHeader)->mSignature)=*(UNSIGNED_EIGHT_BYTE_TYPE*)gSignature.mSignature
#define SIGNATURE_MATCH(pHeader) *(UNSIGNED_EIGHT_BYTE_TYPE*)((pHeader)->mSignature)==*(UNSIGNED_EIGHT_BYTE_TYPE*)gSignature.mSignature

#define SCRATCH_SIGNATURE(pHeader) *(UNSIGNED_EIGHT_BYTE_TYPE*)((pHeader)->mSignature)=*(UNSIGNED_EIGHT_BYTE_TYPE*)gScratchSig.mSignature
#define SIGNATURE_SCRATCHED(pHeader) *(UNSIGNED_EIGHT_BYTE_TYPE*)((pHeader)->mSignature)==*(UNSIGNED_EIGHT_BYTE_TYPE*)gScratchSig.mSignature

#define DEFER_SIGNATURE(pHeader) *(UNSIGNED_EIGHT_BYTE_TYPE*)((pHeader)->mSignature)=*(UNSIGNED_EIGHT_BYTE_TYPE*)gDeferSig.mSignature
#define SIGNATURE_DEFERRED(pHeader) *(UNSIGNED_EIGHT_BYTE_TYPE*)((pHeader)->mSignature)==*(UNSIGNED_EIGHT_BYTE_TYPE*)gDeferSig.mSignature

#define GROUND_HEADER(pHeader) \
	SCRATCH_SIGNATURE((pHeader))

#define FREE_BLOCK(pHeader) \
	SIGNATURE_SCRATCHED((pHeader))

#define IS_MY_BLOCK(pHeader,pUserSpace,pRawBlockCandidate) \
	SIGNATURE_MATCH(pHeader) \
	&& (pHeader)->p.mUserAddr==(pUserSpace) \
	&& (pHeader)->mRawBlockAddr==(pRawBlockCandidate)

const AT_SIGNATURE_BYTES gDeadPageStamp = { 'D', 'E', 'A', 'D', 'P', 'A', 'G', 'E' };
const AT_SIGNATURE_BYTES gDeadPageUnstamp = { 'L', 'I', 'V', 'E', 'P', 'A', 'G', 'E' };

#define StampDeadPage(pDeadPage) *(UNSIGNED_EIGHT_BYTE_TYPE*)(pDeadPage)=*(UNSIGNED_EIGHT_BYTE_TYPE*)gDeadPageStamp.mSignature
#define UnstampDeadPage(pDeadPage) *(UNSIGNED_EIGHT_BYTE_TYPE*)(pDeadPage)=*(UNSIGNED_EIGHT_BYTE_TYPE*)gDeadPageUnstamp.mSignature

#endif // _REPLACEMENT_MALLOC_IMPL_H
