#!/bin/bash
#

input=ragel-guide.tex

for fig; do
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

