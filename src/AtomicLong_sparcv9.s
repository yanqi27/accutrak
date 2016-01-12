!==============================================================================================
!	FILENAME	:	AtomicLong_sparcv9.s
!	AUTHOR		:	Michael Yan
!	CREATION	:	10/3/2002
!	Copyright (C) 
!==============================================================================================	
	
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!  Atomic increment, decrement and swap routines for sparc
!  using CAS (compare-and-swap) Atomic instructions
!  this requires option -xarch=v8plus to assembler
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	.file "AtomicLong_sparcv9.s"

!
!  Perform the sequence a = a + 1 Atomically with respect to other
!  fetch-and-adds to location a in a wait-free fashion.
!
!  usage : val = InterlockedIncrement(address)
!  return: current value (you would think this would be old val)
!
!  -----------------------
!  Note on REGISTER USAGE:
!  as this is a LEAF procedure, a new stack frame is not created
!  we use the caller stack frame so what would normally be %i (input)
!  registers are actually %o (output registers).  Also, we must not
!  overwrite the contents of %l (local) registers as they are not
!  assumed to be volatile during calls.
!
!  So, the registers used are:
!     %o0  [input]   - the address of the value to increment
!     %o1  [local]   - work register
!     %o2  [local]   - work register
!     %o3  [local]   - work register
!-------------------------------------------------------------------------
!
!        ENTRY(InterlockedIncrement)          ! standard assembler/ELF prologue
	.section	".text"
	.align	8
	.global InterlockedIncrement
	.type InterlockedIncrement,#function
InterlockedIncrement:
retryAI:
        ldx     [%o0], %o2              ! set o2 to the current value
        add     %o2, 0x1, %o3           ! calc the new value
	mov     %o3, %o1                ! save the return value
        casx    [%o0], %o2, %o3         ! atomically set if o0 hasnt changed
        cmp     %o2, %o3                ! see if we set the value
        bne     retryAI                 ! if not, try again
	nop                             ! empty out the branch pipeline
        retl                            ! return back to the caller
        mov     %o1, %o0                ! set the return code to the new value

!        SET_SIZE(InterlockedIncrement)       ! standard assembler/ELF epilogue
	.size	InterlockedIncrement, (.-InterlockedIncrement)
!  end


!  ======================================================================
!
!  Perform the sequence a = a - 1 Atomically with respect to other
!  fetch-and-decs to location a in a wait-free fashion.
!
!  usage : val = InterlockedDecrement(address)
!  return: current value (youd think this would be old val)
!
!  -----------------------
!  Note on REGISTER USAGE:
!  as this is a LEAF procedure, a new stack frame is not created
!  we use the callers stack frame so what would normally be %i (input)
!  registers are actually %o (output registers).  Also, we must not
!  overwrite the contents of %l (local) registers as they are not
!  assumed to be volatile during calls.
!
!  So, the registers used are:
!     %o0  [input]   - the address of the value to increment
!     %o1  [local]   - work register
!     %o2  [local]   - work register
!     %o3  [local]   - work register
!  -----------------------

!       ENTRY(InterlockedDecrement)          ! standard assembler/ELF prologue
	.section	".text"
	.align	8
	.global InterlockedDecrement
	.type InterlockedDecrement,#function
InterlockedDecrement:
retryAD:
        ldx     [%o0], %o2              ! set o2 to the current value
        sub     %o2, 0x1, %o3           ! calc the new value
	mov     %o3, %o1                ! save the return value
        casx    [%o0], %o2, %o3         ! atomically set if o0 hasnt changed
        cmp     %o2, %o3                ! see if we set the value
        bne     retryAD                 ! if not, try again
        nop                             ! empty out the branch pipeline
        retl                            ! return back to the caller
        mov     %o1, %o0                ! set the return code to the new value

!       SET_SIZE(InterlockedDecrement)       ! standard assembler/ELF epilogue
	.size	InterlockedDecrement, (.-InterlockedDecrement)
!  end

	
!  ======================================================================
!
!  Perform the sequence a = b Atomically with respect to other
!  fetch-and-stores to location a in a wait-free fashion.
!
!  usage : old_val = InterlockedExchange(address, newval)
!
!  -----------------------
!  Note on REGISTER USAGE:
!  as this is a LEAF procedure, a new stack frame is not created
!  we use the callers stack frame so what would normally be %i (input)
!  registers are actually %o (output registers).  Also, we must not
!  overwrite the contents of %l (local) registers as they are not
!  assumed to be volatile during calls.
!
!  So, the registers used are:
!     %o0  [input]   - the address of the value to increment
!     %o1  [input]   - the new value to set for [%o0]
!     %o2  [local]   - work register
!     %o3  [local]   - work register
!  -----------------------
	.section	".text"
	.align	8
	.global InterlockedExchange
	.type InterlockedExchange,#function
InterlockedExchange:
retryAS:
        ldx     [%o0], %o2              ! set o2 to the current value
        mov     %o1, %o3                ! set up the new value
	mov     %o2, %o4                ! save the return value
        casx    [%o0], %o2, %o3         ! atomically set if o0 hasnt changed
        cmp     %o2, %o3                ! see if we set the value
        bne     retryAS                 ! if not, try again
        nop                             ! empty out the branch pipeline
        retl                            ! return back to the caller
        mov     %o4, %o0                ! set the return code to the prev value

	.size	InterlockedExchange, (.-InterlockedExchange)
!  end

	
!  ======================================================================
!
!  Perform the sequence a = a + b Atomically with respect to other
!  fetch-and-adds to location a in a wait-free fashion.
!
!  usage : newval = InterlockedAdd(address, val)
!  return: the value after addition
!
!       ENTRY(InterlockedAdd)                ! standard assembler/ELF prologue
	.section	".text"
	.align	8
	.global InterlockedAdd
	.type InterlockedAdd,#function
InterlockedAdd:
retryAA:
        ldx     [%o0], %o2              ! set o2 to the current value
        add     %o2, %o1, %o3           ! calc the new value
	mov     %o2, %o4                ! save the return value
        casx    [%o0], %o2, %o3         ! atomically set if o0 hasnt changed
        cmp     %o2, %o3                ! see if we set the value
        bne     retryAA                 ! if not, try again
	nop                             ! empty out the branch pipeline
	retl                            ! return back to the caller
        mov     %o4, %o0                ! set the return code to the old value

!       SET_SIZE(InterlockedAdd)             ! standard assembler/ELF epilogue
	.size	InterlockedAdd, (.-InterlockedAdd)

!  end

!-------------------------------------------------------------------------
!  InterlockedCompareOneSetZero
!  The registers used are:
!     %o0  [input]   - the address of the value to exchange
!     %o2  [local]   - the value to compare (i.e. one)
!     %o3  [local]   - the new value to set for [%o0] (i.e. zero)
!-------------------------------------------------------------------------

!		ENTRY(InterlockedCompareOneSetZero)       ! standard assembler/ELF prologue
	.section	".text"
	.align	8
	.global InterlockedCompareOneSetZero
	.type InterlockedCompareOneSetZero,#function
InterlockedCompareOneSetZero:
		mov     %g0, %o3                ! set up the new value
		mov     0x1, %o2                ! old value to test for
		casx    [%o0], %o2, %o3         ! atomically clear if [o0] contains one
		retl                            ! return back to the caller
		mov     %o3, %o0                ! set the return code to the prev value

!		SET_SIZE(InterlockedCompareOneSetZero)    ! standard assembler/ELF epilogue
	.size	InterlockedCompareOneSetZero, (.-InterlockedCompareOneSetZero)

!
!  end
!
!  ======================================================================
!
