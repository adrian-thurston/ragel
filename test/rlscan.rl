/*
 * Lexes Ragel input files.
 *
 * @LANG: c++
 * @PROHIBIT_FEATURES: --var-backend
 *
 * Test works with split code gen.
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef PERF_TEST

/* Calibrated to 1s on yoho. */
#define perf_iters ( 240984ll * S )

int _perf_dummy = 0;
#define perf_cout(...) ( _perf_dummy += 1 )
#define perf_loop long _pi; for ( _pi = 0; _pi < perf_iters; _pi++ )

#else

#define perf_cout(...) __VA_ARGS__
#define perf_loop

#endif

using namespace std;

void escapeXML( const char *data )
{
	while ( *data != 0 ) {
		switch ( *data ) {
			case '<': perf_cout( cout << "&lt;" ); break;
			case '>': perf_cout( cout << "&gt;" ); break;
			case '&': perf_cout( cout << "&amp;" ); break;
			default: perf_cout( cout << *data ); break;
		}
		data += 1;
	}
}

void escapeXML( char c )
{
	switch ( c ) {
		case '<': perf_cout( cout << "&lt;" ); break;
		case '>': perf_cout( cout << "&gt;" ); break;
		case '&': perf_cout( cout << "&amp;" ); break;
		default: perf_cout( cout << c ); break;
	}
}

void escapeXML( const char *data, int len )
{
	for ( const char *end = data + len; data != end; data++  ) {
		switch ( *data ) {
			case '<': perf_cout( cout << "&lt;" ); break;
			case '>': perf_cout( cout << "&gt;" ); break;
			case '&': perf_cout( cout << "&amp;" ); break;
			default: perf_cout( cout << *data ); break;
		}
	}
}

inline void write( const char *data )
{
	perf_cout( cout << data );
}

inline void write( char c )
{
	perf_cout( cout << c );
}

inline void write( const char *data, int len )
{
	perf_cout( cout.write( data, len ) );
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

void test( const char *data )
{
	std::ios::sync_with_stdio(false);

	int cs, act;
	perf_loop
	{
		int len = strlen( data );
		const char *ts, *te;
		int stack[1], top;
		memset( stack, 0, sizeof(stack) );

		bool single_line = false;
		int inline_depth = 0;

		%% write init;

		/* Read in a block. */
		const char *p = data;
		const char *pe = data + len;
		const char *eof = pe;

		%% write exec;
	}

	if ( cs == RagelScan_error ) {
		/* Machine failed before finding a token. */
		perf_cout( cerr << "PARSE ERROR" << endl );
		exit(1);
	}
}

#define BUFSIZE 2048

int main()
{
	std::ios::sync_with_stdio(false);

	test(
		"hi %%{ /'}%%'/ { /*{*/ {} } + '\\'' }%%there\n"
		"hi %%{ /'}%%'/ { /*{*/ {} } + '\\'' }%%there\n"
		"hi %%{ /'}%%'/ { /*{*/ {} } + '\\'' }%%there\n"
		"hi %%{ /'}%%'/ { /*{*/ {} } + '\\'' }%%there\n"
		"hi %%{ /'}%%'/ { /*{*/ {} } + '\\'' }%%there\n"
		"hi %%{ /'}%%'/ { /*{*/ {} } + '\\'' }%%there\n"
	);

	return 0;
}
##### OUTPUT #####
hi <section>
<re_lit>/'}%%'/</re_lit>
<inline>{ /*{*/ {} }</inline>
<symbol>+</symbol>
<single_lit>'\''</single_lit>
</section>
there
hi <section>
<re_lit>/'}%%'/</re_lit>
<inline>{ /*{*/ {} }</inline>
<symbol>+</symbol>
<single_lit>'\''</single_lit>
</section>
there
hi <section>
<re_lit>/'}%%'/</re_lit>
<inline>{ /*{*/ {} }</inline>
<symbol>+</symbol>
<single_lit>'\''</single_lit>
</section>
there
hi <section>
<re_lit>/'}%%'/</re_lit>
<inline>{ /*{*/ {} }</inline>
<symbol>+</symbol>
<single_lit>'\''</single_lit>
</section>
there
hi <section>
<re_lit>/'}%%'/</re_lit>
<inline>{ /*{*/ {} }</inline>
<symbol>+</symbol>
<single_lit>'\''</single_lit>
</section>
there
hi <section>
<re_lit>/'}%%'/</re_lit>
<inline>{ /*{*/ {} }</inline>
<symbol>+</symbol>
<single_lit>'\''</single_lit>
</section>
there
