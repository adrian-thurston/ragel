#!/bin/bash
#

# This wrapper around the colm program (and bootstrap programs) allows us to
# limit ourselves to one output file per makefile rule. It packs up multiple
# colm output files into one pack file, which is used as an intermediate file.
# We can extract the individual files from the pack one at a time, in separate
# rules.
#
# Ultimately this functionality should be rolled into the colm program itself.
# Until that is complete, this wrapper exists.
#

unset CMD
unset ARGS
unset OUTPUT
unset PACKS

while getopts "w:p:o:e:x:RcD:I:L:vdlirS:M:vHh?-:sVa:m:b:E:" opt; do

	# For the colm wrapper case.
	case "$opt" in
		w)
			# Which command to wrap.
			CMD=$OPTARG
		;;
		o)
			# Pack file name. For wrapping.
			OUTPUT=$OPTARG
		;;
		[pexm]) 
			ARGS="$ARGS -$opt $OPTARG.pack"
			PACKS="$PACKS $OPTARG.pack"
		;;
		[DILSMambE-])   ARGS="$ARGS -$opt $OPTARG" ;;
		[RcvdlirvHhsV]) ARGS="$ARGS -$opt" ;;
		?)
			exit 1;
		;;
	esac
done

# Shift over the args.
shift $((OPTIND - 1));

INPUT="$1"
if [ -z "$INPUT" ]; then
	echo colm-wrap: no input file given >&2
	exit 1
fi

if [ -z "$OUTPUT" ]; then
	echo colm-wrap: no output file given >&2
	exit 1
fi

# Default command to colm.
if [ "${INPUT%.pack}" != "$INPUT" ]; then
	tar -xmf "$INPUT" "$OUTPUT.pack"
	mv $OUTPUT.pack $OUTPUT
	EXIT_STATUS=$?
else
	CMD=${CMD:=colm}
	if [ "`basename $0`" != "$0" ] && [ -x "`dirname $0`/$CMD" ]; then
		COLM="`dirname $0`/$CMD"
	else
		COLM=@prefix@/bin/$CMD
	fi

	$COLM $ARGS "$INPUT"
	EXIT_STATUS=$?
	if [ $EXIT_STATUS = 0 ]; then
		tar -cf "$OUTPUT" $PACKS
	fi
	rm -f $PACKS
fi

exit $EXIT_STATUS

