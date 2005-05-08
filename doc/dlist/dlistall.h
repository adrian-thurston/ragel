/**
 * \defgroup dlist Double List
 * \brief A double linked list.
 *
 * In DList and DListMel, any structure can become an element that goes in a
 * Double List by inheriting from the DListEl template class or by simply
 * having next and previous pointers. List elements are
 * allocated and owned by the user of the list. You can use these stuctures to
 * link together elements that are statically allocated, or that belong to
 * some other data structure. DList and DListMel will not delete the list
 * contents in their destructors.
 *
 * DListMel allows a data structure to be an element in multiple lists. The
 * element structure inherits multiple element base class and the name of the
 * appropriate base class is passed to DListMel so it can resolve the
 * ambiguity in accessing the next and previous pointers. 
 *
 * DListVal is a value oriented list that behaves much like the STL list
 * class. In this structure the list manages the allocation and deallocation
 * of the list elements. The user is concerned with inserting a
 * user value which gets copied into a newly allocated list element.
 */

#include "dlist.h"
#include "dlistmel.h"
#include "dlistval.h"
