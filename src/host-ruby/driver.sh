#!/bin/bash

#
# Parse args enough to get language, input, output, include args. Stash rest.
#

lang=""
input=""
output=""
include=""

while getopts "Ro:I:" opt; do
	case $opt in
		R)
			lang=ruby;
		;;
		o)
			output="$OPTARG"
		;;
		I)
			include="$include $OPTARG"
		;;
		*)
			exit 1;
		;;
	esac
done

shift $((OPTIND - 1));
input=$1

# derive an output file name from input if not present.
if [ -z "$output" ]; then
	output=${input%.rl}.c
	if [ "$input" = "$output" ]; then
		echo input equal output
	fi
fi

# Derive a root for output purposes.
root=${output%.*}

set -x

./$lang $root.rfi $input $include
../ragel --colm-backend -P -o $root.rbi $root.rfi
./rlhc $output $root.rbi
