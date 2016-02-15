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

while getopts "gcnmleB:T:F:G:P:CDJRAZOUKY-:" opt; do
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
		C|D|J|R|A|Z|O|R|K|Y|U)
			langflags="$langflags -$opt"
		;;
		-)
			case $OPTARG in
				asm)
					langflags="$langflags --$OPTARG"
					gen_opts="$gen_opts --$OPTARG"
				;;
				integral-tables|string-tables)
					encflags="$encflags --$OPTARG"
					gen_opts="$gen_opts --$OPTARG"
				;;
				kelbt-frontend|colm-frontend|reduce-frontend)
					frontflags="$frontflags --$OPTARG"
					gen_opts="$gen_opts --$OPTARG"
				;;
				direct-backend|colm-backend)
					backflags="$backflags --$OPTARG"
					gen_opts="$gen_opts --$OPTARG"
				;;
				var-backend|goto-backend)
					featflags="$featflags --$OPTARG"
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



[ -z "$minflags" ]    && minflags="-n -m -l -e"
[ -z "$genflags" ]    && genflags="-T0 -T1 -F0 -F1 -G0 -G1 -G2"
[ -z "$encflags" ]    && encflags="--integral-tables --string-tables"
[ -z "$langflags" ]   && langflags="-C -D -J -R -A -Z -O --asm -U -K -Y"
[ -z "$frontflags" ]  && frontflags="--kelbt-frontend --colm-frontend --reduce-frontend"
[ -z "$backflags" ]   && backflags="--direct-backend --colm-backend"
[ -z "$featflags" ]   && featflags="--var-backend --goto-backend"

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
rust_compiler="@RUST@"
crack_interpreter="@CRACK@"
julia_interpreter="@JULIA@"

#
# Remove any unsupported host languages.
#
supported_host_langs=`$ragel --supported-host-langs`
supported_frontends=`$ragel --supported-frontends`
supported_backends=`$ragel --supported-backends`

function test_error
{
	exit 1;
}

function run_test()
{
	opts="$lang_opt $min_opt $gen_opt $enc_opt $f_opt $b_opt $v_opt"
	args="-I. $opts -o $wk/$code_src $translated"
	echo "$ragel $args"
	if ! $ragel $args; then
		test_error;
	fi

	out_args=""
	[ $lang != java ] && out_args="-o $wk/$binary";
	[ $lang == csharp ] && out_args="-out:$wk/$binary";

	# Some langs are just interpreted.
	if [ $interpreted != "true" ]; then
		scode=""
		if [ -n "$support" ]; then
			echo $ragel -o $wk/$code_src-support.c $support
			$ragel -o $wk/$code_src-support.c $support
			scode="$wk/$code_src-support.c"
		fi

		echo "$compiler $flags $out_args $wk/$code_src $scode $libs"
		if ! $compiler $flags $out_args $wk/$code_src $scode $libs; then
			test_error;
		fi
	fi

	if [ "$compile_only" != "true" ]; then
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

function file_names()
{
	code_src=$root.$code_suffix;
	binary=$root.bin;
	output=$root.out;
}

function lang_opts()
{
	lang=$1

	case $lang in
		c)
			lang_opt=-C;
			code_suffix=c;
			interpreted=false
			compiler=$c_compiler;
			flags="-Wall -O3 -I. -Wno-variadic-macros"
			libs=""
			prohibit_minflags=""
			prohibit_genflags=""
			prohibit_featflags=""
			prohibit_frontflags=""
			prohibit_backflags=""
			prohibit_encflags=""
			file_names;
			exec_cmd=./$wk/$binary
		;;
		c++)
			lang_opt=-C;
			code_suffix=cpp;
			interpreted=false
			compiler=$cxx_compiler;
			flags="-Wall -O3 -I. -Wno-variadic-macros"
			libs=""
			prohibit_minflags=""
			prohibit_genflags=""
			prohibit_featflags=""
			prohibit_frontflags=""
			prohibit_backflags=""
			prohibit_encflags=""
			file_names;
			exec_cmd=./$wk/$binary
		;;
		obj-c)
			lang_opt=-C;
			code_suffix=m;
			interpreted=false
			compiler=$objc_compiler
			flags="-Wall -O3 -fno-strict-aliasing -I/usr/include/GNUstep"
			libs="-lobjc -lgnustep-base"
			prohibit_minflags=""
			prohibit_genflags=""
			prohibit_featflags=""
			prohibit_frontflags=""
			prohibit_backflags=""
			prohibit_encflags=""
			file_names;
			exec_cmd=./$wk/$binary
		;;
		d)
			lang_opt=-D;
			code_suffix=d;
			interpreted=false
			compiler=$d_compiler;
			flags="-Wall -O3"
			libs=""
			prohibit_minflags=""
			prohibit_genflags=""
			prohibit_featflags=""
			prohibit_frontflags="--kelbt-frontend --reduce-frontend"
			prohibit_backflags="--direct-backend --reduce-frontend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd=./$wk/$binary
		;;
		java)
			lang_opt=-J;
			code_suffix=java;
			interpreted=false
			compiler=$java_compiler
			flags=""
			libs=""
			prohibit_minflags=""
			prohibit_genflags="-G0 -G1 -G2"
			prohibit_featflags=""
			prohibit_frontflags="--kelbt-frontend --reduce-frontend"
			prohibit_backflags="--direct-backend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd="java -classpath $wk $root"
		;;
		ruby)
			lang_opt=-R;
			code_suffix=rb;
			interpreted=true
			compiler=$ruby_engine
			flags=""
			libs=""
			prohibit_minflags=""
			prohibit_genflags="-G0 -G1 -G2"
			prohibit_featflags="--goto-backend"
			prohibit_frontflags="--kelbt-frontend --reduce-frontend"
			prohibit_backflags="--direct-backend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd="ruby $wk/$code_src"
		;;
		csharp)
			lang_opt="-A";
			code_suffix=cs;
			interpreted=false
			compiler=$csharp_compiler
			flags=""
			libs=""
			prohibit_minflags=""
			prohibit_genflags="-G2"
			prohibit_featflags=""
			prohibit_frontflags="--kelbt-frontend --reduce-frontend"
			prohibit_backflags="--direct-backend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd="mono $wk/$binary"
		;;
		go)
			lang_opt="-Z"
			code_suffix=go
			interpreted=false
			compiler=$go_compiler
			flags="build"
			libs=""
			prohibit_minflags=""
			prohibit_genflags=""
			prohibit_featflags=""
			prohibit_frontflags="--kelbt-frontend --reduce-frontend"
			prohibit_backflags="--direct-backend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd=./$wk/$binary
		;;
		ocaml)
			lang_opt="-O"
			code_suffix=ml
			interpreted=true
			compiler=$ocaml_compiler
			flags=""
			libs=""
			prohibit_minflags=""
			prohibit_genflags="-G0 -G1 -G2"
			prohibit_featflags="--goto-backend"
			prohibit_frontflags="--kelbt-frontend --reduce-frontend"
			prohibit_backflags="--direct-backend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd="ocaml $wk/$code_src"
		;;
		asm)
			lang_opt="--asm"
			code_suffix=s
			interpreted=false
			compiler="gcc"
			flags=""
			libs=""
			prohibit_minflags=""
			prohibit_genflags="-T0 -T1 -F0 -F1 -G0 -G1"
			prohibit_featflags=""
			prohibit_frontflags="--colm-frontend"
			prohibit_backflags="--colm-backend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd=./$wk/$binary
		;;
		rust)
			lang_opt="-U"
			code_suffix=rs
			interpreted=false
			compiler=$rust_compiler
			flags="-A non_upper_case_globals -A dead_code \
					-A unused_variables -A unused_assignments \
					-A unused_mut -A unused_parens"
			libs=""
			prohibit_minflags=""
			prohibit_genflags="-G0 -G1 -G2"
			prohibit_featflags="--goto-backend"
			prohibit_frontflags="--kelbt-frontend --reduce-frontend"
			prohibit_backflags="--direct-backend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd=./$wk/$binary
		;;
		crack)
			lang_opt="-K"
			code_suffix=crk
			interpreted=true
			compiler=$crack_interpreter
			prohibit_minflags=""
			prohibit_genflags="-G0 -G1 -G2"
			prohibit_featflags="--goto-backend"
			prohibit_frontflags="--kelbt-frontend --reduce-frontend"
			prohibit_backflags="--direct-backend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd="$crack_interpreter $wk/$code_src"
		;;
		julia)
			lang_opt="-Y"
			code_suffix=jl
			interpreted=true
			compiler=$julia_interpreter
			prohibit_minflags=""
			prohibit_genflags="-G0 -G1 -G2"
			prohibit_featflags="--goto-backend"
			prohibit_frontflags="--kelbt-frontend --reduce-frontend"
			prohibit_backflags="--direct-backend"
			prohibit_encflags="--string-tables"
			file_names;
			exec_cmd="$julia_interpreter $wk/$code_src"
		;;
		indep)
		;;
		*)
			echo "$translated: unknown language type '$lang'" >&2
			exit 1;
		;;
	esac

	prohibit_minflags="$prohibit_minflags $case_prohibit_minflags"
	prohibit_genflags="$prohibit_genflags $case_prohibit_genflags"
	prohibit_featflags="$prohibit_featflags $case_prohibit_featflags"
	prohibit_frontflags="$prohibit_frontflags $case_prohibit_frontflags"
	prohibit_backflags="$prohibit_backflags $case_prohibit_backflags"
}

function run_options()
{
	translated=$1

	root=`basename $translated`
	root=${root%.rl};

	# maybe translated to multiple targets, re-read each lang.
	lang=`sed '/@LANG:/{s/^.*: *//;s/ *$//;p};d' $translated`

	lang_opts $lang

	[ -n "$additional_cflags" ] && flags="$flags $additional_cflags"

	# If we have no compiler for the source program then skip it.
	[ -z "$compiler" ] && continue

	# Make sure that we are interested in the host language.
	echo "$langflags" | grep -qe $lang_opt || return

	# Make sure that ragel supports the host language
	echo "$supported_host_langs" | grep -qe $lang_opt || return

	for min_opt in $minflags; do
		echo "" "$prohibit_minflags" | \
				grep -e $min_opt >/dev/null && continue

		for gen_opt in $genflags; do
			echo "" "$prohibit_genflags" | \
					grep -e $gen_opt >/dev/null && continue

			for enc_opt in $encflags; do
				echo "" "$prohibit_encflags" | \
						grep -e $enc_opt >/dev/null && continue

				for f_opt in $frontflags; do
					echo "" "$prohibit_frontflags" | \
							grep -e $f_opt >/dev/null && continue

					# Ragel must support the frontend.
					echo "$supported_frontends" | grep -qe $f_opt || continue

					for b_opt in $backflags; do
						echo "" "$prohibit_backflags" | \
								grep -e $b_opt >/dev/null && continue

						# Ragel must support the backend.
						echo "$supported_backends" | grep -qe $b_opt || continue

						for v_opt in $featflags; do

							echo "" "$prohibit_featflags" | \
									grep -e $v_opt >/dev/null && continue

							[ $gen_opt = -G0 ] && [ $v_opt = --var-backend ] && continue
							[ $gen_opt = -G1 ] && [ $v_opt = --var-backend ] && continue
							[ $gen_opt = -G2 ] && [ $v_opt = --var-backend ] && continue

							run_test
						done
					done
				done
			done
		done
	done
}

function run_internal()
{
	test_case=$1

	root=`basename $test_case`
	root=${root%.rl};

	output=$root.out;
	args="-I. $internal -o $wk/unused $test_case"
	exec_cmd="$ragel $args"

	echo -n "running: $exec_cmd ... "

	$exec_cmd 2>&1 > $wk/$output;
	if diff --strip-trailing-cr $wk/$expected_out $wk/$output > /dev/null;
	then
		echo "passed";
	else
		echo "FAILED";
		test_error;
	fi;
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
	enabled=`sed '/@ENABLED:/s/^.*: *//p;d' $test_case`
    if [ -n "$enabled" ] || [ "$enabled" = true ]; then
        continue;
    fi

	# If the generated flag is given make sure that the test case is generated.
	is_generated=`sed '/@GENERATED:/s/^.*: *//p;d' $test_case`
	if [ "$is_generated" = true ] && [ "$allow_generated" != true ]; then
		continue;
	fi

	expected_out=$root.exp;
	case_rl=${root}.rl

	# Create the expected output.
	sed '1,/^#\+ * OUTPUT #\+/d;' $test_case > $wk/$expected_out

	# internal consistency check?
	internal=`sed '/@INTERNAL:/{s/^.*: *//;s/ *$//;p};d' $test_case`
	if [ -n "$internal" ]; then
		run_internal $test_case
	else 
		prohibit_languages=`sed '/@PROHIBIT_LANGUAGES:/s/^.*: *//p;d' $test_case`

		# Add these into the langugage-specific defaults selected in run_options
		case_prohibit_minflags=`sed '/@PROHIBIT_MINFLAGS:/s/^.*: *//p;d' $test_case`
		case_prohibit_genflags=`sed '/@PROHIBIT_GENFLAGS:/s/^.*: *//p;d' $test_case`
		case_prohibit_featflags=`sed '/@PROHIBIT_FEATFLAGS:/s/^.*: *//p;d' $test_case`
		case_prohibit_frontflags=`sed '/@PROHIBIT_FRONTFLAGS:/s/^.*: *//p;d' $test_case`
		case_prohibit_backflags=`sed '/@PROHIBIT_BACKFLAGS:/s/^.*: *//p;d' $test_case`

		additional_cflags=`sed '/@CFLAGS:/s/^.*: *//p;d' $test_case`
		support=`sed '/@SUPPORT:/s/^.*: *//p;d' $test_case`

		lang=`sed '/@LANG:/s/^.*: *//p;d' $test_case`
		if [ -z "$lang" ]; then
			echo "$test_case: language unset"; >&2
			exit 1;
		fi

		cases=""

		if [ $lang == indep ]; then
			for lang in c asm d cs go java ruby ocaml rust crack julia; do
				case $lang in 
					c) lf="-C" ;;
					asm) lf="--asm" ;;
					d) lf="-D" ;;
					cs) lf="-A" ;;
					go) lf="-Z" ;;
					java) lf="-J" ;;
					ruby) lf="-R" ;;
					ocaml) lf="-O" ;;
					rust) lf="-U" ;;
					crack) lf="-K" ;;
					julia) lf="-Y" ;;
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
	fi
}

for test_case; do
	run_translate $test_case
done
