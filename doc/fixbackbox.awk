#!/usr/bin/awk
#

NF == 16 && $16 == 5 {
	$7 = 1
	print $0
	next;
}

{ print $0; }
