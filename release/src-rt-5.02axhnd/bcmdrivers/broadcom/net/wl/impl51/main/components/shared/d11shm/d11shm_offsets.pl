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
#	File Name:	d11shm_offsets.pl
#	Abstract:
#
#
#	$History:$
#
#********************************************************************
#
# Auto-generation of shmdefs offsets file
#
# Usage: perl d11shm_offsets.pl --ucode_type=<ucode type> \
#                               --i_shmdefs=<shmdefs file> \
#                               --o_shmdefs_offsets=<shmdefs offsets file>
#

use strict;
use warnings;

use Getopt::Long;

my $text;
my $shm;
my $arg_count;
my $ucode_type;

my $d11shm_file_shmdefs;
my $d11shm_file_shmdefs_offsets;

our $VERBOSE = 'true';
GetOptions ("verbose=s" => \$VERBOSE,
		"ucode_type=s" => \$ucode_type,
		"i_shmdefs=s" => \$d11shm_file_shmdefs,
		"o_shmdefs_offsets=s" => \$d11shm_file_shmdefs_offsets);

open(FILE_D11SHMDEFS, $d11shm_file_shmdefs) ||
	die "$0: Error: $d11shm_file_shmdefs: $!";
open(FILE_D11SHMDEFS_OFFSETS, '>'.$d11shm_file_shmdefs_offsets) ||
	die "$0: Error: $d11shm_file_shmdefs_offsets: $!";

# Trim function - remove leading and trailing whitespace
sub trim($)
{
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}

sub d11print {
	print("DEBUG: $_[0]\n") if $VERBOSE;
}

print FILE_D11SHMDEFS_OFFSETS "#ifndef _D11SHMDEFS_OFFSETS_H\n";
print FILE_D11SHMDEFS_OFFSETS "#define _D11SHMDEFS_OFFSETS_H\n\n";

while (my $line = <FILE_D11SHMDEFS>) {
	if ($line =~ /D11_REV\ \=\=\ /) {
		print FILE_D11SHMDEFS_OFFSETS $line;
	}


	if ($line =~ /\(\(/) {
		# Extract shm name
		$line =~ m/#define\s+(.*)\s+\(/;
		$shm = trim($1);

		# Extract shm offset
		$line =~ m/\(\(.*\+\((.*)\)\)\*2\)*/;
		$text = join "", "#define\t", "$shm", "_OFFSET\t", "($1 * 2)";

		print FILE_D11SHMDEFS_OFFSETS "$text\n";
	}

	if ($line =~ /#endif/) {
		print FILE_D11SHMDEFS_OFFSETS $line;
	}
}

print FILE_D11SHMDEFS_OFFSETS "\n#endif\n";

close FILE_D11SHMDEFS;
close FILE_D11SHMDEFS_OFFSETS;
