
PLATFORM = AIX_ppc32
COMPILER = xlC_r
LINKER   = $(COMPILER)

PATCHFORMAT = PatchXcoff
BACKTRACE   = BackTrace_AIX
LIBSUFFIX   = a

COMP_OPT = -q32 -c -g -qlinedebug -I$(INC) -I$(INC_PUBLIC)

LINK_OPT = -q32 -G -g -qmkshrobj -bf -qlinedebug 

EXEC_LDFLAGS = -q32 -g -qlinedebug -lpthread

include ../MakeCommon
