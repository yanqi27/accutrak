
PLATFORM = HP-UX_IA64
COMPILER = aCC
LINKER   = $(COMPILER)
AS       = /usr/ccs/bin/as

PATCHFORMAT = PatchElf_HP-UX_IA64
BACKTRACE   = BackTrace_HP-UX_IA64
LINK_SHLIB  = -lsmartheap_smp64
#LINK_MBLIB  = -lmb

EXTRA_TEST  = TestPrivateHeap
LIBSUFFIX   = so

#DEFINES  = -D__64BIT__ -D_SMARTHEAP -D_HEAP_PROFILE
DEFINES  = -D__64BIT__ -D_SMARTHEAP
OPTIMIZE = -O2
COMP_OPT = -g $(OPTIMIZE) -c -mt +z -z +d +noobjdebug +DD64 $(DEFINES) -I$(INC) -I$(INC_PUBLIC)

AT_EXP   = -Wl,+e,AT_StartAccuTrak -Wl,+e,AT_StopAccuTrak -Wl,+e,AT_PatchMemRoutines
LINK_OPT = -mt -g $(OPTIMIZE) -dynamic -b -Wl,-a,shared_archive +z +noobjdebug +DD64

EXEC_LDFLAGS = -mt -g $(OPTIMIZE) -dynamic +DD64

SYSLIBS = -lpthread

ATOMICOBJ = atomic_ia64.o

include ../MakeCommon

# special object rule
atomic_ia64.o: $(SRC)/atomic_ia64.s
	$(AS) +DD64 $< -o $@
