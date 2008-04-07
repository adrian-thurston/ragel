#!/bin/bash
#

file=$1

[ -f $file ] || exit 1
root=${file%.rl}
class=${root}_csharp

# Make a temporary version of the test case using the Java language translations.
sed -n '/\/\*/,/\*\//d;p' $file | txl -q stdin langtrans_csharp.txl - $class > $file.pr

# Begin writing out the test case.
cat << EOF
/*
 * @LANG: csharp
 * @GENERATED: yes
EOF

grep '@ALLOW_GENFLAGS:' $file | sed 's/-G2//g'
grep '@ALLOW_MINFLAGS:' $file

cat << EOF
 */
using System;
// Disables lots of warnings that appear in the test suite
#pragma warning disable 0168, 0169, 0219, 0162, 0414
namespace Test {
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

	void exec( char[] data, int len )
	{
		int p = 0;
		int pe = len;
		int eof = len;
		string _s;
		%% write exec;
	}

	void finish( )
	{
		if ( cs >= ${class}_first_final )
			Console.WriteLine( "ACCEPT" );
		else
			Console.WriteLine( "FAIL" );
	}

EOF

# Write out the test data.
sed -n '0,/\/\* _____INPUT_____/d; /_____INPUT_____ \*\//q; p;' $file | awk '
BEGIN {
	print "	static readonly string[] inp = {"
}
{
	print "		" $0 ","
}
END {
	print "	};"
	print ""
	print "	static readonly int inplen = " NR ";"
}'


# Write out the main routine.
cat << EOF

	public static void Main (string[] args)
	{
		$class machine = new $class();
		for ( int i = 0; i < inplen; i++ ) {
			machine.init();
			machine.exec( inp[i].ToCharArray(), inp[i].Length );
			machine.finish();
		}
	}
}
}
EOF

# Write out the expected output.
sed -n '/\/\* _____OUTPUT_____/,/_____OUTPUT_____ \*\//p;' $file

# Don't need this language-specific file anymore.
rm $file.pr
