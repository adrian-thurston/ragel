#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

using std::cout;

int main_lib( int argc, const char **argv );

int libragel_main( char **result, int argc, const char **argv, const char *input );

extern "C"
int libragel( char **result, char *cmd, const char *input )
{
	const int max = 32;
	int argc = 0;
	const char *argv[max+1];

	char *s = cmd;
	while ( 1 ) {
		char *tok = strtok( s, " " );
		s = NULL;

		if ( tok == NULL )
			break;

		argv[argc++] = strdup(tok);

		if ( argc == max )
			break;
	}
	argv[argc] = NULL;

	int es = libragel_main( result, argc, argv, input );

	for ( int i = 0; i < argc; i++ )
		free( (void*)argv[i] );

	return es;
}
