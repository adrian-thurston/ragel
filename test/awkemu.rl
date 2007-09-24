/*
 * @LANG: c
 */

/*
 * Emulate the basic parser of the awk program. Breaks lines up into
 * words and prints the words.
 */

#include <stdio.h>
#include <string.h>

#define LINEBUF 2048
static char lineBuf[LINEBUF];
static char blineBuf[LINEBUF];
static int lineLen;
static int blineLen;
static int words;

void finishLine();

struct awkemu
{
	int cs;
};

%%{
	machine awkemu;

	variable cs fsm->cs;

	# Starts a line. Will initialize all the data necessary for capturing the line.
	action startline {
		lineLen = 0;	
		blineLen = 0;	
		words = 0;
	}

	# Will be executed on every character seen in a word. Captures the word
	# to the broken up line buffer.
	action wordchar {
		blineBuf[blineLen++] = fc;
	}

	# Terminate a word. Adds the null after the word and increments the word count
	# for the line.
	action termword {
		blineBuf[blineLen++] = 0;
		words += 1;
	}

	# Will be executed on every character seen in a line (not including 
	# the newline itself.
	action linechar {
		lineBuf[lineLen++] = fc;
	}

	# This section of the machine deals with breaking up lines into fields.
	# Lines are separed by the whitespace and put in an array of words.

	# Words in a line.
	word = (extend - [ \t\n])+;

	# The whitespace separating words in a line.
	whitespace = [ \t];

	# The components in a line to break up. Either a word or a single char of
	# whitespace. On the word capture characters.
	blineElements = word $wordchar %termword | whitespace;

	# Star the break line elements. Just be careful to decrement the leaving
	# priority as we don't want multiple character identifiers to be treated as
	# multiple single char identifiers.
	breakLine = ( blineElements $1 %0 )* . '\n';

	# This machine lets us capture entire lines. We do it separate from the words
	# in a line.
	bufLine = (extend - '\n')* $linechar %{ finishLine(); } . '\n';

	# A line can then consist of the machine that will break up the line into
	# words and a machine that will buffer the entire line. 
	line = ( breakLine | bufLine ) > startline;

	# Any number of lines.
	main := line*;
}%%

void finishLine()
{
	int i;
	char *pword = blineBuf;
	lineBuf[lineLen] = 0;
	printf("endline(%i): %s\n", words, lineBuf );
	for ( i = 0; i < words; i++ ) {
		printf("  word: %s\n", pword );
		pword += strlen(pword) + 1;
	}
}

%% write data;

void awkemu_init( struct awkemu *fsm )
{
    %% write init;
}

void awkemu_execute( struct awkemu *fsm, const char *_data, int _len )
{
    const char *p = _data;
    const char *pe = _data+_len;
	%% write exec;
}

int awkemu_finish( struct awkemu *fsm )
{
	if ( fsm->cs == awkemu_error ) 
		return -1;
	if ( fsm->cs >= awkemu_first_final ) 
		return 1;
	return 0;
}

#include <stdio.h>
#define BUFSIZE 2048

struct awkemu fsm;
char buf[BUFSIZE];

void test( char *buf )
{
	int len = strlen( buf );
	awkemu_init( &fsm );
	awkemu_execute( &fsm, buf, len );
	if ( awkemu_finish( &fsm ) > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}

int main()
{
	test( "" );
	test( "one line with no newline" );
	test( "one line\n" );
	return 0;
}

#ifdef _____OUTPUT_____
ACCEPT
FAIL
endline(2): one line
  word: one
  word: line
ACCEPT
#endif
