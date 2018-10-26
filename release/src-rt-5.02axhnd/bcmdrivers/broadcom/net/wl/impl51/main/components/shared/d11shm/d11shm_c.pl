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
#	File Name:	d11shm_c.pl
#	Abstract:
#
#
#	$History:$
#
#********************************************************************
#
# Auto-generation of the following files:
#       - d11shm.c
#
# Usage: perl d11shm_c.pl = --ucode_type=<ucode type> --d11rev=<d11 rev> \
#                           --i_d11shm_partial=d11shm_partial.c \
#                           --o_d11shm_c=d11shm.c
#

use strict;
use warnings;

use Getopt::Long;

my $ucode_type;
my $d11rev;
my $code_found = 0;
my $d11shm_onetime = 1;

my $d11shm_file_partial;
my $d11shm_file_c;

our $VERBOSE = '';

GetOptions ("verbose=s" => \$VERBOSE,
		"ucode_type=s" => \$ucode_type,
		"d11rev=s" => \$d11rev,
		"i_d11shm_partial=s" => \$d11shm_file_partial,
		"o_d11shm_c=s" => \$d11shm_file_c);

if (-e $d11shm_file_c) {
	$d11shm_onetime = 0;
}

open(FILE_D11SHM_C_PARTIAL, $d11shm_file_partial) ||
	die "$0: Error: $d11shm_file_partial: $!";
open(FILE_D11SHM_C, '>>'.$d11shm_file_c) ||
	die "$0: Error: $d11shm_file_c: $!";

sub d11print {
	print("DEBUG: $_[0]\n") if $VERBOSE;
}

if ($d11shm_onetime == 1) {
	print FILE_D11SHM_C "#include <typedefs.h>\n";
	print FILE_D11SHM_C "#include <bcmdefs.h>\n";
	print FILE_D11SHM_C "#include \"d11shm_declarations.h\"\n\n";
	print FILE_D11SHM_C "#define INVALID 0xFFFF\n\n";
}

print FILE_D11SHM_C "\n\/\* $ucode_type d11rev$d11rev related structures \*\/\n";

while (my $line = <FILE_D11SHM_C_PARTIAL>) {
	my $text = $line;

	# Search for begin of code to copy
	if ($code_found == 0) {
		if ($text =~ /static const d11shm/) {
			d11print("code found!");
			$code_found = 1;
		} else {
			next;
		}
	}

	print FILE_D11SHM_C $line;
}

close FILE_D11SHM_C_PARTIAL;
close FILE_D11SHM_C;
