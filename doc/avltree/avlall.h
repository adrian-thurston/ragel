/** 
 * \defgroup avltree AVL Tree
 * \brief Balanced binary tree.
 * 
 * AVL Tree is a balanced binary tree. Regular binary trees can suffer
 * performance issues when items are inserted in sorted order. Balanced
 * trees correct this problem by guaranteeing an evenly distributed tree
 * regardless of the order in which items are inserted. AVL is just one of
 * many mechansims for balancing trees.
 *
 * Like Binary Search %Table, AVL Tree can be used to implement a map.
 * However, there are important differences between the two data
 * structures. In some cases avl tree is more appropriate and in others
 * binary search table is more appropriate.
 *
 * All operations are O(log(N)). To put that into perspective, a tree with
 * thirty-two thousand elements requires at most fifteen comparisons to find
 * an element. In practice, the avl tree may need to do two or three more
 * because attaining a perfectly level tree is not possible.
 *
 * Elements in a tree are individually allocated by the user of the tree.
 * With the exception of map and set style trees, the user generally goes
 *
 *     avltree.insert( new Element(...) ) or
 *
 * where the Element is a class that contains the necessary pointers needed
 * by the Avl algorithms, the key that is used to compare items, the compare
 * routine itself and any other user data. In short, any class can become a
 * element that goes into an avl tree by inheriting from the right classes.
 *
 * Elements do not necessarily need to be allocated on the heap. Any allocation
 * type can be used. For this reason the non-map and non-set trees will not
 * delete element in their destructors. It is also possible to have the tree
 * allocate the element on behalf of the user, in which case the element class must
 * have an appropriate constructor.
 *
 *     avltree.insert( "the key" );
 *
 * Elements can be elements in multiple trees by inheriting from multiple AvlTreeEl
 * base structures (which contain the left, right and parent pointers, etc.)
 * and specifying to the tree template how to resolve the ambiguities. This
 * feature is available with AvlMel, AvlMelKey.
 *
 * This implementation of avl tree cannot contain duplicate keys. It cannot
 * be used to directly implement a so called multi-map. Binary search table
 * does support inserting duplicate keys. Of course an avl tree could be
 * used to store lists of elements in order to accomplish a multi-map,
 * though this is not provided by aapl.
 *
 * AVL tree solves the common map problem of inserting elements under the
 * condition that they are not already there by allowing the currently
 * existing element to be returned from an insert operation that fails.
 * 
 * AVL tree supports the maintainance of an ordered list of the elements
 * without adding more than a constant amount of work to each operation. It is
 * then easy to walk the list of items in the tree by simply using the next
 * and prev pointers in the element. The classes that support this are Avli*
 * named classes.
 */

#include "avltree.h"
#include "avlmel.h"
#include "avlmelkey.h"
#include "avlmap.h"
#include "avlset.h"

#include "avlitree.h"
#include "avlimel.h"
#include "avlimelkey.h"
#include "avlimap.h"
#include "avliset.h"
