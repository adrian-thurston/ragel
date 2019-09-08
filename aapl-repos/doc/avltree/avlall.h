/** 
 * \defgroup avltree AVL Tree
 * \brief Balanced binary tree.
 * 
 * AVL tree provides a balanaced binary tree. All operations are O(log(N)).
 * AVL Tree is very useful as a general purpose map.
 * 
 * In the current implementation. A single avl tree cannot contain two
 * elements with the same key. If this functionality is required, a binary
 * search table must be used instead.
 * 
 * As with the double linked list, the AVL trees are divided into two types:
 * those that leave element management up to the user and those that manage
 * the allocation of elements. All trees except AvlMap and AvlSet leave
 * element allocation management up to the user.
 *
 * The AVL trees can be iterated using each class's Iter structure. This
 * iteration traverses the tree structure to produce an inorder walk of the
 * nodes. An single iteration step runs in O(log(N)) time. In the worst case
 * moving to the next node requires traversing the tree from a leaf node up to
 * the root node and then back down agin to a leaf node. If this traversal is
 * too costly or if pointers to next and previous nodes are desired then the
 * trees in the Linked AVL Tree group can be used.
 */

#include "avltree.h"
#include "avlmel.h"
#include "avlmelkey.h"
#include "avlmap.h"
#include "avlset.h"
#include "avlbasic.h"
#include "avlkeyless.h"
