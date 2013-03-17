#!/bin/bash
#

for fn in $@; do
	[ -f $fn ] || continue
	(
		echo '##### LM #####';
		cat $fn

		if [ -f ${fn%.lm}.args ]; then
			echo '##### ARGS #####';
			cat ${fn%.lm}.args
			rm ${fn%.lm}.args
		fi

		if [ -f ${fn%.lm}.in ]; then
			echo '##### IN #####';
			cat ${fn%.lm}.in
			rm ${fn%.lm}.in
		fi

		if [ -f ${fn%.lm}.exp ]; then
			echo '##### EXP #####';
			cat ${fn%.lm}.exp
			rm ${fn%.lm}.exp
		fi

	) \
	> ${fn%.lm}.tst;
	cat ${fn%.lm}.tst > $fn;
	rm ${fn%.lm}.tst;
done
