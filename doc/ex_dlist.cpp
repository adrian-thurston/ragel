#include "dlist.h"

struct ListEl : public DListEl<ListEl>
{
    // List customizations
    int data;
};

ListEl els[10];

int main()
{
    DList<ListEl> list;

    list.append( &els[0] );
    list.append( &els[1] );
    list.append( &els[2] );

    return 0;
}
