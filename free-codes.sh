#!/bin/bash
#

TYPE="$1"
FN="$2"

diff -u <(
	for I in `seq 0 255`; do
		printf "0x%02x\\n" $I
	done
) <(
	sed -n "/#define ${TYPE}_/s/#define ${TYPE}_[A-Z0-9_]* *//p" $FN | sort
) | sed -n '/^[\+\-]0/s/^.//p'
