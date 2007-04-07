/*
 * Lexes Ragel input files.
 *
 * @LANG: c++
 *
 * Test works with split code gen.
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

inline void write( char *data )
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
		escapeXML( tokstart, tokend-tokstart );
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

		default => { escapeXML( *tokstart ); };
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
			write( tokstart, tokend-tokstart );
			write( "</word>\n" );
		};

		# Decimal integer.
		integer {
			write( "<int>" );
			write( tokstart, tokend-tokstart );
			write( "</int>\n" );
		};

		# Hexidecimal integer.
		hex {
			write( "<hex>" );
			write( tokstart, tokend-tokstart );
			write( "</hex>\n" );
		};

		# Consume comments.
		'#' [^\n]* '\n';

		# Single literal string.
		"'" ( [^'\\] | /\\./ )* "'" {
			write( "<single_lit>" );
			escapeXML( tokstart, tokend-tokstart );
			write( "</single_lit>\n" );
		};

		# Double literal string.
		'"' ( [^"\\] | /\\./ )* '"' {
			write( "<double_lit>" );
			escapeXML( tokstart, tokend-tokstart );
			write( "</double_lit>\n" );
		};

		# Or literal.
		'[' ( [^\]\\] | /\\./ )* ']' {
			write( "<or_lit>" );
			escapeXML( tokstart, tokend-tokstart );
			write( "</or_lit>\n" );
		};

		# Regex Literal.
		'/' ( [^/\\] | /\\./ ) * '/' {
			write( "<re_lit>" );
			escapeXML( tokstart, tokend-tokstart );
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
			escapeXML( tokstart, tokend-tokstart );
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
			escapeXML( *tokstart );
		};

		# EOF.
		EOF;
	*|;
}%%

%% write data nofinal;

void test( char *data )
{
	std::ios::sync_with_stdio(false);

	int cs, act;
	char *tokstart, *tokend;
	int stack[1], top;

	bool single_line = false;
	int inline_depth = 0;

	%% write init;

	/* Read in a block. */
	char *p = data;
	char *pe = data + strlen( data );
	%% write exec;

	if ( cs == RagelScan_error ) {
		/* Machine failed before finding a token. */
		cerr << "PARSE ERROR" << endl;
		exit(1);
	}
}

#define BUFSIZE 2048

int main()
{
	std::ios::sync_with_stdio(false);

	test("hi %%{ /'}%%'/ { /*{*/ {} } + '\\'' }%%there\n");

	return 0;
}
#ifdef _____OUTPUT_____
hi <section>
<re_lit>/'}%%'/</re_lit>
<inline>{ /*{*/ {} }</inline>
<symbol>+</symbol>
<single_lit>'\''</single_lit>
</section>
there
#endif
