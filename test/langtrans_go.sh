#!/bin/bash
#

file=$1

[ -f $file ] || exit 1
root=${file%.rl}

# Get the machine name.
machine=`sed -n 's/^[\t ]*machine[\t ]*\([a-zA-Z_0-9]*\)[\t ]*;[\t ]*$/\1/p' $file`


# Make a temporary version of the test case using the Java language translations.
sed -n '/\/\*/,/\*\//d;p' $file | txl -q stdin langtrans_go.txl > $file.pr

needs_eof=`sed '/@NEEDS_EOF/s/^.*$/yes/p;d' $file`
if [ "$needs_eof" != 'yes' ]; then
	needs_eof=`sed -n '/\/\*/,/\*\//d;p' $file | txl -q stdin checkeofact.txl`
fi

# Begin writing out the test case.
cat << EOF
/*
 * @LANG: go
 * @GENERATED: yes
EOF

grep '@ALLOW_GENFLAGS:' $file
grep '@ALLOW_MINFLAGS:' $file

cat << EOF
 */

package main

import "fmt"

EOF

# Write the data declarations
sed -n '/^%%$/q;{s/^/\t/;p}' $file.pr

# Write out the machine specification.
sed -n '/^%%{$/,/^}%%/{s/^/\t/;p}' $file.pr

# Write out the init and execute routines.
cat << EOF

var	cs int
%% write data;

func prepare() {
EOF

sed -n '0,/^%%$/d; /^%%{$/q; {s/^/\t\t/;p}' $file.pr

cat << EOF
    %% write init;
}

func exec(data string) {
    var p int = 0
    var pe int = len(data)
EOF

[ "$needs_eof" = "yes" ] && echo "    var eof int = pe"

cat << EOF

    %% write exec;
}

func finish() {
    if cs >= %%{ write first_final; }%% {
        fmt.Println("ACCEPT")
    } else {
        fmt.Println("FAIL")
    }
}
EOF

# Write out the test data.
sed -n '0,/\/\* _____INPUT_____/d; /_____INPUT_____ \*\//q; p;' $file | awk '
BEGIN {
	printf "var inp []string = []string{"
}
{
	printf "%s, ", $0
}
END {
	print "}"
	print ""
}'


# Write out the main routine.
cat << EOF

func main() {
    for _, data := range inp {
        prepare()
        exec(data)
        finish()
    }
}
EOF

# Write out the expected output.
sed -n '/\/\* _____OUTPUT_____/,/_____OUTPUT_____ \*\//p;' $file

# Don't need this language-specific file anymore.
rm $file.pr
