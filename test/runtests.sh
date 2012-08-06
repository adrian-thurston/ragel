#!/bin/bash
#

# Test files have the extention 'tst'. They contain sections giving config
# data, function names, SQL code and expected output.

#   ##### FNC #####
#   Function name.
#   ##### CNF #####
#   Configuration data.
#   ##### SQL #####
#   SQL code to run ahead of the test.
#   ##### PCY #####
#   Policy to use for the test.
#   ##### EXP #####
#   Expected Output.
#######################################

cd `dirname $0`


function die()
{
	echo
	echo "$@"
	echo
	exit 1
}

function sig_exit()
{
	echo
	exit 1;
}

errors=0

#trap sig_exit SIGINT
#trap sig_exit SIGQUIT

COLM=../colm/colm

[ -d $DATA ] || die "error: data directory not found"

# Parse args.
while getopts vdm opt; do
	case $opt in
		v)
			verbose=true;
		;;
		d)
			diff=true;
		;;
		m)
			VALGRIND="valgrind --leak-check=full --show-reachable=yes --suppressions=../dpi.supp"
		;;
	esac
done
shift $(($OPTIND - 1))

# The files to process. If none given then glob all functions and pcap test confs.
if [ $# != 0 ]; then
	TEST_PAT="$*"
else
	TEST_PAT='*.lm'
fi 

function runtests()
{
	for TST in $TEST_PAT; do
		INP=${TST/.lm}.in
		EXP=${TST/.lm}.exp
		ARGS=${TST/.lm}.args

		BIN=${TST/.lm}.bin
		OUT=${TST/.lm}.out
		DIFF=${TST/.lm}.diff
		LOG=${TST/.lm}.log

		cmdargs=""
		if [ -f $ARGS ]; then
			cmdargs=`cat $ARGS`
		fi

		echo -n "running test $TST ... "

		# Check for expected output.
		if [ '!' -f $EXP ]; then
			echo "FAILED no expected output"
			errors=$(( errors + 1 ))
			continue
		
		fi

		if [ "$verbose" = true ]; then
			echo
			echo $COLM $TST
			echo -n ...
		fi

		# Compilation.
		$COLM $TST &> $LOG 
		if [ $? != 0 ]; then
			echo "FAILED compilation"
			errors=$(( errors + 1 ))
			continue
		fi

		if [ "$verbose" = true ]; then
			echo

			if [ -f $INP ]; then
				echo "./$BIN $cmdargs < $INP > $OUT 2>> $LOG"
			else
				echo "./$BIN $cmdargs > $OUT"
			fi
			
			echo -n ...
		fi

		# Execution
		if [ -f $INP ]; then
			./$BIN $cmdargs < $INP > $OUT 2>> $LOG
		else
			./$BIN $cmdargs > $OUT
		fi
		if [ $? != 0 ]; then
			echo "FAILED execution"
			errors=$(( errors + 1 ))
			continue
		fi

		# Diff of output
		diff -u $EXP $OUT > $DIFF
		if [ $? != 0 ]; then
			echo "FAILED output diff"
			errors=$(( errors + 1 ))
			continue
		fi

		echo ok
	done

	if [ $errors != 0 ]; then
		[ $errors != 1 ] && plural="s";
		echo
		echo "TESTING FAILED: $errors failure$plural"
		echo
		EXIT=1
	fi
}

function diffs()
{
	for TST in $TEST_PAT; do
		EXP=${TST/.*}.exp
		OUT=${TST/.*}.out

		[ -f $EXP ] || die "error: the expected output file $EXP was not found"
		diff -u $EXP $OUT
	done
}

[ -d $workingdir ] || mkdir $workingdir

runtests;

if [ "$diff" = true ]; then
	diffs;
fi

exit $EXIT;

