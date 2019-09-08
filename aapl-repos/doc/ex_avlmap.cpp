#include "avlmap.h"

int main()
{
	AvlMap< char *, int, CmpStr > avlmap;
	avlmap.insert( "the key", 1 );
	return 0;
}
