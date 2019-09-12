#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 4096

typedef struct _Scanner {
	/* Scanner state. */
    int cs;
    int act;
    int have;
    int curline;
    char *ts;
    char *te;
    char *p;
    char *pe;
    char *eof;
	FILE *file;
	int done;

	/* Token data */
	char *data;
	int len;
    int value;

	char buf[BUFSIZE];
} Scanner;


%%{
	machine Scanner;
	write data;
}%%

void scan_init( Scanner *s, FILE *file )
{
	memset (s, '\0', sizeof(Scanner));
	s->curline = 1;
	s->file = file;
	s->eof = 0;
	%% write init;
}

#define TK_NO_TOKEN (-1)
#define TK_ERR 128
#define TK_EOF 129
#define TK_Identifier 130
#define TK_Number 131
#define TK_String 132

#define ret_tok( _tok ) token = _tok; s->data = s->ts

int scan( Scanner *s )
{
	int token = TK_NO_TOKEN;
	int space, readlen;

	while ( 1 ) {
		if ( s->p == s->pe ) {
			printf("scanner: need more data\n");

			if ( s->ts == 0 )
				s->have = 0;
			else {
				/* There is data that needs to be shifted over. */
				printf("scanner: buffer broken mid token\n");
				s->have = s->pe - s->ts;
				memmove( s->buf, s->ts, s->have );
				s->te -= (s->ts-s->buf);
				s->ts = s->buf;
			}

			s->p = s->buf + s->have;
			space = BUFSIZE - s->have;

			if ( space == 0 ) {
				/* We filled up the buffer trying to scan a token. */
				printf("scanner: out of buffer space\n");
				return TK_ERR;
			}

			if ( s->done ) {
				printf("scanner: end of file\n");
				s->p[0] = 0;
				readlen = 1;
			}
			else {
				readlen = fread( s->p, 1, space, s->file );
				if ( readlen < space )
					s->done = 1;
			}

			s->pe = s->p + readlen;
		}

		%%{
			machine Scanner;
			access s->;
			variable p s->p;
			variable pe s->pe;
			variable eof s->eof;

			main := |*

			# Identifiers
			( [a-zA-Z_] [a-zA-Z0-9_]* ) =>
				{ ret_tok( TK_Identifier ); fbreak; };

			# Whitespace
			[ \t\n];

			'"' ( [^\\"] | '\\' any ) * '"' =>
				{ ret_tok( TK_String ); fbreak; };

			# Number
			digit+ => 
				{ ret_tok( TK_Number ); fbreak; };
			
			# EOF
			0 =>
				{ ret_tok( TK_EOF ); fbreak; };

			# Anything else
			any => 
				{ ret_tok( *s->p ); fbreak; };

			*|;

			write exec;
		}%%

		if ( s->cs == Scanner_error )
			return TK_ERR;

		if ( token != TK_NO_TOKEN ) {
			s->len = s->p - s->data;
			return token;
		}
	}
}


int main (int argc, char** argv)
{
	Scanner ss;
	int tok;

	scan_init(&ss, stdin);

	while ( 1 ) {
		tok = scan (&ss);
		if ( tok == TK_EOF ) {
			printf ("parser: EOF\n");
			break;
		}
		else if ( tok == TK_ERR ) {
			printf ("parser: ERR\n");
			break;
		}
		else {
			printf ("parser: %d \"", tok);
			fwrite ( ss.data, 1, ss.len, stdout );
			printf ("\"\n" );
		}
	}

	return 0;
}


