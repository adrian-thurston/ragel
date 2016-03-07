#!/bin/bash
#

diff -U0 \
<(
  for i in `seq 0 255`; do
    printf "0x%02x\\n" $i
  done
) \
<(sed -n '/^ *$/d; s/#define [A-Z0-9_]* *//; p' in | sort | uniq)
