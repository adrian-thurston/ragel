/**
 * \mainpage Aapl C++ Template Library
 *
 * \section Introduction
 *
 * Aapl is a C++ template library for generic programming. It contains
 * implementations of %AVL Tree, Linked List, %Vector, Binary Search %Table
 * and %Sort.
 *
 * Aapl supports different generic programming paradigms by providing
 * variations of standard data structures. For example, a by-value linked list
 * template may be used to store a user supplied type such as an integer. A
 * different list template allows the user to define the data structure that
 * is to be used as the element. A third list template allows a single
 * instance of a data structure to be an element in multiple lists.
 *
 * Wherever possible, Aapl data structures do not depend on heap memory
 * allocation. There are variations of the linked list and AVL tree that allow
 * the programmer to allocate a collection of elements statically and
 * insert/remove them at will.
 *
 * Aapl data structures do not have data members that are hidden behind a
 * strict abstraction layer. Aapl makes very little use of the private
 * keyword.  Though data abstractions can be a useful programming technique to
 * quickly produce very robust code, they can inhibit functionality when the
 * data structure is the centre of much attention. Therefore Aapl leaves the
 * use of abstractions up to the programmer.
 *
 * Browse documentation by:
 * <ul>
 * <li><a class="qindex" href="modules.html">Modules</a></li>
 * <li><a class="qindex" href="hierarchy.html">Class Hierarchy</a></li>
 * <li><a class="qindex" href="classes.html">Alphabetical List</a></li>
 * <li><a class="qindex" href="annotated.html">Compound List</a></li>
 * <li><a class="qindex" href="functions.html">Compound Members</a></li>
 * </ul>
 */

/** 
 * \defgroup iterators Iterators
 * \brief Iterators for walking Aapl data structures.
 *
 * The Iter classes provide a simple and concise abstraction of data structure
 * walking. All Aapl iterators have an identical interface. 
 *
 * A single iterator can be used to perform forward and backward movement.
 * This allows a traversal to reverse directions, provided the iterator has
 * not walked off the end of the structure. All movement and data reference
 * routines are valid only if the iterator already points to valid item. The
 * routines lte() and gtb() can be used to ensure this.  These stand
 * for "less-than end" and "greater-than beginning". In Aapl iterators, "end"
 * refers to the state resulting from issuing a increment operation while
 * pointing to the last item and "beginning" refers to the state resulting
 * from issuing a decrement operation while pointing to the first item.
 *
 * The negative sense of the lte() and gtb() routines are end() and beg().
 * They return true when the iterater has gone one past the last item and
 * arrived at the end, or gone one past the first item and arrived at the
 * beginning. To allow a traversal to stop before going off the end, the
 * first() and last() routines are provided. They indicate when the iterator
 * is at the first item or the last item, respectively.
 *
 * The element that the iterator points to can be retrieved using
 * iter.ptr. Iterators also contain the implicit cast operator () which
 * returns the pointer, the -> operator which also returns the pointer and the
 * dereference operator * which, when applied as in *iter, returns
 * *(iter.ptr).
 *
 * Some data structures use predefined elements that contain the user's types,
 * For exampe maps and sets store the user defined key and value types. Since
 * all iterators point to the data structure's element, in these cases the
 * user must manually reference the key or value using either iter->key or
 * iter->value.
 *
 * DList iterators keep a pointer to the current element and use the
 * element's next and previous pointers for traversal.
 *
 * AvlTree iterators keep a pointer to the current tree node and traverse the
 * structure in-order using the head, left and right pointers of tree nodes.
 * Note that the increment and decrement operators of the AvlTree iterators
 * run in O(log(N)) time.  AvliTree iterators can increment and decrement in
 * O(1) time because they have next and previous pointers available in the
 * nodes.
 *
 * Vector iterators (also used for binary search tables) keep a pointer to the
 * current item and walk the structure by incrementing and decrementing the
 * pointer. Unlike the list and tree iterators, a Vector iterator may become
 * invalid if the Vector being traversed is modified.
 *
 * \include ex_iters.cpp
 */

