#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

using std::cout;

int main_lib( int argc, const char **argv );

extern "C"
int libragel( char *cmd )
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

	int es = main_lib( argc, argv );

	for ( int i = 0; i < argc; i++ )
		free( (void*)argv[i] );

	return es;
}
