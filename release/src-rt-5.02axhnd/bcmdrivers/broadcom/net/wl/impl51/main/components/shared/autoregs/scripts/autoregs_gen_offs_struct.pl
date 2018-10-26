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
#	File Name:	autoregs_gen_offs_struct.pl
#	<<Broadcom-WL-IPTag/Proprietary:>>
#	Abstract:
#		 [similar to d11shm counterpart]
#
#	$History:$
#
#********************************************************************
#
# Auto-updation of the following files:
#       - {core_name}regsoffs.h      - Contains register map structure. works ONLY for 1 rev [eg: dongle]
#
# Usage: perl autoregs_gen_offs_struct.pl = --named_init=<named init of structs> \
#			--i_autoregs_offs_c=<{core_name}regs offsets file> \
#			--i_autoregs_structs_inits={core_name}regs_structs_inits.c \
#			--i_autoregs_count=<number of revisions> \
#			--o_autoregs_offs_struct_h={core_name}regsoffs.h => note that, this file will be overwritten.
#
#	- note that this script will execute ONLY if there is a single revision
#

use strict;
use warnings;
use Getopt::Long;

my $core_name;
my $autoregs_named_init;
my $autoregs_offs_c;
my $autoregs_offs_struct_h;
my $autoregs_file_structs_inits;
my $autoregs_count;

our $VERBOSE = '';
GetOptions ("named_init=s" => \$autoregs_named_init,
		"core_name=s" => \$core_name,
		"i_autoregs_offs_c=s" => \$autoregs_offs_c,
		"i_autoregs_structs_inits=s" => \$autoregs_file_structs_inits,
		"i_autoregs_count=s" => \$autoregs_count,
		"o_autoregs_offs_struct_h=s" => \$autoregs_offs_struct_h);

if ($autoregs_count > 1) {
	print "NOT generating structs for revisions:$autoregs_count being > 1\n";
	exit(0);
}

my $CORE_NAME = uc $core_name;

open(FILE_AUTOREGS_OFFS_C, $autoregs_offs_c) ||
	die "$0: Error: $autoregs_offs_c: $!";
open(FILE_AUTOREGS_STRUCTS_INITS, $autoregs_file_structs_inits) ||
	die "$0: Error: $autoregs_file_structs_inits: $!";
open(FILE_AUTOREGS_OFFS_STRUCT_H, '>'.$autoregs_offs_struct_h) ||
	die "$0: Error: $autoregs_offs_struct_h: $!";

# Trim function - remove leading and trailing whitespace
sub trim($)
{
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}

sub autoregs_print {
	print("DEBUG: $_[0]\n") if $VERBOSE;
}

my @offs_tbl;
my $temp_line;
my $name;
my $offs;
my @sizes;
my @sizes_check_name;
my $idx = 0;

#parse FILE_AUTOREGS_STRUCTS_INITS line by line to get sizes
while(my $line = <FILE_AUTOREGS_STRUCTS_INITS>) {
	# Skip the blank lines
	if ($line =~ /^\s*$/) {
		next;
	}

	# match line which contains offsets
	if ($line =~ /\.(\w+)\s=\s(\w+),\s\/\*\s(\w+)\s\*\//) {
		push @sizes, $3;
		push @sizes_check_name, $1;
	}
}

my @macros;
my $sz_macro;
my $reg_name;

#parse FILE_AUTOREGS_OFFS_C line by line to make an array with offsets
while(my $line = <FILE_AUTOREGS_OFFS_C>) {
	# Skip the blank lines
	if ($line =~ /^\s*$/) {
		next;
	}

	# match line which contains offsets
	if ($line =~ /\s\.(\w+)\s=\s\((\w+)\)/) {
		$name = $1;
		$offs = sprintf('%4x', hex($2));
		autoregs_print "$1 -> $2  $sizes[$idx] $sizes_check_name[$idx]\n";
		# create tbls with format: $offs $name $sizes[$idx] $sizes_check_name[$idx]
		$temp_line = "$offs $name $sizes[$idx] $sizes_check_name[$idx]\n";
		push @offs_tbl, $temp_line;
		# double check
		if ($name ne $sizes_check_name[$idx]) {
			die "ERROR:${core_name}regs:name_mismatch: $name, $sizes_check_name[$idx]!\n";
		}
		$sz_macro = $sizes[$idx];
		$reg_name = substr($name, 0, length($name)-3); # remove _ID from end
		# write macros
		push @macros, "#define ${CORE_NAME}_$reg_name(ptr) ((volatile $sz_macro *)&((ptr)->regs->$name))\n";
		push @macros, "#define ${CORE_NAME}_${reg_name}_ALTBASE(base, offstbl) ((volatile $sz_macro *)(((volatile uint8*) (base)) + OFFSETOF(${core_name}regs_t, $name)))\n";
		push @macros, "#define ${CORE_NAME}_${reg_name}_OFFSET(ptr) (OFFSETOF(${core_name}regs_t, $name))\n";
		$idx = $idx + 1;
	}
	# match invalid's to increase idx
	if ($line =~ /\s\.(\w+)\s=\sINVALID/) {
		$reg_name = substr($1, 0, length($1)-3); # remove _ID from end;
		$sz_macro = "uint16";
		# write macros
		push @macros, "#define ${CORE_NAME}_$reg_name(ptr) ((volatile $sz_macro *)&((ptr)->regs->INVALID_ID))\n";
		push @macros, "#define ${CORE_NAME}_${reg_name}_ALTBASE(base, offstbl) ((volatile $sz_macro *)(((volatile uint8*) (base)) + OFFSETOF(${core_name}regs_t, INVALID_ID)))\n";
		push @macros, "#define ${CORE_NAME}_${reg_name}_OFFSET(ptr) (OFFSETOF(${core_name}regs_t, INVALID_ID))\n";
		$idx = $idx + 1;
	}
}
# write invalid field corr to invalid offset [0xffff] to structure
$offs = sprintf('%4x', hex("0x0ffc"));
$temp_line = "$offs INVALID_ID uint16 INVALID_ID\n";
push @offs_tbl, $temp_line;

# sort tbl
@offs_tbl = sort @offs_tbl;
$offs = 0;
my $offs_hex = 0;
my $prev_offs_hex = 0;
my $prev_sz = 0;
my $cur_offs_pad_str;
my $cur_offs_str;

# start writing into file
print FILE_AUTOREGS_OFFS_STRUCT_H "#ifndef _${CORE_NAME}REGS_OFFS_STRUCT_H\n";
print FILE_AUTOREGS_OFFS_STRUCT_H "#define _${CORE_NAME}REGS_OFFS_STRUCT_H\n\n";
print FILE_AUTOREGS_OFFS_STRUCT_H "#include \"${core_name}regs_declarations.h\"\n\n";
print FILE_AUTOREGS_OFFS_STRUCT_H "typedef volatile struct _${core_name}regs {\n";

foreach my $line (@offs_tbl) {
	# format: $offs $name $sizes[$idx] $sizes_check_name[$idx]
	if ($line =~ /(\w+)\s(\w+)\s(\w+)\s(\w+)/) {
		$offs_hex = hex($1);
		autoregs_print "sorted:$1 $2 $3 $4\n";
		my $name = $2; # DONOT substr($2, 0, length($2)-3); # remove _ID from end
		my $sz_to_pad = ($offs_hex - $prev_offs_hex) - $prev_sz;
		# check if sz_to_pad is > 0 and even bytes
		$sz_to_pad = $sz_to_pad/2;
		$cur_offs_pad_str = sprintf('0x%04x', $prev_offs_hex + $prev_sz);
		# check if padding required
		if ($sz_to_pad > 0) {
			print FILE_AUTOREGS_OFFS_STRUCT_H "\tuint16 PAD[$sz_to_pad]; /* $cur_offs_pad_str */\n";
		}
		# put the struct element
		$cur_offs_str = sprintf('0x%04x', $offs_hex);
		print FILE_AUTOREGS_OFFS_STRUCT_H "\t$3 $name; /* $cur_offs_str */\n";
		$prev_sz = ($3 eq "uint16") ? 2 : (
				($3 eq "uint32") ? 4 : 0);
		if ($prev_sz == 0) {
			die "ERROR:${core_name}regs:size_error for: $name!\n";
		}
		$prev_offs_hex = $offs_hex;
	}
}
print FILE_AUTOREGS_OFFS_STRUCT_H "} ${core_name}regs_t;\n";

# write macros
print FILE_AUTOREGS_OFFS_STRUCT_H @macros;

print FILE_AUTOREGS_OFFS_STRUCT_H "\n#endif /* _${CORE_NAME}REGS_OFFS_STRUCT_H */\n";

close FILE_AUTOREGS_STRUCTS_INITS;
close FILE_AUTOREGS_OFFS_STRUCT_H;
close FILE_AUTOREGS_OFFS_C;

