#!/bin/bash
#

# This wrapper around the colm program (and bootstrap programs) allows us to
# stick to one output per run of the program. It packs up the multiple colm
# output files into one file. We can extract them one at a time afterwards.

CMD=$1;
shift 1

ARGS=""
while getopts "p:o:e:x:RcD:I:L:vdlirS:M:vHh?-:sVa:m:b:E:" opt; do
	# For the unpack case.
	if [ "$opt" = 'o' ]; then
		output=$OPTARG
	fi

	# For the colm wrapper case.
	case "$opt" in
		p)
			# Pack file name
			pack_file=$OPTARG
		;;
		[oexm]) 
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

if [ $CMD = "unpack" ]; then
	tar -xmf "$@" $output
	EXIT_STATUS=$?
else
	if [ "`basename $0`" != "$0" ] && [ -x "`dirname $0`/$CMD" ]; then
		COLM="`dirname $0`/$CMD"
	else
		COLM=@prefix@/bin/$CMD
	fi

	$COLM $ARGS "$@"
	EXIT_STATUS=$?
	tar --transform 's/.pack$//' -cf "$pack_file" $PACKS
	rm -f $PACKS
fi

exit $EXIT_STATUS

