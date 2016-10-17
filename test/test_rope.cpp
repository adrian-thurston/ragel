#include "rope.h"
#include <iostream>

using std::cout;

int main()
{
	Rope r;

	r.append( "dude\n", 5 );
	r.append( "dude\n", 5 );

	for ( RopeBlock *rb = r.hblk; rb != 0; rb = rb->next ) {
		cout.write( r.data(rb), r.length(rb) );
	}

	return 0;
}

