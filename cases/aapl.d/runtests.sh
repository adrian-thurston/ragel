#!/bin/bash
#

TESTS="
	test_allavl
	test_allsort
	test_avlikeyless
	test_avliter
	test_avlkeyless
	test_bstmap
	test_bstset
	test_bsttable
	test_bubblesort
	test_compare
	test_dlistval
	test_doublelist
	test_insertsort
	test_mergesort
	test_quicksort
	test_rope
	test_sbstmap
	test_sbstset
	test_sbsttable
	test_string
	test_svector
	test_vector
"

for t in $TESTS; do
	./$t > $t.out

	if diff $t.out $t.exp >/dev/null; then
		echo $t test passed
	else
		echo $t test FAILED
	fi
done
