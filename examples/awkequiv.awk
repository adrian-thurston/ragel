#!/usr/bin/awk -f
#


{
	print "endline(" NF "): " $0
	for ( i = 1; i <= NF; i++ ) {
		print "  word: " $i
	}
}
