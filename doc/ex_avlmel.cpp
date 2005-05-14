#include "avlmel.h"

/* Forward the element struct so we can pass it to AvlTreeEl. */
struct CustomEl;

/* Make two base structures that will be used by 
 * the tree to resolve ambiguites. */
struct CustomElBase1 : public AvlTreeEl<CustomEl> {};
struct CustomElBase2 : public AvlTreeEl<CustomEl> {};

struct CustomEl :
        public CustomElBase1,
        public CustomElBase2,
        public CmpOrd<int>
{
    /* For our purposes. */
    CustomEl(int key, const char *data) : key(key), data(data) { }

    const int getKey() { return key; }
    int key;
    const char *data;
};

int main()
{
    /* Specify to AvlMel which base to use. */
    AvlMel< CustomEl, int, CustomElBase1 > avltree1;
    AvlMel< CustomEl, int, CustomElBase2 > avltree2;

    /* With AvlMel it makes the most sense to new the element ourseves
     * rather than letting the tree do it for use. */
    CustomEl *avlElement = new CustomEl( 1, "CustomEl" );

    /* These two calls are completely independant and safe. */
    avltree1.insert( avlElement );
    avltree2.insert( avlElement );

    return 0;
}
