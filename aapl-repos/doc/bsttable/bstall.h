/**
 * \defgroup bst Binary Search Table
 * \brief A dynamic array implementing binary search.
 *
 * This is a Binary Search %Table built on top of Vector. It allows for
 * O(log(N)) finds but is constrained to O(N) insertions and deletions as
 * it is table based.
 *
 * Binary Search %Table is instantiated by supplying an element or key and
 * value types which are stored directly in the underlying vector. This is
 * opposed to requiring that the element inherit from a base element as is the
 * case with AvlTree. A Compare class that can compare the key type must be
 * supplied.
 *
 * Being table based means that it is simple to get an ordered walk of the
 * elements in the table.
 */

#include "bsttable.h"
#include "bstmap.h"
#include "bstset.h"
#include "sbsttable.h"
#include "sbstmap.h"
#include "sbstset.h"
