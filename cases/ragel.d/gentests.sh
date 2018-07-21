#!/bin/bash

#
# Test Case Features
# ------------------
#    If the test case has a directory by the same name, copy it into the
#    working direcotory.
#
#    @RAGEL_FILE: file name to pass on the command line instead of file created
#    by extracting section. Does not work with translated test cases.
# 

TRANS=./trans

# Make available to to test directories below us that are not part of this
# repository and cannot source one dir up.
export RAGEL_BIN="@RAGEL_BIN@"
export RAGEL_CPPFLAGS="@RAGEL_CPPFLAGS@"
export RAGEL_LDFLAGS="@RAGEL_LDFLAGS@"
export LD_LIBRARY_PATH="@LD_LIBRARY_PATH@"

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
			genflags="$genflags -$opt"
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
					genflags="$genflags --$OPTARG"
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

[ -z "$langflags" ]   && langflags="-C --asm -R -Y -O -U -J -Z -D -A -K"
[ -z "$genflags" ]    && genflags="-T0 -T1 -F0 -F1 -G0 -G1 -G2 -n -m -e --string-tables"

shift $((OPTIND - 1));

[ -z "$*" ] && set -- *.rl

ragel="@RAGEL_BIN@"

cxx_compiler="@CXX@"
c_compiler="@CC@"
objc_compiler="@CC@"
d_compiler="gdc-5"
java_compiler="javac"
ruby_engine="ruby"
csharp_compiler="mcs"
go_compiler="go"
ocaml_compiler="ocaml"
rust_compiler="rustc"
crack_interpreter="$HOME/pkgs/crack/bin/crack"
julia_interpreter="julia"

function test_error
{
	exit 1;
}

function exec_cmd()
{
	lang=$1

	case $lang in
		c) exec_cmd=./$binary ;;
		c++) exec_cmd=./$binary ;;
		obj-c) exec_cmd=./$binary ;;
		d) exec_cmd=./$binary ;;
		java) exec_cmd="java -classpath $wk $classname" ;;
		ruby) exec_cmd="ruby $code_src" ;;
		csharp) exec_cmd="mono $binary" ;;
		go) exec_cmd=./$binary ;;
		ocaml) exec_cmd="ocaml $code_src" ;;
		asm) exec_cmd=./$binary ;;
		rust) exec_cmd=./$binary ;;
		crack) exec_cmd="$crack_interpreter $code_src" ;;
		julia) exec_cmd="$julia_interpreter $code_src" ;;
		indep) ;;
	esac
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
			#host_ragel=$ragel
			host_ragel=`dirname $ragel`/host-c/ragel-c
			flags="-Wall -O3 -I. -Wno-variadic-macros"
			libs=""
			prohibit_flags=""
		;;
		c++)
			lang_opt=-C;
			code_suffix=cpp;
			interpreted=false
			compiler=$cxx_compiler;
			#host_ragel=$ragel
			host_ragel=`dirname $ragel`/host-c/ragel-c
			flags="-Wall -O3 -I. -Wno-variadic-macros"
			libs=""
			prohibit_flags=""
		;;
		obj-c)
			lang_opt=-C;
			code_suffix=m;
			interpreted=false
			compiler=$objc_compiler
			#host_ragel=$ragel
			host_ragel=`dirname $ragel`/host-c/ragel-c
			flags="`gnustep-config --objc-flags`"
			libs="-lobjc -lgnustep-base"
			prohibit_flags=""
		;;
		d)
			lang_opt=-D;
			code_suffix=d;
			interpreted=false
			compiler=$d_compiler;
			host_ragel=`dirname $ragel`/host-d/ragel-d
			flags="-Wall -O3"
			libs=""
			prohibit_flags="--string-tables"
		;;
		java)
			lang_opt=-J;
			code_suffix=java;
			interpreted=false
			compiler=$java_compiler
			host_ragel=`dirname $ragel`/host-java/ragel-java
			flags=""
			libs=""
			prohibit_flags="-G0 -G1 -G2 --goto-backend --string-tables"
		;;
		ruby)
			lang_opt=-R;
			code_suffix=rb;
			interpreted=true
			compiler=$ruby_engine
			host_ragel=`dirname $ragel`/host-ruby/ragel-ruby
			flags=""
			libs=""
			prohibit_flags="-G0 -G1 -G2 --goto-backend --string-tables"
		;;
		csharp)
			lang_opt="-A";
			code_suffix=cs;
			interpreted=false
			compiler=$csharp_compiler
			host_ragel=`dirname $ragel`/host-csharp/ragel-csharp
			flags=""
			libs=""
			prohibit_flags="-G2 --string-tables"
		;;
		go)
			lang_opt="-Z"
			code_suffix=go
			interpreted=false
			compiler=$go_compiler
			host_ragel=`dirname $ragel`/host-go/ragel-go
			flags="build"
			libs=""
			prohibit_flags="--string-tables"
		;;
		ocaml)
			lang_opt="-O"
			code_suffix=ml
			interpreted=true
			compiler=$ocaml_compiler
			host_ragel=`dirname $ragel`/host-ocaml/ragel-ocaml
			flags=""
			libs=""
			prohibit_flags="-G0 -G1 -G2 --goto-backend --string-tables"
		;;
		asm)
			lang_opt="--asm"
			code_suffix=s
			interpreted=false
			compiler="gcc"
			host_ragel=`dirname $ragel`/host-asm/ragel-asm
			flags=""
			libs=""
			prohibit_flags="-T0 -T1 -F0 -F1 -G0 -G1 --string-tables"
		;;
		rust)
			lang_opt="-U"
			code_suffix=rs
			interpreted=false
			compiler=$rust_compiler
			host_ragel=`dirname $ragel`/host-rust/ragel-rust
			flags="-A non_upper_case_globals -A dead_code -A unused_variables -A unused_assignments -A unused_mut -A unused_parens"
			libs=""
			prohibit_flags="-G0 -G1 -G2 --goto-backend --string-tables"
		;;
		crack)
			lang_opt="-K"
			code_suffix=crk
			interpreted=true
			compiler=$crack_interpreter
			host_ragel=`dirname $ragel`/host-crack/ragel-crack
			prohibit_flags="-G0 -G1 -G2 --goto-backend --string-tables"
		;;
		julia)
			lang_opt="-Y"
			code_suffix=jl
			interpreted=true
			compiler=$julia_interpreter
			host_ragel=`dirname $ragel`/host-julia/ragel-julia
			prohibit_flags="-G0 -G1 -G2 --goto-backend --string-tables"
		;;
		indep)
		;;
		*)
			echo "$translated: unknown language type '$lang'" >&2
			exit 1;
		;;
	esac

	prohibit_flags="$prohibit_flags $case_prohibit_flags"
}

function run_test()
{
	code_src=$wk/`echo $lroot$gen_opt.$code_suffix | sed 's/-\+/_/g'`
	binary=$wk/`echo $lroot$gen_opt.bin | sed 's/-\+/_/g'`
	output=$wk/`echo $lroot$gen_opt.out | sed 's/-\+/_/g'`
	diff=$wk/`echo $lroot$gen_opt.diff | sed 's/-\+/_/g'`
	sh=$wk/`echo $lroot$gen_opt.sh | sed 's/-\+/_/g'`
	log=$wk/`echo $lroot$gen_opt.log | sed 's/-\+/_/g'`
	intermed=$wk/`echo $lroot$gen_opt.ri | sed 's/-\+/_/g'`
	classfile=$wk/`echo $lroot$gen_opt.class | sed 's/-\+/_/g'`
	classname=`echo $lroot$gen_opt | sed 's/-\+/_/g'`

	opts="$gen_opt $min_opt $enc_opt $f_opt"
	args="-I. $opts -o $code_src $translated"

	cat >> $sh <<-EOF
	echo testing $lroot $opts
	$host_ragel $args
	EOF

	if [ $lang == java ]; then
		cat >> $sh <<-EOF
		sed -i 's/\<$lroot\>/$classname/g' $code_src
		EOF
	fi

	out_args=""
	[ $lang != java ] && out_args="-o $binary";
	[ $lang == csharp ] && out_args="-out:$binary";

	# Some langs are just interpreted.
	if [ $interpreted != "true" ]; then
		cat >> $sh <<-EOF
		$compiler $flags $out_args $code_src \
				$libs >>$log 2>>$log
		EOF
	fi

	exec_cmd $lang
	if [ "$compile_only" != "true" ]; then
		if [ -n "$FILTER" ]; then
			exec_cmd="$exec_cmd | $FILTER"
		fi		

		cat >> $sh <<-EOF
		$exec_cmd 2>> $log >> $output
		EOF

		cat >> $sh <<-EOF
		diff -u --strip-trailing-cr $expected_out $output > $diff
		rm -f $intermed $code_src $binary $classfile $output 
		EOF

	fi

	echo $sh
}


function run_options()
{
	translated=$1

	lroot=`basename $translated`
	lroot=${lroot%.rl};

	lang_opts $lang

	[ -n "$additional_cflags" ] && flags="$flags $additional_cflags"

	# If we have no compiler for the source program then skip it.
	[ -z "$compiler" ] && continue

	# Make sure that we are interested in the host language.
	echo "$langflags" | grep -qe $lang_opt || return

	for gen_opt in $genflags; do
		echo "" "$prohibit_flags" | \
				grep -e $gen_opt >/dev/null && continue

		run_test
	done
	unset gen_opt
}

function run_translate()
{
	test_case=$1

	# Recompute the root.
	root=`basename $test_case`
	root=${root%.rl};

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

	# Override the test case file name.
	RAGEL_FILE=`sed '/@RAGEL_FILE:/s/^.*: *//p;d' $test_case`

	# Filter to pass output through. Shell code.
	FILTER=`sed '/@FILTER:/s/^.*: *//p;d' $test_case`

	# If the test case has a directory by the same name, copy it into the
	# working direcotory.
	if [ -d $root ]; then
		cp -a $root working/
	fi

	expected_out=$wk/$root.exp;
	case_rl=${root}.rl

	sed '1,/^#\+ * OUTPUT #\+/{ d };' $test_case > $expected_out

	prohibit_languages=`sed '/@PROHIBIT_LANGUAGES:/s/^.*: *//p;d' $test_case`

	# Add these into the langugage-specific defaults selected in run_options
	case_prohibit_flags=`sed '/@PROHIBIT_FLAGS:/s/^.*: *//p;d' $test_case`

	lang=`sed '/@LANG:/s/^.*: *//p;d' $test_case`
	if [ -z "$lang" ]; then
		echo "$test_case: language unset"; >&2
		exit 1;
	fi

	cases=""

	if [ $lang == indep ]; then
		for lang in c asm d csharp go java ruby ocaml rust crack julia; do
			case $lang in 
				c) lf="-C" ;;
				asm) lf="--asm" ;;
				d) lf="-D" ;;
				csharp) lf="-A" ;;
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

			# Translate to target language and strip off output.
			targ=${root}_$lang.rl

			$TRANS $lang $wk/$targ $test_case ${root}_${lang}

			cases="$cases $wk/$targ"

			run_options $wk/$targ
		done
	else

		sed '/^#\+ * OUTPUT #\+/,$d' $test_case > $wk/$case_rl

		cases=$wk/$case_rl

		if [ -n "$RAGEL_FILE" ]; then
			cases="$RAGEL_FILE"
		fi

		run_options $cases
	fi
}

go()
{
	for test_case; do
		run_translate $test_case
	done
}

echo working/* | xargs rm -Rf
go "$@"
