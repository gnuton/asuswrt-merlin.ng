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
#	File Name:	autoregs.pl
#	<<Broadcom-WL-IPTag/Proprietary:>>
#	Abstract:
#		 [similar to d11shm counterpart]
#
#	$History:$
#
#********************************************************************
#
# Auto-generation of the following files:
#       - autoregs_defaults.h         - Contains the default defs of regdefs
#       - autoregs_structs_decl.h     - Contains the feature specific regdefs structures
#       - autoregs_func_decl.h        - Declarations of the above functions
#       - autoregs_declarations.h     - Declarations of regdefs pointers
#       - autoregs_structs_inits.c    - Contains the inits of feature specific regdefs structures
#       - autoregs_main_structs.c     - Functions for switching the regdefs structures
#       - autoregs.h                  - Contains M_* macros used in the code
#       - autoregs_regdefs_t.h        - Contains the main regdefs structure
#
# Usage: perl autoregs.pl = --core_rev=<core_rev> \
#                       --named_init=<named init of structs> \
#			--i_autoregs_template=<autoregs template file> \
#			--i_autoregs_count=<number of revisions> \
#			--o_autoregs_defaults=autoregs_defaults.h \
#			--o_autoregs_structs_decl=autoregs_structs_decl.h \
#			--o_autoregs_regdefs_t=autoregs_regdefs_t.h \
#			--o_autoregs_structs_inits=autoregs_structs_inits.c \
#			--o_autoregs_main_structs=autoregs_main_structs.c \
#			--o_autoregs_declarations=autoregs_declarations.h \
#			--o_autoregs_hdr=autoregs.h\
#			--o_autoregs_func_partial=autoregs_func_<ucode type>_partial.c
#

use strict;
use warnings;
use Getopt::Long;

my $core_rev;
my $core_name;
my $autoregs_named_init;

my $autoregs_file_template;
my $autoregs_file_defaults;
my $autoregs_file_structs_decl;
my $autoregs_file_regdefs_t;
my $autoregs_file_structs_inits;
my $autoregs_file_main_structs;
my $autoregs_file_declarations;
my $autoregs_file_hdr;
my $autoregs_file_func_partial;
my $autoregs_count;

our $VERBOSE = '';
GetOptions ("verbose=s" => \$VERBOSE,
		"core_rev=s" => \$core_rev,
		"core_name=s" => \$core_name,
		"named_init=s" => \$autoregs_named_init,
		"i_autoregs_count=s" => \$autoregs_count,
		"i_autoregs_template=s" => \$autoregs_file_template,
		"o_autoregs_defaults=s" => \$autoregs_file_defaults,
		"o_autoregs_structs_decl=s" => \$autoregs_file_structs_decl,
		"o_autoregs_regdefs_t=s" => \$autoregs_file_regdefs_t,
		"o_autoregs_structs_inits=s" => \$autoregs_file_structs_inits,
		"o_autoregs_main_structs=s" => \$autoregs_file_main_structs,
		"o_autoregs_declarations=s" => \$autoregs_file_declarations,
		"o_autoregs_hdr=s" => \$autoregs_file_hdr,
		"o_autoregs_func_partial=s" => \$autoregs_file_func_partial);


my $CORE_NAME = uc $core_name;

my $autoregs_onetime = 1;
my $autoregs_func = join "_", "${core_name}regs_struct", "${core_name}rev", $core_rev;

if (-e $autoregs_file_structs_inits) {
	$autoregs_onetime = 0;
}

open(FILE_AUTOREGS_TPL, $autoregs_file_template) ||
	die "$0: Error: $autoregs_file_template: $!";
open(FILE_AUTOREGS_STRUCTS_INITS, '>'.$autoregs_file_structs_inits) ||
	die "$0: Error: $autoregs_file_structs_inits: $!";
open(FILE_AUTOREGS_MAIN_STRUCTS, '>>'.$autoregs_file_main_structs) ||
	die "$0: Error: $autoregs_file_main_structs: $!";
open(FILE_AUTOREGS_FUNC_PARTIAL, '>>'.$autoregs_file_func_partial) ||
	die "$0: Error: $autoregs_file_func_partial: $!";

if ($autoregs_onetime == 1) {
	open(FILE_AUTOREGS_DEFAULTS, '>'.$autoregs_file_defaults) ||
		die "$0: Error: $autoregs_file_defaults: $!";
	open(FILE_AUTOREGS_STRUCTS_DECL, '>'.$autoregs_file_structs_decl) ||
		die "$0: Error: $autoregs_file_structs_decl: $!";
	open(FILE_AUTOREGS_REGDEFS_T, '+>'.$autoregs_file_regdefs_t) ||
		die "$0: Error: $autoregs_file_regdefs_t: $!";
	open(FILE_AUTOREGS_DECL, '>'.$autoregs_file_declarations) ||
		die "$0: Error: $autoregs_file_declarations: $!";
	open(FILE_AUTOREGS_HDR, '>'.$autoregs_file_hdr) ||
		die "$0: Error: $autoregs_file_hdr: $!";
}

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

my $regs;
my $regs_id;
my $regs_line;
my $regs_type;
my $in_feature_blk = 0;
my $feature;
my $feature_t;
my $feature_ptr;
my $feature_struct;
my $config;
my $config_present = 0;

print FILE_AUTOREGS_MAIN_STRUCTS "\n\/\* regdefs structure for $core_name rev$core_rev \*\/\n";
if ($autoregs_count == 1) {
	print FILE_AUTOREGS_MAIN_STRUCTS "\n#ifndef DONGLEBUILD\n";
}
print FILE_AUTOREGS_MAIN_STRUCTS "static const ${core_name}regdefs_t $autoregs_func = \n\{\n";

print FILE_AUTOREGS_FUNC_PARTIAL "\tcase $core_rev\:\n";
print FILE_AUTOREGS_FUNC_PARTIAL "\t\t*regdefs = &$autoregs_func;\n";
print FILE_AUTOREGS_FUNC_PARTIAL "\t\tbreak;\n";

if ($autoregs_onetime == 1) {
	print FILE_AUTOREGS_DEFAULTS "#ifndef _${CORE_NAME}REGS_DEFAULTS_H\n";
	print FILE_AUTOREGS_DEFAULTS "#define _${CORE_NAME}REGS_DEFAULTS_H\n\n";

	print FILE_AUTOREGS_STRUCTS_DECL "#ifndef _${CORE_NAME}REGS_STRUCTS_DECL_H\n";
	print FILE_AUTOREGS_STRUCTS_DECL "#define _${CORE_NAME}REGS_STRUCTS_DECL_H\n\n";
	print FILE_AUTOREGS_STRUCTS_DECL "#include <typedefs.h>\n\n";

	print FILE_AUTOREGS_DECL "#ifndef _${CORE_NAME}REGS_DECLARATIONS_H\n";
	print FILE_AUTOREGS_DECL "#define _${CORE_NAME}REGS_DECLARATIONS_H\n\n";
	print FILE_AUTOREGS_DECL "#include \"${core_name}regs_structs_decl.h\"\n";
	print FILE_AUTOREGS_DECL "#include \"${core_name}regs_func_decl.h\"\n\n";

	print FILE_AUTOREGS_HDR "#ifndef _${CORE_NAME}REGS_HDR_H\n";
	print FILE_AUTOREGS_HDR "#define _${CORE_NAME}REGS_HDR_H\n\n";
	print FILE_AUTOREGS_HDR "#include \"${core_name}regs_declarations.h\"\n\n";
	print "GENERATING FOR $autoregs_count corerevs\n";
	# for dongle build which has only 1 revision, there is a special processing to
	# generate reg structure and corresponding macros
	if ($autoregs_count == 1) {
		print FILE_AUTOREGS_HDR "#ifdef DONGLEBUILD\n\n";
		print FILE_AUTOREGS_HDR "#include \"${core_name}regs_offs_struct.h\"\n\n";
		print FILE_AUTOREGS_HDR "#else /* DONGLEBUILD */\n\n";
	}
	print FILE_AUTOREGS_HDR "typedef volatile uint8 ${core_name}regs_t;\n\n";
}


print FILE_AUTOREGS_STRUCTS_INITS "#include \"${core_name}regs_structs_decl.h\"\n";
print FILE_AUTOREGS_STRUCTS_INITS "#include \"${core_name}regs_defaults.h\"\n\n";

if ($autoregs_onetime == 1) {
	print FILE_AUTOREGS_REGDEFS_T "typedef struct ${core_name}regdefs_struct {\n";
	if ($autoregs_count == 1) {
		print FILE_AUTOREGS_REGDEFS_T "#ifndef DONGLEBUILD\n";
	}
}

while(my $line = <FILE_AUTOREGS_TPL>) {
	# Skip the blank lines
	if ($line =~ /^\s*$/) {
		next;
	}

	# Search of begin of feature block
	if ($line =~ /FEATURE_BEGIN/) {
		if ($in_feature_blk == 1) {
			die "\nFEATURE_END: nested blocks not allowed!\n";
		}

		# Extracting name of feature block
		$line =~ m/FEATURE_BEGIN\[(.*)\]/;
		$feature = trim($1);
		$feature_t = join "", "${core_name}regs_", $feature, "_t";
		$feature_ptr = join "", $feature, "_ptr";
		$feature_struct = join "_", $feature, $core_rev;

		autoregs_print("Found begin of feature block:[$feature]");
		$in_feature_blk = 1;

		if ($autoregs_onetime == 1) {
			if ($autoregs_count == 1) {
				print FILE_AUTOREGS_STRUCTS_DECL "#ifndef DONGLEBUILD\n";
			}
			print FILE_AUTOREGS_STRUCTS_DECL "typedef struct {\n";
		}

		# Search for config string of the feature block
		$line = <FILE_AUTOREGS_TPL>;

		if ($line =~ m/IF\[.*\]/) {
			$line =~ m/IF\[(.*)\]/;
			$config = $1;
			autoregs_print("Found config for this block: $config");

			print FILE_AUTOREGS_STRUCTS_INITS "#if $config\n";
			print FILE_AUTOREGS_STRUCTS_INITS "static const $feature_t $feature_struct = {\n";
			$config_present = 1;
			next;
		} else {
			print FILE_AUTOREGS_STRUCTS_INITS "static const $feature_t $feature_struct = {\n";
			$config_present = 0
		}
	}

	# Search for end of feature block
	if ($line =~ /FEATURE_END/) {
		if ($in_feature_blk == 0) {
			die "\nFEATURE_END: nested blocks not allowed!\n";
		}

		if ($autoregs_onetime == 1) {
			print FILE_AUTOREGS_STRUCTS_DECL "} $feature_t;\n\n";
			print FILE_AUTOREGS_REGDEFS_T "\tconst $feature_t *$feature;\n";
			if ($autoregs_count == 1) {
				print FILE_AUTOREGS_STRUCTS_DECL "#endif /* DONGLEBUILD */\n";
			}
		}

		print FILE_AUTOREGS_STRUCTS_INITS "};\n";
		if ($config_present == 1) {
			print FILE_AUTOREGS_STRUCTS_INITS "#endif\n";
			if ($autoregs_onetime == 1) {
				print FILE_AUTOREGS_STRUCTS_INITS "const $feature_t *$feature_ptr =\n";
				print FILE_AUTOREGS_STRUCTS_INITS "#if $config\n";
				print FILE_AUTOREGS_STRUCTS_INITS "&$feature_struct;\n";
				print FILE_AUTOREGS_STRUCTS_INITS "#else\n";
				print FILE_AUTOREGS_STRUCTS_INITS "NULL;\n";
				print FILE_AUTOREGS_STRUCTS_INITS "#endif\n\n\n";
			}

			if ($autoregs_named_init eq "1") {
				print FILE_AUTOREGS_MAIN_STRUCTS "\t.$feature = \n";
			}
			print FILE_AUTOREGS_MAIN_STRUCTS "\t#if $config\n";
			print FILE_AUTOREGS_MAIN_STRUCTS "\t&$feature_struct,\n";
			print FILE_AUTOREGS_MAIN_STRUCTS "\t#else\n";
			print FILE_AUTOREGS_MAIN_STRUCTS "\tNULL,\n";
			print FILE_AUTOREGS_MAIN_STRUCTS "\t#endif\n";
		} else {
			if ($autoregs_onetime == 1) {
				print FILE_AUTOREGS_STRUCTS_INITS "const $feature_t *$feature_ptr = &$feature_struct;\n\n\n";
			}

			if ($autoregs_named_init eq "1") {
				print FILE_AUTOREGS_MAIN_STRUCTS "\t.$feature = &$feature_struct,\n";
			} else {
				print FILE_AUTOREGS_MAIN_STRUCTS "\t&$feature_struct,\n";
			}
		}

		autoregs_print("Found end of feature block");
		$in_feature_blk = 0;
		$config_present = 0;
		next;
	}

	# Skip the lines outside the feature block
	if ($in_feature_blk == 0) {
	       next;
	}

	# Strip out the comments and white spaces
	$regs_line = $line;
	$regs_line =~ s/\#.*//;
	$regs_line = trim($regs_line);

	if ($regs_line =~ /(\w+).(\w+)/) {
		$regs = $2;
		$regs_type = $1;
	}
	$regs_id = join "", $regs, "_ID";

	autoregs_print(" \tFound regs: $regs");

	if ($autoregs_onetime == 1) {
		print FILE_AUTOREGS_STRUCTS_DECL "\tuint16 $regs_id;\n";
	}

	if ($autoregs_named_init eq "1") {
		print FILE_AUTOREGS_STRUCTS_INITS "\t.$regs_id = $regs, /* $regs_type */\n";
	} else {
		print FILE_AUTOREGS_STRUCTS_INITS "\t$regs,\n";
	}

	if ($autoregs_onetime == 1) {
		print FILE_AUTOREGS_HDR "#define ${CORE_NAME}_$regs(ptr) ((volatile $regs_type *)(((volatile uint8*) ((ptr)->regs)) + ((ptr)->regoffsets->$feature->$regs_id)))\n";
		print FILE_AUTOREGS_HDR "#define ${CORE_NAME}_${regs}_ALTBASE(base, offstbl) ((volatile $regs_type *)(((volatile uint8*) (base)) + (offstbl->$feature->$regs_id)))\n";
		print FILE_AUTOREGS_HDR "#define ${CORE_NAME}_${regs}_OFFSET(ptr) ((ptr)->regoffsets->$feature->$regs_id)\n";

		print FILE_AUTOREGS_DEFAULTS "#ifndef $regs\n";
		print FILE_AUTOREGS_DEFAULTS "#define $regs INVALID\n";
		print FILE_AUTOREGS_DEFAULTS "#endif\n";
	}
}

print FILE_AUTOREGS_MAIN_STRUCTS "\};\n";
if ($autoregs_count == 1) {
	print FILE_AUTOREGS_MAIN_STRUCTS "\n#endif /* DONGLEBUILD */\n";
}

if ($autoregs_onetime == 1) {
	if ($autoregs_count == 1) {
		print FILE_AUTOREGS_REGDEFS_T "#endif /* DONGLEBUILD */\n";
	}
	print FILE_AUTOREGS_REGDEFS_T "} ${core_name}regdefs_t;\n";

	# Copy regdefs_t structure to ${core_name}regs_structs_decl.h
	seek FILE_AUTOREGS_REGDEFS_T, 0, 0;
	foreach(<FILE_AUTOREGS_REGDEFS_T>){
		 print FILE_AUTOREGS_STRUCTS_DECL $_;
	}
}

if ($autoregs_onetime == 1) {
	print FILE_AUTOREGS_DEFAULTS "\n#endif\n";
	print FILE_AUTOREGS_STRUCTS_DECL "\n#endif\n";
	print FILE_AUTOREGS_DECL "\n#endif\n";
	if ($autoregs_count == 1) {
		print FILE_AUTOREGS_HDR "#endif /* else-DONGLEBUILD */\n\n";
	}
	print FILE_AUTOREGS_HDR "\n#endif\n";

	close FILE_AUTOREGS_DEFAULTS;
	close FILE_AUTOREGS_DECL;
	close FILE_AUTOREGS_HDR;

	close FILE_AUTOREGS_REGDEFS_T;
}

close FILE_AUTOREGS_TPL;
close FILE_AUTOREGS_STRUCTS_INITS;
close FILE_AUTOREGS_FUNC_PARTIAL;
