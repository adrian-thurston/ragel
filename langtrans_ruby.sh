#!/bin/bash
#

file=$1

[ -f $file ] || exit 1

# Get the machine name.
machine=`sed -n 's/^[\t ]*machine[\t ]*\([a-zA-Z_0-9]*\)[\t ]*;[\t ]*$/\1/p' \
	$file | tr '[A-Z]' '[a-z]'`

# Make a temporary version of the test case using the Ruby language translations.
sed -n '/\/\*/,/\*\//d;p' $file | txl -q stdin langtrans_ruby.txl > $file.pr

# Begin writing out the test case.
cat << EOF
#
# @LANG: ruby
# @GENERATED: yes
EOF

grep '@ALLOW_GENFLAGS:' $file | sed 's/^ *\*/#/' | sed 's/-G.//g'
grep '@ALLOW_MINFLAGS:' $file | sed 's/^ *\*/#/'

cat << EOF
#

EOF

# Write out the machine specification.
sed -n '/^%%{$/,/^}%%/{s/^/\t/;p}' $file.pr

# Write out the init and execute routines.
cat << EOF

	%% write data;

	def run_machine( data )
		p = 0
		pe = data.length
		eof = data.length
		cs = 0;
EOF

# Write the data declarations
sed -n '/^%%$/q;{s/^/\t/;p}' $file.pr

# Write the data initializations
sed -n '0,/^%%$/d; /^%%{$/q; {s/^/\t\t/;p}' $file.pr

cat << EOF

		%% write init;
		%% write exec;
		if cs >= ${machine}_first_final
			puts "ACCEPT"
		else
			puts "FAIL"
		end
	end

EOF

# Write out the test data.
sed -n '0,/\/\* _____INPUT_____/d; /_____INPUT_____ \*\//q; p;' $file | awk '
BEGIN {
	print "	inp = ["
}
{
	print "		" $0 ","
}
END {
	print "	]"
	print ""
	print "	inplen = " NR ";"
}'


# Write out the main routine.
cat << EOF

	inp.each { |str| 
		run_machine(str.unpack("c*"))
	}

EOF

# Write out the expected output.
echo "=begin _____OUTPUT_____"

sed -n '/\/\* _____OUTPUT_____/,/_____OUTPUT_____ \*\//{/_____OUTPUT_____/d;p;};' $file

echo "=end _____OUTPUT_____"

# Don't need this language-specific file anymore.
rm $file.pr
