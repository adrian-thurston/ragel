#!/bin/bash
#

if [ $1 = "unpack" ]; then
	shift 1
	while getopts "o:" opt; do
		case $opt in
			o) parser_output=$OPTARG;;
		esac
	done
	shift $((OPTIND - 1));

	tar -xmf "$@" $parser_output
	exit $?

else

	if [ "`basename $0`" != "$0" ] && [ -x "`dirname $0`/$1" ]; then
		COLM="`dirname $0`/$1"
	else
		COLM=@prefix@/bin/$1
	fi

	shift 1

	ARGS=""

	# Using silent mode to pass other options through.
	while getopts "p:o:e:x:RcD:I:L:vdlirS:M:vHh?-:sVa:m:b:E:" opt; do

		case $opt in
			p)
				# Pack file name
				pack_file=$OPTARG
			;;
			[oexm]) 
				# Parser Output (C)
				parser_output=$OPTARG
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

	shift $((OPTIND - 1));

	$COLM $ARGS "$@"
	tar --transform 's/.pack$//' -cf "$pack_file" $PACKS
	rm -f $PACKS
fi
