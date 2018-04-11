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
[ -z "$langflags" ]   && langflags="-C --asm -R -Y -O -U -J -Z -D -A -K"
[ -z "$featflags" ]   && featflags="--goto-backend"

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
crack_interpreter="$HOME/pkgs/crack-0.11/bin/crack"
julia_interpreter="julia"

function test_error
{
	exit 1;
}

function exec_cmd()
{
	lang=$1

	case $lang in
		c) exec_cmd=./$wk/$_binary ;;
		c++) exec_cmd=./$wk/$_binary ;;
		obj-c) exec_cmd=./$wk/$_binary ;;
		d) exec_cmd=./$wk/$_binary ;;
		java) exec_cmd="java -classpath $wk $_lroot" ;;
		ruby) exec_cmd="ruby $wk/$_code_src" ;;
		csharp) exec_cmd="mono $wk/$_binary" ;;
		go) exec_cmd=./$wk/$_binary ;;
		ocaml) exec_cmd="ocaml $wk/$_code_src" ;;
		asm) exec_cmd=./$wk/$_binary ;;
		rust) exec_cmd=./$wk/$_binary ;;
		crack) exec_cmd="$crack_interpreter $wk/$_code_src" ;;
		julia) exec_cmd="$julia_interpreter $wk/$_code_src" ;;
		indep) ;;
	esac
}

function file_names()
{
	code_src=$lroot.$code_suffix;
	binary=$lroot.bin;
	output=$lroot.out;
	diff=$lroot.diff;
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
			file_names;
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
			file_names;
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
			file_names;
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
			file_names;
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
			file_names;
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
			file_names;
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
			file_names;
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
			file_names;
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
			file_names;
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
			file_names;
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
			file_names;
		;;
		crack)
			lang_opt="-K"
			code_suffix=crk
			interpreted=true
			compiler=$crack_interpreter
			host_ragel=`dirname $ragel`/host-crack/ragel-crack
			prohibit_flags="-G0 -G1 -G2 --goto-backend --string-tables"
			file_names;
		;;
		julia)
			lang_opt="-Y"
			code_suffix=jl
			interpreted=true
			compiler=$julia_interpreter
			host_ragel=`dirname $ragel`/host-julia/ragel-julia
			prohibit_flags="-G0 -G1 -G2 --goto-backend --string-tables"
			file_names;
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
	_lroot=`echo s$min_opt$gen_opt$enc_opt$f_opt-$lroot | sed 's/-\+/_/g'`
	_code_src=`echo s$min_opt$gen_opt$enc_opt$f_opt-$code_src | sed 's/-\+/_/g'`
	_binary=`echo s$min_opt$gen_opt$enc_opt$f_opt-$binary | sed 's/-\+/_/g'`
	_output=`echo s$min_opt$gen_opt$enc_opt$f_opt-$output | sed 's/-\+/_/g'`
	_diff=`echo s$min_opt$gen_opt$enc_opt$f_opt-$diff | sed 's/-\+/_/g'`
	_sh=$wk/`echo s$min_opt$gen_opt$enc_opt$f_opt-$lroot.sh | sed 's/-\+/_/g'`
	_log=`echo s$min_opt$gen_opt$enc_opt$f_opt-$lroot.log | sed 's/-\+/_/g'`

	opts="$min_opt $gen_opt $enc_opt $f_opt"
	args="-I. $opts -o $wk/$_code_src $translated"

	cat >> $_sh <<-EOF
	echo "testing $lroot $opts"
	$host_ragel $args
	EOF

	if [ $lang == java ]; then
		cat >> $_sh <<-EOF
		sed -i 's/\<$lroot\>/$_lroot/g' $wk/$_code_src
		EOF
	fi

	out_args=""
	[ $lang != java ] && out_args="-o $wk/$_binary";
	[ $lang == csharp ] && out_args="-out:$wk/$_binary";

	# Some langs are just interpreted.
	if [ $interpreted != "true" ]; then
		scode=""
		if [ -n "$support" ]; then
			echo $ragel -o $wk/$code_src-support.c $support
			$ragel -o $wk/$code_src-support.c $support
			scode="$wk/$code_src-support.c"
		fi

		cat >> $_sh <<-EOF
		$compiler $flags $out_args $wk/$_code_src \
				$scode $libs >$wk/$_log 2>$wk/$_log
		EOF
	fi

	exec_cmd $lang
	if [ "$compile_only" != "true" ]; then
		if [ -n "$FILTER" ]; then
			exec_cmd="$exec_cmd | $FILTER"
		fi		

		cat >> $_sh <<-EOF
		$exec_cmd 2> $wk/$_log > $wk/$_output
		EOF

		cat >> $_sh <<-EOF
		diff --strip-trailing-cr $wk/$expected_out $wk/$_output > $wk/$_diff
		rm -f $wk/$_lroot.ri $wk/$_code_src $wk/$_binary $wk/$_lroot.class $wk/$_output 
		EOF

	fi

	echo $_sh
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

	for min_opt in $minflags; do
		echo "" "$prohibit_flags" | \
				grep -e $min_opt >/dev/null && continue

		for gen_opt in $genflags; do
			echo "" "$prohibit_flags" | \
					grep -e $gen_opt >/dev/null && continue

			for enc_opt in $encflags; do
				echo "" "$prohibit_flags" | \
						grep -e $enc_opt >/dev/null && continue

				run_test
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
	if diff --strip-trailing-cr $wk/$expected_out $wk/$output > $wk/$diff
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

	expected_out=$root.exp;
	case_rl=${root}.rl

	sed '1,/^#\+ * OUTPUT #\+/{ d };' $test_case > $wk/$expected_out

	# internal consistency check?
	internal=`sed '/@INTERNAL:/{s/^.*: *//;s/ *$//;p};d' $test_case`
	if [ -n "$internal" ]; then
		run_internal $test_case
	else 
		prohibit_languages=`sed '/@PROHIBIT_LANGUAGES:/s/^.*: *//p;d' $test_case`

		# Add these into the langugage-specific defaults selected in run_options
		case_prohibit_flags=`sed '/@PROHIBIT_FLAGS:/s/^.*: *//p;d' $test_case`

		additional_cflags=`sed '/@CFLAGS:/s/^.*: *//p;d' $test_case`
		support=`sed '/@SUPPORT:/s/^.*: *//p;d' $test_case`

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
	fi
}

MF=working/run.mk
echo working/* | xargs rm -Rf

rm -f $MF
echo "do: all" >> $MF

go()
{
	for test_case; do
		run_translate $test_case
	done
}

echo -----------
go "$@" | xargs -P 4 -n 1 bash

echo ----  cases   ------
find working -name '*.sh' -not -size 0 | wc -l

echo ---- failures ------
find working -name '*.diff' -not -size 0 | wc -l

echo ---- warnings ------
find working -name '*.log' -not -size 0 | wc -l
