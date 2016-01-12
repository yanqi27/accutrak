#!/bin/perl

$maxModules = shift;

die "Usage: $0 [max numbre of modules]" if(!$maxModules);

$ofname = "MallocLogFuncs.inc";

open(OUT, ">$ofname");

print OUT "\n";
print OUT "\n";
print OUT "/////////////////////////////////////////////////////////////////////\n";
print OUT "// Per Module malloc/free routines\n";
print OUT "/////////////////////////////////////////////////////////////////////\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "static void* MallocLog_$i(size_t iBlockSize)\n";
	print OUT "{\n";
	print OUT "\treturn AT_MallocLog(iBlockSize, $i);\n";
	print OUT "};\n\n";
}

for($i=0; $i<$maxModules; $i++) {
	print OUT "static void FreeLog_$i(void* ipBlock)\n";
	print OUT "{\n";
	print OUT "\tAT_FreeLog(ipBlock, $i);\n";
	print OUT "};\n\n";
}

for($i=0; $i<$maxModules; $i++) {
	print OUT "static void* ReallocLog_$i(void* ipBlock, size_t iNewSize)\n";
	print OUT "{\n";
	print OUT "\treturn AT_ReallocLog(ipBlock, iNewSize, $i);\n";
	print OUT "};\n\n";
}

for($i=0; $i<$maxModules; $i++) {
	print OUT "static void* CallocLog_$i(size_t iNumElement, size_t iElementSize)\n";
	print OUT "{\n";
	print OUT "\treturn AT_CallocLog(iNumElement, iElementSize, $i);\n";
	print OUT "};\n\n";
}

for($i=0; $i<$maxModules; $i++) {
	print OUT "static void* NewLog_$i(size_t iBlockSize)\n";
	print OUT "{\n";
	print OUT "\treturn AT_NewLog(iBlockSize, $i);\n";
	print OUT "};\n\n";
}

for($i=0; $i<$maxModules; $i++) {
	print OUT "static void DeleteLog_$i(void* ipBlock)\n";
	print OUT "{\n";
	print OUT "\tAT_DeleteLog(ipBlock, $i);\n";
	print OUT "};\n\n";
}

for($i=0; $i<$maxModules; $i++) {
	print OUT "static void* NewArrayLog_$i(size_t iBlockSize)\n";
	print OUT "{\n";
	print OUT "\treturn AT_NewArrayLog(iBlockSize, $i);\n";
	print OUT "};\n\n";
}

for($i=0; $i<$maxModules; $i++) {
	print OUT "static void DeleteArrayLog_$i(void* ipBlock)\n";
	print OUT "{\n";
	print OUT "\tAT_DeleteArrayLog(ipBlock, $i);\n";
	print OUT "};\n\n";
}

print OUT "\n\n";
print OUT "/////////////////////////////////////////////////////////////////////\n";
print OUT "// Array of routines index by module number\n";
print OUT "// These routines are patched into correspondig modules\n";
print OUT "/////////////////////////////////////////////////////////////////////\n";

print OUT "\n";
print OUT "void* (* MallocBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t) =\n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\t(MallocFuncPtr) MallocLog_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

print OUT "\n";
print OUT "void  (* FreeBlockWithLog[MAX_MODULES_TO_RECORD]) (void*) = \n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\t(FreeFuncPtr) FreeLog_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

print OUT "\n";
print OUT "void* (* ReallocBlockWithLog[MAX_MODULES_TO_RECORD]) (void*, size_t) =\n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\t(ReallocFuncPtr) ReallocLog_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

print OUT "\n";
print OUT "void* (* CallocBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t, size_t) =\n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\t(CallocFuncPtr) CallocLog_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

print OUT "\n";
print OUT "void* (* NewBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t) =\n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\t(OperatorNewPtr) NewLog_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

print OUT "\n";
print OUT "void (* DeleteBlockWithLog[MAX_MODULES_TO_RECORD]) (void*) =\n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\t(OperatorDeletePtr) DeleteLog_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

print OUT "\n";
print OUT "void* (* NewArrayBlockWithLog[MAX_MODULES_TO_RECORD]) (size_t) =\n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\t(OperatorNewArrayPtr) NewArrayLog_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

print OUT "\n";
print OUT "void (* DeleteArrayBlockWithLog[MAX_MODULES_TO_RECORD]) (void*) =\n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\t(OperatorDeleteArrayPtr) DeleteArrayLog_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

close(OUT);
