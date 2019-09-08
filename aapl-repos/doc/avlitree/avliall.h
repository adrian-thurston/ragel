/** 
 * \defgroup avlitree Linked AVL Tree
 * \brief Linked balanced binary tree.
 * 
 * These data structures are similar to the regular AVL tree data structure
 * except elements are implicitly linked together in an ordered list. This
 * does not add more than a constant amount of work to each operation and
 * allows the use of next and previous pointers for walking the items in the
 * tree.
 *
 * AVL tree provides a balanaced binary tree. All operations are O(log(N)).
 * AVL Tree is very useful as a general purpose map.
 * 
 * In the current implementation. A single avl tree cannot contain two
 * elements with the same key. If this functionality is required, a binary
 * search table must be used instead.
 * 
 * As with the double linked list, the AVL trees are divided into two
 * types: those that leave element management up to the user and those that
 * manage the allocation of elements. All trees except AvlMap and AvlSet leave
 * element allocation management up to the user.
 */

#include "avlitree.h"
#include "avlimel.h"
#include "avlimelkey.h"
#include "avlimap.h"
#include "avliset.h"
#include "avlibasic.h"
#include "avlikeyless.h"
