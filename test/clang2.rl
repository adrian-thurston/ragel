/*
 * @LANG: obj-c
 * A mini C-like language scanner.
 */

#include <stdio.h>
#include <objc/Object.h>
#include <string.h>

#define IDENT_BUFLEN 256

@interface Clang : Object
{
@public 
	/* State machine operation data. */
	int cs;

	/* Parsing data. */
	char identBuf[IDENT_BUFLEN+1];
	int identLen;
	int curLine;
};

- (void) initFsm;
- (void) executeWithData:(const char *)data len:(int)len;
- (int) finish;

@end

%%{ 
	machine Clang;

	# Function to buffer a character.
	action bufChar {
		if ( identLen < IDENT_BUFLEN ) {
			identBuf[identLen] = fc;
			identLen += 1;
		}
	}

	# Function to clear the buffer.
	action clearBuf {
		identLen = 0;
	}

	# Functions to dump tokens as they are matched.
	action ident {
		identBuf[identLen] = 0;
		printf("ident(%i): %s\n", curLine, identBuf);
	}
	action literal {
		identBuf[identLen] = 0;
		printf("literal(%i): %s\n", curLine, identBuf);
	}
	action float {
		identBuf[identLen] = 0;
		printf("float(%i): %s\n", curLine, identBuf);
	}
	action int {
		identBuf[identLen] = 0;
		printf("int(%i): %s\n", curLine, identBuf);
	}
	action hex {
		identBuf[identLen] = 0;
		printf("hex(%i): 0x%s\n", curLine, identBuf);
	}
	action symbol {
		identBuf[identLen] = 0;
		printf("symbol(%i): %s\n", curLine, identBuf);
	}

	# Alpha numberic characters or underscore.
	alnumu = alnum | '_';

	# Alpha charactres or underscore.
	alphau = alpha | '_';

	# Symbols. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving dump the symbol.
	symbol = ( punct - [_'"] ) >clearBuf $bufChar %symbol;

	# Identifier. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving, dump the identifier.
	ident = (alphau . alnumu*) >clearBuf $bufChar %ident;

	# Match single characters inside literal strings. Or match 
	# an escape sequence. Buffers the charater matched.
	sliteralChar =
			( extend - ['\\] ) @bufChar |
			( '\\' . extend @bufChar );
	dliteralChar =
			( extend - ["\\] ) @bufChar |
			( '\\' . extend @bufChar );

	# Single quote and double quota literals. At the start clear
	# the buffer. Upon leaving dump the literal.
	sliteral = ('\'' @clearBuf . sliteralChar* . '\'' ) %literal;
	dliteral = ('"' @clearBuf . dliteralChar* . '"' ) %literal;
	literal = sliteral | dliteral;

	# Whitespace is standard ws, newlines and control codes.
	whitespace = any - 0x21..0x7e;

	# Describe both c style comments and c++ style comments. The
	# priority bump on tne terminator of the comments brings us
	# out of the extend* which matches everything.
	ccComment = '//' . extend* $0 . '\n' @1;
	cComment = '/*' . extend* $0 . '*/' @1;

	# Match an integer. We don't bother clearing the buf or filling it.
	# The float machine overlaps with int and it will do it.
	int = digit+ %int;

	# Match a float. Upon entering the machine clear the buf, buffer
	# characters on every trans and dump the float upon leaving.
	float =  ( digit+ . '.' . digit+ ) >clearBuf $bufChar %float;

	# Match a hex. Upon entering the hex part, clear the buf, buffer characters
	# on every trans and dump the hex on leaving transitions.
	hex = '0x' . xdigit+ >clearBuf $bufChar %hex;

	# Or together all the lanuage elements.
	fin = ( ccComment |
		cComment |
		symbol |
		ident |
		literal |
		whitespace |
		int |
		float |
		hex );

	# Star the language elements. It is critical in this type of application
	# that we decrease the priority of out transitions before doing so. This
	# is so that when we see 'aa' we stay in the fin machine to match an ident
	# of length two and not wrap around to the front to match two idents of 
	# length one.
	clang_main = ( fin $1 %0 )*;

	# This machine matches everything, taking note of newlines.
	newline = ( any | '\n' @{ curLine += 1; } )*;

	# The final fsm is the lexer intersected with the newline machine which
	# will count lines for us. Since the newline machine accepts everything,
	# the strings accepted is goverened by the clang_main machine, onto which
	# the newline machine overlays line counting.
	main := clang_main & newline;
}%%

@implementation Clang

%% write data;

- (void) initFsm;
{
	identLen = 0;
	curLine = 1;
	%% write init;
}

- (void) executeWithData:(const char *)data len:(int)len;
{
	const char *p = data; 
	const char *pe = data + len;
	const char *eof = pe;

	%% write exec;
}

- (int) finish;
{
	if ( cs == Clang_error ) 
		return -1;
	if ( cs >= Clang_first_final ) 
		return 1;
	return 0;
}

@end

#define BUFSIZE 2048

Clang *fsm;
char buf[BUFSIZE];

void test( char *buf )
{
	int len = strlen(buf);
	fsm = [[Clang alloc] init];
	[fsm initFsm];
	[fsm executeWithData:buf len:len];
	if ( [fsm finish] > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}

int main()
{
	test( 
		"999 0xaAFF99 99.99 /*\n"
		"*/ 'lksdj' //\n"
		"\"\n"
		"\n"
		"literal\n"
		"\n"
		"\n"
		"\"0x00aba foobardd.ddsf 0x0.9\n" );

	test( 
		"wordwithnum00asdf\n"
		"000wordfollowsnum,makes new symbol\n"
		"\n"
		"finishing early /* unfinished ...\n" );

	test( 
		"/*\n"
		" *  Copyright\n"
		" */\n"
		"\n"
		"/*  Aapl.\n"
		" */\n"
		"\n"
		"#define _AAPL_RESIZE_H\n"
		"\n"
		"#include <assert.h>\n"
		"\n"
		"#ifdef AAPL_NAMESPACE\n"
		"namespace Aapl {\n"
		"#endif\n"
		"#define LIN_DEFAULT_STEP 256\n"
		"#define EXPN_UP( existing, needed ) \\\n"
		"		need > eng ? (ned<<1) : eing\n"
		"	\n"
		"\n"
		"/*@}*/\n"
		"#undef EXPN_UP\n"
		"#ifdef AAPL_NAMESPACE\n"
		"#endif /* _AAPL_RESIZE_H */\n" );
	return 0;
}

#ifdef _____OUTPUT_____
int(1): 999
hex(1): 0xaAFF99
float(1): 99.99
literal(2): lksdj
literal(8): 

literal



hex(8): 0x00aba
ident(8): foobardd
symbol(8): .
ident(8): ddsf
hex(8): 0x0
symbol(8): .
int(8): 9
ACCEPT
ident(1): wordwithnum00asdf
int(2): 000
ident(2): wordfollowsnum
symbol(2): ,
ident(2): makes
ident(2): new
ident(2): symbol
ident(4): finishing
ident(4): early
FAIL
symbol(8): #
ident(8): define
ident(8): _AAPL_RESIZE_H
symbol(10): #
ident(10): include
symbol(10): <
ident(10): assert
symbol(10): .
ident(10): h
symbol(10): >
symbol(12): #
ident(12): ifdef
ident(12): AAPL_NAMESPACE
ident(13): namespace
ident(13): Aapl
symbol(13): {
symbol(14): #
ident(14): endif
symbol(15): #
ident(15): define
ident(15): LIN_DEFAULT_STEP
int(15): 256
symbol(16): #
ident(16): define
ident(16): EXPN_UP
symbol(16): (
ident(16): existing
symbol(16): ,
ident(16): needed
symbol(16): )
symbol(16): \
ident(17): need
symbol(17): >
ident(17): eng
symbol(17): ?
symbol(17): (
ident(17): ned
symbol(17): <
symbol(17): <
int(17): 1
symbol(17): )
symbol(17): :
ident(17): eing
symbol(21): #
ident(21): undef
ident(21): EXPN_UP
symbol(22): #
ident(22): ifdef
ident(22): AAPL_NAMESPACE
symbol(23): #
ident(23): endif
ACCEPT
#endif
