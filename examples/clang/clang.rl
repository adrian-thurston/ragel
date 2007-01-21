/*
 * A mini C-like language scanner.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

%%{
	machine clang;

	newline = '\n' @{curline += 1;};
	any_count_line = any | newline;

	# Consume a C comment.
	c_comment := any_count_line* :>> '*/' @{fgoto main;};

	main := |*

	# Alpha numberic characters or underscore.
	alnum_u = alnum | '_';

	# Alpha charactres or underscore.
	alpha_u = alpha | '_';

	# Symbols. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving dump the symbol.
	( punct - [_'"] ) {
		printf( "symbol(%i): %c\n", curline, tokstart[0] );
	};

	# Identifier. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving, dump the identifier.
	alpha_u alnum_u* {
		printf( "ident(%i): ", curline );
		fwrite( tokstart, 1, tokend-tokstart, stdout );
		printf("\n");
	};

	# Single Quote.
	sliteralChar = [^'\\] | newline | ( '\\' . any_count_line );
	'\'' . sliteralChar* . '\'' {
		printf( "single_lit(%i): ", curline );
		fwrite( tokstart, 1, tokend-tokstart, stdout );
		printf("\n");
	};

	# Double Quote.
	dliteralChar = [^"\\] | newline | ( '\\' any_count_line );
	'"' . dliteralChar* . '"' {
		printf( "double_lit(%i): ", curline );
		fwrite( tokstart, 1, tokend-tokstart, stdout );
		printf("\n");
	};

	# Whitespace is standard ws, newlines and control codes.
	any_count_line - 0x21..0x7e;

	# Describe both c style comments and c++ style comments. The
	# priority bump on tne terminator of the comments brings us
	# out of the extend* which matches everything.
	'//' [^\n]* newline;

	'/*' { fgoto c_comment; };

	# Match an integer. We don't bother clearing the buf or filling it.
	# The float machine overlaps with int and it will do it.
	digit+ {
		printf( "int(%i): ", curline );
		fwrite( tokstart, 1, tokend-tokstart, stdout );
		printf("\n");
	};

	# Match a float. Upon entering the machine clear the buf, buffer
	# characters on every trans and dump the float upon leaving.
	digit+ '.' digit+ {
		printf( "float(%i): ", curline );
		fwrite( tokstart, 1, tokend-tokstart, stdout );
		printf("\n");
	};

	# Match a hex. Upon entering the hex part, clear the buf, buffer characters
	# on every trans and dump the hex on leaving transitions.
	'0x' xdigit+ {
		printf( "hex(%i): ", curline );
		fwrite( tokstart, 1, tokend-tokstart, stdout );
		printf("\n");
	};

	*|;
}%%

%% write data nofinal;

#define BUFSIZE 128

void scanner()
{
	static char buf[BUFSIZE];
	int cs, act, have = 0, curline = 1;
	char *tokstart, *tokend = 0;
	int done = 0;

	%% write init;

	while ( !done ) {
		char *p = buf + have, *pe;
		int len, space = BUFSIZE - have;
		
		if ( space == 0 ) {
			/* We've used up the entire buffer storing an already-parsed token
			 * prefix that must be preserved. */
			fprintf(stderr, "OUT OF BUFFER SPACE\n" );
			exit(1);
		}

		len = fread( p, 1, space, stdin );

		/* If this is the last buffer, tack on an EOF. */
		if ( len < space ) {
			p[len++] = 0;
			done = 1;
		}
			
		pe = p + len;
		%% write exec;

		if ( cs == clang_error ) {
			fprintf(stderr, "PARSE ERROR\n" );
			break;
		}

		if ( tokstart == 0 )
			have = 0;
		else {
			/* There is a prefix to preserve, shift it over. */
			have = pe - tokstart;
			memmove( buf, tokstart, have );
			tokend = buf + (tokend-tokstart);
			tokstart = buf;
		}
	}
}

int main()
{
	scanner();
	return 0;
}

