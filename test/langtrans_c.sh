#!/bin/bash
#

file=$1

[ -f $file ] || exit 1

# Get the machine name.
machine=`sed -n 's/^[\t ]*machine[\t ]*\([a-zA-Z_0-9]*\)[\t ]*;[\t ]*$/\1/p' $file`

# Make a temporary version of the test case using the C language translations.
sed -n '/\/\*/,/\*\//d;p' $file | txl -q stdin langtrans_c.txl > $file.pr

# Begin writing out the test case.
cat << EOF
/*
 * @LANG: c
 * @GENERATED: yes
 */
#include <string.h>
#include <stdio.h>
EOF

# Write the data declarations
sed -n '/^%%$/q;p' $file.pr

# Write out the machine specification.
sed -n '/^%%{$/,/^}%%/p' $file.pr

# Write out the init and execute routines.
cat << EOF
int cs;
%% write data;
void init()
{
EOF

sed -n '0,/^%%$/d; /^%%{$/q; {s/^/\t/;p}' $file.pr

cat << EOF
	%% write init;
}

void exec( char *data, int len )
{
	char *p = data;
	char *pe = data + len;
	%% write exec;
}

void finish( )
{
	%% write eof;
	if ( cs >= ${machine}_first_final )
		printf( "ACCEPT\\n" );
	else
		printf( "FAIL\\n" );
}
EOF

# Write out the test data.
sed -n '0,/\/\* _____INPUT_____/d; /_____INPUT_____ \*\//q; p;' $file | awk '
BEGIN {
	print "char *inp[] = {"
}
{
	print "	" $0 ","
}
END {
	print "};"
	print ""
	print "int inplen = " NR ";"
}'

# Write out the main routine.
cat << EOF

int main( )
{
	int i;
	for ( i = 0; i < inplen; i++ ) {
		init();
		exec( inp[i], strlen(inp[i]) );
		finish();
	}
	return 0;
}
#ifdef _____OUTPUT_____
EOF

# Write out the expected output.
sed -n '0,/\/\* _____OUTPUT_____/d; /_____OUTPUT_____ \*\//q; p;' $file
echo "#endif"

# Don't need this language-specific file anymore.
rm $file.pr
