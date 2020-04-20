
PLATFORM = Linux_x86_64
COMPILER = g++
LINKER   = $(COMPILER)

PATCHFORMAT = PatchElf64
BACKTRACE   = BackTrace_Linux_x86_64
#LINK_SHLIB  = -lsmartheap_smp64
#LINK_MBLIB  = -lmb
SYSLIBS     = -lpthread -ldl
#EXTRA_TEST  = TestPrivateHeap
LIBSUFFIX   = so

ATOMICOBJ = Atomic_x86.o

# The version control script changes in-core link-map mysteriously. !!FIX!!
#AT_EXP   = -Wl,--version-script=$(SRC)/accutrak.map

#DEFINES  = -D__64BIT__ -D_SMARTHEAP -D_HEAP_PROFILE
DEFINES  = -D__64BIT__
#OPTIMIZER = -O2
COMP_OPT = $(OPTIMIZER) -g -c -w -m64 -fPIC $(DEFINES) -I$(INC) -I$(INC_PUBLIC)

LINK_OPT = -g -shared -m64 -Wl,--no-undefined

EXEC_LDFLAGS = -g -m64 -Wl,--no-undefined -Wl,-rpath=$(LIBPATH)

# temperary
MSTR_VIEW = /home/myan/views/orion
CHART_LIB_PATH = $(MSTR_VIEW)/BIN/Linux/lib
MSTR_INC  = -I$(MSTR_VIEW)/Common -I$(MSTR_VIEW)/Kernel
EXTRA_TARGETS = MallocHeapViewer

include ../MakeCommon
