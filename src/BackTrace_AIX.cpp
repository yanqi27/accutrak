#include <sys/ucontext.h>
#include <sys/debug.h>
#include <string.h>
#include <sys/ldr.h>
#include <stdio.h>
#include <errno.h>
#include <demangle.h>

#include "BackTrace.h"

 /*
 * set of routines to walk calling thread's call stack...
 * returns a pointer to the current stack pointer in the caller of this
 * routine
 */

typedef struct stackFrame {
	struct stackFrame *back;             /* -> previous stack.*/
	void              *savedCR;
	void              *savedLR;          /* Saved link address.*/  
} stackFrame;

void GetBackTrace(address_t* addr_buf, int nLevel, int nSkip)
{
	// Get current stack pointer
	ucontext_t u;
	if(0 != getcontext(&u))
		return;
	stackFrame* sp = (stackFrame *)u.uc_mcontext.jmp_context.gpr[1];

	int index =0;
	int nPos =0;

	while(sp && nPos++<(nSkip +nLevel))
	{
		if(index<nLevel && nPos>nSkip)
		{
			if(sp->savedLR)
				addr_buf[index++] = sp->savedLR;
			else
				break;
		}
		sp=sp->back;
	}
	// seal the array
	if(index < nLevel)
	{
		addr_buf[index] = 0;
	}
}


// FUNCTION.......... Given an instruction address, find the traceback table.
//
// Given the address of an instruction, scan forward until
// you find a fullword of zeroes.  (All instructions in S/6000 are fullword.)
// Just beyond this fullword is the debug info table which is present
// even when the code was compiled without -g.
//
// LIMITATIONS....... AIX on S/6000 ONLY.
static tbtable *findTable(int *instr)  // I -> instruction address.
{
	tbtable *result;
	// Seek out the fullword of zeroes.
	for(; *instr; instr++)
		;
	// Bump past those zeroes, and we're at the traceback table.
	instr++;
	result =(tbtable *)instr;
	
	return(result);
}


// FUNCTION..........  Given the pointer to the traceback table, 
//                     extract the function name.
// 
// Some fields in the structure may or may NOT
// physically exist, depending on the settings of some control bits.  
// 
// One must bump down the table member by member, under the direction
// of the control bits, in order to find the desired info.
// 
// LIMITATIONS....... returns a pointer to a work area.
//                    Overwritten on each call.
// 
// RETURN VALUES..... always a function name.
//                    if the function is not named, return "'un-named function'"
static char *findName(tbtable *tb)     // traceback table at end of code.
{
	char  *name;                         // -> function name.
	short *nameLen;                      // -> length of function name.
	char  *z;                            // A scratch pointer.
	char  *result = NULL;
	static char funcname[1024];          // place to hold func name.
	if(!tb)
		result = "NULL_TBTABLE";
	// If name present, go find it.
	else if(tb->tb.name_present) {
		// Start at the extension block.
		z = (char *) &(tb->tb_ext);
		// Skip over the parm info if present.
		if( (tb->tb.fixedparms) || (tb->tb.floatparms))
			z = z + sizeof(tb->tb_ext.parminfo);
		// Skip over the tb offset if present.
		if(tb->tb.has_tboff)
			z = z + sizeof(tb->tb_ext.tb_offset);
		// Skip over interrupt handler info if present.
		if(tb->tb.int_hndl)
			z = z + sizeof(tb->tb_ext.hand_mask);
		// Skip controlled storage info if present.
		if(tb->tb.has_ctl) {
			unsigned long count;
			count = ((*(int *)z) + 1) * sizeof(int);
			z = z + count;  // Skip ctl count.
		}
		// Now we are positioned at the name length indicator.
		nameLen = (short *) z;
		// The name is immediately behind that.
		name = (char *) (nameLen+1);
		// Copy the name into our work space.
		if(*nameLen < 1024)
			memcpy(funcname, name, *nameLen);
		else
			strcpy(funcname, "Func Name Too Long");
		// Add null terminator.
		funcname[*nameLen] = 0;
		result = funcname;
	}  // End length present.
	// Else name not present, wing it.
	else
		result = NULL;
	// Return work area.
	return(result);  
}

static const char* GetFuncName(void* instr)
{
  tbtable *tt;
  char *name=NULL;
  
  if(!instr)
    return "Instruction Addr is NULL";
  
  tt = findTable((int *)instr);
  if(tt)
    name = findName(tt);
  else
    name = "Can't find trace table";
  return name;
}

static const char* GetModuleName(void* instr)
{
	static first=1;
	static modulesFound = 0;
	static char buf[1024*32];
	if(first)
	{
		first = 0;
		memset(buf, 0, sizeof(buf));
		errno = 0;
		if(-1 == loadquery(L_GETINFO, buf, sizeof(buf)))
		{
			fprintf(stderr, "Failed to loadquery, errno = %d, (%s)\n",errno,strerror(errno));
			return 	strerror(errno);
			//scrap the area
			memset(buf, 0, sizeof(buf));
		}
		else
		{
			modulesFound = 1;
		}
	}

	if(modulesFound==0)
		return NULL;

	ld_info *li;
	for(li=(ld_info *)buf;
	    ;
	    li=(ld_info *)((char *)li+li->ldinfo_next))
	{
		if(instr >= li->ldinfo_textorg
		   && instr < (char*)li->ldinfo_textorg+li->ldinfo_textsize-1)
		{
			return li->ldinfo_filename;
		}
		if(li->ldinfo_next == 0)
			break;
	}
	return "Unknown_Module";
}

void PrintFuncName(FILE* ipFilePtr, address_t iInstrAddress)
{
	if(!ipFilePtr || !iInstrAddress)
	{
		return;
	}

	::fprintf(ipFilePtr, "0x%lx %s:%s\n", iInstrAddress, GetModuleName(iInstrAddress), GetFuncName(iInstrAddress));
}
