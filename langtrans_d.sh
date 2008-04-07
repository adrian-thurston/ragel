#!/bin/bash
#

file=$1

[ -f $file ] || exit 1

# Get the amchine name.
machine=`sed -n 's/^[\t ]*machine[\t ]*\([a-zA-Z_0-9]*\)[\t ]*;[\t ]*$/\1/p' $file`

# Make a temporary version of the test case the D language translations.
sed -n '/\/\*/,/\*\//d;p' $file | txl -q stdin langtrans_d.txl > $file.pr

# Begin writing out the test case.
cat << EOF
/*
 * @LANG: d
 * @GENERATED: yes
EOF

grep '@ALLOW_GENFLAGS:' $file
grep '@ALLOW_MINFLAGS:' $file

cat << EOF
 */
import std.stdio;
import std.string;

class $machine
{
EOF

# Write the data declarations
sed -n '/^%%$/q;{s/^/\t/;p}' $file.pr

# Write out the machine specification.
sed -n '/^%%{$/,/^}%%/{s/^/\t/;p}' $file.pr

# Write out the init and execute routines.
cat << EOF
	int cs;
	%% write data;
	void init()
	{
EOF

sed -n '0,/^%%$/d; /^%%{$/q; {s/^/\t\t/;p}' $file.pr

cat << EOF
		%% write init;
	}

	void exec( char data[] )
	{
		char *p = data.ptr;
		char *pe = data.ptr + data.length;
		char *eof = pe;
		char _s[];

		%% write exec;
	}

	void finish( )
	{
		if ( cs >= ${machine}_first_final )
			writefln( "ACCEPT" );
		else
			writefln( "FAIL" );
	}

EOF

# Write out the test data.
sed -n '0,/\/\* _____INPUT_____/d; /_____INPUT_____ \*\//q; p;' $file | awk '
BEGIN {
	print "	char[][] inp = ["
}
{
	print "		" $0 ","
}
END {
	print "	];"
	print ""
	print "	int inplen = " NR ";"
}'

# Write out the main routine.
cat << EOF
}

int main( )
{
	$machine m = new $machine();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}
/* _____OUTPUT_____
EOF

# Write out the expected output.
sed -n '0,/\/\* _____OUTPUT_____/d; /_____OUTPUT_____ \*\//q; p;' $file
echo "*/"

# Don't need this language-specific file anymore.
rm $file.pr
