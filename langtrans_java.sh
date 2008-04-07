#!/bin/bash
#

file=$1

[ -f $file ] || exit 1
root=${file%.rl}
class=${root}_java

# Make a temporary version of the test case using the Java language translations.
sed -n '/\/\*/,/\*\//d;p' $file | txl -q stdin langtrans_java.txl - $class > $file.pr

# Begin writing out the test case.
cat << EOF
/*
 * @LANG: java
 * @GENERATED: yes
EOF

grep '@ALLOW_GENFLAGS:' $file
grep '@ALLOW_MINFLAGS:' $file

cat << EOF
 */

class $class
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

	void exec( char data[], int len )
	{
		int p = 0;
		int pe = len;
		int eof = len;
		String _s;
		%% write exec;
	}

	void finish( )
	{
		if ( cs >= ${class}_first_final )
			System.out.println( "ACCEPT" );
		else
			System.out.println( "FAIL" );
	}

EOF

# Write out the test data.
sed -n '0,/\/\* _____INPUT_____/d; /_____INPUT_____ \*\//q; p;' $file | awk '
BEGIN {
	print "	static final String inp[] = {"
}
{
	print "		" $0 ","
}
END {
	print "	};"
	print ""
	print "	static final int inplen = " NR ";"
}'


# Write out the main routine.
cat << EOF

	public static void main (String[] args)
	{
		$class machine = new $class();
		for ( int i = 0; i < inplen; i++ ) {
			machine.init();
			machine.exec( inp[i].toCharArray(), inp[i].length() );
			machine.finish();
		}
	}
}

EOF

# Write out the expected output.
sed -n '/\/\* _____OUTPUT_____/,/_____OUTPUT_____ \*\//p;' $file

# Don't need this language-specific file anymore.
rm $file.pr
