/*
 * Lexes Ragel input files.
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace std;

void escapeXML( char *data )
{
	while ( *data != 0 ) {
		switch ( *data ) {
			case '<': cout << "&lt;"; break;
			case '>': cout << "&gt;"; break;
			case '&': cout << "&amp;"; break;
			default: cout << *data; break;
		}
		data += 1;
	}
}

void escapeXML( char c )
{
	switch ( c ) {
		case '<': cout << "&lt;"; break;
		case '>': cout << "&gt;"; break;
		case '&': cout << "&amp;"; break;
		default: cout << c; break;
	}
}

void escapeXML( char *data, int len )
{
	for ( char *end = data + len; data != end; data++  ) {
		switch ( *data ) {
			case '<': cout << "&lt;"; break;
			case '>': cout << "&gt;"; break;
			case '&': cout << "&amp;"; break;
			default: cout << *data; break;
		}
	}
}

inline void write( const char *data )
{
	cout << data;
}

inline void write( char c )
{
	cout << c;
}

inline void write( char *data, int len )
{
	cout.write( data, len );
}


%%{
	machine RagelScan;

	word = [a-zA-Z_][a-zA-Z_0-9]*;
	integer = [0-9]+;
	hex = '0x' [0-9a-fA-F] [0-9a-fA-F]*;

	default = ^0;
	EOF = 0;

	# Handles comments in outside code and inline blocks.
	c_comment := 
		( default* :>> '*/' )
		${ escapeXML( fc ); }
		@{ fret; };

	action emit {
		escapeXML( ts, te-ts );
	}

	#
	# Inline action code
	#

	ilscan := |*

		"'" ( [^'\\] | /\\./ )* "'" => emit;
		'"' ( [^"\\] | /\\./ )* '"' => emit;
		'/*' {
			write( "/*" );
			fcall c_comment;
		};
		'//' [^\n]* '\n' => emit;

		'{' {
			write( '{' );
			inline_depth += 1; 
		};

		'}' {
			write( '}' );
			/* If dropping down to the last } then return 
			 * to ragel code. */
			if ( --inline_depth == 0 ) {
				write( "</inline>\n" );
				fgoto rlscan;
			}
		};

		default => { escapeXML( *ts ); };
	*|;

	#
	# Ragel Tokens
	#

	rlscan := |*
		'}%%' {
			if ( !single_line ) {
				write( "</section>\n" );
				fgoto main;
			}
		};

		'\n' {
			if ( single_line ) {
				write( "</section>\n" );
				fgoto main;
			}
		};

		# Word
		word {
			write( "<word>" );
			write( ts, te-ts );
			write( "</word>\n" );
		};

		# Decimal integer.
		integer {
			write( "<int>" );
			write( ts, te-ts );
			write( "</int>\n" );
		};

		# Hexidecimal integer.
		hex {
			write( "<hex>" );
			write( ts, te-ts );
			write( "</hex>\n" );
		};

		# Consume comments.
		'#' [^\n]* '\n';

		# Single literal string.
		"'" ( [^'\\] | /\\./ )* "'" {
			write( "<single_lit>" );
			escapeXML( ts, te-ts );
			write( "</single_lit>\n" );
		};

		# Double literal string.
		'"' ( [^"\\] | /\\./ )* '"' {
			write( "<double_lit>" );
			escapeXML( ts, te-ts );
			write( "</double_lit>\n" );
		};

		# Or literal.
		'[' ( [^\]\\] | /\\./ )* ']' {
			write( "<or_lit>" );
			escapeXML( ts, te-ts );
			write( "</or_lit>\n" );
		};

		# Regex Literal.
		'/' ( [^/\\] | /\\./ ) * '/' {
			write( "<re_lit>" );
			escapeXML( ts, te-ts );
			write( "</re_lit>\n" );
		};

		# Open an inline block
		'{' {
			inline_depth = 1;
			write( "<inline>{" );
			fgoto ilscan;
		};

		punct {
			write( "<symbol>" );
			escapeXML( fc );
			write( "</symbol>\n" );
		};
		
		default;
	*|;

	#
	# Outside code.
	#

	main := |*

		"'" ( [^'\\] | /\\./ )* "'" => emit;
		'"' ( [^"\\] | /\\./ )* '"' => emit;

		'/*' {
			escapeXML( ts, te-ts );
			fcall c_comment;
		};

		'//' [^\n]* '\n' => emit;

		'%%{' { 
			write( "<section>\n" );
			single_line = false;
			fgoto rlscan;
		};

		'%%' {
			write( "<section>\n" ); 
			single_line = true; 
			fgoto rlscan;
		};

		default { 
			escapeXML( *ts );
		};

		# EOF.
		EOF;
	*|;
}%%

%% write data nofinal;

#define BUFSIZE 2048

int main()
{
	std::ios::sync_with_stdio(false);

	int cs, act;
	char *ts, *te;
	int stack[1], top;

	static char inbuf[BUFSIZE];
	bool single_line = false;
	int inline_depth = 0;

	%% write init;

	bool done = false;
	int have = 0;
	while ( !done ) {
		/* How much space is in the buffer? */
		int space = BUFSIZE - have;
		if ( space == 0 ) {
			/* Buffer is full. */
			cerr << "TOKEN TOO BIG" << endl;
			exit(1);
		}

		/* Read in a block. */
		char *p = inbuf + have;
		cin.read( p, space );
		int len = cin.gcount();
		char *pe = p + len;
		char *eof = 0;

		/* Check for EOF. */
		if ( len == 0 ) {
			eof = pe;
			done = true;
		}

		%% write exec;

		if ( cs == RagelScan_error ) {
			/* Machine failed before finding a token. */
			cerr << "PARSE ERROR" << endl;
			exit(1);
		}

		if ( ts == 0 )
			have = 0;
		else {
			/* There is a prefix to preserve, shift it over. */
			have = pe - ts;
			memmove( inbuf, ts, have );
			te = inbuf + (te-ts);
			ts = inbuf;
		}
	}
	return 0;
}
