/*
 * Parse command line arguments.
 */

#include <stdio.h>
#include <string.h>

#define BUFLEN 1024

struct params
{
	char buffer[BUFLEN+1];
	int buflen;
	int cs;
};

%%{
	machine params;
	access fsm->;

	# A buffer to collect argurments

	# Append to the buffer.
	action append {
		if ( fsm->buflen < BUFLEN )
			fsm->buffer[fsm->buflen++] = fc;
	}

	# Terminate a buffer.
	action term {
		if ( fsm->buflen < BUFLEN )
			fsm->buffer[fsm->buflen++] = 0;
	}

	# Clear out the buffer
	action clear { fsm->buflen = 0; }

	action help { printf("help\n"); }
	action version { printf("version\n"); }
	action output { printf("output: \"%s\"\n", fsm->buffer); }
	action spec { printf("spec: \"%s\"\n", fsm->buffer); }
	action mach { printf("machine: \"%s\"\n", fsm->buffer); }

	# Helpers that collect strings
	string = [^\0]+ >clear $append %term;

	# Different arguments.
	help = ( '-h' | '-H' | '-?' | '--help' ) 0 @help;
	version = ( '-v' | '--version' ) 0 @version;
	output = '-o' 0? string 0 @output;
	spec = '-S' 0? string 0 @spec;
	mach = '-M' 0? string 0 @mach;

	main := ( 
		help | 
		version | 
		output |
		spec |
		mach
	)*;
}%%

%% write data;

void params_init( struct params *fsm )
{
	fsm->buflen = 0;
	%% write init;
}

void params_execute( struct params *fsm, const char *data, int len )
{
	const char *p = data;
	const char *pe = data + len;

	%% write exec;
}

int params_finish( struct params *fsm )
{
	if ( fsm->cs == params_error )
		return -1;
	if ( fsm->cs >= params_first_final )
		return 1;
	return 0;
}

#define BUFSIZE 2048

int main( int argc, char **argv )
{
	int a;
	struct params params;

	params_init( &params );
	for ( a = 1; a < argc; a++ )
		params_execute( &params, argv[a], strlen(argv[a])+1 );
	if ( params_finish( &params ) != 1 )
		fprintf( stderr, "params: error processing arguments\n" );

	return 0;
}
