#!/bin/bash
#

# Test cases contain sections giving the program, input and expected output.

###### COLM #####
#   colm program
###### ARGS #####
#   program arguments
###### IN #####
#   program input
###### EXP #####
#   expected output

#######################################

WORKING=working
COLM=../colm/colm
ERRORS=0

cd `dirname $0`
test -d $WORKING || mkdir $WORKING

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
			VALGRIND="valgrind --leak-check=full --show-reachable=yes "
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

function section
{
	local section=$1
	local nth=$2
	local in=$3
	local out=$4

	# Print Nth instance of the section
	awk -vsection=$section -vnth=$nth '
		/#+ *[a-zA-Z]+ *#+/ {
			gsub( "[ #\n]", "", $0 );
			in_section = 0
			if ( $0 == section ) {
				if ( n == nth ) {
					in_section = 1;
					found = 1;
				}
				n += 1
			}
			next;
		}

		in_section {
			print $0;
		}

		END {
			exit( found ? 0 : 1 )
		}
	' $in | awk '
		/--noeol$/ {
			gsub(/--noeol$/,"");
			printf("%s", $0);
			next;
		}
		{ print $0 }
	' > $out

	# Remove the file if no section was found
	[ ${PIPESTATUS[0]} = 0 ] || rm $out
}

function runtests()
{
	for TST in $TEST_PAT; do
		ROOT=${TST/.lm}
		LM=$WORKING/$ROOT.lm
		ARGS=$WORKING/$ROOT.args
		IN=$WORKING/$ROOT.in
		EXP=$WORKING/$ROOT.exp

		section LM 0 $TST $LM

		BIN=$WORKING/$ROOT
		OUT=$WORKING/$ROOT.out
		DIFF=$WORKING/$ROOT.diff
		LOG=$WORKING/$ROOT.log

		if [ '!' -f $LM ]; then
			echo "ERROR: $TST cannot be run: no LM section"
			ERRORS=$(( ERRORS + 1 ))
			continue
		fi

		# Compilation.
		$COLM $LM &> $LOG 
		if [ $? != 0 ]; then
			echo "ERROR: $TST cannot be run: compilation error"
			ERRORS=$(( ERRORS + 1 ))
			continue
		fi

		Nth=0
		while true; do
			section EXP $Nth $TST $EXP

			# Stop when we have no Nth expected output.
			if [ '!' -f $EXP ]; then
				break;
			fi

			section ARGS $Nth $TST $ARGS
			section IN $Nth $TST $IN

			cmdargs=""
			if [ -f $ARGS ]; then
				cmdargs=`cat $ARGS`
			fi

			echo -n "running test $TST ($Nth)... "

			if [ "$verbose" = true ]; then
				echo
				echo $COLM $TST
			fi

			if [ '!' -f $IN ] && [ -f $ROOT.in ]; then
				IN=$ROOT.in;
			fi

			if [ "$verbose" = true ]; then
				if [ -f $IN ]; then
					echo "${VALGRIND}./$BIN $cmdargs < $IN > $OUT 2>> $LOG"
				else
					echo "${VALGRIND}./$BIN $cmdargs > $OUT 2>>$LOG"
				fi
			fi

			# Execution
			if [ -f $IN ]; then
				${VALGRIND}./$BIN $cmdargs < $IN > $OUT 2>> $LOG
			else
				${VALGRIND}./$BIN $cmdargs > $OUT 2>>$LOG
			fi
			if [ $? != 0 ]; then
				echo "FAILED: execution error"
				ERRORS=$(( ERRORS + 1 ))
				Nth=$((Nth + 1))
				continue
			fi

			# Diff of output
			diff -u $EXP $OUT > $DIFF
			if [ $? != 0 ]; then
				echo "FAILED: output differs from expected output"
				ERRORS=$(( ERRORS + 1 ))
				Nth=$((Nth + 1))
				if [ "$diff" = true ]; then
					echo
					cat $DIFF
					echo
				fi
				continue
			fi

			echo ok
			Nth=$((Nth + 1))
		done
	done

	if [ $ERRORS != 0 ]; then
		[ $ERRORS != 1 ] && plural="s";
		echo
		echo "TESTING FAILED: $ERRORS failure$plural"
		echo
		EXIT=1
	fi
}

[ -d $workingdir ] || mkdir $workingdir

runtests;

exit $EXIT;

