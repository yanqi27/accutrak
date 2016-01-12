
PLATFORM = SunOS_sparcv9
COMPILER = CC
LINKER   = $(COMPILER)
AS       = /usr/ccs/bin/as

PATCHFORMAT = PatchElf64
BACKTRACE   = BackTrace_SunOS_sparcv9
LINK_SHLIB  = -lsmartheap_smp64
#LINK_MBLIB  = -lmb

EXTRA_TEST  = TestPrivateHeap
LIBSUFFIX   = so

AT_EXP   = -M $(SRC)/accutrak.map

#DEFINES   = -D__64BIT__ -D_STRUCTURED_PROC=1 -D_SMARTHEAP -D_HEAP_PROFILE
DEFINES   = -D__64BIT__ -D_STRUCTURED_PROC=1 -D_SMARTHEAP
OPTIMIZER = -xO2
COMP_OPT  = -g $(OPTIMIZER) -xarch=v9 -mt -xs -KPIC -c $(DEFINES) -I$(INC) -I$(INC_PUBLIC)

LINK_OPT = -g $(OPTIMIZER) -xarch=v9 -mt -xs -G -Bdynamic -z defs

EXEC_LDFLAGS = -g $(OPTIMIZER) -xarch=v9 -mt -z defs

SYSLIBS = -lc -lCstd -lCrun -ldl -lpthread

# temperary
MSTR_VIEW = /home/myan/views/orion
CHART_LIB_PATH = $(MSTR_VIEW)/BIN/SunOS/lib
MSTR_INC  = -I$(MSTR_VIEW)/Common -I$(MSTR_VIEW)/Kernel

ATOMICOBJ = AtomicLong_sparcv9.o

include ../MakeCommon

# special object rule
MallocReplay.o: $(SRC)/MallocReplay.cpp $(INC_FILES)
	$(COMPILER) $(COMP_OPT) -D_RWSTD_ALLOCATOR $<

AtomicLong_sparcv9.o: $(SRC)/AtomicLong_sparcv9.s
	$(AS) -xarch=v9a $< -o $@
