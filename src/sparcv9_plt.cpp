///////////////////////////////////////////////////////////////////////////////
// This file is intended to be includeded in another source file.
///////////////////////////////////////////////////////////////////////////////

// From open solaris /cmd/sgs/rtld/sparcv9/boot_elf.s
// instruction encodings. 
#define	M_SAVESP64	0x9de3bfc0	/* save %sp, -64, %sp */
#define	M_CALL		0x40000000
#define	M_JMPL		0x81c06000	/* jmpl %g1 + simm13, %g0 */
#define	M_SETHIG0	0x01000000	/* sethi %hi(val), %g0 */
#define	M_SETHIG1	0x03000000	/* sethi %hi(val), %g1 */
#define	M_STO7G1IM	0xde206000	/* st	 %o7,[%g1 + %lo(val)] */
#define	M_SUBFPSPG1	0x8227800e	/* sub	%fp,%sp,%g1 */
#define	M_NOP		0x01000000	/* sethi 0, %o0 (nop) */
#define	M_BA_A		0x30800000	/* ba,a */
#define	M_BA_A_PT	0x30480000	/* ba,a %icc, <dst> */
#define	M_MOVO7TOG1	0x8210000f	/* mov %o7, %g1 */
#define	M_MOVO7TOG5	0x8a10000f	/* mov %o7, %g5 */
#define	M_MOVI7TOG1	0x8210001f	/* mov %i7, %g1 */
#define	M_BA_A_XCC	0x30680000	/* ba,a %xcc */
#define	M_JMPL_G5G0	0x81c16000	/* jmpl %g5 + 0, %g0 */
#define	M_XNOR_G5G1	0x82396000	/* xnor	%g5, 0, %g1 */
#define	M_OR_G1G5G5	0x8a104005	/* or %g1, %g5, %g5 */
#define	M_SLLX_G132G1	0x83287020	/* sllx %g1, 32, %g1 */
#define	M_OR_G1G1	0x82106000	/* or %g1, 0x0, %g1 */
#define	M_SETHI_G5	0x0b000000	/* sethi 0x0, %g5 */
#define	M_SETHI_G1	0x03000000	/* sethi 0x0, %g1 */
#define	M_JMPL_G1G0	0x81c06000	/* jmpl %g1 + 0, %g0 */
#define	M_SLLX_G112G1	0x8328700c	/* sllx %g1, 12, %g1 */

#define	S_MASK(n)	(unsigned long)((1ul << (n)) - 1ul)
#define LO10(x)		(unsigned int)((x) & S_MASK(10))
#define HM10(x)		(unsigned int)(((x) >> 32) & S_MASK(10))
#define LM22(x)		(unsigned int)(((x) >> 10) & S_MASK(22))
#define HH22(x)		(unsigned int)(((x) >> 42) & S_MASK(22))

#define	BITS(v, u, l)	(((v) >> (l)) & S_MASK((u) - (l) + 1))
#define	H44(v)		BITS(v, 43, 22)
#define	M44(v)		BITS(v, 21, 12)
#define	L44(v)		BITS(v, 11, 0)

#define	S_INRANGE(v, n)	(((-(1 << (n)) - 1) < (v)) && ((v) < (1 << (n))))

typedef unsigned long psaddr_t;

// Patch a PLT entry to jump to a new address 
static int SetFuntionPtr(void *pvPltEntry, void *funcptr) 
{
	// test to see how far away our destination address lies.
	// If it is close enough that a branch can be used instead of a jmpl
	// we will fill the plt in with single branch.
	// The branches are much quicker then a jmpl instruction

	// Assume near plt for now.

	uintptr_t pltaddr = (uintptr_t)pvPltEntry;
	long disp = (char*)funcptr - (char*)pvPltEntry - 4;
	uint_t* pltent = (uint_t*)pltaddr;

	// Test if the destination address is close enough to use
	// a ba,a... instruction to reach it.
	if (S_INRANGE(disp, 23)) 
	{
		uint_t bainstr;

		// ba,a,pt %icc, <dest>
		//
		// is the most efficient of the PLT's.  If we
		// are within +-20 bits - use that branch.
		if (S_INRANGE(disp, 20)) 
		{
			bainstr = M_BA_A_PT;	// ba,a,pt %icc,<dest>
			bainstr |= (uint_t)(S_MASK(19) & (disp >> 2));
		}
		else
		{
			// Otherwise - we fall back to the good old
			//
			// ba,a	<dest>
			//
			// Which still beats a jmpl instruction.
			bainstr = M_BA_A;		// ba,a <dest>
			bainstr |= (uint_t)(S_MASK(22) & (disp >> 2));
		}

		pltent[2] = M_NOP;		// nop instr
		pltent[1] = bainstr;
		pltent[0] = M_NOP;		// nop instr

		return(0);
	}
	
	uintptr_t nsym = ~((uintptr_t)funcptr);
	ulong_t	symval = (ulong_t)funcptr;
	if ((nsym >> 32) == 0) 
	{
		// This version gets anywhere in the top 32 bits:
		pltent[3] = M_JMPL_G1G0;
		pltent[2] = (uint_t)(M_XNOR_G5G1 | LO10(nsym));
		*(ulong_t *)pltaddr = ((ulong_t)M_NOP << 32) | (M_SETHI_G5 | LM22(nsym));
		return(0);
	}
	else if ((nsym >> 44) == 0) 
	{
		// This version gets us anywhere in the top 44 bits of the address space
		// since this is where shared objects live most of the time,
		// this case is worth optimizing.
		pltent[4] = (uint_t)(M_JMPL_G1G0 | L44(symval));
		pltent[3] = M_SLLX_G112G1;
		pltent[2] = (uint_t)(M_XNOR_G5G1 | M44(nsym));
		*(ulong_t *)pltaddr = ((ulong_t)M_NOP << 32) | (M_SETHI_G5 | H44(nsym));
		return(0);
	}

	// The PLT destination is not in reach of a branch instruction
	// so we fall back to a 'jmpl' sequence.
	pltent[6] = M_JMPL_G5G0 | LO10(symval);
	pltent[5] = M_OR_G1G5G5;
	pltent[4] = M_SLLX_G132G1;
	pltent[3] = M_OR_G1G1 | HM10(symval);
	pltent[2] = M_SETHI_G5 | LM22(symval);
	*(unsigned long*)pltaddr = ((ulong_t)M_NOP << 32) | (M_SETHI_G1 | HH22(symval));

   return(0);
}

// iSymbolAddr shall be a CALL instruction
// replace the callee with a new funciton pointer
// Return true if successful.
bool FixCallLabel(Elf64_Addr  iSymbolAddr, 
				  FunctionPtr iFuncPtr,
				  FunctionPtr* oOldFuncPtr)
{
	Elf64_Addr lCalleeAddr = 0;
	// verify the MSB 2bits are 01
	int* instr = (int*) iSymbolAddr;
	if( (instr[0] & 0xc0000000) == M_CALL)
	{
		Elf64_Addr lDisp30 = (instr[0] & (~M_CALL));
		// disp30 is zero, means rtld doesn't relocate this entry yet
		if(lDisp30 != 0)
		{
			lCalleeAddr = iSymbolAddr + 4 + (lDisp30 << 2);
			// Calc the disp against the new function addr
			lDisp30 = ((Elf64_Addr)iFuncPtr - (iSymbolAddr + 4)) >> 2;
			// make sure the disp30 fits in the LSB 30bits
			if( (lDisp30 & 0xffffffffc0000000) == 0)
			{
				instr[0] = M_CALL | lDisp30;
				*oOldFuncPtr = (FunctionPtr)lCalleeAddr;
				return true;
			}
		}
	}
	return false;
}

// Pass in the location of CALL instruction address
// return the vaddr of its callee
static Elf64_Addr GetCallLabel(Elf64_Addr iSymbolAddr)
{
	Elf64_Addr lResult = 0;
	// verify the MSB 2bits are 01
	int* instr = (int*) iSymbolAddr;
	if( (instr[0] & 0xc0000000) == M_CALL)
	{
		Elf64_Addr lDisp30 = (instr[0] & (~M_CALL));
		// disp30 is zero, means rtld doesn't relocate this entry yet
		if(lDisp30 != 0)
		{
			lResult = iSymbolAddr + 4;
			lResult += lDisp30 * 4;
		}
	}
	return lResult;
}

static void GetFuntionPtr(Elf64_Addr iSymbolAddr, FunctionPtr* oOldFuncPtr)
{
	// The second part of the V9 ABI (sec. 5.2.4)
	// applies to plt entries greater than 0x8000 (32,768).
	// This is handled by FAR PLT, assume near plt for now.
	//
	// ELF64 NEAR PLT's
	int* instr = (int*) iSymbolAddr;
	psaddr_t pltaddr = iSymbolAddr;
	psaddr_t destaddr = 0;

	if ((instr[0] != M_NOP) && ((instr[1] & (~(S_MASK(19)))) == M_BA_A_XCC)) 
	{
		// Unbound PLT
		// The relocation hasn't yet been lazily resolved and instead
		// points to the dynamic linker trampoline at the start of the PLT 
	} 
	else if((instr[0] == M_NOP) && ((instr[1] & (~(S_MASK(22)))) == M_BA_A))
	{
		// Resolved 64-bit PLT entry format (b+-8mb):
		// .PLT
		// 0	nop
		// 1	ba,a	<dest>
		// 2	nop
		// 3	nop
		// 4	nop
		// 5	nop
		// 6	nop
		// 7	nop
		uint_t d22 = instr[1] & S_MASK(22);
		destaddr = ((long)pltaddr + 4) + (((int)d22 << 10) >> 8);
	}
	else if((instr[0] == M_NOP) && ((instr[1] & (~(S_MASK(19)))) == M_BA_A_PT))
	{
		// Resolved 64-bit PLT entry format (b+-2mb):
		// .PLT
		// 0	nop
		// 1	ba,a,pt	%icc, <dest>
		// 2	nop
		// 3	nop
		// 4	nop
		// 5	nop
		// 6	nop
		// 7	nop
		uint_t d19 = instr[1] & S_MASK(22);
		destaddr = ((long)pltaddr + 4) + (((int)d19 << 13) >> 11);
	}
	else if((instr[6] & (~(S_MASK(13)))) == M_JMPL_G5G0) 
	{
		// Resolved 64-bit PLT entry format (abs-64):
		// .PLT
		// 0	nop
		// 1	sethi	%hh(dest), %g1
		// 2	sethi	%lm(dest), %g5
		// 3	or	%g1, %hm(dest), %g1
		// 4	sllx	%g1, 32, %g1
		// 5	or	%g1, %g5, %g5
		// 6	jmpl	%g5 + %lo(dest), %g0
		// 7	nop
		uintptr_t	hh_bits;
		uintptr_t	hm_bits;
		uintptr_t	lm_bits;
		uintptr_t	lo_bits;
		hh_bits = instr[1] & S_MASK(22); /* 63..42 */
		hm_bits = instr[3] & S_MASK(10); /* 41..32 */
		lm_bits = instr[2] & S_MASK(22); /* 31..10 */
		lo_bits = instr[6] & S_MASK(10); /* 09..00 */
		destaddr = (hh_bits << 42) | (hm_bits << 32) | (lm_bits << 10) | lo_bits;
	}
	else if (instr[3] == M_JMPL)
	{
		// Resolved 64-bit PLT entry format (top-32):
		// .PLT:
		// 0	nop
		// 1	sethi	%hi(~dest), %g5
		// 2	xnor	%g5, %lo(~dest), %g1
		// 3	jmpl	%g1, %g0
		// 4	nop
		// 5	nop
		// 6	nop
		// 7	nop
		uintptr_t hi_bits;
		uintptr_t lo_bits;
		hi_bits = (instr[1] & S_MASK(22)) << 10;
		lo_bits = (instr[2] & S_MASK(10));
		destaddr = hi_bits ^ ~lo_bits;
	}
	else if ((instr[2] & (~(S_MASK(13)))) == M_XNOR_G5G1) 
	{
		// Resolved 64-bit PLT entry format (top-44):
		// .PLT:
		// 0	nop
		// 1	sethi	%h44(~dest), %g5
		// 2	xnor	%g5, %m44(~dest), %g1
		// 3	slxx	%g1, 12, %g1
		// 4	jmpl %g1 + %l44(dest), %g0
		// 5	nop
		// 6	nop
		// 7	nop
		uintptr_t h44_bits;
		uintptr_t	m44_bits;
		uintptr_t	l44_bits;
		h44_bits = (((long)instr[1] & S_MASK(22))
			<< 10);
		m44_bits = (((long)instr[2] & S_MASK(13))
			<< 41) >> 41;
		l44_bits = (((long)instr[4] & S_MASK(13))
			<< 41) >> 41;
		destaddr = (~(h44_bits ^ m44_bits) << 12) + l44_bits;
	}
	else
	{
		// don't know what is going on now
	}
	*oOldFuncPtr = (FunctionPtr)destaddr;
}

static void InterceptFunciton(Elf64_Addr iSymbolAddr, 
							  FunctionPtr iFuncPtr,
							  FunctionPtr* oOldFuncPtr)
{
	// Get the old function ptr
	GetFuntionPtr(iSymbolAddr, oOldFuncPtr);
	// Now patch the PLT entry 
	SetFuntionPtr((void*)iSymbolAddr, iFuncPtr);
}
