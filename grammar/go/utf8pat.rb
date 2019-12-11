# utf8pat.rb
#
# Generate utf8-encoded ragel or colm patterns for unicode code character sets.
#
# The dict structure is a hash mapping upper ends of ranges to a hash that
# contains:
#  1. The lower end.
#  2. A dict for the tail of the pattern.
#
# We index by the upper end of the range because we assume all unicode points
# to be read in increasing order and we check for extension as we add points by
# looking up the upper end.
#
# dict: { upper => { :lower => lower, :dict => dict } }
#

#
# utf8pat.rb <category-regex> <indentation-level> <unicode-data-file>
#
#
# ruby utf8pat.rb 'L[lutmo]' 1 UnicodeData.txt

target_category = Regexp.new( ARGV[0] )
indentation_level = ARGV[1].to_i
unicode_data = ARGV[2]

def utf8_enc( n )
	if n <= 0x7F
		return [ n ]
	elsif n <= 0x7FF
		return [
			0xC0 | (n >> 6),
			0x80 | (n & 0x3F)
		]
	elsif n <= 0xFFFF
		return [
			0xE0 | (n >> 12),
			0x80 | (n >>  6) & 0x3F,
			0x80 |  n        & 0x3F
		]
	elsif n <= 0x10ffff
		return [
			0xF0 | (n >> 18),
			0x80 | (n >> 12) & 0x3F,
			0x80 | (n >>  6) & 0x3F,
			0x80 |  n        & 0x3F
		]
	end
end

def add_to_dict( dict, utf8val )
	return if utf8val.size == 0 
		
	nk = utf8val[0]

	if utf8val.size == 1 && nk > 0 && dict.key?( nk - 1 )
		dict[nk] = dict[nk - 1]
		dict.delete( nk - 1 )
	else
		if ! dict.key?( utf8val[0] )
			dict[nk] = { :lower => nk, :dict => {} }
		end
	end

	add_to_dict( dict[nk][:dict], utf8val[1..-1] )
end

def compare( dict1, dict2 )
	# First check if we have equal size. If so, iterate dict1 and endsure key
	# is present in dict2. Then check lower end of the range matches and
	# recurse on the tails.
	return false if dict1.size != dict2.size

	dict1.each do |key, value|
		return false if !dict2.key?( key )

		return false if value[:lower] != dict2[key][:lower]

		return false if !compare( value[:dict], dict2[key][:dict] )
	end

	return true
end

def merge( dict )
	previous = nil
	dict.each do |key, value|
		# First recurse, ensuring dict is merged.
		merge( value[:dict] )
		if !previous.nil? && ( previous + 1 ) == value[:lower] &&
				compare( dict[previous][:dict], value[:dict] )
			# The previous and cur entries make a contiguous range AND the two
			# tails are identical patterns.
			value[:lower] = dict[previous][:lower]
			dict[previous][:lower] = -1
			dict.delete( previous )
		end

		previous = key
	end
end

def indent( level )
	for l in 1..level
		print "\t"
	end
end

def print_level( level, dict )
	first = true
	dict.each do |key, value|
		print " |\n" if !first

		indent( level )
		if value[:lower] != key
			print "0x%02X .. " % value[:lower]
		end

		print "0x%02X" % key
		if value[:dict].size > 0 
			print " (\n"
			print_level( level + 1, value[:dict] )
			indent( level )
			print ")"
		end
		first = false
	end
	print "\n"
end

file = open( unicode_data )
dict = {}

file.each_line do |line|
	next if line =~ /^[ \t\v]*#/;
	next if line =~ /^[ \t\v]*$/;
	range, description, category = line.split(/;/)

	if category =~ target_category
		add_to_dict( dict, utf8_enc( range.hex ) )
	end
end

merge( dict )
print_level( indentation_level, dict )
