/*
 * Partial printf implementation.
 */

#define BUFLEN 1024
#include <stdio.h>

typedef void (*WriteFunc)( char *data, int len );

struct format
{
	char buf[BUFLEN+1];
	int buflen;
	WriteFunc write;

	int flags;
	int width;
	int prec;
	int cs;
};

void do_conv( struct format *fsm, char c )
{
	printf( "flags: %x\n", fsm->flags );
	printf( "width: %i\n", fsm->width );
	printf( "prec: %i\n", fsm->prec );
	printf( "conv: %c\n", c );
	printf( "\n" );
}

#define FL_HASH          0x01
#define FL_ZERO          0x02
#define FL_DASH          0x04
#define FL_SPACE         0x08
#define FL_PLUS          0x10

#define FL_HAS_WIDTH   0x0100
#define FL_WIDTH_ARG   0x0200
#define FL_HAS_PREC    0x0400
#define FL_PREC_ARG    0x0800

#define FL_LEN_H     0x010000
#define FL_LEN_HH    0x020000
#define FL_LEN_L     0x040000
#define FL_LEN_LL    0x080000

%%{
	machine format;
	access fsm->;

	action clear {
		fsm->flags = 0;
		fsm->width = 0;
		fsm->prec = 0;
	}

	# A non-zero number.
	nznum = [1-9] [0-9]*;

	# Width
	action width_num { fsm->width = 10 * fsm->width + (fc-'0'); }
	action width_arg { fsm->flags |= FL_WIDTH_ARG; }
	action width { fsm->flags |= FL_HAS_WIDTH; }
	width = ( ( nznum $width_num | '*' @width_arg ) %width )?;

	# Precision
	action prec_num { fsm->prec = 10 * fsm->prec + (fc-'0'); }
	action prec_arg { fsm->flags |= FL_PREC_ARG; }
	action prec { fsm->flags |= FL_HAS_PREC; }
	precision = ( '.' ( digit* $prec_num %prec | '*' @prec_arg ) )?;

	# Flags
	action flags_hash { fsm->flags |= FL_HASH; }
	action flags_zero { fsm->flags |= FL_ZERO; }
	action flags_dash { fsm->flags |= FL_DASH; }
	action flags_space { fsm->flags |= FL_SPACE; }
	action flags_plus { fsm->flags |= FL_PLUS; }

	flags = ( 
		'#' @flags_hash |
		'0' @flags_zero |
		'-' @flags_dash |
		' ' @flags_space |
		'+' @flags_plus )*;

	action length_h  { fsm->flags |= FL_LEN_H; }
	action length_l  { fsm->flags |= FL_LEN_L; }
	action length_hh { fsm->flags |= FL_LEN_HH; }
	action length_ll { fsm->flags |= FL_LEN_LL; }

	# Must use leaving transitions on 'h' and 'l' because they are
	# prefixes for 'hh' and 'll'.
	length = (
		'h' %length_h | 
		'l' %length_l |
		'hh' @length_hh |
		'll' @length_ll )?;
	
	action conversion { 
		do_conv( fsm, fc );
	}

	conversion = [diouxXcsp] @conversion;

	fmt_spec = 
			'%' @clear 
			flags
			width
			precision
			length
			conversion;
	
	action emit {
		if ( fsm->buflen == BUFLEN ) {
			fsm->write( fsm->buf, fsm->buflen );
			fsm->buflen = 0;
		}
		fsm->buf[fsm->buflen++] = fc;
	}

	action finish_ok {
		if ( fsm->buflen > 0 )
			fsm->write( fsm->buf, fsm->buflen );
	}
	action finish_err {
		printf("EOF IN FORMAT\n");
	}
	action err_char {
		printf("ERROR ON CHAR: 0x%x\n", fc );
	}

	main := ( 
			[^%] @emit | 
			'%%' @emit | 
			fmt_spec 
		)* @/finish_err %/finish_ok $!err_char;
}%%

%% write data;

void format_init( struct format *fsm )
{
	fsm->buflen = 0;
	%% write init;
}

void format_execute( struct format *fsm, const char *data, int len, int isEof )
{
	const char *p = data;
	const char *pe = data + len;
	const char *eof = isEof ? pe : 0;

	%% write exec;
}

int format_finish( struct format *fsm )
{
	if ( fsm->cs == format_error )
		return -1;
	if ( fsm->cs >= format_first_final )
		return 1;
	return 0;
}


#define INPUT_BUFSIZE 2048

struct format fsm;
char buf[INPUT_BUFSIZE];

void write(char *data, int len )
{
	fwrite( data, 1, len, stdout );
}

int main()
{
	fsm.write = write;
	format_init( &fsm );
	while ( 1 ) {
		int len = fread( buf, 1, INPUT_BUFSIZE, stdin );
		int eof = len != INPUT_BUFSIZE;
		format_execute( &fsm, buf, len, eof );
		if ( eof )
			break;
	}
	if ( format_finish( &fsm ) <= 0 )
		printf("FAIL\n");
	return 0;
}

