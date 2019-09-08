#include "avliset.h"

int main()
{
    AvliSet< int, CmpOrd<int> > avlset;
    avlset.insert( 1 );
    avlset.remove( 1 );
    return 0;
}
