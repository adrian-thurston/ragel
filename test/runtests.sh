#!/bin/bash

#
#   Copyright 2006-2015 Adrian Thurston <thurston@complang.org>
#

#   This file is part of Ragel.
#
#   Ragel is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   Ragel is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Ragel; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

function sig_exit()
{
	echo
	exit 1;
}

trap sig_exit SIGINT
trap sig_exit SIGQUIT
trap sig_exit SIGTERM

wk=working
test -d $wk || mkdir $wk

while getopts "gcnmleB:T:F:G:P:CDJRAZO-:" opt; do
	case $opt in
		B|T|F|G|P) 
			genflags="$genflags -$opt$OPTARG"
			gen_opts="$gen_opts -$opt$OPTARG"
		;;
		n|m|l|e) 
			minflags="$minflags -$opt"
			gen_opts="$gen_opts -$opt"
		;;
		c) 
			compile_only="true"
			gen_opts="$gen_opts -$opt"
		;;
		g) 
			allow_generated="true"
		;;
		C|D|J|R|A|Z|O)
			langflags="$langflags -$opt"
		;;
		-)
			case $OPTARG in
				asm)
					langflags="$langflags --$OPTARG"
					gen_opts="$gen_opts --$OPTARG"
				;;
				integral-tables)
					encflags="$encflags --$OPTARG"
					gen_opts="$gen_opts --$OPTARG"
				;;
				string-tables)
					encflags="$encflags --$OPTARG"
					gen_opts="$gen_opts --$OPTARG"
				;;
				*)
					echo "$0: unrecognized option --$OPTARG" >&2
					exit 1
				;;
			esac
		;;
	esac
done

# Prohibitied genflags for specific languages.
cs_prohibit_genflags="-G2"
java_prohibit_genflags="-T1 -F0 -F1 -G0 -G1 -G2"
ruby_prohibit_genflags="-F0 -F1 -G0 -G1 -G2"
ocaml_prohibit_genflags="-F0 -F1 -G0 -G1 -G2"
asm_prohibit_genflags="-T0 -T1 -F0 -F1 -G0 -G1"

[ -z "$minflags" ] && minflags="-n -m -l -e"
[ -z "$genflags" ] && genflags="-T0 -T1 -F0 -F1 -G0 -G1 -G2"
[ -z "$encflags" ] && encflags="--integral-tables --string-tables"
[ -z "$langflags" ] && langflags="-C -D -J -R -A -Z -O --asm"

shift $((OPTIND - 1));

[ -z "$*" ] && set -- *.rl

ragel="@SUBJECT@"

cxx_compiler="@CXX@"
c_compiler="@CC@"
objc_compiler="@GOBJC@"
d_compiler="@GDC@"
java_compiler="@JAVAC@"
ruby_engine="@RUBY@"
csharp_compiler="@GMCS@"
go_compiler="@GOBIN@"
ocaml_compiler="@OCAML@"

#
# Remove any unsupported host languages.
#
supported_host_langs=`$ragel --host-lang-args`

function test_error
{
	exit 1;
}

function run_test()
{
	echo "$ragel -I. $lang_opt $min_opt $gen_opt $enc_opt -o $wk/$code_src $translated"
	if ! $ragel -I. $lang_opt $min_opt $gen_opt $enc_opt -o $wk/$code_src $translated; then
		test_error;
	fi

	out_args=""
	[ $lang != java ] && out_args="-o $wk/$binary";
	[ $lang == csharp ] && out_args="-out:$wk/$binary";

	# Ruby and OCaml don't need to be copiled.
	if [ $lang != ruby ] && [ $lang != ocaml ]; then
		echo "$compiler $flags $out_args $wk/$code_src"
		if ! $compiler $flags $out_args $wk/$code_src; then
			test_error;
		fi
	fi

	if [ "$compile_only" != "true" ]; then
		
		exec_cmd=./$wk/$binary
		[ $lang = java ] && exec_cmd="java -classpath $wk ${root}"
		[ $lang = ruby ] && exec_cmd="ruby $wk/$code_src"
		[ $lang = csharp ] && exec_cmd="mono $wk/$binary"
		[ $lang = ocaml ] && exec_cmd="ocaml $wk/$code_src"

		echo -n "running $exec_cmd ... ";

		$exec_cmd 2>&1 > $wk/$output;
		EXIT_STATUS=$?
		if test $EXIT_STATUS = 0 && \
				diff --strip-trailing-cr $wk/$expected_out $wk/$output > /dev/null;
		then
			echo "passed";
		else
			echo "FAILED";
			test_error;
		fi;
	fi
}

function run_options()
{
	translated=$1

	root=`basename $translated`
	root=${root%.rl};

	# maybe translated to multiple targets, re-read each lang.
	lang=`sed '/@LANG:/s/^.*: *//p;d' $translated`

	case $lang in
		c)
			lang_opt=-C;
			code_suffix=c;
			compiler=$c_compiler;
			flags="-pedantic -ansi -Wall -O3 -I. -Wno-variadic-macros"
		;;
		c++)
			lang_opt=-C;
			code_suffix=cpp;
			compiler=$cxx_compiler;
			flags="-pedantic -ansi -Wall -O3 -I. -Wno-variadic-macros"
		;;
		obj-c)
			lang_opt=-C;
			code_suffix=m;
			compiler=$objc_compiler
			flags="-Wall -O3 -fno-strict-aliasing -lobjc"
		;;
		d)
			lang_opt=-D;
			code_suffix=d;
			compiler=$d_compiler;
			flags="-Wall -O3"
		;;
		java)
			lang_opt=-J;
			code_suffix=java;
			compiler=$java_compiler
			flags=""
		;;
		ruby)
			lang_opt=-R;
			code_suffix=rb;
			compiler=$ruby_engine
			flags=""
		;;
		csharp)
			lang_opt="-A";
			code_suffix=cs;
			compiler=$csharp_compiler
			flags=""
		;;
		go)
			lang_opt="-Z"
			code_suffix=go
			compiler=$go_compiler
			flags="build"
		;;
		ocaml)
			lang_opt="-O"
			code_suffix=ml
			compiler=$ocaml_compiler
			flags=""
		;;
		asm)
			lang_opt="--asm"
			code_suffix=s
			compiler="gcc"
			flags=""
		;;
		indep)
		;;
		*)
			echo "$translated: unknown language type $lang" >&2
			exit 1;
		;;
	esac


	# Make sure that we are interested in the host language.
	echo "$langflags" | grep -qe $lang_opt || return

	# Make sure that ragel supports the host language
	echo "$supported_host_langs" | grep -qe $lang_opt || return

	code_src=$root.$code_suffix;
	binary=$root.bin;
	output=$root.out;

	# If we have no compiler for the source program then skip it.
	[ -z "$compiler" ] && continue

	[ -n "$additional_cflags" ] && flags="$flags $additional_cflags"

	case $lang in
	csharp) lang_prohibit_genflags="$prohibit_genflags $cs_prohibit_genflags";;
	java) lang_prohibit_genflags="$prohibit_genflags $java_prohibit_genflags";;
	ruby) lang_prohibit_genflags="$prohibit_genflags $ruby_prohibit_genflags";;
	ocaml) lang_prohibit_genflags="$prohibit_genflags $ocaml_prohibit_genflags";;
	asm) lang_prohibit_genflags="$prohibit_genflags $asm_prohibit_genflags";;
	*) lang_prohibit_genflags="$prohibit_genflags";;
	esac

	if [ $lang != c ] && [ $lang != c++ ]; then
		prohibit_encflags="--string-tables"
	fi

	# Eh, need to remove this.
	[ $lang == obj-c ] && continue;

	for min_opt in $minflags; do
		echo "" "$prohibit_minflags" | grep -e $min_opt >/dev/null && continue
		for gen_opt in $genflags; do
			echo "" "$lang_prohibit_genflags" | grep -e $gen_opt >/dev/null && continue
			for enc_opt in $encflags; do
				echo "" "$prohibit_encflags" | grep -e $enc_opt >/dev/null && continue
				run_test
			done
		done
	done
}

function run_translate()
{
	test_case=$1

	# Recompute the root.
	root=`basename $test_case`
	root=${root%.rl};

	if ! [ -f "$test_case" ]; then
		echo "runtests: not a file: $test_case"; >&2
		exit 1;
	fi

	# Check if we should ignore the test case
	ignore=`sed '/@IGNORE:/s/^.*: *//p;d' $test_case`
    if [ "$ignore" = yes ]; then
        continue;
    fi

	# If the generated flag is given make sure that the test case is generated.
	is_generated=`sed '/@GENERATED:/s/^.*: *//p;d' $test_case`
	if [ "$is_generated" = yes ] && [ "$allow_generated" != true ]; then
		continue;
	fi

	expected_out=$root.exp;
	case_rl=${root}.rl

	prohibit_minflags=`sed '/@PROHIBIT_MINFLAGS:/s/^.*: *//p;d' $test_case`
	prohibit_genflags=`sed '/@PROHIBIT_GENFLAGS:/s/^.*: *//p;d' $test_case`
	prohibit_languages=`sed '/@PROHIBIT_LANGUAGES:/s/^.*: *//p;d' $test_case`

	# Create the expected output.
	sed '1,/^#\+ * OUTPUT #\+/d;' $test_case > $wk/$expected_out

	additional_cflags=`sed '/@CFLAGS:/s/^.*: *//p;d' $test_case`

	lang=`sed '/@LANG:/s/^.*: *//p;d' $test_case`
	if [ -z "$lang" ]; then
		echo "$test_case: language unset"; >&2
		exit 1;
	fi

	cases=""

	if [ $lang == indep ]; then
		for lang in c asm d cs go java ruby ocaml; do
			case $lang in 
				c) lf="-C" ;;
				asm) lf="--asm" ;;
				d) lf="-D" ;;
				cs) lf="-A" ;;
				go) lf="-Z" ;;
				java) lf="-J" ;;
				ruby) lf="-R" ;;
				ocaml) lf="-O" ;;
			esac

			echo "$prohibit_languages" | grep -q "\<$lang\>" && continue;
			echo "$langflags" | grep -qe $lf || continue
			echo "$supported_host_langs" | grep -qe $lf || continue

			# Translate to target language and strip off output.
			targ=${root}_$lang.rl
			echo "./trans $lang $wk/$targ $test_case ${root}_${lang}"
			if ! ./trans $lang $wk/$targ $test_case ${root}_${lang}; then
				test_error
			fi

			cases="$cases $wk/$targ"
		done
	else
		sed '/^#\+ * OUTPUT #\+/,$d' $test_case > $wk/$case_rl
		cases=$wk/$case_rl
	fi

	for translated in $cases; do
		run_options $translated
	done
}

for test_case; do
	run_translate $test_case
done
