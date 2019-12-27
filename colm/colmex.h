#ifndef _COLMEX_H
#define _COLMEX_H

#include <colm/colm.h>
#include <colm/tree.h>
#include <colm/colmex.h>
#include <string.h>
#include <string>
#include <vector>

inline void appendString( colm_print_args *args, const char *data, int length )
{
	std::string *str = (std::string*)args->arg;
	*str += std::string( data, length );
}

inline std::string printTreeStr( colm_program *prg, colm_tree *tree, bool trim )
{
	std::string str;
	struct indent_impl indent = { -1, 0 };
	colm_print_args printArgs = { &str, 1, 0, trim, &indent, &appendString, 
			&colm_print_null, &colm_print_term_tree, &colm_print_null };
	colm_print_tree_args( prg, colm_vm_root(prg), &printArgs, tree );
	return str;
}

struct ExportTree
{
	ExportTree( colm_program *prg, colm_tree *tree )
		: __prg(prg), __tree(tree) {}

	std::string text() { return printTreeStr( __prg, __tree, true ); }
	colm_location *loc() { return colm_find_location( __prg, __tree ); }
	std::string text_notrim() { return printTreeStr( __prg, __tree, false ); }
	std::string text_ws() { return printTreeStr( __prg, __tree, false ); }
	colm_data *data() { return __tree->tokdata; }
	operator colm_tree *() { return __tree; }

	colm_program *__prg;
	colm_tree *__tree;
};

/* Non-recursive tree iterator. Runs an in-order traversal and when it finds a
 * search target it yields it and then resumes searching the next child. It
 * does not go into what it finds. This iterator can be used to search lists,
 * regardless if they are left-recursive or right-recursive. */
template <class SearchType> struct RepeatIter
{
	RepeatIter( const ExportTree &root )
	:
		prg(root.__prg),
		search_id(SearchType::ID)
	{
		/* We use one code path for the first call to forward and all
		 * subsequent calls. To achieve this we create a sentinal in front of
		 * root called first and point cur to it. On the first forward() call
		 * it will be as if we just visited the sentinal.
		 *
		 * Note that we are also creating a kid for root, rather than
		 * jump into root's child list so we can entertain the
		 * possiblity that root is exactly the thing we want to visit.
		 */

		memset( &first, 0, sizeof(first) );
		memset( &kid, 0, sizeof(kid) );

		first.next = &kid;
		kid.tree = root.__tree;
		cur = &first;
		next();
	}

	colm_program *prg;
	colm_kid first, kid, *cur;
	int search_id;
	std::vector<colm_kid*> stack;

	void next()
	{
		goto return_to;
		recurse:

		if ( cur->tree->id == search_id )
			return;
		else {
			stack.push_back( cur );
			cur = tree_child( prg, cur->tree );
			while ( cur != 0 ) {
				goto recurse;
				return_to: cur = cur->next;
			}
			if ( stack.size() == 0 ) {
				cur = 0;
				return;
			}
			cur = stack.back();
			stack.pop_back();
			goto return_to;
		}
	}

	bool end()
		{ return cur == 0; }

	SearchType value()
		{ return SearchType( prg, cur->tree ); }
};

#endif
