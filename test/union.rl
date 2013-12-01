/*
 * @LANG: c++
 * Show off concurrent abilities.
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#define BUFSIZE 2048

struct Concurrent
{
	int cur_char;
	int start_word;
	int start_comment;
	int start_literal;

	int cs;

	// Initialize the machine. Invokes any init statement blocks. Returns 0
	// if the machine begins in a non-accepting state and 1 if the machine
	// begins in an accepting state.
	void init( );

	// Execute the machine on a block of data. Returns -1 if after processing
	// the data, the machine is in the error state and can never accept, 0 if
	// the machine is in a non-accepting state and 1 if the machine is in an
	// accepting state.
	void execute( const char *data, int len );

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( );
};

%%{
	machine Concurrent;

	action next_char {
		cur_char += 1;
	}

	action start_word {
		start_word = cur_char;
	}
	action end_word {
		cout << "word: " << start_word << 
				" " << cur_char-1 << endl;
	}

	action start_comment {
		start_comment = cur_char;
	}
	action end_comment {
		cout << "comment: " << start_comment <<
				" " << cur_char-1 << endl;
	}

	action start_literal {
		start_literal = cur_char;
	}
	action end_literal {
		cout << "literal: " << start_literal <<
				" " << cur_char-1 << endl;
	}

	# Count characters.
	chars = ( any @next_char )*;

	# Words are non-whitespace. 
	word = ( any-space )+ >start_word %end_word;
	words = ( ( word | space ) $1 %0 )*;

	# Finds C style comments. 
	comment = ( '/*' any* $0 '*/'@1 ) >start_comment %end_comment;
	comments = ( ( comment | any ) $1 %0 )*;

	# Finds single quoted strings. 
	literalChar = ( any - ['\\] ) | ( '\\' . any );
	literal = ('\'' literalChar* '\'' ) >start_literal %end_literal;
	literals = ( ( literal | (any-'\'') ) $1 %0 )*;

	main := chars | words | comments | literals;
}%%

%% write data;

void Concurrent::init( )
{
	cur_char = 0;
	start_word = 0;
	start_comment = 0;
	start_literal = 0;
	%% write init;
}

void Concurrent::execute( const char *data, int len )
{
	const char *p = data;
	const char *pe = data + len;
	const char *eof = pe;

	%% write exec;
}

int Concurrent::finish( )
{
	if ( cs == Concurrent_error )
		return -1;
	if ( cs >= Concurrent_first_final )
		return 1;
	return 0;
}

void test( const char *buf )
{
	Concurrent concurrent;
	concurrent.init();
	concurrent.execute( buf, strlen(buf) );
	if ( concurrent.finish() > 0 )
		cout << "ACCEPT" << endl;
	else
		cout << "FAIL" << endl;
}

int main()
{
	test( 
		"/* in a comment,\n"
		" * ' and now in a literal string\n"
		" */ \n"
		" \n"
		"the comment has now ended but the literal string lives on\n"
		"\n"
		"' comment closed\n" );
	test( "/* * ' \\' */ \\' '\n" );
	test( "/**/'\\''/*/*/\n" );
	return 0;
}

#ifdef _____OUTPUT_____
word: 1 2
word: 4 5
word: 7 7
word: 9 16
word: 19 19
word: 21 21
word: 23 25
word: 27 29
word: 31 32
word: 34 34
word: 36 42
word: 44 49
word: 52 53
comment: 1 53
word: 58 60
word: 62 68
word: 70 72
word: 74 76
word: 78 82
word: 84 86
word: 88 90
word: 92 98
word: 100 105
word: 107 111
word: 113 114
word: 117 117
literal: 21 117
word: 119 125
word: 127 132
ACCEPT
word: 1 2
word: 4 4
word: 6 6
word: 8 9
word: 11 12
comment: 1 12
word: 14 15
word: 17 17
literal: 6 17
ACCEPT
comment: 1 4
literal: 5 8
word: 1 13
comment: 9 13
ACCEPT
#endif
