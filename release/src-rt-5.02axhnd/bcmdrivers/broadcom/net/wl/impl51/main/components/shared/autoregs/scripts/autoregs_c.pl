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
#	File Name:	autoregs_c.pl
#	<<Broadcom-WL-IPTag/Proprietary:>>
#	Abstract:
#
#
#	$History:$
#
#********************************************************************
#
# Auto-generation of the following files:
#       - d11regs.c
#		 [similar to d11shm counterpart]
# Usage: perl autoregs_c.pl = --core_rev=<core_rev> \
#				--core_name=<core name>\
#				--i_autoregs_count=<number of revisions> \
#				--i_autoregs_partial={core_name}regs_partial.c \
#				--o_autoregs_c={core_name}regs.c
#

use strict;
use warnings;

use Getopt::Long;

my $core_rev;
my $core_name;
my $code_found = 0;
my $autoregs_onetime = 1;
my $autoregs_count;

my $autoregs_file_partial;
my $autoregs_file_c;

our $VERBOSE = '';

GetOptions ("verbose=s" => \$VERBOSE,
		"core_rev=s" => \$core_rev,
		"core_name=s" => \$core_name,
		"i_autoregs_count=s" => \$autoregs_count,
		"i_autoregs_partial=s" => \$autoregs_file_partial,
		"o_autoregs_c=s" => \$autoregs_file_c);

if (-e $autoregs_file_c) {
	$autoregs_onetime = 0;
}

open(FILE_AUTOREGS_C_PARTIAL, $autoregs_file_partial) ||
	die "$0: Error: $autoregs_file_partial: $!";
open(FILE_AUTOREGS_C, '>>'.$autoregs_file_c) ||
	die "$0: Error: $autoregs_file_c: $!";

sub autoregs_print {
	print("DEBUG: $_[0]\n") if $VERBOSE;
}

if ($autoregs_onetime == 1) {
	print FILE_AUTOREGS_C "#include <typedefs.h>\n";
	print FILE_AUTOREGS_C "#include <bcmdefs.h>\n";
	print FILE_AUTOREGS_C "#include \"${core_name}regs_declarations.h\"\n\n";
	print FILE_AUTOREGS_C "#define INVALID 0xFFFF\n\n";
}

print FILE_AUTOREGS_C "\n\/\* ${core_name} rev$core_rev related structures \*\/\n";

if ($autoregs_count == 1) {
	print FILE_AUTOREGS_C "#ifndef DONGLEBUILD\n\n";
}
while (my $line = <FILE_AUTOREGS_C_PARTIAL>) {
	my $text = $line;

	# Search for begin of code to copy
	if ($code_found == 0) {
		if ($text =~ /static const ${core_name}regs/) {
			autoregs_print("code found!");
			$code_found = 1;
		} else {
			next;
		}
	}

	print FILE_AUTOREGS_C $line;
}
if ($autoregs_count == 1) {
	print FILE_AUTOREGS_C "#endif /* DONGLEBUILD */\n\n";
}
close FILE_AUTOREGS_C_PARTIAL;
close FILE_AUTOREGS_C;
