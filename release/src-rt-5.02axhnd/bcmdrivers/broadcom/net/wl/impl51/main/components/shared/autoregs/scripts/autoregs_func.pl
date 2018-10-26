#********************************************************************
#	THIS INFORMATION IS PROPRIETARY TO
#	BROADCOM CORP.
#-------------------------------------------------------------------
#
#			Copyright (c) 2002 Broadcom Corp.
#					ALL RIGHTS RESERVED
#
#********************************************************************
#********************************************************************
#	File Name:	autoregs_func.pl
#	<<Broadcom-WL-IPTag/Proprietary:>>
#	Abstract:
#
#
#	$History:$
#
#********************************************************************
#
# Auto-generation of the autoregs functions
#	 [similar to d11shm counterpart]
# Usage: perl autoregs_func.pl = --core_name=<core_name> \
#                              --i_autoregs_func_partial={core_name}regs_func_ucode_<ucode type>_partial.c \
#                              --o_autoregs_main_functions={core_name}regs_main_functions.c \
#                              --o_autoregs_func_decl={core_name}regs_func_decl.h
#

use strict;
use warnings;

use Getopt::Long;

my $line;

my $code_found;
my $core_name;

my $autoregs_file_main_functions;
my $autoregs_file_func_decl;
my $autoregs_file_func_partial;

our $VERBOSE = 'true';

GetOptions ("verbose=s" => \$VERBOSE,
		"core_name=s" => \$core_name,
		"i_autoregs_func_partial=s" => \$autoregs_file_func_partial,
		"o_autoregs_main_functions=s" => \$autoregs_file_main_functions,
		"o_autoregs_func_decl=s" => \$autoregs_file_func_decl);

open(FILE_AUTOREGS_FUNC_MAIN,">>".$autoregs_file_main_functions) ||
	die "$0: Error: $autoregs_file_main_functions: $!";
open(FILE_AUTOREGS_FUNC_DECL, '>>'.$autoregs_file_func_decl) ||
	die "$0: Error: $autoregs_file_func_decl: $!";
open(FILE_AUTOREGS_C_FUNC_PARTIAL, $autoregs_file_func_partial) ||
	die "$0: Error: $autoregs_file_func_partial: $!";

sub autoregs_print {
	print("DEBUG: $_[0]\n") if $VERBOSE;
}

print FILE_AUTOREGS_FUNC_MAIN "\n\/\* regdefs selection function \*\/\n";
print FILE_AUTOREGS_FUNC_MAIN "#ifndef DONGLEBUILD\n";
print FILE_AUTOREGS_FUNC_MAIN "int BCMRAMFN(${core_name}regs_select_offsets_tbl)(const ${core_name}regdefs_t **regdefs, int ${core_name}rev)\n";
print FILE_AUTOREGS_FUNC_MAIN "{\n";
print FILE_AUTOREGS_FUNC_MAIN "\tint ret = 0;\n";
print FILE_AUTOREGS_FUNC_MAIN "\tswitch(${core_name}rev) {\n";

print FILE_AUTOREGS_FUNC_DECL "extern void ${core_name}regs_select_regdefs(int ${core_name}rev);\n";
print FILE_AUTOREGS_FUNC_DECL "#ifdef DONGLEBUILD\n";
print FILE_AUTOREGS_FUNC_DECL "#define ${core_name}regs_select_offsets_tbl(a, b) (b & 0)\n";
print FILE_AUTOREGS_FUNC_DECL "#else /* not-DONGLEBUILD */\n";
print FILE_AUTOREGS_FUNC_DECL "extern int ${core_name}regs_select_offsets_tbl(const ${core_name}regdefs_t **regdefs, int ${core_name}rev);\n";
print FILE_AUTOREGS_FUNC_DECL "#endif /* not-DONGLEBUILD */\n";

while ($line = <FILE_AUTOREGS_C_FUNC_PARTIAL>) {
	print FILE_AUTOREGS_FUNC_MAIN $line;
}

print FILE_AUTOREGS_FUNC_MAIN "\t\tdefault:\n\t\tret = -1;";
print FILE_AUTOREGS_FUNC_MAIN "\t}\n";
print FILE_AUTOREGS_FUNC_MAIN "\treturn ret;\n";
print FILE_AUTOREGS_FUNC_MAIN "}\n";
print FILE_AUTOREGS_FUNC_MAIN "#endif /* not-DONGLEBUILD */\n";

close FILE_AUTOREGS_C_FUNC_PARTIAL;
close FILE_AUTOREGS_FUNC_MAIN;
close FILE_AUTOREGS_FUNC_DECL;
