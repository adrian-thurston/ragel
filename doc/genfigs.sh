#!/bin/bash
#

input=ragel-guide.tex

if [ $# = 0 ]; then
	figs=`awk '/^% GENERATE: *[a-z0-9A-Z_\.\-]+ *$/ {print $3;}' $input`
else
	figs="$@"
fi

for fig in $figs; do
	if awk -f extract.awk -vexname=$fig $input > /dev/null; then
		echo generating ${fig}.fig
		awk -f extract.awk -vexname=$fig $input | \
			ragel | rlcodegen -Vp | \
			dot -Tfig | awk -f fixbackbox.awk > ${fig}.fig;
	else
		echo "$0: internal error: figure $fig not found in $input" >&2
		exit 1
	fi
done

