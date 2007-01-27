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
		opt=`awk -f extract.awk -vexname=$fig $input | 
				sed '/^ *OPT:/s/^.*: *//p;d'`
		awk -f extract.awk -vexname=$fig $input | \
			ragel | rlcodegen -V $opt | \
			dot -Tfig | awk -f fixbackbox.awk > ${fig}.fig;
	else
		echo "$0: internal error: figure $fig not found in $input" >&2
		exit 1
	fi
done

