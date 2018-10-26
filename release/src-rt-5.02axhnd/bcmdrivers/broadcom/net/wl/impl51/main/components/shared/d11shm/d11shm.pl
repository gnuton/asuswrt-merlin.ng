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
#	File Name:	d11shm.pl
#	Abstract:
#
#
#	$History:$
#
#********************************************************************
#
# Auto-generation of the following files:
#       - d11shm_defaults.h         - Contains the default defs of shmdefs
#       - d11shm_structs_decl.h     - Contains the feature specific shmdefs structures
#       - d11shm_func_decl.h        - Declarations of the above functions
#       - d11shm_declarations.h     - Declarations of shmdefs pointers
#       - d11shm_structs_inits.c    - Contains the inits of feature specific shmdefs structures
#       - d11shm_main_structs.c     - Functions for switching the shmdefs structures
#       - d11shm.h                  - Contains M_* macros used in the code
#       - d11shm_shmdefs_t.h        - Contains the main shmdefs structure
#
# Usage: perl d11shm.pl = --ucode_type=<ucode type> --d11rev=<d11 rev> \
#                       --named_init=<named init of structs> \
#			--i_d11shm_template=<d11shm template file> \
#			--o_d11shm_defaults=d11shm_defaults.h \
#			--o_d11shm_structs_decl=d11shm_structs_decl.h \
#			--o_d11shm_shmdefs_t=d11shm_shmdefs_t.h \
#			--o_d11shm_structs_inits=d11shm_structs_inits.c \
#			--o_d11shm_main_structs=d11shm_main_structs.c \
#			--o_d11shm_declarations=d11shm_declarations.h \
#			--o_d11shm_hdr=d11shm.h\
#			--o_d11shm_func_partial=d11shm_func_<ucode type>_partial.c
#

use strict;
use warnings;
use Getopt::Long;

my $ucode_type;
my $d11rev;
my $d11shm_named_init;

my $d11shm_file_template;
my $d11shm_file_defaults;
my $d11shm_file_structs_decl;
my $d11shm_file_shmdefs_t;
my $d11shm_file_structs_inits;
my $d11shm_file_main_structs;
my $d11shm_file_declarations;
my $d11shm_file_hdr;
my $d11shm_file_func_partial;

our $VERBOSE = '';
GetOptions ("verbose=s" => \$VERBOSE,
		"ucode_type=s" => \$ucode_type,
		"d11rev=s" => \$d11rev,
		"named_init=s" => \$d11shm_named_init,
		"i_d11shm_template=s" => \$d11shm_file_template,
		"o_d11shm_defaults=s" => \$d11shm_file_defaults,
		"o_d11shm_structs_decl=s" => \$d11shm_file_structs_decl,
		"o_d11shm_shmdefs_t=s" => \$d11shm_file_shmdefs_t,
		"o_d11shm_structs_inits=s" => \$d11shm_file_structs_inits,
		"o_d11shm_main_structs=s" => \$d11shm_file_main_structs,
		"o_d11shm_declarations=s" => \$d11shm_file_declarations,
		"o_d11shm_hdr=s" => \$d11shm_file_hdr,
		"o_d11shm_func_partial=s" => \$d11shm_file_func_partial);

my $d11shm_onetime = 1;
my $d11shm_func = join "_", "d11shm_struct", $ucode_type, "d11rev", $d11rev;

if (-e $d11shm_file_structs_inits) {
	$d11shm_onetime = 0;
}

open(FILE_D11SHM_TPL, $d11shm_file_template) ||
	die "$0: Error: $d11shm_file_template: $!";
open(FILE_D11SHM_STRUCTS_INITS, '>'.$d11shm_file_structs_inits) ||
	die "$0: Error: $d11shm_file_structs_inits: $!";
open(FILE_D11SHM_MAIN_STRUCTS, '>>'.$d11shm_file_main_structs) ||
	die "$0: Error: $d11shm_file_main_structs: $!";
open(FILE_D11SHM_FUNC_PARTIAL, '>>'.$d11shm_file_func_partial) ||
	die "$0: Error: $d11shm_file_func_partial: $!";

if ($d11shm_onetime == 1) {
	open(FILE_D11SHM_DEFAULTS, '>'.$d11shm_file_defaults) ||
		die "$0: Error: $d11shm_file_defaults: $!";
	open(FILE_D11SHM_STRUCTS_DECL, '>'.$d11shm_file_structs_decl) ||
		die "$0: Error: $d11shm_file_structs_decl: $!";
	open(FILE_D11SHM_SHMDEFS_T, '+>'.$d11shm_file_shmdefs_t) ||
		die "$0: Error: $d11shm_file_shmdefs_t: $!";
	open(FILE_D11SHM_DECL, '>'.$d11shm_file_declarations) ||
		die "$0: Error: $d11shm_file_declarations: $!";
	open(FILE_D11SHM_HDR, '>'.$d11shm_file_hdr) ||
		die "$0: Error: $d11shm_file_hdr: $!";
}

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

my $shm;
my $shm_id;
my $in_feature_blk = 0;
my $feature;
my $feature_t;
my $feature_ptr;
my $feature_struct;
my $config;
my $config_present = 0;

print FILE_D11SHM_MAIN_STRUCTS "\n\/\* shmdefs structure for $ucode_type - $d11rev \*\/\n";
print FILE_D11SHM_MAIN_STRUCTS "static const shmdefs_t $d11shm_func = \n\{\n";

print FILE_D11SHM_FUNC_PARTIAL "\tcase $d11rev\:\n";
print FILE_D11SHM_FUNC_PARTIAL "\t\t*shmdefs = &$d11shm_func;\n";
print FILE_D11SHM_FUNC_PARTIAL "\t\tbreak;\n";

if ($d11shm_onetime == 1) {
	print FILE_D11SHM_DEFAULTS "#ifndef _D11SHM_DEFAULTS_H\n";
	print FILE_D11SHM_DEFAULTS "#define _D11SHM_DEFAULTS_H\n\n";

	print FILE_D11SHM_STRUCTS_DECL "#ifndef _D11SHM_STRUCTS_DECL_H\n";
	print FILE_D11SHM_STRUCTS_DECL "#define _D11SHM_STRUCTS_DECL_H\n\n";
	print FILE_D11SHM_STRUCTS_DECL "#include <typedefs.h>\n\n";

	print FILE_D11SHM_DECL "#ifndef _D11SHM_DECLARATIONS_H\n";
	print FILE_D11SHM_DECL "#define _D11SHM_DECLARATIONS_H\n\n";
	print FILE_D11SHM_DECL "#include \"d11shm_structs_decl.h\"\n";
	print FILE_D11SHM_DECL "#include \"d11shm_func_decl.h\"\n\n";

	print FILE_D11SHM_HDR "#ifndef _D11SHM_HDR_H\n";
	print FILE_D11SHM_HDR "#define _D11SHM_HDR_H\n\n";
	print FILE_D11SHM_HDR "#include \"d11shm_declarations.h\"\n\n";
}

print FILE_D11SHM_STRUCTS_INITS "#include \"d11shm_structs_decl.h\"\n";
print FILE_D11SHM_STRUCTS_INITS "#include \"d11shm_defaults.h\"\n\n";

if ($d11shm_onetime == 1) {
	print FILE_D11SHM_SHMDEFS_T "typedef struct shmdefs_struct {\n";
}

while(my $line = <FILE_D11SHM_TPL>) {
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
		$feature_t = join "", "d11shm_", $feature, "_t";
		$feature_ptr = join "", $feature, "_ptr";
		$feature_struct = join "_", $feature, $ucode_type, $d11rev;

		d11print("Found begin of feature block:[$feature]");
		$in_feature_blk = 1;

		if ($d11shm_onetime == 1) {
			print FILE_D11SHM_STRUCTS_DECL "typedef struct {\n";
		}

		# Search for config string of the feature block
		$line = <FILE_D11SHM_TPL>;

		if ($line =~ m/IF\[.*\]/) {
			$line =~ m/IF\[(.*)\]/;
			$config = $1;
			d11print("Found config for this block: $config");

			print FILE_D11SHM_STRUCTS_INITS "#if $config\n";
			print FILE_D11SHM_STRUCTS_INITS "static const $feature_t $feature_struct = {\n";
			$config_present = 1;
			next;
		} else {
			print FILE_D11SHM_STRUCTS_INITS "static const $feature_t $feature_struct = {\n";
			$config_present = 0
		}
	}

	# Search for end of feature block
	if ($line =~ /FEATURE_END/) {
		if ($in_feature_blk == 0) {
			die "\nFEATURE_END: nested blocks not allowed!\n";
		}

		if ($d11shm_onetime == 1) {
			print FILE_D11SHM_STRUCTS_DECL "} $feature_t;\n\n";
			print FILE_D11SHM_SHMDEFS_T "\tconst $feature_t *$feature;\n";
		}

		print FILE_D11SHM_STRUCTS_INITS "};\n";
		if ($config_present == 1) {
			print FILE_D11SHM_STRUCTS_INITS "#endif\n";
			if ($d11shm_onetime == 1) {
				print FILE_D11SHM_STRUCTS_INITS "const $feature_t *$feature_ptr =\n";
				print FILE_D11SHM_STRUCTS_INITS "#if $config\n";
				print FILE_D11SHM_STRUCTS_INITS "&$feature_struct;\n";
				print FILE_D11SHM_STRUCTS_INITS "#else\n";
				print FILE_D11SHM_STRUCTS_INITS "NULL;\n";
				print FILE_D11SHM_STRUCTS_INITS "#endif\n\n\n";
			}

			if ($d11shm_named_init eq "1") {
				print FILE_D11SHM_MAIN_STRUCTS "\t.$feature = \n";
			}
			print FILE_D11SHM_MAIN_STRUCTS "\t#if $config\n";
			print FILE_D11SHM_MAIN_STRUCTS "\t&$feature_struct,\n";
			print FILE_D11SHM_MAIN_STRUCTS "\t#else\n";
			print FILE_D11SHM_MAIN_STRUCTS "\tNULL,\n";
			print FILE_D11SHM_MAIN_STRUCTS "\t#endif\n";
		} else {
			if ($d11shm_onetime == 1) {
				print FILE_D11SHM_STRUCTS_INITS "const $feature_t *$feature_ptr = &$feature_struct;\n\n\n";
			}

			if ($d11shm_named_init eq "1") {
				print FILE_D11SHM_MAIN_STRUCTS "\t.$feature = &$feature_struct,\n";
			} else {
				print FILE_D11SHM_MAIN_STRUCTS "\t&$feature_struct,\n";
			}
		}

		d11print("Found end of feature block");
		$in_feature_blk = 0;
		$config_present = 0;
		next;
	}

	# Skip the lines outside the feature block
	if ($in_feature_blk == 0) {
	       next;
	}

	# Strip out the comments and white spaces
	$shm = $line;
	$shm =~ s/\#.*//;
	$shm = trim($shm);

	$shm_id = join "", $shm, "_ID";

	d11print(" \tFound SHM: $shm");

	if ($d11shm_onetime == 1) {
		print FILE_D11SHM_STRUCTS_DECL "\tuint16 $shm_id;\n";
	}

	if ($d11shm_named_init eq "1") {
		print FILE_D11SHM_STRUCTS_INITS "\t.$shm_id = $shm,\n";
	} else {
		print FILE_D11SHM_STRUCTS_INITS "\t$shm,\n";
	}

	if ($d11shm_onetime == 1) {
		print FILE_D11SHM_HDR "#define $shm(ptr) ((ptr)->shmdefs->$feature->$shm_id)\n";

		print FILE_D11SHM_DEFAULTS "#ifndef $shm\n";
		print FILE_D11SHM_DEFAULTS "#define $shm INVALID\n";
		print FILE_D11SHM_DEFAULTS "#endif\n";
	}
}

print FILE_D11SHM_MAIN_STRUCTS "\};\n";

if ($d11shm_onetime == 1) {
	print FILE_D11SHM_SHMDEFS_T "} shmdefs_t;\n";

	# Copy shmdefs_t structure to d11shm_structs_decl.h
	seek FILE_D11SHM_SHMDEFS_T, 0, 0;
	foreach(<FILE_D11SHM_SHMDEFS_T>){
		 print FILE_D11SHM_STRUCTS_DECL $_;
	}
}

if ($d11shm_onetime == 1) {
	print FILE_D11SHM_DEFAULTS "\n#endif\n";
	print FILE_D11SHM_STRUCTS_DECL "\n#endif\n";
	print FILE_D11SHM_DECL "\n#endif\n";
	print FILE_D11SHM_HDR "\n#endif\n";

	close FILE_D11SHM_DEFAULTS;
	close FILE_D11SHM_DECL;
	close FILE_D11SHM_HDR;

	close FILE_D11SHM_SHMDEFS_T;
}

close FILE_D11SHM_TPL;
close FILE_D11SHM_STRUCTS_INITS;
close FILE_D11SHM_FUNC_PARTIAL;
