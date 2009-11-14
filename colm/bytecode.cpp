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

#include <iostream>
#include <sstream>
#include <alloca.h>
#include <sys/mman.h>
#include <sstream>

#include "bytecode.h"
#include "pdarun.h"
#include "fsmrun.h"
#include "pdarun.h"

using std::cout;
using std::cerr;
using std::endl;
using std::ostringstream;
using std::string;

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

#define read_word_p( i, p ) do { \
	i = ((Word)  p[0]); \
	i |= ((Word) p[1]) << 8; \
	i |= ((Word) p[2]) << 16; \
	i |= ((Word) p[3]) << 24; \
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

int colm_log_bytecode = 0;
int colm_log_parse = 0;
int colm_log_match = 0;
int colm_log_compile = 0;
int colm_log_conds = 0;

Tree *prep_parse_tree( Program *prg, Tree **sp, Tree *tree )
{
	/* If the tree was produced by a parsing function then we need to send a
	 * copy of it because the parsing that we are about to do requires fresh
	 * parsing algorithm data. */
	if ( !(tree->flags & AF_PARSE_TREE) || tree->flags & AF_PARSED ) {
		#ifdef COLM_LOG_BYTECODE
		if ( colm_log_bytecode ) {
			cerr << "copying tree in send function" << endl;
		}
		#endif
		Kid *unused = 0;
		Tree *newTree = copy_real_tree( prg, tree, 0, unused, true );
		tree_upref( newTree );

		tree_downref( prg, sp, tree );

		tree = newTree;
	}
	return  tree;
}

void send_tree( Tree **root, Program *prg, PdaRun *pdaRun, Tree *tree, bool ignore )
{
	tree = prep_parse_tree( prg, root, tree );

	if ( tree->id >= prg->rtd->firstNonTermId )
		tree->id = prg->rtd->lelInfo[tree->id].termDupId;

	tree->flags |= AF_ARTIFICIAL;

	/* FIXME: Do we need to remove the ignore tokens 
	 * at this point? Will it cause a leak? */

	Kid *kid = prg->kidPool.allocate();
	kid->tree = tree;

	if ( pdaRun->queue == 0 )
		pdaRun->queue = pdaRun->queueLast = kid;
	else {
		pdaRun->queueLast->next = kid;
		pdaRun->queueLast = kid;
	}
}

struct ParserRet
{
	Tree *tree;
	FsmRun *fsmRun;
};

void call_parser( ParserRet &ret, Tree **&sp, Program *prg, Stream *stream, 
		long parserId, long stopId, CodeVect *&cv, bool revertOn )
{
	PdaTables *tables = prg->rtd->pdaTables;
	FsmRun *fsmRun = new FsmRun( prg );
	PdaRun pdaRun( prg, tables, fsmRun, parserId, stopId, revertOn );
	parse( sp, &pdaRun, fsmRun, stream->in );
	commit_full( sp, &pdaRun, 0 );
	Tree *tree = get_parsed_root( &pdaRun, stopId > 0 );
	tree_upref( tree );
	clean_parser( sp, &pdaRun );

	/* Maybe return the reverse code. */
	if ( revertOn )
		cv = pdaRun.allReverseCode;
	else {
		delete pdaRun.allReverseCode;
		cv = 0;
	}

	/* Indicate that this tree came out of a parser. */
	tree->flags |= AF_PARSED;

	ret.tree = tree;
	ret.fsmRun = fsmRun;
}

Head *tree_to_str( Tree **sp, Program *prg, Tree *tree )
{
	/* Collect the tree data. */
	ostringstream sout;
	print_tree( sout, sp, prg, tree );

	/* Set up the input stream. */
	string s = sout.str();
	return string_alloc_full( prg, s.c_str(), s.size() );
}


Tree *call_tree_parser( Tree **&sp, Program *prg, Tree *input, 
		long parserId, long stopId, CodeVect *&cv, bool revertOn )
{
	PdaTables *tables = prg->rtd->pdaTables;

	/* Collect the tree data. */
	ostringstream sout;
	print_tree( sout, sp, prg, input );

	/* Set up the input stream. */
	string s = sout.str();
	InputStreamString inputStream( s.c_str(), s.size() );
	init_input_stream( &inputStream );

	FsmRun fsmRun( prg );
	PdaRun pdaRun( prg, tables, &fsmRun, parserId, stopId, revertOn );
	parse( sp, &pdaRun, &fsmRun, &inputStream );
	commit_full( sp, &pdaRun, 0 );
	Tree *tree = get_parsed_root( &pdaRun, stopId > 0 );
	tree_upref( tree );
	clean_parser( sp, &pdaRun );

	/* Maybe return the reverse code. */
	if ( revertOn )
		cv = pdaRun.allReverseCode;
	else {
		delete pdaRun.allReverseCode;
		cv = 0;
	}

	/* Indicate that this tree came out of a parser. */
	tree->flags |= AF_PARSED;

	return tree;
}

void call_parser_frag( Tree **&sp, Program *prg, Tree *input, Accum *accum )
{
	if ( input->id == LEL_ID_STR ) {
		/* Collect the tree data. */
		ostringstream sout;
		print_tree( sout, sp, prg, input );

		/* Load it into the input. */
		string s = sout.str();
		accum->inputStream->append( s.c_str(), s.size() );

		/* Parse. */
		parse_frag( sp, accum->pdaRun, accum->fsmRun, accum->inputStream );
	}
	else {
		/* Cause a flush */
		accum->inputStream->flush = true;
		parse_frag( sp, accum->pdaRun, accum->fsmRun, accum->inputStream );

		tree_upref( input );
		send_tree( sp, prg, accum->pdaRun, input, false );
		send_queued_tokens( sp, accum->pdaRun, accum->fsmRun, accum->inputStream );
	}
}


Tree *parser_frag_finish( Tree **&sp, Program *prg, Accum *accum )
{
	parse_frag_finish( sp, accum->pdaRun, accum->fsmRun, accum->inputStream );

	commit_full( sp, accum->pdaRun, 0 );
	Tree *tree = get_parsed_root( accum->pdaRun, false );
	tree_upref( tree );
	clean_parser( sp, accum->pdaRun );

	delete accum->pdaRun->allReverseCode;

	/* Indicate that this tree came out of a parser. */
	tree->flags |= AF_PARSED;

	return tree;
}

void undo_parse( Tree **&sp, Program *prg, FsmRun *fsmRun, Stream *stream, 
		long parserId, Tree *tree, CodeVect *rev )
{
	PdaTables *tables = prg->rtd->pdaTables;
	PdaRun pdaRun( prg, tables, fsmRun, parserId, 0, false );
	undo_parse( sp, stream->in, fsmRun, &pdaRun, tree, rev );
}

Tree *stream_pull( Program *prg, FsmRun *fsmRun, Stream *stream, Tree *length )
{
	long len = ((Int*)length)->value;
	Head *tokdata = extract_prefix( prg, fsmRun, stream->in, len );
	return construct_string( prg, tokdata );
}


void undo_pull( Program *prg, FsmRun *fsmRun, Stream *stream, Tree *str )
{
	const char *data = string_data( ( (Str*)str )->value );
	long length = string_length( ( (Str*)str )->value );
//	send_back_text( fsmRun, stream->in, data, length );
	undo_stream_pull( fsmRun, stream->in, data, length );
}

Word stream_push( Tree **&sp, Program *prg, FsmRun *fsmRun, Stream *stream, Tree *any )
{
	std::stringstream ss;
	print_tree( ss, sp, prg, any );
	stream_push( fsmRun, stream->in, ss.str().c_str(), ss.str().size());
	return ss.str().size();
}

void undo_stream_push( Tree **&sp, Program *prg, FsmRun *fsmRun, Stream *stream, Word len )
{
	undo_stream_push( fsmRun, stream->in, len );
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

void downref_local_trees( Program *prg, Tree **sp, Tree **frame, char *trees, long treesLen )
{
	for ( long i = 0; i < treesLen; i++ ) {
		#ifdef COLM_LOG_BYTECODE
		if ( colm_log_bytecode ) {
			cerr << "local tree downref: " << (long)trees[i] << endl;
		}
		#endif

		tree_downref( prg, sp, local((long)trees[i]) );
	}
}

UserIter *uiter_create( Tree **&sp, Program *prg, FunctionInfo *fi, long searchId )
{
	pushn( sizeof(UserIter) / sizeof(Word) );
	void *mem = ptop();

	UserIter *uiter = new(mem) UserIter( ptop(), fi->argSize, searchId );
	return uiter;
}

void uiter_init( Program *prg, Tree **sp, UserIter *uiter, 
		FunctionInfo *fi, bool revertOn )
{
	/* Set up the first yeild so when we resume it starts at the beginning. */
	uiter->ref.kid = 0;
	uiter->stackSize = uiter->stackRoot - ptop();
	uiter->frame = &uiter->stackRoot[-IFR_AA];

	if ( revertOn )
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWV;
	else
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWC;
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

Tree *construct_argv( Program *prg, int argc, char **argv )
{
	Tree *list = create_generic( prg, 1 );
	tree_upref( list );
	for ( int i = 0; i < argc; i++ ) {
		Head *head = string_alloc_pointer( prg, argv[i], strlen(argv[i]) );
		Tree *arg = construct_string( prg, head );
		tree_upref( arg );
		list_append( prg, (List*)list, arg );
	}
	return list;
}


/*
 * Execution environment
 */

Program::Program( int argc, char **argv, bool ctxDepParsing, RuntimeData *rtd )
:
	argc(argc), argv(argv),
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

void Program::clear( Tree **vm_stack, Tree **sp )
{
	#ifdef COLM_LOG_BYTECODE
	if ( colm_log_bytecode ) {
		cerr << "clearing the prg" << endl;
	}
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
		cerr << "warning: lost kids: " << kidLost << endl;

	long treeLost = treePool.numlost();
	if ( treeLost )
		cerr << "warning: lost trees: " << treeLost << endl;

	long parseTreeLost = parseTreePool.numlost();
	if ( parseTreeLost )
		cerr << "warning: lost parse trees: " << parseTreeLost << endl;

	long listLost = listElPool.numlost();
	if ( listLost )
		cerr << "warning: lost listEls: " << listLost << endl;

	long mapLost = mapElPool.numlost();
	if ( mapLost )
		cerr << "warning: lost mapEls: " << mapLost << endl;

	long headLost = headPool.numlost();
	if ( headLost )
		cerr << "warning: lost heads: " << headLost << endl;

	long locationLost = locationPool.numlost();
	if ( locationLost )
		cerr << "warning: lost locations: " << locationLost << endl;

	kidPool.clear();
	treePool.clear();
	parseTreePool.clear();
	listElPool.clear();
	mapElPool.clear();
	locationPool.clear();

	//reverseCode.empty();
	//memset( vm_stack, 0, sizeof(Tree*) * VM_STACK_SIZE);
}

void Program::allocGlobal()
{
	/* Alloc the global. */
	Tree *tree = treePool.allocate();
	tree->child = alloc_attrs( this, rtd->globalSize );
	tree->refs = 1;
	global = tree;
}

Tree **stack_alloc()
{
	//return new Tree*[VM_STACK_SIZE];

	return (Tree**)mmap( 0, sizeof(Tree*)*VM_STACK_SIZE,
		PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0 );
}

void Program::run()
{
	assert( sizeof(Int)      <= sizeof(Tree) );
	assert( sizeof(Str)      <= sizeof(Tree) );
	assert( sizeof(Pointer)  <= sizeof(Tree) );
	assert( sizeof(Map)      <= sizeof(MapEl) );
	assert( sizeof(List)     <= sizeof(MapEl) );
	assert( sizeof(Stream)   <= sizeof(MapEl) );
	assert( sizeof(Accum)    <= sizeof(MapEl) );

	/* Allocate the global variable. */
	allocGlobal();

	/* 
	 * Allocate the VM stack.
	 */

	Tree **vm_stack = stack_alloc();
	Tree **root = &vm_stack[VM_STACK_SIZE];

	/*
	 * Execute
	 */

	if ( rtd->rootCodeLen > 0 ) {
		CodeVect reverseCode;
		Execution execution( this, reverseCode, 0, 0, rtd->rootCode, 0, 0, 0, 0 );
		execution.execute( root );

		/* Pull out the reverse code and free it. */
		#ifdef COLM_LOG_BYTECODE
		if ( colm_log_bytecode ) {
			cerr << "freeing the root reverse code" << endl;
		}
		#endif

		/* The root code should all be commit code and reverseCode
		 * should be empty. */
		assert( reverseCode.length() == 0 );
	}

	/* Clear */
	clear( vm_stack, root );
}

Execution::Execution( Program *prg, CodeVect &reverseCode,
		PdaRun *pdaRun, FsmRun *fsmRun, Code *code, Tree *lhs,
		long genId, Head *matchText, char **captures )
: 
	prg(prg), 
	pdaRun(pdaRun), 
	fsmRun(fsmRun), 
	code(code), 
	frame(0), iframe(0),
	lhs(lhs),
	parsed(0),
	genId(genId),
	matchText(matchText),
	reject(false), 
	reverseCode(reverseCode),
	rcodeUnitLen(0),
	captures(captures)
{
	if ( lhs != 0 ) {
		assert( lhs->refs == 1 );
	}
}

void rcode_downref_all( Program *prg, Tree **sp, CodeVect *rev )
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
		rcode_downref( prg, sp, prcode );

		/* Backup over it. */
		rev->tabLen -= len + 4;
	}
}

void rcode_downref( Program *prg, Tree **sp, Code *instr )
{
again:
	switch ( *instr++ ) {
		case IN_RESTORE_LHS: {
			Tree *lhs;
			read_tree( lhs );
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_RESTORE_LHS" << endl;
			}
			#endif
			tree_downref( prg, sp, lhs );
			break;
		}
		case IN_PARSE_BKT: {
			Half parserId;
			Tree *stream, *tree;
			Word fsmRet, wrev;
			read_half( parserId );
			read_word( fsmRet );
			read_tree( stream );
			read_tree( tree );
			read_word( wrev );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_BKT " << parserId << endl;
			}
			#endif

			rcode_downref_all( prg, sp, (CodeVect*)wrev );
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
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PULL_BKT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PUSH_BKT" << endl;
			}
			#endif

			tree_downref( prg, sp, stream );
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_GLOBAL_BKT" << endl;
			}
			#endif
			break;
		}
		case IN_GET_FIELD_BKT: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_FIELD_BKT " << field << endl;
			}
			#endif
			break;
		}
		case IN_SET_FIELD_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_FIELD_BKT " << field << endl;
			}
			#endif

			tree_downref( prg, sp, val );
			break;
		}
		case IN_PTR_DEREF_BKT: {
			Tree *ptr;
			read_tree( ptr );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PTR_DEREF_BKT" << endl;
			}
			#endif

			tree_downref( prg, sp, ptr );
			break;
		}
		case IN_SET_TOKEN_DATA_BKT: {
			Word oldval;
			read_word( oldval );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_TOKEN_DATA_BKT " << endl;
			}
			#endif

			Head *head = (Head*)oldval;
			string_free( prg, head );
			break;
		}
		case IN_LIST_APPEND_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_APPEND_BKT" << endl;
			}
			#endif
			break;
		}
		case IN_LIST_REMOVE_END_BKT: {
			Tree *val;
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_REMOVE_END_BKT" << endl;
			}
			#endif

			tree_downref( prg, sp, val );
			break;
		}
		case IN_GET_LIST_MEM_BKT: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_LIST_MEM_BKT " << field << endl;
			}
			#endif
			break;
		}
		case IN_SET_LIST_MEM_BKT: {
			Half field;
			Tree *val;
			read_half( field );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_LIST_MEM_BKT " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_INSERT_BKT" << endl;
			}
			#endif
			
			tree_downref( prg, sp, key );
			break;
		}
		case IN_MAP_STORE_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_STORE_BKT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_REMOVE_BKT" << endl;
			}
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

bool make_reverse_code( CodeVect *all, CodeVect &reverseCode )
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
		case IN_SAVE_LHS: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SAVE_LHS" << endl;
			}
			#endif

			assert( lhs != 0 );

			/* Save and upref before writing. We don't generate a restore
			 * here. Instead in the parser we will check if it actually
			 * changed and insert the instruction then. The presence of this
			 * instruction here is just a conservative approximation.  */
			parsed = lhs;
			tree_upref( parsed );
			break;
		}
		case IN_RESTORE_LHS: {
			Tree *restore;
			read_tree( restore );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_RESTORE_LHS" << endl;
			}
			#endif
			assert( lhs == 0 );
			lhs = restore;
			break;
		}
		case IN_LOAD_NIL: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_NIL" << endl;
			}
			#endif

			push( 0 );
			break;
		}
		case IN_LOAD_TRUE: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_TRUE" << endl;
			}
			#endif

			tree_upref( prg->trueVal );
			push( prg->trueVal );
			break;
		}
		case IN_LOAD_FALSE: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_FALSE" << endl;
			}
			#endif

			tree_upref( prg->falseVal );
			push( prg->falseVal );
			break;
		}
		case IN_LOAD_INT: {
			Word i;
			read_word( i );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_INT " << i << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_STR " << offset << endl;
			}
			#endif

			Head *lit = make_literal( prg, offset );
			Tree *tree = construct_string( prg, lit );
			tree_upref( tree );
			push( tree );
			break;
		}
		case IN_PRINT: {
			int n;
			read_byte( n );
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PRINT " << n << endl;
			}
			#endif

			while ( n-- > 0 ) {
				Tree *tree = pop();
				print_tree( cout, sp, prg, tree );
				tree_downref( prg, sp, tree );
			}
			break;
		}
		case IN_PRINT_XML_AC: {
			int n;
			read_byte( n );
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PRINT_XML_AC" << n << endl;
			}
			#endif

			while ( n-- > 0 ) {
				Tree *tree = pop();
				print_xml_tree( sp, prg, tree, true );
				tree_downref( prg, sp, tree );
			}
			break;
		}
		case IN_PRINT_XML: {
			int n;
			read_byte( n );
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PRINT_XML" << n << endl;
			}
			#endif

			while ( n-- > 0 ) {
				Tree *tree = pop();
				print_xml_tree( sp, prg, tree, false );
				tree_downref( prg, sp, tree );
			}
			break;
		}
		case IN_PRINT_STREAM: {
			int n;
			read_byte( n );
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PRINT_STREAM" << n << endl;
			}
			#endif

			Stream *stream = (Stream*)pop();
			while ( n-- > 0 ) {
				Tree *tree = pop();
				print_tree2( stream->file, sp, prg, tree );
				tree_downref( prg, sp, tree );
			}
			tree_downref( prg, sp, (Tree*)stream );
			break;
		}
		case IN_LOAD_GLOBAL_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_GLOBAL_R" << endl;
			}
			#endif

			tree_upref( prg->global );
			push( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_GLOBAL_WV" << endl;
			}
			#endif

			tree_upref( prg->global );
			push( prg->global );

			/* Set up the reverse instruction. */
			reverseCode.append( IN_LOAD_GLOBAL_BKT );
			rcodeUnitLen = 1;
			break;
		}
		case IN_LOAD_GLOBAL_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_GLOBAL_WC" << endl;
			}
			#endif

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			tree_upref( prg->global );
			push( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_GLOBAL_BKT" << endl;
			}
			#endif

			tree_upref( prg->global );
			push( prg->global );
			break;
		}
		case IN_INIT_CAPTURES: {
			uchar ncaps;
			read_byte( ncaps );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_INIT_CAPTURES " << ncaps << endl;
			}
			#endif

			/* If there are captures (this is a translate block) then copy them into
			 * the local frame now. */
			LangElInfo *lelInfo = prg->rtd->lelInfo;
			char **mark = captures;

			for ( int i = 0; i < lelInfo[genId].numCaptureAttr; i++ ) {
				CaptureAttr *ca = &prg->rtd->captureAttr[lelInfo[genId].captureAttr + i];
				Head *data = string_alloc_full( prg, 
						mark[ca->mark_enter], mark[ca->mark_leave] - mark[ca->mark_enter] );
				Tree *string = construct_string( prg, data );
				tree_upref( string );
				set_local( frame, -1 - i, string );
			}
			break;
		}
		case IN_INIT_RHS_EL: {
			Half position;
			short field;
			read_half( position );
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_INIT_RHS_EL " << position << " " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_UITER_ADVANCE " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_UITER_GET_CUR_R " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_UITER_GET_CUR_WC " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_UITER_SET_CUR_WC " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_LOCAL_R " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_LOCAL_WC " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_LOCAL_WC " << field << endl;
			}
			#endif

			Tree *val = pop();
			tree_downref( prg, sp, local(field) );
			set_local( frame, field, val );
			break;
		}
		case IN_SAVE_RET: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SAVE_RET " << endl;
			}
			#endif

			Tree *val = pop();
			local(FR_RV) = val;
			break;
		}
		case IN_GET_LOCAL_REF_R: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_LOCAL_REF_R " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_LOCAL_REF_WC " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_LOCAL_REF_WC " << field << endl;
			}
			#endif

			Tree *val = pop();
			Ref *ref = (Ref*) plocal(field);
			split_ref( sp, prg, ref );
			ref_set_value( ref, val );
			break;
		}
		case IN_GET_FIELD_R: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_FIELD_R " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_FIELD_WC " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_FIELD_WV " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_FIELD_BKT " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_FIELD_WC " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_FIELD_WV " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_FIELD_BKT " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_FIELD_LEAVE_WC " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_POP" << endl;
			}
			#endif

			Tree *val = pop();
			tree_downref( prg, sp, val );
			break;
		}
		case IN_POP_N_WORDS: {
			short n;
			read_half( n );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_POP_N_WORDS " << n << endl;
			}
			#endif

			popn( n );
			break;
		}
		case IN_STR_ATOI: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STR_ATOI" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_INT_TO_STR" << endl;
			}
			#endif

			Int *i = (Int*)pop();
			Head *res = int_to_str( prg, i->value );
			Tree *str = construct_string( prg, res );
			tree_upref( str );
			push( str );
			tree_downref( prg, sp, (Tree*) i );
			break;
		}
		case IN_TREE_TO_STR: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TREE_TO_STR" << endl;
			}
			#endif

			Tree *tree = pop();
			Head *res = tree_to_str( sp, prg, tree );
			Tree *str = construct_string( prg, res );
			tree_upref( str );
			push( str );
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_CONCAT_STR: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_CONCAT_STR" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_STR_UORD8" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_STR_UORD16" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_STR_LENGTH" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_JMP_FALSE " << dist << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_JMP_TRUE " << dist << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_JMP " << dist << endl;
			}
			#endif

			instr += dist;
			break;
		}
		case IN_REJECT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_REJECT" << endl;
			}
			#endif
			reject = true;
			break;
		}

		/*
		 * Binary comparison operators.
		 */
		case IN_TST_EQL: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TST_EQL" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TST_NOT_EQL" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TST_LESS" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TST_LESS_EQL" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TST_GRTR" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TST_GRTR_EQL" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TST_LOGICAL_AND" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TST_LOGICAL_OR" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_NOT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_ADD_INT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MULT_INT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SUB_INT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_DUP_TOP" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TRITER_FROM_REF " << field << " " << searchTypeId << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TRITER_DESTROY " << field << endl;
			}
			#endif

			TreeIter *iter = (TreeIter*) plocal(field);
			tree_iter_destroy( sp, iter );
			break;
		}
		case IN_REV_TRITER_FROM_REF: {
			short field;
			Half searchTypeId;
			read_half( field );
			read_half( searchTypeId );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_REV_TRITER_FROM_REF " << field << " " << searchTypeId << endl;
			}
			#endif

			Ref rootRef;
			rootRef.kid = (Kid*)pop();
			rootRef.next = (Ref*)pop();

			Tree **stackRoot = ptop();

			int children = 0;
			Kid *kid = tree_child( prg, rootRef.kid->tree );
			while ( kid != 0 ) {
				children++;
				push( (SW) kid );
				kid = kid->next;
			}

			void *mem = plocal(field);
			new(mem) RevTreeIter( rootRef, searchTypeId, stackRoot, children );

			break;
		}
		case IN_REV_TRITER_DESTROY: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_REV_TRITER_DESTROY " << field << endl;
			}
			#endif

			RevTreeIter *iter = (RevTreeIter*) plocal(field);
			long curStackSize = iter->stackRoot - ptop();
			assert( iter->stackSize == curStackSize );
			popn( iter->stackSize );
			break;
		}
		case IN_TREE_SEARCH: {
			Word id;
			read_word( id );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TREE_SEARCH " << id << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TRITER_ADVANCE " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TRITER_NEXT_CHILD " << field << endl;
			}
			#endif

			TreeIter *iter = (TreeIter*) plocal(field);
			Tree *res = tree_iter_next_child( prg, sp, iter );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_REV_TRITER_PREV_CHILD: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_REV_TRITER_PREV_CHILD " << field << endl;
			}
			#endif

			RevTreeIter *iter = (RevTreeIter*) plocal(field);
			Tree *res = tree_rev_iter_prev_child( prg, sp, iter );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_TRITER_NEXT_REPEAT: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode )
				cerr << "IN_TRITER_NEXT_REPEAT " << field << endl;
			#endif

			TreeIter *iter = (TreeIter*) plocal(field);
			Tree *res = tree_iter_next_repeat( prg, sp, iter );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_TRITER_PREV_REPEAT: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode )
				cerr << "IN_TRITER_PREV_REPEAT " << field << endl;
			#endif

			TreeIter *iter = (TreeIter*) plocal(field);
			Tree *res = tree_iter_prev_repeat( prg, sp, iter );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_TRITER_GET_CUR_R: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TRITER_GET_CUR_R " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TRITER_GET_CUR_WC " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_TRITER_SET_CUR_WC " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MATCH " << patternId << endl;
			}
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
			if ( colm_log_match ) {
				cerr << "match result: " << matched << endl;
			}
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
		case IN_PARSE_WV: {
			Half parserId, stopId;
			read_half( parserId );
			read_half( stopId );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_WV " << parserId << " " << stopId << endl;
			}
			#endif

			/* Comes back from parse upreffed. */
			CodeVect *cv;
			Tree *stream = pop();
			ParserRet ret;
			call_parser( ret, sp, prg, (Stream*)stream, parserId, stopId, cv, true );
			push( ret.tree );

			/* Single unit. */
			tree_upref( ret.tree );
			reverseCode.append( IN_PARSE_BKT );
			reverseCode.appendHalf( parserId );
			reverseCode.appendWord( (Word) ret.fsmRun );
			reverseCode.appendWord( (Word) stream );
			reverseCode.appendWord( (Word) ret.tree );
			reverseCode.appendWord( (Word) cv );
			reverseCode.append( 19 );
			break;
		}
		case IN_PARSE_WC: {
			Half parserId, stopId;
			read_half( parserId );
			read_half( stopId );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_WC " << parserId << " " << stopId << endl;
			}
			#endif

			/* Comes back from parse upreffed. */
			CodeVect *cv;
			Tree *stream = pop();
			ParserRet ret;
			call_parser( ret, sp, prg, (Stream*)stream, parserId, stopId, cv, false );
			push( ret.tree );

			tree_downref( prg, sp, (Tree*)stream );
			break;
		}
		case IN_PARSE_TREE_WC: {
			Half parserId, stopId;
			read_half( parserId );
			read_half( stopId );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_TREE_WC " << parserId << " " << stopId << endl;
			}
			#endif

			/* Comes back from parse upreffed. */
			CodeVect *cv;
			Tree *input = pop();
			Tree *res = call_tree_parser( sp, prg, input, parserId, stopId, cv, false );
			push( res );

			tree_downref( prg, sp, input );
			break;
		}
		case IN_PARSE_FRAG_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_FRAG_WC " << endl;
			}
			#endif

			Tree *input = pop();
			Tree *accum = pop();
			call_parser_frag( sp, prg, input, (Accum*)accum );
			tree_downref( prg, sp, input );
			tree_downref( prg, sp, accum );
			break;
		}
		case IN_ACCUM_FINISH_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_ACCUM_FINISH_WC " << endl;
			}
			#endif

			Tree *accum = pop();
			Tree *result = parser_frag_finish( sp, prg, (Accum*)accum );
			push( result );
			tree_downref( prg, sp, accum );
			break;
		}
		case IN_STREAM_PULL: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PULL" << endl;
			}
			#endif
			Tree *len = pop();
			Tree *stream = pop();
			Tree *string = stream_pull( prg, fsmRun, (Stream*)stream, len );
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
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PULL_BKT" << endl;
			}
			#endif

			undo_pull( prg, fsmRun, (Stream*)stream, string );
			tree_downref( prg, sp, stream );
			tree_downref( prg, sp, string );
			break;
		}
		case IN_STREAM_PUSH: {
			/* FIXME: Need to check the refcounting here. */

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PUSH" << endl;
			}
			#endif
			Tree *tree = pop();
			Tree *stream = pop();
			Word len = stream_push( sp, prg, 
					fsmRun, (Stream*)stream, tree );
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
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PUSH_BKT" << endl;
			}
			#endif

			undo_stream_push( sp, prg, fsmRun, (Stream*)stream, len );
			tree_downref( prg, sp, stream );
			break;
		}
		case IN_PARSE_BKT: {
			Half parserId;
			Tree *stream, *tree;
			Word fsmRet, wrev;
			read_half( parserId );
			read_word( fsmRet );
			read_tree( stream );
			read_tree( tree );
			read_word( wrev );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_BKT " << parserId << endl;
			}
			#endif

			undo_parse( sp, prg, (FsmRun*)fsmRet, (Stream*)stream, parserId, tree, (CodeVect*)wrev );
			tree_downref( prg, sp, stream );
			delete (CodeVect*)wrev;
			break;
		}
		case IN_CONSTRUCT: {
			Half patternId;
			read_half( patternId );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_CONSTRUCT " << patternId << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_CONSTRUCT_TERM " << tokenId << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAKE_TOKEN " << (ulong) nargs << endl;
			}
			#endif

			Tree *result = make_token( sp, prg, nargs );
			for ( long i = 0; i < nargs; i++ )
				tree_downref( prg, sp, pop() );
			push( result );
			break;
		}
		case IN_MAKE_TREE: {
			uchar nargs;
			read_byte( nargs );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_MAKE_TREE " << (ulong) nargs << endl;
			}
			#endif

			Tree *result = make_tree( sp, prg, nargs );
			for ( long i = 0; i < nargs; i++ )
				tree_downref( prg, sp, pop() );
			push( result );
			break;
		}
		case IN_SEND: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SEND" << endl;
			}
			#endif

			Tree *tree = pop();
			send_tree( sp, prg, pdaRun, tree, false );
			push( 0 );
			break;
		}
		case IN_IGNORE: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_IGNORE" << endl;
			}
			#endif

			Tree *tree = pop();
			send_tree( sp, prg, pdaRun, tree, true );
			push( 0 );
			break;
		}
		case IN_TREE_NEW: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TREE_NEW " << endl;
			}
			#endif

			Tree *tree = pop();
			Tree *res = construct_pointer( prg, tree );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_PTR_DEREF_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PTR_DEREF_R" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_PTR_DEREF_WC" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_PTR_DEREF_WV" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_PTR_DEREF_BKT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_REF_FROM_LOCAL " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_REF_FROM_REF " << field << endl;
			}
			#endif

			Ref *ref = (Ref*)plocal(field);
			push( (SW)ref );
			push( (SW)ref->kid );
			break;
		}
		case IN_REF_FROM_QUAL_REF: {
			short int back;
			short int field;
			read_half( back );
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_REF_FROM_QUAL_REF " << back << " " << field << endl;
			}
			#endif

			Ref *ref = (Ref*)(sp + back);

			Tree *obj = ref->kid->tree;
			Kid *attr_kid = get_field_kid( obj, field );

			push( (SW)ref );
			push( (SW)attr_kid );
			break;
		}
		case IN_TRITER_REF_FROM_CUR: {
			short int field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TRITER_REF_FROM_CUR " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_UITER_REF_FROM_CUR " << field << endl;
			}
			#endif

			/* Push the next pointer first, then the kid. */
			UserIter *uiter = (UserIter*) local(field);
			push( (SW)uiter->ref.next );
			push( (SW)uiter->ref.kid );
			break;
		}
		case IN_GET_TOKEN_DATA_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_TOKEN_DATA_R" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_TOKEN_DATA_WC" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_TOKEN_DATA_WV" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_TOKEN_DATA_BKT " << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_TOKEN_POS_R" << endl;
			}
			#endif

			Tree *tree = (Tree*) pop();
			Tree *integer = 0;
			if ( tree->tokdata->location ) {
				integer = construct_integer( prg, tree->tokdata->location->byte );
				tree_upref( integer );
			}
			push( integer );
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_GET_MATCH_LENGTH_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_MATCH_LENGTH_R" << endl;
			}
			#endif
			Tree *integer = construct_integer( prg, string_length(matchText) );
			tree_upref( integer );
			push( integer );
			break;
		}
		case IN_GET_MATCH_TEXT_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_MATCH_TEXT_R" << endl;
			}
			#endif
			Head *s = string_copy( prg, matchText );
			Tree *tree = construct_string( prg, s );
			tree_upref( tree );
			push( tree );
			break;
		}
		case IN_LIST_LENGTH: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_LENGTH" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_APPEND_WV" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_APPEND_WC" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_APPEND_BKT" << endl;
			}
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *tree = list_remove_end( prg, (List*)obj );
			tree_downref( prg, sp, tree );
			break;
		}
		case IN_LIST_REMOVE_END_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_REMOVE_END_WC" << endl;
			}
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *end = list_remove_end( prg, (List*)obj );
			push( end );
			break;
		}
		case IN_LIST_REMOVE_END_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_REMOVE_END_WV" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_REMOVE_END_BKT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_LIST_MEM_R " << field << endl;
			}
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = get_list_mem( (List*)obj, field );
			tree_upref( val );
			push( val );
			break;
		}
		case IN_GET_LIST_MEM_WC: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_LIST_MEM_WC " << field << endl;
			}
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = get_list_mem_split( prg, (List*)obj, field );
			tree_upref( val );
			push( val );
			break;
		}
		case IN_GET_LIST_MEM_WV: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_LIST_MEM_WV " << field << endl;
			}
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = get_list_mem_split( prg, (List*)obj, field );
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
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_LIST_MEM_BKT " << field << endl;
			}
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *res = get_list_mem_split( prg, (List*)obj, field );
			tree_upref( res );
			push( res );
			break;
		}
		case IN_SET_LIST_MEM_WC: {
			Half field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_LIST_MEM_WC " << field << endl;
			}
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = pop();
			Tree *existing = set_list_mem( (List*)obj, field, val );
			tree_downref( prg, sp, existing );
			break;
		}
		case IN_SET_LIST_MEM_WV: {
			Half field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_LIST_MEM_WV " << field << endl;
			}
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *val = pop();
			Tree *existing = set_list_mem( (List*)obj, field, val );

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
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_LIST_MEM_BKT " << field << endl;
			}
			#endif

			Tree *obj = pop();
			tree_downref( prg, sp, obj );

			Tree *undid = set_list_mem( (List*)obj, field, val );
			tree_downref( prg, sp, undid );
			break;
		}
		case IN_MAP_INSERT_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_INSERT_WV" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_INSERT_WC" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_INSERT_BKT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_STORE_WC" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_STORE_WV" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_STORE_BKT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_REMOVE_WC" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_REMOVE_WV" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_REMOVE_BKT" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_LENGTH" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_FIND" << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_INIT_LOCALS " << size << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_POP_LOCALS " << frameId << " " << size << endl;
			}
			#endif

			FrameInfo *fi = &prg->rtd->frameInfo[frameId];
			downref_local_trees( prg, sp, frame, fi->trees, fi->treesLen );
			popn( size );
			break;
		}
		case IN_CALL_WV: {
			Half funcId;
			read_half( funcId );

			FunctionInfo *fi = &prg->rtd->functionInfo[funcId];

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_CALL_WV " << fi->name << endl;
			}
			#endif

			push( 0 ); /* Return value. */
			push( (SW)instr );
			push( (SW)frame );

			instr = prg->rtd->frameInfo[fi->frameId].codeWV;
			frame = ptop();
			break;
		}
		case IN_CALL_WC: {
			Half funcId;
			read_half( funcId );

			FunctionInfo *fi = &prg->rtd->functionInfo[funcId];

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_CALL_WC " << fi->name << endl;
			}
			#endif

			push( 0 ); /* Return value. */
			push( (SW)instr );
			push( (SW)frame );

			instr = prg->rtd->frameInfo[fi->frameId].codeWC;
			frame = ptop();
			break;
		}
		case IN_YIELD: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_YIELD" << endl;
			}
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
		case IN_UITER_CREATE_WV: {
			short field;
			Half funcId, searchId;
			read_half( field );
			read_half( funcId );
			read_half( searchId );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_UITER_CREATE_WV " << field << " " << 
						funcId << " " << searchId << endl;
			}
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

			uiter_init( prg, sp, uiter, fi, true );
			break;
		}
		case IN_UITER_CREATE_WC: {
			short field;
			Half funcId, searchId;
			read_half( field );
			read_half( funcId );
			read_half( searchId );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_UITER_CREATE_WC " << field << " " << 
						funcId << " " << searchId << endl;
			}
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

			uiter_init( prg, sp, uiter, fi, false );
			break;
		}
		case IN_UITER_DESTROY: {
			short field;
			read_half( field );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_UITER_DESTROY " << field << endl;
			}
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
			if ( colm_log_bytecode ) {
				cerr << "IN_RET " << fui->name << endl;
			}
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
		case IN_TO_UPPER: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TO_UPPER" << endl;
			}
			#endif
			Tree *in = pop();
			Head *head = string_toupper( in->tokdata );
			Tree *upper = construct_string( prg, head );
			tree_upref( upper );
			push( upper );
			tree_downref( prg, sp, in );
			break;
		}
		case IN_TO_LOWER: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TO_LOWER" << endl;
			}
			#endif
			Tree *in = pop();
			Head *head = string_tolower( in->tokdata );
			Tree *lower = construct_string( prg, head );
			tree_upref( lower );
			push( lower );
			tree_downref( prg, sp, in );
			break;
		}
		case IN_EXIT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_EXIT" << endl;
			}
			#endif

			Int *status = (Int*)pop();
			exit( status->value );
			break;
		}
		case IN_OPEN_FILE: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_OPEN_FILE" << endl;
			}
			#endif

			Tree *mode = pop();
			Tree *name = pop();
			Tree *res = (Tree*)open_file( prg, name, mode );
			tree_upref( res );
			push( res );
			tree_downref( prg, sp, name );
			tree_downref( prg, sp, mode );
			break;
		}
		case IN_GET_STDIN: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_STDIN" << endl;
			}
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
		case IN_LOAD_ARGV: {
			Half field;
			read_half( field );
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_ARGV " << field << endl;
			}
			#endif

			/* Tree comes back upreffed. */
			Tree *tree = construct_argv( prg, prg->argc, prg->argv );
			set_field( prg, prg->global, field, tree );
			break;
		}
		case IN_STOP: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STOP" << endl;
			}
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
