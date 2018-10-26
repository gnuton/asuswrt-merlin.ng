#********************************************************************
#	THIS INFORMATION IS PROPRIETARY TO
#	BROADCOM CORP.
#-------------------------------------------------------------------
#
#	$ Copyright Broadcom $
#	
#	
#	<<Broadcom-WL-IPTag/Proprietary:>>
#
#********************************************************************
#********************************************************************
#	File Name:	d11shm_func.pl
#	Abstract:
#
#
#	$History:$
#
#********************************************************************
#
# Auto-generation of the d11shm functions
#
# Usage: perl d11shm_func.pl = --ucode_type=<ucode type> --d11rev=<d11 rev> \
#                              --ucode_type=ucode_<ucode type>\
#                              --i_d11shm_func_partial=d11shm_func_ucode_<ucode type>_partial.c \
#                              --o_d11shm_main_functions=d11shm_main_functions.c \
#                              --o_d11shm_func_decl=d11shm_func_decl.h
#

use strict;
use warnings;

use Getopt::Long;

my $line;

my $ucode_type;
my $code_found;

my $d11shm_file_main_functions;
my $d11shm_file_func_decl;
my $d11shm_file_func_partial_ucode_type;

our $VERBOSE = 'true';

GetOptions ("verbose=s" => \$VERBOSE,
		"ucode_type=s" => \$ucode_type,
		"i_d11shm_func_partial=s" => \$d11shm_file_func_partial_ucode_type,
		"o_d11shm_main_functions=s" => \$d11shm_file_main_functions,
		"o_d11shm_func_decl=s" => \$d11shm_file_func_decl);

open(FILE_D11SHM_FUNC_MAIN,">>".$d11shm_file_main_functions) ||
	die "$0: Error: $d11shm_file_main_functions: $!";
open(FILE_D11SHM_FUNC_DECL, '>>'.$d11shm_file_func_decl) ||
	die "$0: Error: $d11shm_file_func_decl: $!";
open(FILE_D11SHM_C_FUNC_PARTIAL_UCODE_TYPE, $d11shm_file_func_partial_ucode_type) ||
	die "$0: Error: $d11shm_file_func_partial_ucode_type: $!";

sub d11print {
	print("DEBUG: $_[0]\n") if $VERBOSE;
}

print FILE_D11SHM_FUNC_MAIN "\n\/\* $ucode_type shmdefs selection function \*\/\n";
print FILE_D11SHM_FUNC_MAIN "void BCMRAMFN(d11shm_select_$ucode_type)(const shmdefs_t **shmdefs, int d11rev)\n";
print FILE_D11SHM_FUNC_MAIN "{\n";
print FILE_D11SHM_FUNC_MAIN "\tswitch(d11rev) {\n";

print FILE_D11SHM_FUNC_DECL "extern void d11shm_select_shmdefs_$ucode_type(int d11rev);\n";
print FILE_D11SHM_FUNC_DECL "extern void d11shm_select_$ucode_type(const shmdefs_t **shmdefs, int d11rev);\n";

while ($line = <FILE_D11SHM_C_FUNC_PARTIAL_UCODE_TYPE>) {
	print FILE_D11SHM_FUNC_MAIN $line;
}

print FILE_D11SHM_FUNC_MAIN "\t}\n";
print FILE_D11SHM_FUNC_MAIN "}\n";

close FILE_D11SHM_C_FUNC_PARTIAL_UCODE_TYPE;
close FILE_D11SHM_FUNC_MAIN;
close FILE_D11SHM_FUNC_DECL;
