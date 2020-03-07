#!/bin/bash
#

# Test cases contain sections which specify the program, the input and the
# expected output. The first section has no header and is always the colm
# program. The sections afterwards can be in any order.

#
#   colm program
#
###### ARGS #####
#
#   program arguments
#
###### COMP ######
#
#   compilation arguments
#
###### IN #####
#
#   program input
#
###### EXP #####
#
#   expected output
#
###### EXIT ######
#
#   expected exit value
#
###### HOST ######
#
# Host program.
#
###### CALL ######
#
# files containing C functions
#

#######################################

WORKING=working
ERRORS=0

# Make available to to test directories below us that are not part of this
# repository and cannot source one dir up.
export COLM_BIN="@SUBJ_COLM_BIN@"
export COLM_CPPFLAGS="@SUBJ_COLM_CPPFLAGS@"
export COLM_LDFLAGS="@SUBJ_COLM_LDFLAGS@"

# cd `dirname $0`
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

function echo_cmd()
{
	echo $@
	$@
}

function check_compilation()
{
	if [ $1 != 0 ]; then
		echo "ERROR: $TST cannot be run: compilation error"
		ERRORS=$(( ERRORS + 1 ))
		continue
		return 1
	fi
}

trap sig_exit SIGINT
trap sig_exit SIGQUIT
trap sig_exit SIGTERM

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

function cat_section
{
	local section=$1
	local nth=$2
	local in=$3

	# Print Nth instance of the section
	awk -v section=$section -v nth=$nth '
		BEGIN {
			if ( section == "LM" ) {
				found = 1
				in_section = 1;
			}
		}

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
	'
	return ${PIPESTATUS[0]};
}

function section
{
	local section=$1
	local nth=$2
	local in=$3
	local out=$4

	cat_section $section $nth $in > $out

	# Remove the file if no section was found
	[ $? = 0 ] || rm $out
}

function runtests()
{
	for TST in $TEST_PAT; do
		if [ -d $TST ]; then
			cd $TST;
			./runtests
			cd ..
			continue
		fi

		ROOT=${TST/.lm}
		LM=$WORKING/$ROOT.lm
		HOST=$WORKING/$ROOT.host.cc
		CALL=$WORKING/$ROOT.call.c
		SH=$WORKING/$ROOT.sh

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

		section CALL 0 $TST $CALL
		section HOST 0 $TST $HOST

		COLM_ADDS=""
		if test -f $CALL; then
			COLM_ADDS="-a $CALL"
		fi

		COMP=`cat_section COMP 0 $TST`

		echo echo testing $ROOT >> $SH
		echo rm -f $DIFF >> $SH
	
		if test -f $HOST; then
			PARSE=$WORKING/$ROOT.parse
			IF=$WORKING/$ROOT.if

			echo $COLM_BIN $COMP -c -o $PARSE.c -e $IF.h -x $IF.cc $LM >> $SH
			if ! check_compilation $?; then
				continue
			fi

			echo gcc -c $COLM_CPPFLAGS $COLM_LDFLAGS -o $PARSE.o $PARSE.c >> $SH
			echo g++ -I. $COLM_CPPFLAGS $COLM_LDFLAGS -o $WORKING/$ROOT $IF.cc $HOST $PARSE.o -lcolm >> $SH

			if ! check_compilation $?; then
				continue
			fi
		else
			# Compilation.
			echo $COLM_BIN $COMP $COLM_ADDS $LM '&>' $LOG >> $SH
			echo "if [ \$? != 0 ]; then echo \"COMPILATION FAILED (see $LOG)\" >> $DIFF; fi" >> $SH
		fi

		Nth=0
		while true; do
			ARGS=$WORKING/$ROOT-$Nth.args
			IN=$WORKING/$ROOT-$Nth.in
			EXP=$WORKING/$ROOT-$Nth.exp

			section EXP $Nth $TST $EXP

			# Stop when we have no Nth expected output, unless, there were no
			# expected outputs at all. In that case we continue to run with an
			# empty expected output.
			if [ '!' -f $EXP ]; then
				if [ $Nth == 0 ]; then
					echo -n > $EXP
				else
					break;
				fi
			fi

			section ARGS $Nth $TST $ARGS
			section IN $Nth $TST $IN
			EXIT=`cat_section EXIT $Nth $TST`
			if [ -z "$EXIT" ]; then
				EXIT=0
			fi

			cmdargs=""
			if [ -f $ARGS ]; then
				cmdargs=`cat $ARGS`
			fi

			if [ '!' -f $IN ] && [ -f $ROOT.in ]; then
				IN=$ROOT.in;
			fi

			# Execution
			if [ -f $IN ]; then
				echo ${VALGRIND}./$BIN $cmdargs '<' $IN '>' $OUT '2>>' $LOG >> $SH
			else
				echo ${VALGRIND}./$BIN $cmdargs '>' $OUT '2>>' $LOG >> $SH
			fi

			cat <<-EOF >> $SH
			e=\$?
			if [ \$e != "$EXIT" ]; then
				echo "FAILED: exit value error: got: \$e expected: $EXIT"
			fi
			EOF

			# Diff of output
			echo diff -u $EXP $OUT '>>' $DIFF >> $SH
			#if [ $? != 0 ]; then
			#	echo "FAILED: output differs from expected output"
			#	ERRORS=$(( ERRORS + 1 ))
			#	Nth=$((Nth + 1))
			#	if [ "$diff" = true ]; then
			#		echo
			#		cat $DIFF
			#		echo
			#	fi
			#	continue
			#fi

			#echo ok
			Nth=$((Nth + 1))

		done
		echo $SH
	done

#	if [ $ERRORS != 0 ]; then
#		[ $ERRORS != 1 ] && plural="s";
#		echo
#		echo "TESTING FAILED: $ERRORS failure$plural"
#		echo
#		EXIT=1
#	fi
}

rm -Rf $WORKING/*
runtests 

#exit $EXIT;

