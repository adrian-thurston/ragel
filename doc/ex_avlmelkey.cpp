#include "avlmelkey.h"

/* Forward the element struct so we can pass it to AvlTreeEl. */
struct CustomEl;

/* Make two base structures that will be used by 
 * the tree to resolve ambiguites. */
struct CustomElBase1 : 
        public AvlTreeEl<CustomEl>
{
    CustomElBase1( int key ) : intKey(key) { }

    const int getKey() { return intKey; }
    int intKey;
};

struct CustomElBase2 : 
        public AvlTreeEl<CustomEl>
{
    CustomElBase2( char *key ) : charStarKey(key) { }

    const char *getKey() { return charStarKey; }
    char *charStarKey;
};

struct CustomEl :
        public CustomElBase1,
        public CustomElBase2
{
    /* For our purposes. */
    CustomEl(int intKey, char *charStarKey, const char *data) :
        CustomElBase1(intKey), 
        CustomElBase2(charStarKey), 
        data(data) { }

    const char *data;
};

int main()
{
	/* Specify to AvlMelKey which base element and base key to use. In this
	 * example, both the key and the AVL tree data are in the CustomElBase
	 * classes. */
    AvlMelKey< CustomEl, int, CustomElBase1, CustomElBase1 > avltree1;
    AvlMelKey< CustomEl, char*, CustomElBase2, CustomElBase2, CmpStr > avltree2;

    /* With AvlMelKey it makes the most sense to new the element ourseves
     * rather than letting the tree do it for use. */
    CustomEl *avlElement = new CustomEl( 1, "strkey", "CustomEl" );

    /* These two calls are completely independant and safe. The first
     * uses the int key whereas the second uses the char* key.  */
    avltree1.insert( avlElement );
    avltree2.insert( avlElement );

    return 0;
}
