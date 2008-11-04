/*
 *  Copyright 2007 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "bytecode.h"
#include "astring.h"
#include "pdarun.h"
#include "dlistval.h"
#include "fsmrun.h"
#include "pdarun.h"
#include <iostream>
#include <sstream>
#include <alloca.h>
#include <sys/mman.h>

using std::cout;
using std::cerr;
using std::endl;

#define push(i) (*(--sp) = (i))
#define pop() (*sp++)
#define top() (*sp)
#define ptop() (sp)
#define popn(n) (sp += (n))
#define pushn(n) (sp -= (n))
#define local(o) (frame[o])
#define plocal(o) (&frame[o])
#define local_iframe(o) (iframe[o])
#define plocal_iframe(o) (&iframe[o])

#define read_byte( i ) do { \
	i = ((uchar) *instr++); \
} while(0)

#define read_word( i ) do { \
	i = ((Word) *instr++); \
	i |= ((Word) *instr++) << 8; \
	i |= ((Word) *instr++) << 16; \
	i |= ((Word) *instr++) << 24; \
} while(0)

#define read_tree( i ) do { \
	Word w; \
	w = ((Word) *instr++); \
	w |= ((Word) *instr++) << 8; \
	w |= ((Word) *instr++) << 16; \
	w |= ((Word) *instr++) << 24; \
	i = (Tree*) w; \
} while(0)

#define read_half( i ) do { \
	i = ((Word) *instr++); \
	i |= ((Word) *instr++) << 8; \
} while(0)

static Tree **vm_stack;

Kid *alloc_attrs( Program *prg, long length )
{
	Kid *cur = 0;
	for ( long i = 0; i < length; i++ ) {
		Kid *next = cur;
		cur = prg->kidPool.allocate();
		cur->next = next;
	}
	return cur;
}

void free_attrs( Program *prg, Kid *attrs )
{
	Kid *cur = attrs;
	while ( cur != 0 ) {
		Kid *next = cur->next;
		prg->kidPool.free( cur );
		cur = next;
	}
}

void set_attr( Tree *tree, long pos, Tree *val )
{
	Kid *cur = tree->child;
	for ( long i = 0; i < pos; i++ )
		cur = cur->next;
	cur->tree = val;
}

Tree *get_attr( Tree *tree, long pos )
{
	Kid *cur = tree->child;
	for ( long i = 0; i < pos; i++ )
		cur = cur->next;
	return cur->tree;
}

Kid *copy_obj_data( Program *prg, long length, Kid *src )
{
	Kid *cur = 0;
	for ( long i = 0; i < length; i++ ) {
		Kid *next = cur;
		cur = prg->kidPool.allocate();
		cur->next = next;
	}

	Kid *dest = cur;
	for ( long i = 0; i < length; i++ ) {
		dest->tree = src->tree;
		dest = dest->next;
		src = src->next;
	}
	return cur;
}

Kid *kid_list_concat( Kid *list1, Kid *list2 )
{
	if ( list1 == 0 )
		return list2;
	else if ( list2 == 0 )
		return list1;

	Kid *dest = list1;
	while ( dest->next != 0 )
		dest = dest->next;
	dest->next = list2;
	return list1;
}

/* Type conversions. */
List *list( Tree *tree ) { return (List*) tree; }
Map *map( Tree *tree )  { return (Map*) tree; }
Str *str( Tree *tree )  { return (Str*) tree; }

void split_iter_cur( Tree **&sp, Program *prg, TreeIter *iter );
Tree *split_tree( Program *prg, Tree *t );
Tree *copy_real_tree( Program *prg, Tree *tree, Kid *oldNextDown, Kid *&newNextDown );

Stream *open_stream_file( Program *prg, FILE *file )
{
	Stream *res = (Stream*)prg->mapElPool.allocate();
	res->id = LEL_ID_STREAM;
	res->file = file;
	res->in = new InputStreamFile( file );
	res->scanner = new FsmRun( prg );
	res->scanner->attachInputStream( res->in );
	return res;
}

Stream *open_stream_fd( Program *prg, long fd )
{
	Stream *res = (Stream*)prg->mapElPool.allocate();
	res->id = LEL_ID_STREAM;
	res->in = new InputStreamFD( fd );
	res->scanner = new FsmRun( prg );
	res->scanner->attachInputStream( res->in );
	return res;
}

Tree *open_file( Program *prg, Tree *name )
{
	Head *head = ((Str*)name)->value;
	FILE *file = fopen( string_data(head), "rb" );
	Tree *res = 0;

	if ( file != 0 ) {
		res = (Tree*) open_stream_file( prg, file );
		res = prg->treePool.allocate();
		res->id = LEL_ID_STREAM;
		((Stream *)res)->file = file;
	}

	return res;
}

void check_parse_tree( Tree *tree )
{
	if ( tree->refs != 1 )
		cerr << "tree->refs: " << tree->refs << endl;
	
	Kid *child = tree->child;
	while ( child != 0 ) {
		check_parse_tree( child->tree );
		child = child->next;
	}
}

void send( Tree **root, Program *prg, PdaRun *parser, Tree *tree, bool ignore )
{
	/* If the tree already has an alg (it has been parsed) then we need to
	 * send a copy of it because the parsing that we are about to do requires
	 * a fresh alg. */
	if ( tree->alg != 0 ) {
		#ifdef COLM_LOG_BYTECODE
		cerr << "copying tree in send because alg is set" << endl;
		#endif
		Kid *unused = 0;
		tree = copy_real_tree( prg, tree, 0, unused );
		tree_upref( tree );
	}

	assert( tree->alg == 0 );
	tree->alg = prg->algPool.allocate();

	if ( tree->id >= prg->rtd->firstNonTermId )
		tree->id = prg->rtd->lelInfo[tree->id].termDupId;

	tree->alg->flags |= AF_ARTIFICIAL;
	if ( ignore )
		tree->alg->flags |= AF_IGNORE;

	/* FIXME: Do we need to remove the ignore tokens 
	 * at this point? Will it cause a leak? */

	Kid *kid = prg->kidPool.allocate();
	kid->tree = tree;

	if ( parser->queue == 0 )
		parser->queue = parser->queueLast = kid;
	else {
		parser->queueLast->next = kid;
		parser->queueLast = kid;
	}
}

Tree *make_token( Tree **root, Program *prg, PdaRun *parser, long nargs )
{
	Tree **const sp = root;
	Tree **base = ptop() + nargs;

	Int *idInt = (Int*)base[-1];
	Str *textStr = (Str*)base[-2];

	long id = idInt->value;
	Head *tokdata = string_copy( prg, textStr->value );

	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Tree *tree;

	if ( lelInfo[id].ignore ) {
		tree = prg->treePool.allocate();
		tree->refs = 1;
		tree->id = id;
		tree->tokdata = tokdata;
	}
	else {
		long objectLength = lelInfo[id].objectLength;
		Kid *attrs = alloc_attrs( prg, objectLength );

		tree = prg->treePool.allocate();
		tree->id = id;
		tree->refs = 1;
		tree->tokdata = tokdata;

		tree->child = attrs;

		assert( nargs-2 <= objectLength );
		for ( long id = 0; id < nargs-2; id++ ) {
			set_attr( tree, id, base[-3-id] );
			tree_upref( get_attr( tree, id) );
		}
	}
	return tree;
}

Tree *make_tree( Tree **root, Program *prg, PdaRun *parser, int nargs )
{
	Tree **const sp = root;
	Tree **base = ptop() + nargs;

	Int *idInt = (Int*)base[-1];

	long id = idInt->value;
	LangElInfo *lelInfo = prg->rtd->lelInfo;

	Tree *tree = prg->treePool.allocate();
	tree->id = id;
	tree->refs = 1;

	long objectLength = lelInfo[id].objectLength;
	Kid *attrs = alloc_attrs( prg, objectLength );

	Kid *last = 0, *child = 0;
	for ( long id = 0; id < nargs-1; id++ ) {
		Kid *kid = prg->kidPool.allocate();
		kid->tree = base[-2-id];
		tree_upref( kid->tree );

		if ( last == 0 )
			child = kid;
		else
			last->next = kid;

		last = kid;
	}

	tree->child = kid_list_concat( attrs, child );

	return tree;
}

Tree *parse( Tree **&sp, Program *prg, Stream *stream, 
		long parserId, long stopId, CodeVect *&cv )
{
	PdaTables *tables = prg->rtd->parsers[parserId];
	PdaRun parser( sp, prg, tables, stream->scanner, stopId );
	parser.run();
	parser.commit();
	Tree *tree = parser.getParsedRoot( stopId > 0 );
	tree_upref( tree );
	parser.clean();

	/* Return the reverse code. */
	cv = parser.allReverseCode;
	return tree;
}

void undo_parse( Tree **&sp, Program *prg, Stream *stream, 
		long parserId, Tree *tree, CodeVect *rev )
{
	PdaTables *tables = prg->rtd->parsers[parserId];
	PdaRun parser( sp, prg, tables, stream->scanner, 0 );
	parser.undoParse( tree, rev );
}

Tree *stream_pull( Program *prg, Stream *stream, Tree *length )
{
	long len = ((Int*)length)->value;
	Head *tokdata = stream->scanner->extractToken( len );

	Str *str = (Str*) prg->treePool.allocate();
	str->id = LEL_ID_STR;
	str->value = tokdata;

	return (Tree*)str;
}

void undo_pull( Program *prg, Stream *stream, Tree *str )
{
	const char *data = string_data( ( (Str*)str )->value );
	long length = string_length( ( (Str*)str )->value );
	stream->scanner->sendBackText( data, length );
}

Word stream_push( Tree **&sp, Program *prg, Stream *stream, Tree *any )
{
	std::stringstream ss;
	print_tree( ss, sp, prg, any );
	stream->scanner->streamPush( ss.str().c_str(), ss.str().size());
	return ss.str().size();
}

void undo_stream_push( Tree **&sp, Program *prg, Stream *stream, Word len )
{
	stream->scanner->undoStreamPush( len );
}


void print_str( Head *str )
{
	cout.write( (char*)(str->data), str->length );
}

void print_ignore_list( Tree **&sp, Program *prg, Tree *tree )
{
	Kid *ignore = tree_ignore( prg, tree );

	/* Record the root of the stack and push everything. */
	Tree **root = ptop();
	while ( tree_is_ignore( prg, ignore ) ) {
		push( (SW)ignore );
		ignore = ignore->next;
	}

	/* Pop them off and print. */
	while ( ptop() != root ) {
		ignore = (Kid*) pop();
		print_tree( sp, prg, ignore->tree );
	}
}

void print_kid( ostream &out, Tree **&sp, Program *prg, Kid *kid, bool printIgnore )
{
	Tree **root = ptop();
	Kid *child;

rec_call:
	/* If not currently skipping ignore data, then print it. Ignore data can
	 * be associated with terminals and nonterminals. */
	if ( printIgnore && tree_ignore( prg, kid->tree ) != 0 ) {
		/* Ignorelists are reversed. */
		print_ignore_list( sp, prg, kid->tree );
		printIgnore = false;
	}

	if ( kid->tree->id < prg->rtd->firstNonTermId ) {
		/* Always turn on ignore printing when we get to a token. */
		printIgnore = true;

		if ( kid->tree->id == LEL_ID_INT )
			out << ((Int*)kid->tree)->value;
		else if ( kid->tree->id == LEL_ID_BOOL ) {
			if ( ((Int*)kid->tree)->value )
				out << "true";
			else
				out << "false";
		}
		else if ( kid->tree->id == LEL_ID_PTR )
			out << '#' << (void*) ((Pointer*)kid->tree)->value;
		else if ( kid->tree->id == LEL_ID_STR )
			print_str( ((Str*)kid->tree)->value );
		else if ( kid->tree->id == LEL_ID_STREAM )
			out << '#' << (void*) ((Stream*)kid->tree)->file;
		else if ( kid->tree->tokdata != 0 && 
				string_length( kid->tree->tokdata ) > 0 )
		{
			out.write( string_data( kid->tree->tokdata ), 
					string_length( kid->tree->tokdata ) );
		}
	}
	else {
		/* Non-terminal. */
		child = tree_child( prg, kid->tree );
		if ( child != 0 ) {
			push( (SW)kid );
			kid = child;
			while ( kid != 0 ) {
				goto rec_call;
				rec_return:
				kid = kid->next;
			}
			kid = (Kid*)pop();
		}
	}

	if ( ptop() != root )
		goto rec_return;
}

void print_tree( Tree **&sp, Program *prg, Tree *tree )
{
	if ( tree == 0 )
		cout << "NIL";
	else {
		Kid kid;
		kid.tree = tree;
		kid.next = 0;
		print_kid( cout, sp, prg, &kid, false );
	}
}

void print_tree( ostream &out, Tree **&sp, Program *prg, Tree *tree )
{
	if ( tree == 0 )
		cout << "NIL";
	else {
		Kid kid;
		kid.tree = tree;
		kid.next = 0;
		print_kid( out, sp, prg, &kid, false );
	}
}

void xml_escape_data( const char *data, long len )
{
	for ( int i = 0; i < len; i++ ) {
		if ( 32 <= data[i] && data[i] <= 126 )
			cout << data[i];
		else
			cout << "&#" << ((unsigned)data[i]) << ';';
	}
}

void xml_print_kid( Tree **&sp, Program *prg, Kid *kid, int depth )
{
	Tree **root = ptop();
	int i = 0;

rec_call:
	for ( i = 0; i < depth; i++ )
		cout << "  ";

	if ( kid->tree == 0 )
		cout << "NIL" << endl;
	else {
		cout << '<' << prg->rtd->lelInfo[kid->tree->id].name;
		if ( kid->tree->child != 0 ) {
			cout << '>' << endl;
			push( (SW) kid );
			kid = kid->tree->child;
			while ( kid != 0 ) {
				depth++;
				goto rec_call;
				rec_return:
				depth--;
				kid = kid->next;

				/* If the parent kid is a repeat then skip this node and go
				 * right to the first child (repeated item). */
				if ( prg->rtd->lelInfo[((Kid*)top())->tree->id].repeat )
					kid = kid->tree->child;
			}
			kid = (Kid*) pop();

			for ( i = 0; i < depth; i++ )
				cout << "  ";
			cout << "</" << prg->rtd->lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_PTR ) {
			cout << '>' << (void*)((Pointer*)kid->tree)->value << 
					"</" << prg->rtd->lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_BOOL ) {
			if ( ((Int*)kid->tree)->value )
				cout << ">true</";
			else
				cout << ">false</";
			cout << prg->rtd->lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_INT ) {
			cout << '>' << ((Int*)kid->tree)->value << 
					"</" << prg->rtd->lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_STR ) {
			Head *head = (Head*) ((Str*)kid->tree)->value;

			cout << '>';
			xml_escape_data( (char*)(head->data), head->length );
			cout << "</" << prg->rtd->lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( 0 < kid->tree->id && kid->tree->id < prg->rtd->firstNonTermId &&
				kid->tree->tokdata != 0 && 
				string_length( kid->tree->tokdata ) > 0 && 
				!prg->rtd->lelInfo[kid->tree->id].literal )
		{
			cout << '>';
			xml_escape_data( string_data( kid->tree->tokdata ), 
					string_length( kid->tree->tokdata ) );
			cout << "</" << prg->rtd->lelInfo[kid->tree->id].name << '>' << endl;
		}
		else
			cout << "/>" << endl;
	}

	if ( ptop() != root )
		goto rec_return;
}

void xml_print_tree( Tree **&sp, Program *prg, Tree *tree )
{
	Kid kid;
	kid.tree = tree;
	kid.next = 0;
	xml_print_kid( sp, prg, &kid, 0 );
}

Tree *get_rhs_el( Program *prg, Tree *lhs, long position )
{
	Kid *pos = tree_child( prg, lhs );
	while ( position > 0 ) {
		pos = pos->next;
		position -= 1;
	}
	return pos->tree;
}

Tree **alloc_obj_data( long length )
{
	Tree **attrs = 0;
	if ( length > 0 ) {
		attrs = new Tree*[length];
		memset( attrs, 0, sizeof(Tree*)*length );
	}
	return attrs;
}

void set_field( Program *prg, Tree *tree, long field, Tree *value )
{
	assert( tree->refs == 1 );
	if ( value != 0 )
		assert( value->refs >= 1 );
	set_attr( tree, field, value );
}

Tree *get_field( Tree *tree, Word field )
{
	return get_attr( tree, field );
}

Tree *get_field_split( Program *prg, Tree *tree, Word field )
{
	Tree *val = get_attr( tree, field );
	Tree *split = split_tree( prg, val );
	set_attr( tree, field, split );
	return split;
}

void set_local( Tree **frame, long field, Tree *tree )
{
	if ( tree != 0 )
		assert( tree->refs >= 1 );
	local(field) = tree;
}

Tree *get_local_split( Program *prg, Tree **frame, long field )
{
	Tree *val = local(field);
	Tree *split = split_tree( prg, val );
	local(field) = split;
	return split;
}

Tree *get_ptr_val( Pointer *ptr )
{
	return ptr->value->tree;
}

Tree *get_ptr_val_split( Program *prg, Pointer *ptr )
{
	Tree *val = ptr->value->tree;
	Tree *split = split_tree( prg, val );
	ptr->value->tree = split;
	return split;
}

void downref_local_trees( Program *prg, Tree **sp, Tree **frame, char *trees, long treesLen )
{
	for ( long i = 0; i < treesLen; i++ ) {
		#ifdef COLM_LOG_BYTECODE
		cerr << "local tree downref: " << (long)trees[i] << endl;
		#endif

		tree_downref( prg, sp, local(trees[i]) );
	}
}

void upref_uiter_args( Tree **frame, long nargs )
{
	for ( long l = IFR_AA; l < IFR_AA + nargs; l++ ) {
		#ifdef COLM_LOG_BYTECODE
		cerr << "upref local " << l << endl;
		#endif
		tree_upref( local(l) );
	}
}

void set_triter_cur( TreeIter *iter, Tree *tree )
{
	iter->ref.kid->tree = tree;
}

UserIter *uiter_create( Tree **&sp, Program *prg, FunctionInfo *fi, long searchId )
{
	pushn( sizeof(UserIter) / sizeof(Word) );
	void *mem = ptop();

	UserIter *uiter = new(mem) UserIter( ptop(), fi->argSize, searchId );
	return uiter;
}

void iter_find( Program *prg, Tree **&sp, TreeIter *iter, bool tryFirst )
{
	bool anyTree = iter->searchId == prg->rtd->anyId;
	Tree **top = iter->stackRoot;
	Kid *child;

rec_call:
	if ( tryFirst && ( iter->ref.kid->tree->id == iter->searchId || anyTree ) )
		return;
	else {
		child = tree_child( prg, iter->ref.kid->tree );
		if ( child != 0 ) {
			push( (SW) iter->ref.next );
			push( (SW) iter->ref.kid );
			iter->ref.kid = child;
			iter->ref.next = (Ref*)ptop();
			while ( iter->ref.kid != 0 ) {
				tryFirst = true;
				goto rec_call;
				rec_return:
				iter->ref.kid = iter->ref.kid->next;
			}
			iter->ref.kid = (Kid*)pop();
			iter->ref.next = (Ref*)pop();
		}
	}

	if ( top != ptop() )
		goto rec_return;
	
	iter->ref.kid = 0;
}

Tree *tree_iter_advance( Program *prg, Tree **&sp, TreeIter *iter )
{
	assert( iter->stackSize == iter->stackRoot - ptop() );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the root. */
		iter->ref = iter->rootRef;
		iter_find( prg, sp, iter, true );
	}
	else {
		/* Have a previous item, continue searching from there. */
		iter_find( prg, sp, iter, false );
	}

	iter->stackSize = iter->stackRoot - ptop();

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

Tree *tree_iter_next_child( Program *prg, Tree **&sp, TreeIter *iter )
{
	assert( iter->stackSize == iter->stackRoot - ptop() );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the first child. */
		if ( iter->rootRef.kid->tree->child == 0 ) {
			iter->ref.kid = 0;
			iter->ref.next = 0;
		}
		else {
			push( (SW) iter->rootRef.next );
			push( (SW) iter->rootRef.kid );
			iter->ref.kid = iter->rootRef.kid->tree->child;
			iter->ref.next = (Ref*)ptop();
		}
	}
	else {
		iter->ref.kid = iter->ref.kid->next;
	}

	bool anyTree = iter->searchId == prg->rtd->anyId;
	if ( ! anyTree ) {
		/* Have a previous item, go to the next sibling. */
		while ( iter->ref.kid != 0 && iter->ref.kid->tree->id != iter->searchId ) {
			iter->ref.kid = iter->ref.kid->next;
		}
	}

	iter->stackSize = iter->stackRoot - ptop();

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

Tree *tree_iter_prev_child( Program *prg, Tree **&sp, TreeIter *iter )
{
	assert( iter->stackSize == iter->stackRoot - ptop() );

	if ( iter->ref.kid == 0 ) {
		/* Kid is zero, start from the first child. */
		if ( iter->rootRef.kid->tree->child == 0 ) {
			iter->ref.kid = 0;
			iter->ref.next = 0;
		}
		else {
			push( (SW) iter->rootRef.next );
			push( (SW) iter->rootRef.kid );

			Kid *last = iter->rootRef.kid->tree->child;
			while ( last->next != 0 )
				last = last->next;

			iter->ref.kid = last;
			iter->ref.next = (Ref*)ptop();
		}
	}
	else {
		/* Have a previous item, go to the prev sibling. */
		Kid *wasAt = iter->ref.kid;
		Kid *parent = (Kid*) top();
		Kid *cur = 0, *next = parent->tree->child;

		while ( next != wasAt ) {
			cur = next;
			next = next->next;
		}

		iter->ref.kid = cur;
	}

	bool anyTree = iter->searchId == prg->rtd->anyId;
	if ( ! anyTree ) {
		/* Have a previous item, go to the next sibling. */
		while ( iter->ref.kid != 0 && iter->ref.kid->tree->id != iter->searchId ) {
			iter->ref.kid = iter->ref.kid->next;
		}
	}

	iter->stackSize = iter->stackRoot - ptop();

	return (iter->ref.kid ? prg->trueVal : prg->falseVal );
}

void tree_iter_destroy( Tree **&sp, TreeIter *iter )
{
	long curStackSize = iter->stackRoot - ptop();
	assert( iter->stackSize == curStackSize );
	popn( iter->stackSize );
}

void user_iter_destroy( Tree **&sp, UserIter *uiter )
{
	/* We should always be coming from a yield. The current stack size will be
	 * nonzero and the stack size in the iterator will be correct. */
	long curStackSize = uiter->stackRoot - ptop();
	assert( uiter->stackSize == curStackSize );

	long argSize = uiter->argSize;

	popn( uiter->stackRoot - ptop() );
	popn( sizeof(UserIter) / sizeof(Word) );
	popn( argSize );
}

Tree *tree_iter_deref_cur( TreeIter *iter )
{
	return iter->ref.kid == 0 ? 0 : iter->ref.kid->tree;
}

Tree *tree_search( Kid *kid, long id )
{
	if ( kid->tree->id == id )
		return kid->tree;

	Tree *res = 0;
	if ( kid->tree->child != 0 )
		res = tree_search( kid->tree->child, id );
	
	if ( res == 0 && kid->next != 0 )
		res = tree_search( kid->next, id );

	return res;	
}

Tree *tree_search( Tree *tree, long id )
{
	Tree *res = 0;
	if ( tree->id == id )
		res = tree;
	else if ( tree->child != 0 )
		res = tree_search( tree->child, id );
	return res;
}

Tree *copy_real_tree( Program *prg, Tree *tree, Kid *oldNextDown, Kid *&newNextDown )
{
	assert( tree->refs >= 2 );

	/* Need to keep a lookout for next down. If 
	 * copying it, return the copy. */
	Tree *newTree = prg->treePool.allocate();

	newTree->id = tree->id;
	newTree->tokdata = string_copy( prg, tree->tokdata );
//	newTree->pos = tree->pos;

	tree->refs -= 1;

	/* Copy the child list, will handle attributes, ignores 
	 * and the children. */
	Kid *child = tree->child, *last = 0;
	while ( child != 0 ) {
		Kid *newChild = prg->kidPool.allocate();

		/* Store the first child. */
		if ( newTree->child == 0 )
			newTree->child = newChild;

		/* Watch out for next down. */
		if ( child == oldNextDown )
			newNextDown = newChild;

		newChild->tree = child->tree;
		newChild->next = 0;

		/* May be an attribute. */
		if ( newChild->tree != 0 )
			newChild->tree->refs += 1;

		if ( last != 0 )
			last->next = newChild;

		child = child->next;
		last = newChild;
	}
	
	return newTree;
}

List *copy_list( Program *prg, List *list, Kid *oldNextDown, Kid *&newNextDown )
{
	if ( list->refs > 1 ) {
		#ifdef COLM_LOG_BYTECODE
		cerr << "splitting list: " << list << " refs: " << 
				list->refs << endl;
		#endif

		/* Not a need copy. */
		List *newList = (List*)prg->mapElPool.allocate();
		newList->id = list->genericInfo->langElId;
		newList->genericInfo = list->genericInfo;

		list->refs -= 1;

		ListEl *src = list->head;
		while( src != 0 ) {
			ListEl *newEl = prg->listElPool.allocate();
			newEl->value = src->value;
			tree_upref( newEl->value );

			newList->append( newEl );

			/* Watch out for next down. */
			if ( (Kid*)src == oldNextDown )
				newNextDown = (Kid*)newEl;

			src = src->next;
		}

		list = newList;
	}
	return list;
}

	
Map *copy_map( Program *prg, Map *map, Kid *oldNextDown, Kid *&newNextDown )
{
	if ( map->refs > 1 ) {
		#ifdef COLM_LOG_BYTECODE
		cerr << "splitting map: " << map << " refs: " << 
				map->refs << endl;
		#endif

		Map *newMap = (Map*)prg->mapElPool.allocate();
		newMap->id = map->genericInfo->langElId;
		newMap->genericInfo = map->genericInfo;
		newMap->treeSize = map->treeSize;
		newMap->root = 0;

		/* If there is a root, copy the tree. */
		if ( map->root != 0 ) {
			newMap->root = newMap->copyBranch( prg, map->root, 
					oldNextDown, newNextDown );
		}

		map->refs -= 1;

		for ( MapEl *el = newMap->head; el != 0; el = el->next ) {
			assert( map->genericInfo->typeArg == TYPE_TREE );
			tree_upref( el->tree );
		}

		map = newMap;
	}
	return map;
}

Tree *copy_tree( Program *prg, Tree *tree, Kid *oldNextDown, Kid *&newNextDown )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	long genericId = lelInfo[tree->id].genericId;
	if ( genericId > 0 ) {
		GenericInfo *generic = &prg->rtd->genericInfo[genericId];
		if ( generic->type == GEN_LIST )
			tree = (Tree*) copy_list( prg, (List*) tree, oldNextDown, newNextDown );
		else if ( generic->type == GEN_MAP )
			tree = (Tree*) copy_map( prg, (Map*) tree, oldNextDown, newNextDown );
		else
			assert(false);
	}
	else if ( tree->id == LEL_ID_PTR )
		assert(false);
	else if ( tree->id == LEL_ID_BOOL )
		assert(false);
	else if ( tree->id == LEL_ID_INT )
		assert(false);
	else if ( tree->id == LEL_ID_STR )
		assert(false);
	else
		tree = copy_real_tree( prg, tree, oldNextDown, newNextDown );

	assert( tree->refs == 0 );
	return tree;
}

Tree *split_tree( Program *prg, Tree *tree )
{
	if ( tree != 0 ) {
		assert( tree->refs >= 1 );

		if ( tree->refs > 1 ) {
			#ifdef COLM_LOG_BYTECODE
			cerr << "splitting tree: " << tree << " refs: " << 
					tree->refs << endl;
			#endif

			Kid *oldNextDown = 0, *newNextDown = 0;
			tree = copy_tree( prg, tree, oldNextDown, newNextDown );
			tree_upref( tree );
		}

		assert( tree->refs == 1 );
	}
	return tree;
}

Tree *create_generic( Program *prg, Word genericId )
{
	GenericInfo *genericInfo = &prg->rtd->genericInfo[genericId];
	Tree *newGeneric = 0;
	switch ( genericInfo->type ) {
		case GEN_MAP: {
			Map *map = (Map*)prg->mapElPool.allocate();
			map->id = genericInfo->langElId;
			map->genericInfo = genericInfo;
			newGeneric = (Tree*) map;
			break;
		}
		case GEN_LIST: {
			List *list = (List*)prg->mapElPool.allocate();
			list->id = genericInfo->langElId;
			list->genericInfo = genericInfo;
			newGeneric = (Tree*) list;
			break;
		}
		default:
			assert(false);
			return 0;
	}

	return newGeneric;
}

bool map_insert( Program *prg, Map *map, Tree *key, Tree *element )
{
	MapEl *mapEl = map->insert( prg, key );

	if ( mapEl != 0 ) {
		mapEl->tree = element;
		return true;
	}

	return false;
}

void map_unremove( Program *prg, Map *map, Tree *key, Tree *element )
{
	MapEl *mapEl = map->insert( prg, key );
	assert( mapEl != 0 );
	mapEl->tree = element;
}

Tree *map_uninsert( Program *prg, Map *map, Tree *key )
{
	MapEl *el = map->detach( key );
	Tree *val = el->tree;
	prg->mapElPool.free( el );
	return val;
}

Tree *map_store( Program *prg, Map *map, Tree *key, Tree *element )
{
	Tree *oldTree = 0;
	MapEl *elInTree = 0;
	MapEl *mapEl = map->insert( prg, key, &elInTree );

	if ( mapEl != 0 )
		mapEl->tree = element;
	else {
		/* Element with key exists. Overwriting the value. */
		oldTree = elInTree->tree;
		elInTree->tree = element;
	}

	return oldTree;
}

Tree *map_unstore( Program *prg, Map *map, Tree *key, Tree *existing )
{
	Tree *stored = 0;
	if ( existing == 0 ) {
		MapEl *mapEl = map->detach( key );
		stored = mapEl->tree;
		prg->mapElPool.free( mapEl );
	}
	else {
		MapEl *mapEl = map->find( key );
		stored = mapEl->tree;
		mapEl->tree = existing;
	}
	return stored;
}

Tree *map_find( Map *map, Tree *key )
{
	MapEl *mapEl = map->find( key );
	return mapEl == 0 ? 0 : mapEl->tree;
}

long map_length( Map *map )
{
	return map->length();
}

long list_length( List *list )
{
	return list->length();
}

void list_append( Program *prg, List *list, Tree *val )
{
	assert( list->refs == 1 );
	if ( val != 0 )
		assert( val->refs >= 1 );
	ListEl *listEl = prg->listElPool.allocate();
	listEl->value = val;
	list->append( listEl );
}

Tree *list_remove_end( Program *prg, List *list )
{
	Tree *tree = list->tail->value;
	prg->listElPool.free( list->detachLast() );
	return tree;
}

Tree *get_list_mem( List *list, Word field )
{
	Tree *result = 0;
	switch ( field ) {
		case 0: 
			result = list->head->value;
			break;
		case 1: 
			result = list->tail->value;
			break;
		default:
			assert( false );
			break;
	}
	return result;
}

Tree *get_list_mem_split( Program *prg, List *list, Word field )
{
	Tree *sv = 0;
	switch ( field ) {
		case 0: 
			sv = split_tree( prg, list->head->value );
			list->head->value = sv; 
			break;
		case 1: 
			sv = split_tree( prg, list->tail->value );
			list->tail->value = sv; 
			break;
		default:
			assert( false );
			break;
	}
	return sv;
}

Tree *set_list_mem( List *list, Half field, Tree *value )
{
	assert( list->refs == 1 );
	if ( value != 0 )
		assert( value->refs >= 1 );

	Tree *existing = 0;
	switch ( field ) {
		case 0:
			existing = list->head->value;
			list->head->value = value;
			break;
		case 1:
			existing = list->tail->value;
			list->tail->value = value;
			break;
		default:
			assert( false );
			break;
	}
	return existing;
}

struct TreePair
{
	TreePair() : key(0), val(0) {}

	Tree *key;
	Tree *val;
};

TreePair map_remove( Program *prg, Map *map, Tree *key )
{
	MapEl *mapEl = map->find( key );
	TreePair result;
	if ( mapEl != 0 ) {
		map->detach( mapEl );
		result.key = mapEl->key;
		result.val = mapEl->tree;
		prg->mapElPool.free( mapEl );
	}

	return result;
}

void split_ref( Tree **&sp, Program *prg, Ref *fromRef )
{
	/* Go up the chain of kids, turing the pointers down. */
	Ref *last = 0, *ref = fromRef, *next = 0;
	while ( ref->next != 0 ) {
		next = ref->next;
		ref->next = last;
		last = ref;
		ref = next;
	}
	ref->next = last;

	/* Now traverse the list, which goes down. */
	while ( ref != 0 ) {
		if ( ref->kid->tree->refs > 1 ) {
			#ifdef COLM_LOG_BYTECODE
			cerr << "splitting tree: " << ref->kid << " refs: " << 
					ref->kid->tree->refs << endl;
			#endif

			Ref *nextDown = ref->next;
			while ( nextDown != 0 && nextDown->kid == ref->kid )
				nextDown = nextDown->next;

			Kid *oldNextKidDown = nextDown != 0 ? nextDown->kid : 0;
			Kid *newNextKidDown = 0;

			Tree *newTree = copy_tree( prg, ref->kid->tree, 
					oldNextKidDown, newNextKidDown );
			tree_upref( newTree );

			while ( ref != 0 && ref != nextDown ) {
				next = ref->next;
				ref->next = 0;

				ref->kid->tree = newTree;
				ref = next;
			}

			/* Correct kid pointers down from ref. */
			while ( nextDown != 0 && nextDown->kid == oldNextKidDown ) {
				nextDown->kid = newNextKidDown;
				nextDown = nextDown->next;
			}
		}
		else {
			/* Reset the list as we go down. */
			next = ref->next;
			ref->next = 0;
			ref = next;
		}
	}
}

void split_iter_cur( Tree **&sp, Program *prg, TreeIter *iter )
{
	if ( iter->ref.kid == 0 )
		return;
	
	split_ref( sp, prg, &iter->ref );
}

void set_ref_value( Ref *ref, Tree *v )
{
	Kid *firstKid = ref->kid;
	while ( ref != 0 && ref->kid == firstKid ) {
		ref->kid->tree = v;
		ref = ref->next;
	}
}

long cmp_tree( const Tree *tree1, const Tree *tree2 )
{
	long cmpres = 0;
	if ( tree1 == 0 ) {
		if ( tree2 == 0 )
			return 0;
		else
			return -1;
	}
	else if ( tree2 == 0 )
		return 1;
	else if ( tree1->id < tree2->id )
		return -1;
	else if ( tree1->id > tree2->id )
		return 1;
	else if ( tree1->id == LEL_ID_PTR ) {
		if ( ((Pointer*)tree1)->value < ((Pointer*)tree2)->value )
			return -1;
		else if ( ((Pointer*)tree1)->value > ((Pointer*)tree2)->value )
			return 1;
	}
	else if ( tree1->id == LEL_ID_INT ) {
		if ( ((Int*)tree1)->value < ((Int*)tree2)->value )
			return -1;
		else if ( ((Int*)tree1)->value > ((Int*)tree2)->value )
			return 1;
	}
	else if ( tree1->id == LEL_ID_STR ) {
		cmpres = cmp_string( ((Str*)tree1)->value, ((Str*)tree2)->value );
		if ( cmpres != 0 )
			return cmpres;
	}
	else {
		if ( tree1->tokdata == 0 && tree2->tokdata != 0 )
			return -1;
		else if ( tree1->tokdata != 0 && tree2->tokdata == 0 )
			return 1;
		else if ( tree1->tokdata != 0 && tree2->tokdata != 0 ) {
			cmpres = cmp_string( tree1->tokdata, tree2->tokdata );
			if ( cmpres != 0 )
				return cmpres;
		}
	}

	Kid *kid1 = tree1->child;
	Kid *kid2 = tree2->child;

	while ( true ) {
		if ( kid1 == 0 && kid2 == 0 )
			return 0;
		else if ( kid1 == 0 && kid2 != 0 )
			return -1;
		else if ( kid1 != 0 && kid2 == 0 )
			return 1;
		else {
			cmpres = cmp_tree( kid1->tree, kid2->tree );
			if ( cmpres != 0 )
				return cmpres;
		}
		kid1 = kid1->next;
		kid2 = kid2->next;
	}
}

/* This must traverse in the same order that the bindId assignments are done
 * in. */
bool match_pattern( Tree **bindings, Program *prg, int pat, Kid *kid, bool checkNext )
{
	PatReplNode *nodes = prg->rtd->patReplNodes;

	#ifdef COLM_LOG_MATCH
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	cerr << "match_pattern " << ( pat == -1 ? "NULL" : lelInfo[nodes[pat].id].name ) <<
			" vs " << ( kid == 0 ? "NULL" : lelInfo[kid->tree->id].name ) << endl;
	#endif

	/* match node, recurse on children. */
	if ( pat != -1 && kid != 0 ) {
		if ( nodes[pat].id == kid->tree->id ) {
			/* If the pattern node has data, then this means we need to match
			 * the data against the token data. */
			if ( nodes[pat].data != 0 ) {
				/* Check the length of token text. */
				if ( nodes[pat].length != string_length( kid->tree->tokdata ) )
					return false;

				/* Check the token text data. */
				if ( nodes[pat].length > 0 && memcmp( nodes[pat].data, 
						string_data( kid->tree->tokdata ), nodes[pat].length ) != 0 )
					return false;
			}

			/* No failure, all okay. */
			if ( nodes[pat].bindId > 0 ) {
				#ifdef COLM_LOG_MATCH
				cerr << "bindId: " << nodes[pat].bindId << endl;
				#endif
				bindings[nodes[pat].bindId] = kid->tree;
			}

			/* If we didn't match a terminal duplicate of a nonterm then check
			 * down the children. */
			if ( !nodes[pat].stop ) {
				/* Check for failure down child branch. */
				bool childCheck = match_pattern( bindings, prg, 
						nodes[pat].child, tree_child( prg, kid->tree ), true );
				if ( ! childCheck )
					return false;
			}

			/* If checking next, then look for failure there. */
			if ( checkNext ) {
				bool nextCheck = match_pattern( bindings, prg, 
						nodes[pat].next, kid->next, true );
				if ( ! nextCheck )
					return false;
			}

			return true;
		}
	}
	else if ( pat == -1 && kid == 0 ) {
		/* Both null is a match. */
		return 1;
	}

	return false;
}

Tree *construct_integer( Program *prg, long i )
{
	Int *integer = (Int*) prg->treePool.allocate();
	integer->id = LEL_ID_INT;
	integer->value = i;

	return (Tree*)integer;
}

Tree *construct_string( Program *prg, Head *s )
{
	Str *str = (Str*) prg->treePool.allocate();
	str->id = LEL_ID_STR;
	str->value = s;

	return (Tree*)str;
}

Tree *construct_pointer( Program *prg, Tree *tree )
{
	Kid *kid = prg->kidPool.allocate();
	kid->tree = tree;
	kid->next = prg->heap;
	prg->heap = kid;

	Pointer *pointer = (Pointer*) prg->treePool.allocate();
	pointer->id = LEL_ID_PTR;
	pointer->value = kid;
	
	return (Tree*)pointer;
}

Tree *construct_term( Program *prg, Word id, Head *tokdata )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;

	Tree *tree = prg->treePool.allocate();
	tree->id = id;
	tree->refs = 0;
	tree->tokdata = tokdata;

	int objectLength = lelInfo[tree->id].objectLength;
	tree->child = alloc_attrs( prg, objectLength );

	return tree;
}

Kid *construct_replacement_kid( Tree **bindings, Program *prg, Kid *prev, int pat );

Kid *construct_ignore_list( Program *prg, long pat )
{
	PatReplNode *nodes = prg->rtd->patReplNodes;
	long ignore = nodes[pat].ignore;

	Kid *first = 0, *last = 0;
	while ( ignore >= 0 ) {
		Head *ignoreData = string_alloc_const( prg, nodes[ignore].data, nodes[ignore].length );

		Tree *ignTree = prg->treePool.allocate();
		ignTree->refs = 1;
		ignTree->id = nodes[ignore].id;
		ignTree->tokdata = ignoreData;

		Kid *ignKid = prg->kidPool.allocate();
		ignKid->tree = ignTree;
		ignKid->next = 0;

		if ( last == 0 )
			first = ignKid;
		else
			last->next = ignKid;

		ignore = nodes[ignore].next;
		last = ignKid;
	}

	return first;
}

/* Returns an uprefed tree. Saves us having to downref and bindings to zero to
 * return a zero-ref tree. */
Tree *construct_replacement_tree( Tree **bindings, Program *prg, int pat )
{
	PatReplNode *nodes = prg->rtd->patReplNodes;
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	Tree *tree = 0;

	if ( nodes[pat].bindId > 0 ) {
		/* All bindings have been uprefed. */
		tree = bindings[nodes[pat].bindId];

		long ignore = nodes[pat].ignore;
		if ( ignore >= 0 ) {
			tree = split_tree( prg, tree );
			tree->child = construct_ignore_list( prg, pat );
		}
	}
	else {
		tree = prg->treePool.allocate();
		tree->id = nodes[pat].id;
		tree->refs = 1;
		tree->tokdata = nodes[pat].length == 0 ? 0 :
				string_alloc_const( prg, 
				nodes[pat].data, nodes[pat].length );

		int objectLength = lelInfo[tree->id].objectLength;

		Kid *attrs = alloc_attrs( prg, objectLength );
		Kid *ignore = construct_ignore_list( prg, pat );
		Kid *child = construct_replacement_kid( bindings, prg, 
				0, nodes[pat].child );

		tree->child = kid_list_concat( attrs, 
				kid_list_concat( ignore, child ) );
	}

	return tree;
}

Kid *construct_replacement_kid( Tree **bindings, Program *prg, Kid *prev, int pat )
{
	PatReplNode *nodes = prg->rtd->patReplNodes;
	Kid *kid = 0;

	if ( pat != -1 ) {
		kid = prg->kidPool.allocate();
		kid->tree = construct_replacement_tree( bindings, prg, pat );

		/* Recurse down next. */
		Kid *next = construct_replacement_kid( bindings, prg, 
				kid, nodes[pat].next );

		kid->next = next;
	}

	return kid;
}

bool test_false( Program *prg, Tree *tree )
{
	bool flse = ( 
		tree == 0 ||
		tree == prg->falseVal ||
		tree->id == LEL_ID_INT && ((Int*)tree)->value == 0 );
	return flse;
}

/*
 * Execution environment
 */

Program::Program( bool ctxDepParsing, RuntimeData *rtd )
:
	ctxDepParsing(ctxDepParsing),
	rtd(rtd),
	global(0),
	heap(0),
	stdinVal(0),
	stdoutVal(0),
	stderrVal(0)
{
	Int *trueInt = (Int*) treePool.allocate();
	trueInt->id = LEL_ID_BOOL;
	trueInt->refs = 1;
	trueInt->value = 1;

	Int *falseInt = (Int*) treePool.allocate();
	falseInt->id = LEL_ID_BOOL;
	falseInt->refs = 1;
	falseInt->value = 0;

	trueVal = (Tree*)trueInt;
	falseVal = (Tree*)falseInt;
}

void Program::clearGlobal( Tree **sp )
{
	/* Downref all the fields in the global object. */
	for ( int g = 0; g < rtd->globalSize; g++ ) {
		//assert( get_attr( global, g )->refs == 1 );
		tree_downref( this, sp, get_attr( global, g ) );
	}

	/* Free the global object. */
	if ( rtd->globalSize > 0 )
		free_attrs( this, global->child );
	treePool.free( global );
}

void Program::clear( Tree **sp )
{
	#ifdef COLM_LOG_BYTECODE
	cerr << "clearing the prg" << endl;
	#endif

	clearGlobal( sp );

	/* Clear the heap. */
	Kid *a = heap;
	while ( a != 0 ) {
		Kid *next = a->next;
		tree_downref( this, sp, a->tree );
		kidPool.free( a );
		a = next;
	}

	//assert( trueVal->refs == 1 );
	//assert( falseVal->refs == 1 );
	tree_downref( this, sp, trueVal );
	tree_downref( this, sp, falseVal );

	tree_downref( this, sp, (Tree*)stdinVal );
	tree_downref( this, sp, (Tree*)stdoutVal );
	tree_downref( this, sp, (Tree*)stderrVal );

	long kidLost = kidPool.numlost();
	if ( kidLost )
		cerr << "warning lost kids: " << kidLost << endl;

	long treeLost = treePool.numlost();
	if ( treeLost )
		cerr << "warning lost trees: " << treeLost << endl;

	long algLost = algPool.numlost();
	if ( algLost )
		cerr << "warning lost algs: " << algLost << endl;

	long listLost = listElPool.numlost();
	if ( listLost )
		cerr << "warning lost listEls: " << listLost << endl;

	long mapLost = mapElPool.numlost();
	if ( mapLost )
		cerr << "warning lost mapEls: " << mapLost << endl;

	kidPool.clear();
	treePool.clear();
	algPool.clear();
	listElPool.clear();
	mapElPool.clear();

	//reverseCode.empty();

	memset( vm_stack, 0, sizeof(Tree*) * VM_STACK_SIZE);
}

void Program::allocGlobal()
{
	/* Alloc the global. */
	Tree *tree = treePool.allocate();
	tree->child = alloc_attrs( this, rtd->globalSize );
	tree->refs = 1;
	global = tree;
}

void Program::run()
{
	assert( sizeof(Int)      <= sizeof(Tree) );
	assert( sizeof(Str)      <= sizeof(Tree) );
	assert( sizeof(Pointer)  <= sizeof(Tree) );
	assert( sizeof(Map)      <= sizeof(MapEl) );
	assert( sizeof(List)     <= sizeof(MapEl) );
	assert( sizeof(Stream)   <= sizeof(MapEl) );

	/* Allocate the global variable. */
	allocGlobal();

	/* 
	 * Allocate the VM stack.
	 */

	//vm_stack = new Tree*[VM_STACK_SIZE];
	vm_stack = (Tree**)mmap( 0, sizeof(Tree*)*VM_STACK_SIZE,
		PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0 );
	Tree **root = &vm_stack[VM_STACK_SIZE];

	/*
	 * Execute
	 */

	if ( rtd->rootCodeLen > 0 ) {
		CodeVect reverseCode;
		CodeVect *allReverseCode = new CodeVect;
		Execution execution( this, reverseCode, 0, rtd->rootCode, 0, 0 );
		execution.execute( root );

		/* Pull out the reverse code and free it. */
		#ifdef COLM_LOG_BYTECODE
		cerr << "freeing the root reverse code" << endl;
		#endif

		bool hasrcode = makeReverseCode( allReverseCode, reverseCode );
		if ( hasrcode )
			rcode_downref( root, this, allReverseCode->data );
		delete allReverseCode;
	}

	/* Clear */
	clear( root );
}

Execution::Execution( Program *prg, CodeVect &reverseCode,
		PdaRun *parser, Code *code, Tree *lhs, Head *matchText )
: 
	prg(prg), 
	parser(parser), 
	code(code), 
	frame(0), iframe(0),
	lhs(lhs),
	matchText(matchText),
	reject(false), 
	reverseCode(reverseCode),
	rcodeUnitLen(0)
{
	if ( lhs != 0 ) {
		assert( lhs->refs == 1 );
	}
}

void rcode_downref_all( Tree **sp, Program *prg, CodeVect *rev )
{
	while ( rev->length() > 0 ) {
		/* Read the length */
		Code *prcode = rev->data + rev->length() - 4;
		Word len;
		read_word_p( len, prcode );

		/* Find the start of block. */
		long start = rev->length() - len - 4;
		prcode = rev->data + start;

		/* Execute it. */
		rcode_downref( sp, prg, prcode );

		/* Backup over it. */
		rev->tabLen -= len + 4;
	}
}

void rcode_downref( Tree **sp, Program *prg, Code *instr )
{
again:
	switch ( *instr++ ) {
		case IN_PARSE_BKT: {
			Half parserId;
			Tree *stream, *tree;
			Word wrev;
			read_half( parserId );
			read_tree( stream );
			read_tree( tree );
			read_word( wrev );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PARSE_BKT " << parserId << endl;
			#endif

			parsed_downref( sp, prg, tree );
			rcode_downref_all( sp, prg, (CodeVect*)wrev );
			tree_downref( prg, sp, stream );
			tree_downref( prg, sp, tree );
			delete (CodeVect*)wrev;
			break;
		}
		case IN_STREAM_PULL_BKT: {
			Tree *stream, *str;
			read_tree( stream );
			read_tree( str );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STREAM_PULL_BKT" << endl;
			#endif

			tree_downref( prg, sp, stream );
			tree_downref( prg, sp, str );
			break;
		}
		case IN_STREAM_PUSH_BKT: {
			Tree *stream;
			Word len;
			read_tree( stream );
			read_word( len );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STREAM_PUSH_BKT" << endl;
			#endif

			tree_downref( prg, sp, stream );
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LOAD_GLOBAL_BKT" << endl;
			#endif
			break;
		}
		case IN_GET_FIELD_BKT: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_FIELD_BKT " << field << endl;
			#endif
			break;
		}
		case IN_SET_FIELD_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_FIELD_BKT " << field << endl;
			#endif

			tree_downref( prg, sp, val );
			break;
		}
		case IN_PTR_DEREF_BKT: {
			Tree *ptr;
			read_tree( ptr );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PTR_DEREF_BKT" << endl;
			#endif

			tree_downref( prg, sp, ptr );
			break;
		}
		case IN_SET_TOKEN_DATA_BKT: {
			Word oldval;
			read_word( oldval );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_TOKEN_DATA_BKT " << endl;
			#endif

			Head *head = (Head*)oldval;
			string_free( prg, head );
			break;
		}
		case IN_LIST_APPEND_BKT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LIST_APPEND_BKT" << endl;
			#endif
			break;
		}
		case IN_LIST_REMOVE_END_BKT: {
			Tree *val;
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LIST_REMOVE_END_BKT" << endl;
			#endif

			tree_downref( prg, sp, val );
			break;
		}
		case IN_GET_LIST_MEM_BKT: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_LIST_MEM_BKT " << field << endl;
			#endif
			break;
		}
		case IN_SET_LIST_MEM_BKT: {
			Half field;
			Tree *val;
			read_half( field );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_LIST_MEM_BKT " << field << endl;
			#endif

			tree_downref( prg, sp, val );
			break;
		}
		case IN_MAP_INSERT_BKT: {
			uchar inserted;
			Tree *key;
			read_byte( inserted );
			read_tree( key );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_INSERT_BKT" << endl;
			#endif
			
			tree_downref( prg, sp, key );
			break;
		}
		case IN_MAP_STORE_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_STORE_BKT" << endl;
			#endif

			tree_downref( prg, sp, key );
			tree_downref( prg, sp, val );
			break;
		}
		case IN_MAP_REMOVE_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_REMOVE_BKT" << endl;
			#endif

			tree_downref( prg, sp, key );
			tree_downref( prg, sp, val );
			break;
		}
		case IN_STOP: {
			return;
		}
		default: {
			cerr << "UNKNOWN INSTRUCTION: " << (ulong)instr[-1] << 
					" -- reverse code downref" << endl;
			assert(false);
			break;
		}
	}
	goto again;
}

void Execution::execute( Tree **root )
{
	Tree **sp = root;

	/* If we have a lhs push it to the stack. */
	bool haveLhs = lhs != 0;
	if ( haveLhs )
		push( lhs );

	/* Execution loop. */
	execute( sp, code );

	/* Take the lhs off the stack. */
	if ( haveLhs )
		lhs = (Tree*) pop();

	assert( sp == root );
}

bool makeReverseCode( CodeVect *all, CodeVect &reverseCode )
{
	/* Do we need to revert the left hand side? */

	/* Check if there was anything generated. */
	if ( reverseCode.length() == 0 )
		return false;

	long prevAllLength = all->length();

	/* Go backwards, group by group, through the reverse code. Push each group
	 * to the global reverse code stack. */
	Code *p = reverseCode.data + reverseCode.length();
	while ( p != reverseCode.data ) {
		p--;
		long len = *p;
		p = p - len;
		all->append( p, len );
	}

	/* Stop, then place a total length in the global stack. */
	all->append( IN_STOP );
	long length = all->length() - prevAllLength;
	all->appendWord( length );

	/* Clear the revere code buffer. */
	reverseCode.tabLen = 0;

	return true;
}

void Execution::rexecute( Tree **root, CodeVect *allRev )
{
	/* Read the length */
	Code *prcode = allRev->data + allRev->length() - 4;
	Word len;
	read_word_p( len, prcode );

	/* Find the start of block. */
	long start = allRev->length() - len - 4;
	prcode = allRev->data + start;

	/* Execute it. */
	Tree **sp = root;
	execute( sp, prcode );
	assert( sp == root );

	/* Backup over it. */
	allRev->tabLen -= len + 4;
}

void Execution::execute( Tree **&sp, Code *instr )
{
again:
	switch ( *instr++ ) {
		case IN_LOAD_NIL: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LOAD_NIL" << endl;
			#endif

			push( 0 );
			break;
		}
		case IN_LOAD_TRUE: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LOAD_TRUE" << endl;
			#endif

			tree_upref( prg->trueVal );
			push( prg->trueVal );
			break;
		}
		case IN_LOAD_FALSE: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LOAD_FALSE" << endl;
			#endif

			tree_upref( prg->falseVal );
			push( prg->falseVal );
			break;
		}
		case IN_LOAD_INT: {
			Word i;
			read_word( i );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LOAD_INT " << i << endl;
			#endif

			Tree *tree = construct_integer( prg, i );
			tree_upref( tree );
			push( tree );
			break;
		}
		case IN_LOAD_STR: {
			Word offset;
			read_word( offset );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LOAD_STR " << offset << endl;
			#endif

			Head *lit = make_literal( prg, offset );
			Tree *tree = construct_string( prg, lit );
			tree_upref( tree );
			push( tree );
			break;
		}
		case IN_PRINT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PRINT" << endl;
			#endif

			Tree *tree = pop();
			print_tree( sp, prg, tree );
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_PRINT_XML: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PRINT_XML" << endl;
			#endif

			Tree *tree = pop();
			xml_print_tree( sp, prg, tree );
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_LOAD_GLOBAL_R: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LOAD_GLOBAL_R" << endl;
			#endif

			tree_upref( prg->global );
			push( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_WV: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LOAD_GLOBAL_WV" << endl;
			#endif

			tree_upref( prg->global );
			push( prg->global );

			/* Set up the reverse instruction. */
			reverseCode.append( IN_LOAD_GLOBAL_BKT );
			rcodeUnitLen = 1;
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LOAD_GLOBAL_BKT" << endl;
			#endif

			tree_upref( prg->global );
			push( prg->global );
			break;
		}
		case IN_INIT_RHS_EL: {
			Half position;
			short field;
			read_half( position );
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_INIT_RHS_EL " << position << " " << field << endl;
			#endif

			Tree *val = get_rhs_el( prg, lhs, position );
			tree_upref( val );
			local(field) = val;
			break;
		}
		case IN_UITER_ADVANCE: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_UITER_ADVANCE " << field << endl;
			#endif

			/* Get the iterator. */
			UserIter *uiter = (UserIter*) local(field);

			long stackSize = uiter->stackRoot - ptop();
			assert( uiter->stackSize == stackSize );

			/* Fix the return instruction pointer. */
			uiter->stackRoot[-IFR_AA + IFR_RIN] = (SW)instr;

			instr = uiter->resume;
			frame = uiter->frame;
			iframe = &uiter->stackRoot[-IFR_AA];
			break;
		}
		case IN_UITER_GET_CUR_R: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_UITER_GET_CUR_R " << field << endl;
			#endif

			UserIter *uiter = (UserIter*) local(field);
			Tree *val = uiter->ref.kid->tree;
			tree_upref( val );
			push( val );
			break;
		}
		case IN_UITER_GET_CUR_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_UITER_GET_CUR_WC " << field << endl;
			#endif

			UserIter *uiter = (UserIter*) local(field);
			split_ref( sp, prg, &uiter->ref );
			Tree *split = uiter->ref.kid->tree;
			tree_upref( split );
			push( split );
			break;
		}
		case IN_UITER_SET_CUR_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_UITER_SET_CUR_WC " << field << endl;
			#endif

			Tree *t = pop();
			UserIter *uiter = (UserIter*) local(field);
			split_ref( sp, prg, &uiter->ref );
			Tree *old = uiter->ref.kid->tree;
			uiter->ref.kid->tree = t;
			tree_downref( prg, sp, old );
			break;
		}
		case IN_GET_LOCAL_R: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_LOCAL_R " << field << endl;
			#endif

			Tree *val = local(field);
			tree_upref( val );
			push( val );
			break;
		}
		case IN_GET_LOCAL_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_LOCAL_WC " << field << endl;
			#endif

			Tree *split = get_local_split( prg, frame, field );
			tree_upref( split );
			push( split );
			break;
		}
		case IN_SET_LOCAL_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_LOCAL_WC " << field << endl;
			#endif

			Tree *val = pop();
			tree_downref( prg, sp, local(field) );
			set_local( frame, field, val );
			break;
		}
		case IN_SAVE_RET: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SAVE_RET " << endl;
			#endif

			Tree *val = pop();
			local(FR_RV) = val;
			break;
		}
		case IN_GET_LOCAL_REF_R: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_LOCAL_REF_R " << field << endl;
			#endif

			Ref *ref = (Ref*) plocal(field);
			Tree *val = ref->kid->tree;
			tree_upref( val );
			push( val );
			break;
		}
		case IN_GET_LOCAL_REF_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_LOCAL_REF_WC " << field << endl;
			#endif

			Ref *ref = (Ref*) plocal(field);
			split_ref( sp, prg, ref );
			Tree *val = ref->kid->tree;
			tree_upref( val );
			push( val );
			break;
		}
		case IN_SET_LOCAL_REF_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_LOCAL_REF_WC " << field << endl;
			#endif

			Tree *val = pop();
			Ref *ref = (Ref*) plocal(field);
			split_ref( sp, prg, ref );
			set_ref_value( ref, val );
			break;
		}
		case IN_GET_FIELD_R: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_FIELD_R " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = get_field( obj, field );
			tree_upref( val );
			push( val );
			break;
		}
		case IN_GET_FIELD_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_FIELD_WC " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *split = get_field_split( prg, obj, field );
			tree_upref( split );
			push( split );
			break;
		}
		case IN_GET_FIELD_WV: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_FIELD_WV " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *split = get_field_split( prg, obj, field );
			tree_upref( split );
			push( split );

			/* Set up the reverse instruction. */
			reverseCode.append( IN_GET_FIELD_BKT );
			reverseCode.appendHalf( field );
			rcodeUnitLen += 3;
			break;
		}
		case IN_GET_FIELD_BKT: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_FIELD_BKT " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *split = get_field_split( prg, obj, field );
			tree_upref( split );
			push( split );
			break;
		}
		case IN_SET_FIELD_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_FIELD_WC " << field << endl;
			#endif

			Tree *obj = pop();
			Tree *val = pop();
			tree_downref( prg, sp, obj );

			/* Downref the old value. */
			Tree *prev = get_field( obj, field );
			tree_downref( prg, sp, prev );

			set_field( prg, obj, field, val );
			break;
		}
		case IN_SET_FIELD_WV: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_FIELD_WV " << field << endl;
			#endif

			Tree *obj = pop();
			Tree *val = pop();
			tree_downref( prg, sp, obj );

			/* Save the old value, then set the field. */
			Tree *prev = get_field( obj, field );
			set_field( prg, obj, field, val );

			/* Set up the reverse instruction. */
			reverseCode.append( IN_SET_FIELD_BKT );
			reverseCode.appendHalf( field );
			reverseCode.appendWord( (Word)prev );
			rcodeUnitLen += 7;
			reverseCode.append( rcodeUnitLen );
			/* FLUSH */
			break;
		}
		case IN_SET_FIELD_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_FIELD_BKT " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			/* Downref the old value. */
			Tree *prev = get_field( obj, field );
			tree_downref( prg, sp, prev );

			set_field( prg, obj, field, val );
			break;
		}
		case IN_SET_FIELD_LEAVE_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_FIELD_LEAVE_WC " << field << endl;
			#endif

			/* Note that we don't downref the object here because we are
			 * leaving it on the stack. */
			Tree *obj = pop();
			Tree *val = pop();

			/* Downref the old value. */
			Tree *prev = get_field( obj, field );
			tree_downref( prg, sp, prev );

			/* Set the field. */
			set_field( prg, obj, field, val );

			/* Leave the object on the top of the stack. */
			push( obj );
			break;
		}
		case IN_POP: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_POP" << endl;
			#endif

			Tree *val = pop();
			tree_downref( prg, sp, val );
			break;
		}
		case IN_STR_ATOI: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STR_ATOI" << endl;
			#endif

			Str *str = (Str*)pop();
			Word res = str_atoi( str->value );
			Tree *integer = construct_integer( prg, res );
			tree_upref( integer );
			push( integer );
			tree_downref( prg, sp, (Tree*)str );
			break;
		}
		case IN_INT_TO_STR: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_INT_TO_STR" << endl;
			#endif

			Int *i = (Int*)pop();
			Head *res = int_to_str( prg, i->value );
			Tree *str = construct_string( prg, res );
			tree_upref( str );
			push( str );
			tree_downref( prg, sp, (Tree*) i );
			break;
		}
		case IN_CONCAT_STR: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_CONCAT_STR" << endl;
			#endif

			Str *s2 = (Str*)pop();
			Str *s1 = (Str*)pop();
			Head *res = concat_str( s1->value, s2->value );
			Tree *str = construct_string( prg, res );
			tree_upref( str );
			tree_downref( prg, sp, (Tree*)s1 );
			tree_downref( prg, sp, (Tree*)s2 );
			push( str );
			break;
		}
		case IN_STR_UORD8: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STR_UORD8" << endl;
			#endif

			Str *str = (Str*)pop();
			Word res = str_uord8( str->value );
			Tree *tree = construct_integer( prg, res );
			tree_upref( tree );
			push( tree );
			tree_downref( prg, sp, (Tree*)str );
			break;
		}
		case IN_STR_UORD16: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STR_UORD16" << endl;
			#endif

			Str *str = (Str*)pop();
			Word res = str_uord16( str->value );
			Tree *tree = construct_integer( prg, res );
			tree_upref( tree );
			push( tree );
			tree_downref( prg, sp, (Tree*)str );
			break;
		}

		case IN_STR_LENGTH: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STR_LENGTH" << endl;
			#endif

			Str *str = (Str*)pop();
			long len = string_length( str->value );
			Tree *res = construct_integer( prg, len );
			tree_upref( res );
			push( res );
			tree_downref( prg, sp, (Tree*)str );
			break;
		}
		case IN_JMP_FALSE: {
			short dist;
			read_half( dist );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_JMP_FALSE " << dist << endl;
			#endif

			Tree *tree = pop();
			if ( test_false( prg, tree ) )
				instr += dist;
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_JMP_TRUE: {
			short dist;
			read_half( dist );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_JMP_TRUE " << dist << endl;
			#endif

			Tree *tree = pop();
			if ( !test_false( prg, tree ) )
				instr += dist;
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_JMP: {
			short dist;
			read_half( dist );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_JMP " << dist << endl;
			#endif

			instr += dist;
			break;
		}
		case IN_REJECT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_REJECT" << endl;
			#endif
			reject = true;
			break;
		}

		/*
		 * Binary comparison operators.
		 */
		case IN_TST_EQL: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TST_EQL" << endl;
			#endif

			Tree *o2 = pop();
			Tree *o1 = pop();
			long r = cmp_tree( o1, o2 );
			Tree *val = r ? prg->falseVal : prg->trueVal;
			tree_upref( val );
			push( val );
			tree_downref( prg, sp, o1 );
			tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_NOT_EQL: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TST_NOT_EQL" << endl;
			#endif

			Tree *o2 = pop();
			Tree *o1 = pop();
			long r = cmp_tree( o1, o2 );
			Tree *val = r ? prg->trueVal : prg->falseVal;
			tree_upref( val );
			push( val );
			tree_downref( prg, sp, o1 );
			tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_LESS: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TST_LESS" << endl;
			#endif

			Tree *o2 = pop();
			Tree *o1 = pop();
			long r = cmp_tree( o1, o2 );
			Tree *val = r < 0 ? prg->trueVal : prg->falseVal;
			tree_upref( val );
			push( val );
			tree_downref( prg, sp, o1 );
			tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_LESS_EQL: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TST_LESS_EQL" << endl;
			#endif

			Tree *o2 = pop();
			Tree *o1 = pop();
			long r = cmp_tree( o1, o2 );
			Tree *val = r <= 0 ? prg->trueVal : prg->falseVal;
			tree_upref( val );
			push( val );
			tree_downref( prg, sp, o1 );
			tree_downref( prg, sp, o2 );
		}
		case IN_TST_GRTR: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TST_GRTR" << endl;
			#endif

			Tree *o2 = pop();
			Tree *o1 = pop();
			long r = cmp_tree( o1, o2 );
			Tree *val = r > 0 ? prg->trueVal : prg->falseVal;
			tree_upref( val );
			push( val );
			tree_downref( prg, sp, o1 );
			tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_GRTR_EQL: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TST_GRTR_EQL" << endl;
			#endif

			Tree *o2 = (Tree*)pop();
			Tree *o1 = (Tree*)pop();
			long r = cmp_tree( o1, o2 );
			Tree *val = r >= 0 ? prg->trueVal : prg->falseVal;
			tree_upref( val );
			push( val );
			tree_downref( prg, sp, o1 );
			tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_LOGICAL_AND: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TST_LOGICAL_AND" << endl;
			#endif

			Tree *o2 = pop();
			Tree *o1 = pop();
			long v2 = !test_false( prg, o2 );
			long v1 = !test_false( prg, o1 );
			Word r = v1 && v2;
			Tree *val = r ? prg->trueVal : prg->falseVal;
			tree_upref( val );
			push( val );
			tree_downref( prg, sp, o1 );
			tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_LOGICAL_OR: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TST_LOGICAL_OR" << endl;
			#endif

			Tree *o2 = pop();
			Tree *o1 = pop();
			long v2 = !test_false( prg, o2 );
			long v1 = !test_false( prg, o1 );
			Word r = v1 || v2;
			Tree *val = r ? prg->trueVal : prg->falseVal;
			tree_upref( val );
			push( val );
			tree_downref( prg, sp, o1 );
			tree_downref( prg, sp, o2 );
			break;
		}
		case IN_NOT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_NOT" << endl;
			#endif

			Tree *tree = (Tree*)pop();
			long r = test_false( prg, tree );
			Tree *val = r ? prg->trueVal : prg->falseVal;
			tree_upref( val );
			push( val );
			tree_downref( prg, sp, tree );
			break;
		}

		case IN_ADD_INT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_ADD_INT" << endl;
			#endif

			Int *o2 = (Int*)pop();
			Int *o1 = (Int*)pop();
			long r = o1->value + o2->value;
			Tree *tree = construct_integer( prg, r );
			tree_upref( tree );
			push( tree );
			tree_downref( prg, sp, (Tree*)o1 );
			tree_downref( prg, sp, (Tree*)o2 );
			break;
		}
		case IN_MULT_INT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MULT_INT" << endl;
			#endif

			Int *o2 = (Int*)pop();
			Int *o1 = (Int*)pop();
			long r = o1->value * o2->value;
			Tree *tree = construct_integer( prg, r );
			tree_upref( tree );
			push( tree );
			tree_downref( prg, sp, (Tree*)o1 );
			tree_downref( prg, sp, (Tree*)o2 );
			break;
		}
		case IN_SUB_INT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SUB_INT" << endl;
			#endif

			Int *o2 = (Int*)pop();
			Int *o1 = (Int*)pop();
			long r = o1->value - o2->value;
			Tree *tree = construct_integer( prg, r );
			tree_upref( tree );
			push( tree );
			tree_downref( prg, sp, (Tree*)o1 );
			tree_downref( prg, sp, (Tree*)o2 );
			break;
		}
		case IN_DUP_TOP: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_DUP_TOP" << endl;
			#endif

			Tree *val = top();
			tree_upref( val );
			push( val );
			break;
		}
		case IN_TRITER_FROM_REF: {
			short field;
			Half searchTypeId;
			read_half( field );
			read_half( searchTypeId );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TRITER_FROM_REF " << field << " " << searchTypeId << endl;
			#endif

			Ref rootRef;
			rootRef.kid = (Kid*)pop();
			rootRef.next = (Ref*)pop();
			void *mem = plocal(field);
			new(mem) TreeIter( rootRef, searchTypeId, ptop() );
			break;
		}
		case IN_TRITER_DESTROY: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TRITER_DESTROY " << field << endl;
			#endif

			TreeIter *iter = (TreeIter*) plocal(field);
			tree_iter_destroy( sp, iter );
			break;
		}
		case IN_TREE_SEARCH: {
			Word id;
			read_word( id );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TREE_SEARCH " << id << endl;
			#endif

			Tree *tree = pop();
			Tree *res = tree_search( tree, id );
			tree_upref( res );
			push( res );
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_TRITER_ADVANCE: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TRITER_ADVANCE " << field << endl;
			#endif

			TreeIter *iter = (TreeIter*) plocal(field);
			Tree *res = tree_iter_advance( prg, sp, iter );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_TRITER_NEXT_CHILD: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TRITER_NEXT_CHILD " << field << endl;
			#endif

			TreeIter *iter = (TreeIter*) plocal(field);
			Tree *res = tree_iter_next_child( prg, sp, iter );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_TRITER_PREV_CHILD: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TRITER_PREV_CHILD " << field << endl;
			#endif

			TreeIter *iter = (TreeIter*) plocal(field);
			Tree *res = tree_iter_prev_child( prg, sp, iter );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_TRITER_GET_CUR_R: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TRITER_GET_CUR_R " << field << endl;
			#endif
			
			TreeIter *iter = (TreeIter*) plocal(field);
			Tree *tree = tree_iter_deref_cur( iter );
			tree_upref( tree );
			push( tree );
			break;
		}
		case IN_TRITER_GET_CUR_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TRITER_GET_CUR_WC " << field << endl;
			#endif
			
			TreeIter *iter = (TreeIter*) plocal(field);
			split_iter_cur( sp, prg, iter );
			Tree *tree = tree_iter_deref_cur( iter );
			tree_upref( tree );
			push( tree );
			break;
		}
		case IN_TRITER_SET_CUR_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TRITER_SET_CUR_WC " << field << endl;
			#endif

			Tree *tree = pop();
			TreeIter *iter = (TreeIter*) plocal(field);
			split_iter_cur( sp, prg, iter );
			Tree *old = tree_iter_deref_cur( iter );
			set_triter_cur( iter, tree );
			tree_downref( prg, sp, old );
			break;
		}
		case IN_MATCH: {
			Half patternId;
			read_half( patternId );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MATCH " << patternId << endl;
			#endif

			Tree *tree = pop();

			/* Run the match, push the result. */
			int rootNode = prg->rtd->patReplInfo[patternId].offset;

			/* Bindings are indexed starting at 1. Zero bindId to represent no
			 * binding. We make a space for it here rather than do math at
			 * access them. */
			long numBindings = prg->rtd->patReplInfo[patternId].numBindings;
			Tree *bindings[1+numBindings];
			memset( bindings, 0, sizeof(Tree*)*(1+numBindings) );

			Kid kid;
			kid.tree = tree;
			kid.next = 0;
			bool matched = match_pattern( bindings, prg, rootNode, &kid, false );

			if ( !matched )
				memset( bindings, 0, sizeof(Tree*)*(1+numBindings) );
			else {
				for ( int b = 1; b <= numBindings; b++ )
					assert( bindings[b] != 0 );
			}

			#ifdef COLM_LOG_MATCH
			cerr << "match result: " << matched << endl;
			#endif

			Tree *result = matched ? tree : 0;
			tree_upref( result );
			push( result ? tree : 0 );
			for ( int b = 1; b <= numBindings; b++ ) {
				tree_upref( bindings[b] );
				push( bindings[b] );
			}

			tree_downref( prg, sp, tree );
			break;
		}
		case IN_PARSE: {
			Half parserId, stopId;
			read_half( parserId );
			read_half( stopId );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PARSE " << parserId << " " << stopId << endl;
			#endif

			/* Comes back from parse upreffed. */
			CodeVect *cv;
			Tree *stream = pop();
			Tree *res = parse( sp, prg, (Stream*)stream, parserId, stopId, cv );
			push( res );

			/* Single unit. */
			tree_upref( res );
			reverseCode.append( IN_PARSE_BKT );
			reverseCode.appendHalf( parserId );
			reverseCode.appendWord( (Word) stream );
			reverseCode.appendWord( (Word) res );
			reverseCode.appendWord( (Word) cv );
			reverseCode.append( 15 );
			break;
		}
		case IN_STREAM_PULL: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STREAM_PULL" << endl;
			#endif
			Tree *len = pop();
			Tree *stream = pop();
			Tree *string = stream_pull( prg, (Stream*)stream, len );
			tree_upref( string );
			push( string );

			/* Single unit. */
			tree_upref( string );
			reverseCode.append( IN_STREAM_PULL_BKT );
			reverseCode.appendWord( (Word) stream );
			reverseCode.appendWord( (Word) string );
			reverseCode.append( 9 );

			tree_downref( prg, sp, len );
			break;
		}
		case IN_STREAM_PULL_BKT: {
			Tree *stream, *string;
			read_tree( stream );
			read_tree( string );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STREAM_PULL_BKT" << endl;
			#endif

			undo_pull( prg, (Stream*)stream, string );
			tree_downref( prg, sp, stream );
			tree_downref( prg, sp, string );
			break;
		}
		case IN_STREAM_PUSH: {
			/* FIXME: Need to check the refcounting here. */

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STREAM_PUSH" << endl;
			#endif
			Tree *tree = pop();
			Tree *stream = pop();
			Word len = stream_push( sp, prg, 
					(Stream*)stream, tree );
			push( 0 );

			/* Single unit. */
			reverseCode.append( IN_STREAM_PUSH_BKT );
			reverseCode.appendWord( (Word)stream );
			reverseCode.appendWord( len );
			reverseCode.append( 9 );

			tree_downref( prg, sp, tree );
			break;
		}
		case IN_STREAM_PUSH_BKT: {
			Tree *stream;
			Word len;
			read_tree( stream );
			read_word( len );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STREAM_PUSH_BKT" << endl;
			#endif

			undo_stream_push( sp, prg, (Stream*)stream, len );
			tree_downref( prg, sp, stream );
			break;
		}
		case IN_PARSE_BKT: {
			Half parserId;
			Tree *stream, *tree;
			Word wrev;
			read_half( parserId );
			read_tree( stream );
			read_tree( tree );
			read_word( wrev );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PARSE_BKT " << parserId << endl;
			#endif

			undo_parse( sp, prg, (Stream*)stream, parserId, tree, (CodeVect*)wrev );
			tree_downref( prg, sp, stream );
			delete (CodeVect*)wrev;
			break;
		}
		case IN_CONSTRUCT: {
			Half patternId;
			read_half( patternId );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_CONSTRUCT " << patternId << endl;
			#endif

			int rootNode = prg->rtd->patReplInfo[patternId].offset;

			/* Note that bindIds are indexed at one. Add one spot for them. */
			int numBindings = prg->rtd->patReplInfo[patternId].numBindings;
			Tree *bindings[1+numBindings];

			for ( int b = 1; b <= numBindings; b++ ) {
				bindings[b] = pop();
				assert( bindings[b] != 0 );
			}

			Tree *replTree = 0;
			PatReplNode *nodes = prg->rtd->patReplNodes;
			LangElInfo *lelInfo = prg->rtd->lelInfo;
			long genericId = lelInfo[nodes[rootNode].id].genericId;
			if ( genericId > 0 ) {
				replTree = create_generic( prg, genericId );
				tree_upref( replTree );
			}
			else {
				replTree = construct_replacement_tree( bindings, 
						prg, rootNode );
			}

			push( replTree );
			break;
		}
		case IN_CONSTRUCT_TERM: {
			Half tokenId;
			read_half( tokenId );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_CONSTRUCT_TERM " << tokenId << endl;
			#endif

			/* Pop the string we are constructing the token from. */
			Str *str = (Str*)pop();
			Tree *res = construct_term( prg, tokenId, str->value );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_MAKE_TOKEN: {
			uchar nargs;
			read_byte( nargs );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAKE_TOKEN " << (ulong) nargs << endl;
			#endif

			Tree *result = make_token( sp, prg, parser, nargs );
			for ( long i = 0; i < nargs; i++ )
				tree_downref( prg, sp, pop() );
			push( result );
			break;
		}
		case IN_MAKE_TREE: {
			uchar nargs;
			read_byte( nargs );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAKE_TREE " << (ulong) nargs << endl;
			#endif

			Tree *result = make_tree( sp, prg, parser, nargs );
			for ( long i = 0; i < nargs; i++ )
				tree_downref( prg, sp, pop() );
			push( result );
			break;
		}
		case IN_SEND: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SEND" << endl;
			#endif

			Tree *tree = pop();
			send( sp, prg, parser, tree, false );
			push( 0 );
			break;
		}
		case IN_IGNORE: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_IGNORE" << endl;
			#endif

			Tree *tree = pop();
			send( sp, prg, parser, tree, true );
			push( 0 );
			break;
		}
		case IN_TREE_NEW: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TREE_NEW " << endl;
			#endif

			Tree *tree = pop();
			Tree *res = construct_pointer( prg, tree );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_PTR_DEREF_R: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PTR_DEREF_R" << endl;
			#endif

			Pointer *ptr = (Pointer*)pop();
			tree_downref( prg, sp, (Tree*)ptr );

			Tree *dval = get_ptr_val( ptr );
			tree_upref( dval );
			push( dval );
			break;
		}
		case IN_PTR_DEREF_WC: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PTR_DEREF_WC" << endl;
			#endif

			Pointer *ptr = (Pointer*)pop();
			tree_downref( prg, sp, (Tree*)ptr );

			Tree *dval = get_ptr_val_split( prg, ptr );
			tree_upref( dval );
			push( dval );
			break;
		}
		case IN_PTR_DEREF_WV: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PTR_DEREF_WV" << endl;
			#endif

			Pointer *ptr = (Pointer*)pop();
			/* Don't downref the pointer since it is going into the reverse
			 * instruction. */

			Tree *dval = get_ptr_val_split( prg, ptr );
			tree_upref( dval );
			push( dval );

			/* This is an initial global load. Need to reverse execute it. */
			reverseCode.append( IN_PTR_DEREF_BKT );
			reverseCode.appendWord( (Word) ptr );
			rcodeUnitLen = 5;
			break;
		}
		case IN_PTR_DEREF_BKT: {
			Word p;
			read_word( p );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_PTR_DEREF_BKT" << endl;
			#endif

			Pointer *ptr = (Pointer*)p;

			Tree *dval = get_ptr_val_split( prg, ptr );
			tree_upref( dval );
			push( dval );

			tree_downref( prg, sp, (Tree*)ptr );
			break;
		}
		case IN_REF_FROM_LOCAL: {
			short int field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_REF_FROM_LOCAL " << field << endl;
			#endif

			/* First push the null next pointer, then the kid pointer. */
			Tree **ptr = plocal(field);
			push( 0 );
			push( (SW)ptr );
			break;
		}
		case IN_REF_FROM_REF: {
			short int field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_REF_FROM_REF " << field << endl;
			#endif

			Ref *ref = (Ref*)plocal(field);
			push( (SW)ref );
			push( (SW)ref->kid );
			break;
		}
		case IN_TRITER_REF_FROM_CUR: {
			short int field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_TRITER_REF_FROM_CUR " << field << endl;
			#endif

			/* Push the next pointer first, then the kid. */
			TreeIter *iter = (TreeIter*) plocal(field);
			push( (SW)&iter->ref );
			push( (SW)iter->ref.kid );
			break;
		}
		case IN_UITER_REF_FROM_CUR: {
			short int field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_UITER_REF_FROM_CUR " << field << endl;
			#endif

			/* Push the next pointer first, then the kid. */
			UserIter *uiter = (UserIter*) local(field);
			push( (SW)uiter->ref.next );
			push( (SW)uiter->ref.kid );
			break;
		}
		case IN_GET_TOKEN_DATA_R: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_TOKEN_DATA_R" << endl;
			#endif

			Tree *tree = (Tree*) pop();
			Head *data = string_copy( prg, tree->tokdata );
			Tree *str = construct_string( prg, data );
			tree_upref( str );
			push( str );
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_SET_TOKEN_DATA_WC: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_TOKEN_DATA_WC" << endl;
			#endif

			Tree *tree = pop();
			Tree *val = pop();
			Head *head = string_copy( prg, ((Str*)val)->value );
			string_free( prg, tree->tokdata );
			tree->tokdata = head;

			tree_downref( prg, sp, tree );
			tree_downref( prg, sp, val );
			break;
		}
		case IN_SET_TOKEN_DATA_WV: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_TOKEN_DATA_WV" << endl;
			#endif

			Tree *tree = pop();
			Tree *val = pop();

			Head *oldval = tree->tokdata;
			Head *head = string_copy( prg, ((Str*)val)->value );
			tree->tokdata = head;

			/* Set up reverse code. Needs no args. */
			reverseCode.append( IN_SET_TOKEN_DATA_BKT );
			reverseCode.appendWord( (Word)oldval );
			rcodeUnitLen += 5;
			reverseCode.append( rcodeUnitLen );

			tree_downref( prg, sp, tree );
			tree_downref( prg, sp, val );
			break;
		}
		case IN_SET_TOKEN_DATA_BKT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_TOKEN_DATA_BKT " << endl;
			#endif

			Word oldval;
			read_word( oldval );

			Tree *tree = pop();
			Head *head = (Head*)oldval;
			string_free( prg, tree->tokdata );
			tree->tokdata = head;
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_GET_TOKEN_POS_R: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_TOKEN_POS_R" << endl;
			#endif

			Tree *tree = (Tree*) pop();
			Tree *integer = construct_integer( prg, 0 );
			tree_upref( integer );
			push( integer );
			tree_downref( prg, sp, tree );

			/* Requires a new implementation. */
			assert( false );
			break;
		}
		case IN_GET_MATCH_LENGTH_R: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_MATCH_LENGTH_R" << endl;
			#endif
			Tree *integer = construct_integer( prg, string_length(matchText) );
			tree_upref( integer );
			push( integer );
			break;
		}
		case IN_GET_MATCH_TEXT_R: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_MATCH_TEXT_R" << endl;
			#endif
			Head *s = string_copy( prg, matchText );
			Tree *tree = construct_string( prg, s );
			tree_upref( tree );
			push( tree );
			break;
		}
		case IN_LIST_LENGTH: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LIST_LENGTH" << endl;
			#endif

			List *list = (List*) pop();
			long len = list_length( list );
			Tree *res = construct_integer( prg, len );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_LIST_APPEND_WV: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LIST_APPEND_WV" << endl;
			#endif

			Tree *obj = pop();
			Tree *val = pop();

			tree_downref( prg, sp, obj );

			list_append( prg, (List*)obj, val );
			tree_upref( prg->trueVal );
			push( prg->trueVal );

			/* Set up reverse code. Needs no args. */
			reverseCode.append( IN_LIST_APPEND_BKT );
			rcodeUnitLen += 1;
			reverseCode.append( rcodeUnitLen );
			/* FLUSH */
			break;
		}
		case IN_LIST_APPEND_WC: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LIST_APPEND_WC" << endl;
			#endif

			Tree *obj = pop();
			Tree *val = pop();

			tree_downref( prg, sp, obj );

			list_append( prg, (List*)obj, val );
			tree_upref( prg->trueVal );
			push( prg->trueVal );
			break;
		}
		case IN_LIST_APPEND_BKT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LIST_APPEND_BKT" << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *tree = list_remove_end( prg, (List*)obj );
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_LIST_REMOVE_END_WC: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LIST_REMOVE_END_WC" << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *end = list_remove_end( prg, (List*)obj );
			push( end );
			break;
		}
		case IN_LIST_REMOVE_END_WV: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LIST_REMOVE_END_WV" << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *end = list_remove_end( prg, (List*)obj );
			push( end );

			/* Set up reverse. The result comes off the list downrefed.
			 * Need it up referenced for the reverse code too. */
			tree_upref( end );
			reverseCode.append( IN_LIST_REMOVE_END_BKT );
			reverseCode.appendWord( (Word)end );
			rcodeUnitLen += 5;
			reverseCode.append( rcodeUnitLen );
			/* FLUSH */
			break;
		}
		case IN_LIST_REMOVE_END_BKT: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_LIST_REMOVE_END_BKT" << endl;
			#endif

			Tree *val;
			read_tree( val );

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			list_append( prg, (List*)obj, val );
			break;
		}
		case IN_GET_LIST_MEM_R: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_LIST_MEM_R " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = get_list_mem( list(obj), field );
			tree_upref( val );
			push( val );
			break;
		}
		case IN_GET_LIST_MEM_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_LIST_MEM_WC " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = get_list_mem_split( prg, list(obj), field );
			tree_upref( val );
			push( val );
			break;
		}
		case IN_GET_LIST_MEM_WV: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_LIST_MEM_WV " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = get_list_mem_split( prg, list(obj), field );
			tree_upref( val );
			push( val );

			/* Set up the reverse instruction. */
			reverseCode.append( IN_GET_LIST_MEM_BKT );
			reverseCode.appendHalf( field );
			rcodeUnitLen += 3;
			break;
		}
		case IN_GET_LIST_MEM_BKT: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_LIST_MEM_BKT " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *res = get_list_mem_split( prg, list(obj), field );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_SET_LIST_MEM_WC: {
			Half field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_LIST_MEM_WC " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = pop();
			Tree *existing = set_list_mem( list(obj), field, val );
			tree_downref( prg, sp, existing );
			break;
		}
		case IN_SET_LIST_MEM_WV: {
			Half field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_LIST_MEM_WV " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = pop();
			Tree *existing = set_list_mem( list(obj), field, val );

			/* Set up the reverse instruction. */
			reverseCode.append( IN_SET_LIST_MEM_BKT );
			reverseCode.appendHalf( field );
			reverseCode.appendWord( (Word)existing );
			rcodeUnitLen += 7;
			reverseCode.append( rcodeUnitLen );
			/* FLUSH */
			break;
		}
		case IN_SET_LIST_MEM_BKT: {
			Half field;
			Tree *val;
			read_half( field );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_SET_LIST_MEM_BKT " << field << endl;
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *undid = set_list_mem( list(obj), field, val );
			tree_downref( prg, sp, undid );
			break;
		}
		case IN_MAP_INSERT_WV: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_INSERT_WV" << endl;
			#endif

			Tree *obj = pop();
			Tree *val = pop();
			Tree *key = pop();

			tree_downref( prg, sp, obj );

			bool inserted = map_insert( prg, (Map*)obj, key, val );
			Tree *result = inserted ? prg->trueVal : prg->falseVal;
			tree_upref( result );
			push( result );

			/* Set up the reverse instruction. If the insert fails still need
			 * to pop the loaded map object. Just use the reverse instruction
			 * since it's nice to see it in the logs. */

			/* Need to upref key for storage in reverse code. */
			tree_upref( key );
			reverseCode.append( IN_MAP_INSERT_BKT );
			reverseCode.append( inserted );
			reverseCode.appendWord( (Word)key );
			rcodeUnitLen += 6;
			reverseCode.append( rcodeUnitLen );

			if ( ! inserted ) {
				tree_downref( prg, sp, key );
				tree_downref( prg, sp, val );
			}
			break;
		}
		case IN_MAP_INSERT_WC: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_INSERT_WC" << endl;
			#endif

			Tree *obj = pop();
			Tree *val = pop();
			Tree *key = pop();

			tree_downref( prg, sp, obj );

			bool inserted = map_insert( prg, (Map*)obj, key, val );
			Tree *result = inserted ? prg->trueVal : prg->falseVal;
			tree_upref( result );
			push( result );

			if ( ! inserted ) {
				tree_downref( prg, sp, key );
				tree_downref( prg, sp, val );
			}
			break;
		}
		case IN_MAP_INSERT_BKT: {
			uchar inserted;
			Tree *key;
			read_byte( inserted );
			read_tree( key );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_INSERT_BKT" << endl;
			#endif
			
			Tree *obj = pop();
			if ( inserted ) {
				Tree *val = map_uninsert( prg, (Map*)obj, key );
				tree_downref( prg, sp, key );
				tree_downref( prg, sp, val );
			}

			tree_downref( prg, sp, obj );
			tree_downref( prg, sp, key );
			break;
		}
		case IN_MAP_STORE_WC: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_STORE_WC" << endl;
			#endif

			Tree *obj = pop();
			Tree *element = pop();
			Tree *key = pop();

			Tree *existing = map_store( prg, (Map*)obj, key, element );
			Tree *result = existing == 0 ? prg->trueVal : prg->falseVal;
			tree_upref( result );
			push( result );

			tree_downref( prg, sp, obj );
			if ( existing != 0 ) {
				tree_downref( prg, sp, key );
				tree_downref( prg, sp, existing );
			}
			break;
		}
		case IN_MAP_STORE_WV: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_STORE_WV" << endl;
			#endif

			Tree *obj = pop();
			Tree *element = pop();
			Tree *key = pop();

			Tree *existing = map_store( prg, (Map*)obj, key, element );
			Tree *result = existing == 0 ? prg->trueVal : prg->falseVal;
			tree_upref( result );
			push( result );

			/* Set up the reverse instruction. */
			tree_upref( key );
			tree_upref( existing );
			reverseCode.append( IN_MAP_STORE_BKT );
			reverseCode.appendWord( (Word)key );
			reverseCode.appendWord( (Word)existing );
			rcodeUnitLen += 9;
			reverseCode.append( rcodeUnitLen );
			/* FLUSH */

			tree_downref( prg, sp, obj );
			if ( existing != 0 ) {
				tree_downref( prg, sp, key );
				tree_downref( prg, sp, existing );
			}
			break;
		}
		case IN_MAP_STORE_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_STORE_BKT" << endl;
			#endif

			Tree *obj = pop();
			Tree *stored = map_unstore( prg, (Map*)obj, key, val );

			tree_downref( prg, sp, stored );
			if ( val == 0 )
				tree_downref( prg, sp, key );

			tree_downref( prg, sp, obj );
			tree_downref( prg, sp, key );
			break;
		}
		case IN_MAP_REMOVE_WC: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_REMOVE_WC" << endl;
			#endif

			Tree *obj = pop();
			Tree *key = pop();
			TreePair pair = map_remove( prg, (Map*)obj, key );

			push( pair.val );

			tree_downref( prg, sp, obj );
			tree_downref( prg, sp, key );
			tree_downref( prg, sp, pair.key );
			break;
		}
		case IN_MAP_REMOVE_WV: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_REMOVE_WV" << endl;
			#endif

			Tree *obj = pop();
			Tree *key = pop();
			TreePair pair = map_remove( prg, (Map*)obj, key );

			tree_upref( pair.val );
			push( pair.val );

			/* Reverse instruction. */
			reverseCode.append( IN_MAP_REMOVE_BKT );
			reverseCode.appendWord( (Word)pair.key );
			reverseCode.appendWord( (Word)pair.val );
			rcodeUnitLen += 9;
			reverseCode.append( rcodeUnitLen );

			tree_downref( prg, sp, obj );
			tree_downref( prg, sp, key );
			break;
		}
		case IN_MAP_REMOVE_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_REMOVE_BKT" << endl;
			#endif

			/* Either both or neither. */
			assert( ( key == 0 ) xor ( val != 0 ) );

			Tree *obj = pop();
			if ( key != 0 )
				map_unremove( prg, (Map*)obj, key, val );

			tree_downref( prg, sp, obj );
			break;
		}
		case IN_MAP_LENGTH: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_LENGTH" << endl;
			#endif

			Tree *obj = pop();
			long len = map_length( (Map*)obj );
			Tree *res = construct_integer( prg, len );
			tree_upref( res );
			push( res );

			tree_downref( prg, sp, obj );
			break;
		}
		case IN_MAP_FIND: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_MAP_FIND" << endl;
			#endif

			Tree *obj = pop();
			Tree *key = pop();
			Tree *result = map_find( (Map*)obj, key );
			tree_upref( result );
			push( result );

			tree_downref( prg, sp, obj );
			tree_downref( prg, sp, key );
			break;
		}
		case IN_INIT_LOCALS: {
			Half size;
			read_half( size );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_INIT_LOCALS " << size << endl;
			#endif

			frame = ptop();
			pushn( size );
			memset( ptop(), 0, sizeof(Word) * size );
			break;
		}
		case IN_POP_LOCALS: {
			Half frameId, size;
			read_half( frameId );
			read_half( size );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_POP_LOCALS " << frameId << " " << size << endl;
			#endif

			FrameInfo *fi = &prg->rtd->frameInfo[frameId];
			downref_local_trees( prg, sp, frame, fi->trees, fi->treesLen );
			popn( size );
			break;
		}
		case IN_CALL: {
			Half funcId;
			read_half( funcId );

			FunctionInfo *fi = &prg->rtd->functionInfo[funcId];

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_CALL " << fi->name << endl;
			#endif

			push( 0 ); /* Return value. */
			push( (SW)instr );
			push( (SW)frame );

			instr = prg->rtd->frameInfo[fi->frameId].code;
			frame = ptop();
			break;
		}
		case IN_YIELD: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_YIELD" << endl;
			#endif

			Kid *kid = (Kid*)pop();
			Ref *next = (Ref*)pop();
			UserIter *uiter = (UserIter*) plocal_iframe( IFR_AA );

			if ( kid == 0 || kid->tree == 0 ||
					kid->tree->id == uiter->searchId || 
					uiter->searchId == prg->rtd->anyId )
			{
				/* Store the yeilded value. */
				uiter->ref.kid = kid;
				uiter->ref.next = next;
				uiter->stackSize = uiter->stackRoot - ptop();
				uiter->resume = instr;
				uiter->frame = frame;

				/* Restore the instruction and frame pointer. */
				instr = (Code*) local_iframe(IFR_RIN);
				frame = (Tree**) local_iframe(IFR_RFR);
				iframe = (Tree**) local_iframe(IFR_RIF);

				/* Return the yield result on the top of the stack. */
				Tree *result = uiter->ref.kid != 0 ? prg->trueVal : prg->falseVal;
				tree_upref( result );
				push( result );
			}
			break;
		}
		case IN_UITER_CREATE: {
			short field;
			Half funcId, searchId;
			read_half( field );
			read_half( funcId );
			read_half( searchId );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_UITER_CREATE " << field << " " << 
					funcId << " " << searchId << endl;
			#endif

			FunctionInfo *fi = prg->rtd->functionInfo + funcId;
			UserIter *uiter = uiter_create( sp, prg, fi, searchId );
			local(field) = (SW) uiter;

			/* This is a setup similar to as a call, only the frame structure
			 * is slightly different for user iterators. We aren't going to do
			 * the call. We don't need to set up the return ip because the
			 * uiter advance will set it. The frame we need to do because it
			 * is set once for the lifetime of the iterator. */
			push( 0 );            /* Return instruction pointer,  */
			push( (SW)iframe ); /* Return iframe. */
			push( (SW)frame );  /* Return frame. */

			/* Now set up the first yeild. */
			uiter->ref.kid = 0;
			uiter->stackSize = uiter->stackRoot - ptop();
			uiter->resume = prg->rtd->frameInfo[fi->frameId].code;
			uiter->frame = &uiter->stackRoot[-IFR_AA];

			upref_uiter_args( frame, fi->argSize );
			break;
		}
		case IN_UITER_DESTROY: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_UITER_DESTROY " << field << endl;
			#endif

			UserIter *uiter = (UserIter*) local(field);
			user_iter_destroy( sp, uiter );
			break;
		}
		case IN_RET: {
			Half funcId;
			read_half( funcId );

			FunctionInfo *fui = &prg->rtd->functionInfo[funcId];

			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_RET " << fui->name << endl;
			#endif

			FrameInfo *fi = &prg->rtd->frameInfo[fui->frameId];
			downref_local_trees( prg, sp, frame, fi->trees, fi->treesLen );

			popn( fui->frameSize );
			frame = (Tree**) pop();
			instr = (Code*) pop();
			Tree *retVal = pop();
			popn( fui->argSize );
			push( retVal );
			break;
		}
		case IN_OPEN_FILE: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_OPEN_FILE" << endl;
			#endif

			Tree *name = pop();
			Tree *res = open_file( prg, name );
			tree_upref( res );
			push( res );
			tree_downref( prg, sp, name );
			break;
		}
		case IN_GET_STDIN: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_GET_STDIN" << endl;
			#endif

			/* Pop the root object. */
			tree_downref( prg, sp, pop() );
			if ( prg->stdinVal == 0 ) {
				prg->stdinVal = open_stream_fd( prg, 0 );
				tree_upref( (Tree*)prg->stdinVal );
			}

			tree_upref( (Tree*)prg->stdinVal );
			push( (Tree*)prg->stdinVal );
			break;
		}
		case IN_STOP: {
			#ifdef COLM_LOG_BYTECODE
			cerr << "IN_STOP" << endl;
			#endif

			cout.flush();
			return;
		}

		/* Halt is a default instruction given by the compiler when it is
		 * asked to generate and instruction it doesn't have. It is deliberate
		 * and can represent "not implemented" or "compiler error" because a
		 * variable holding instructions was not properly initialize. */
		case IN_HALT: {
			cerr << "IN_HALT -- compiler did something wrong" << endl;
			exit(1);
			break;
		}
		default: {
			cerr << "UNKNOWN INSTRUCTION: " << (ulong)instr[-1] << 
					" -- something is wrong" << endl;
			assert(false);
			break;
		}
	}
	goto again;
}
