/**
 * \defgroup dlist Double List
 * \brief A double linked list.
 *
 * Any structure can become an element that goes in a Double List by
 * inheriting from the DListEl template class or by simply having next and
 * previous pointers. Elements in the list are usually allocated by the
 * user. 
 *
 * Elements can also have static allocation. For this reason, DList and
 * DListMel will not delete the list contents in their destructors.
 *
 * Classes can be elements in multiple lists by inheriting from multiple
 * DListEl base structures (which contain the next and previous pointers) and
 * specifying to the list template how to resolve the ambiguities. This
 * feature is available with DListMel.
 *
 * DListVal is a value oriented list that behaves much like the STL list class
 * is also provided.
 */

#include "dlist.h"
#include "dlistmel.h"
#include "dlistval.h"
