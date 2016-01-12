#!/bin/perl

$maxModules = shift;

die "Usage: $0 [max numbre of modules]" if(!$maxModules);

$ofname = "CustomMalloc.cpp";

open(OUT, ">$ofname");

print OUT "\n";
print OUT "/////////////////////////////////////////////////////////////////////\n";
print OUT "// Generated Custom malloc routines\n";
print OUT "// AccuTrak MalloReplay will import/use these per-module routines\n";
print OUT "// User are encouraged to play with these routines to achieve best performace/resource\n";
print OUT "/////////////////////////////////////////////////////////////////////\n";
print OUT "#include <stdlib.h>\n";
print OUT "#include \"CustomMalloc.h\"\n";
print OUT "\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "void* Malloc_$i(size_t iSize)\n";
	print OUT "{\n";
	print OUT "\treturn malloc(iSize);\n";
	print OUT "};\n\n";
}

for($i=0; $i<$maxModules; $i++) {
	print OUT "void Free_$i(void* ipBlock)\n";
	print OUT "{\n";
	print OUT "\tfree(ipBlock);\n";
	print OUT "};\n\n";
}

for($i=0; $i<$maxModules; $i++) {
	print OUT "void* Realloc_$i(void* ipBlock, size_t iNewSize)\n";
	print OUT "{\n";
	print OUT "\treturn realloc(ipBlock, iNewSize);\n";
	print OUT "};\n\n";
}

print OUT "\n\n";
print OUT "/////////////////////////////////////////////////////////////////////\n";
print OUT "// Array of routines index by module number\n";
print OUT "/////////////////////////////////////////////////////////////////////\n";

print OUT "\n";
print OUT "void* (* CustomMallocs[MAX_MODULES_TO_RECORD]) (size_t) =\n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\tMalloc_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

print OUT "\n";
print OUT "void  (* CustomFrees[MAX_MODULES_TO_RECORD]) (void*) = \n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\tFree_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

print OUT "\n";
print OUT "void* (* CustomReallocs[MAX_MODULES_TO_RECORD]) (void*, size_t) =\n";
print OUT "{\n";
for($i=0; $i<$maxModules; $i++) {
	print OUT "\tRealloc_$i";
	if($i != $maxModules-1) {
		print OUT ",";
	}
	print OUT "\n";
}
print OUT "};\n";

close(OUT);
