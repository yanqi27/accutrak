
PLATFORM = AIX_ppc64
COMPILER = xlC_r
LINKER   = $(COMPILER)

PATCHFORMAT = PatchXcoff
BACKTRACE   = BackTrace_AIX
LINK_SHLIB  = -lsmartheap_smp64
#LINK_MBLIB  = -lmb
EXTRA_TEST  = TestPrivateHeap
LIBSUFFIX   = a

#DEFINES  = -D_SMARTHEAP -D_HEAP_PROFILE
DEFINES  = -D_SMARTHEAP
OPTIMIZE = -O2
COMP_OPT = -q64 -g $(OPTIMIZE) -qlinedebug -c $(DEFINES) -I$(INC) -I$(INC_PUBLIC)

LINK_OPT = -g -qlinedebug -q64 -G -qmkshrobj -bf 

EXEC_LDFLAGS = -g -qlinedebug -q64 -bdynamic -lpthread

include ../MakeCommon
