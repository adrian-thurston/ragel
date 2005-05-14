/**
 * \defgroup vector Vector
 * \brief A dynamic array.
 *
 * Vector is a dynamic array capable of storing classes with non-trivial
 * initialization as well as simple types such as integers and pointers.
 * Vector stores items by value. Memory is managed automatically, with several
 * reallocation policies available. It is also possible to use a custom
 * reallocation scheme.
 *
 * VectSimp has the same interface as Vector and provides a 
 * performance improvement for types that do not have constructors or
 * destructors. VectSimp uses memcpy for copying data as opposed to invoking
 * copy constructors.
 *
 * SVector and SVectSimp are implicitly shared copy-on-write vectors. Array
 * contents are shared when copied, however the actual contents are only
 * duplicated when they are modified with insert, replace, append, prepend
 * setAs or remove.
 *
 * Vector should not be used for classes that contain pointers to members or
 * to self. Classes that go into vectors need to be translatable to any memory
 * location. Pointers to items in a Vector should not be taken. After a
 * modification of the Vector, the pointer may become invalid due to a
 * required reallocation.
 */

#include "vector.h"
#include "vectsimp.h"
#include "svector.h"
#include "svectsimp.h"
