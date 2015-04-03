#include <colm/tree.h>
#include <colm/bytecode.h>
#include <stdio.h>
#include <string.h>

Str *c_alphcount( Program *prg, Tree **sp, Str *a1 )
{
	int p, count = 0;
	for ( p = 0; p < a1->value->length; p++ ) {
		char c = a1->value->data[p];
		if ( 'a' <= c && c <= 'z' )
			count++;
	}

	char strc[64];
	sprintf( strc, "%d", count );

	Head *h = stringAllocFull( prg, strc, strlen( strc ) );
	Tree *s = constructString( prg, h );
	treeUpref( s );
	treeDownref( prg, sp, a1 );
	return (Str*)s;
}
