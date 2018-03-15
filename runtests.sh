#!/bin/bash

SUBDIRS="aapl ragel colm"

for d in $SUBDIRS; do

	cd $d
	./runtests
	cd ..

done
