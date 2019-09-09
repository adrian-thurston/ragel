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

mkdir -p working

rm -f working/*

[ $# = 0 ] && set -- $TESTS

for t; do
	rm -f working/$t.sh

	echo echo testing $t >> working/$t.sh
	echo ./$t '>' working/$t.out >> working/$t.sh
	echo diff working/$t.out $t.exp '>' working/$t.diff >> working/$t.sh

	echo working/$t.sh
done
