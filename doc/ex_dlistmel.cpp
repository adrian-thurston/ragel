#include "dlistmel.h"

struct ListEl;

struct ListEl1 { ListEl *prev, *next; };
struct ListEl2 { ListEl *prev, *next; };

struct ListEl :
        public ListEl1,
        public ListEl2
{
    // Element customizations
    int data;
};

int main()
{
    DListMel<ListEl, ListEl1> list1;
    DListMel<ListEl, ListEl2> list2;
    list1.append( new ListEl );
    list2.append( list1.head );
    return 0;
}
