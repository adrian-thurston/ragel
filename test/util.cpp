#include <iostream>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#define TAB_WIDTH 10

using namespace std;

void handleAlarm( int )
{
	cout << endl;
	exit(0);
}

void processArgs( int argc, char** argv )
{
	if ( argc > 1 ) {
		int secs = atoi( argv[1] );
		if ( secs > 0 ) {
			signal( SIGALRM, &handleAlarm );
			alarm( secs );
		}
	}
}

/* Expand the tabs in a buffer a buffer. */
void expandTab( char *dst, char *src )
{
	char *pd = dst;
	for ( ; *src != 0; src++ ) {
		if ( *src != '\t' )
			*pd++ = *src;
		else {
			int n = TAB_WIDTH - ((pd - dst)%TAB_WIDTH);
			memset( pd, ' ', n );
			pd += n;
		}
	}
	*pd = 0;
}


