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
			git rm ${fn%.lm}.args &>/dev/null
		fi

		if [ -f ${fn%.lm}.in ]; then
			echo '##### IN #####';
			cat ${fn%.lm}.in
			git rm ${fn%.lm}.in &>/dev/null
		fi

		if [ -f ${fn%.lm}.exp ]; then
			echo '##### EXP #####';
			cat ${fn%.lm}.exp
			git rm ${fn%.lm}.exp &>/dev/null
		fi

	) \
	> ${fn%.lm}.tst;
	cat ${fn%.lm}.tst > $fn;
	rm ${fn%.lm}.tst;
	git add $fn
done
