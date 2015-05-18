/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
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

#include <colm/pdarun.h>
#include <colm/tree.h>
#include <colm/bytecode.h>
#include <colm/pool.h>
#include <colm/debug.h>
#include <colm/config.h>
#include <colm/struct.h>

#include <alloca.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef struct colm_struct struct_t;

tree_t **host_call( program_t *prg, long code, tree_t **sp );

#define TRUE_VAL  1
#define FALSE_VAL 0

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

#define true 1
#define false 0

#define read_byte( i ) do { \
	i = ((uchar) *instr++); \
} while(0)

#define read_half( i ) do { \
	i = ((word_t) *instr++); \
	i |= ((word_t) *instr++) << 8; \
} while(0)

/* There are better ways. */
#if SIZEOF_LONG == 4

	#define read_type( type, i ) do { \
		word_t w; \
		w = ((word_t) *instr++); \
		w |= ((word_t) *instr++) << 8; \
		w |= ((word_t) *instr++) << 16; \
		w |= ((word_t) *instr++) << 24; \
		i = (type) w; \
	} while(0)

	#define read_type_p( type, i, p ) do { \
		i = ((Type)  p[0]); \
		i |= ((Type) p[1]) << 8; \
		i |= ((Type) p[2]) << 16; \
		i |= ((Type) p[3]) << 24; \
	} while(0)

	#define consume_word() instr += 4

#else

	#define read_type( type, i ) do { \
		word_t _w; \
		_w = ((word_t) *instr++); \
		_w |= ((word_t) *instr++) << 8; \
		_w |= ((word_t) *instr++) << 16; \
		_w |= ((word_t) *instr++) << 24; \
		_w |= ((word_t) *instr++) << 32; \
		_w |= ((word_t) *instr++) << 40; \
		_w |= ((word_t) *instr++) << 48; \
		_w |= ((word_t) *instr++) << 56; \
		i = (type) _w; \
	} while(0)

	#define read_type_p( type, i, p ) do { \
		i = ((type)  p[0]); \
		i |= ((type) p[1]) << 8; \
		i |= ((type) p[2]) << 16; \
		i |= ((type) p[3]) << 24; \
		i |= ((type) p[4]) << 32; \
		i |= ((type) p[5]) << 40; \
		i |= ((type) p[6]) << 48; \
		i |= ((type) p[7]) << 56; \
	} while(0)

	#define consume_word() instr += 8
#endif

#define read_tree( i )   read_type( tree_t*, i )
#define read_parser( i ) read_type( parser_t*, i )
#define read_word( i )   read_type( word_t, i )
#define read_stream( i ) read_type( stream_t*, i )

#define read_word_p( i, p ) read_type_p( word_t, i, p )

#define consume_byte() instr += 1
#define consume_half() instr += 2

static void rcode_downref( program_t *prg, tree_t **sp, code_t *instr );

void colm_parser_set_context( program_t *prg, tree_t **sp, parser_t *parser, struct_t *val )
{
	parser->pdaRun->context = val;
}

static head_t *tree_to_str( program_t *prg, tree_t **sp, tree_t *tree, int trim )
{
	/* Collect the tree data. */
	StrCollect collect;
	initStrCollect( &collect );

	printTreeCollect( prg, sp, &collect, tree, trim );

	/* Set up the input stream. */
	head_t *ret = stringAllocFull( prg, collect.data, collect.length );

	strCollectDestroy( &collect );

	return ret;
}

static word_t stream_append_tree( program_t *prg, tree_t **sp, stream_t *dest, tree_t *input )
{
	long length = 0;
	struct stream_impl *impl = streamToImpl( dest );

	if ( input->id == LEL_ID_STR ) {
		/* Collect the tree data. */
		StrCollect collect;
		initStrCollect( &collect );
		printTreeCollect( prg, sp, &collect, input, false );

		/* Load it into the input. */
		impl->funcs->appendData( impl, collect.data, collect.length );
		length = collect.length;
		strCollectDestroy( &collect );
	}
	else if ( input->id == LEL_ID_PTR ) {
		colm_tree_upref( input );
		impl->funcs->appendStream( impl, input );
	}
	else {
		colm_tree_upref( input );
		impl->funcs->appendTree( impl, input );
	}

	return length;
}

static word_t stream_append_stream( program_t *prg, tree_t **sp, stream_t *dest, tree_t *stream )
{
	long length = 0;

	struct stream_impl *impl = streamToImpl( dest );
	impl->funcs->appendStream( impl, stream );

	return length;
}

static void stream_undo_append( program_t *prg, tree_t **sp,
		struct stream_impl *is, tree_t *input, long length )
{
	if ( input->id == LEL_ID_STR )
		is->funcs->undoAppendData( is, length );
	else if ( input->id == LEL_ID_PTR )
		is->funcs->undoAppendStream( is );
	else {
		tree_t *tree = is->funcs->undoAppendTree( is );
		colm_tree_downref( prg, sp, tree );
	}
}

static void stream_undo_append_stream( program_t *prg, tree_t **sp, struct stream_impl *is,
		tree_t *input, long length )
{
	is->funcs->undoAppendStream( is );
}

static tree_t *stream_pull_bc( program_t *prg, tree_t **sp, struct pda_run *pdaRun,
		stream_t *stream, tree_t *length )
{
	long len = ((long)length);
	struct stream_impl *impl = streamToImpl( stream );
	head_t *tokdata = colm_stream_pull( prg, sp, pdaRun, impl, len );
	return constructString( prg, tokdata );
}

static void undo_pull( program_t *prg, stream_t *stream, tree_t *str )
{
	struct stream_impl *impl = streamToImpl( stream );
	const char *data = stringData( ( (str_t*)str )->value );
	long length = stringLength( ( (str_t*)str )->value );
	undoStreamPull( impl, data, length );
}

static long stream_push( program_t *prg, tree_t **sp, struct stream_impl *in, tree_t *tree, int ignore )
{
	if ( tree->id == LEL_ID_STR ) {
		/* This should become a compile error. If it's text, it's up to the
		 * scanner to decide. Want to force it then send a token. */
		assert( !ignore );
			
		/* Collect the tree data. */
		StrCollect collect;
		initStrCollect( &collect );
		printTreeCollect( prg, sp, &collect, tree, false );

		colm_stream_push_text( in, collect.data, collect.length );
		long length = collect.length;
		strCollectDestroy( &collect );

		return length;
	}
	else if ( tree->id == LEL_ID_PTR ) {
		colm_tree_upref( tree );
		colm_stream_push_stream( in, tree );
		return -1;
	}
	else {
		colm_tree_upref( tree );
		colm_stream_push_tree( in, tree, ignore );
		return -1;
	}
}

static long stream_push_stream( program_t *prg, tree_t **sp,
		struct stream_impl *in, stream_t *stream )
{
	colm_stream_push_stream( in, (tree_t*)stream );
	return -1;
}

static void set_local( Execution *exec, long field, tree_t *tree )
{
	if ( tree != 0 )
		assert( tree->refs >= 1 );
	vm_set_local( exec, field, tree );
}

static tree_t *get_local_split( program_t *prg, Execution *exec, long field )
{
	tree_t *val = vm_get_local( exec, field );
	tree_t *split = splitTree( prg, val );
	vm_set_local( exec, field, split );
	return split;
}

static void downref_local_trees( program_t *prg, tree_t **sp,
		Execution *exec, struct local_info *locals, long localsLen )
{
	long i;
	for ( i = localsLen-1; i >= 0; i-- ) {
		if ( locals[i].type == LI_Tree ) {
			debug( prg, REALM_BYTECODE, "local tree downref: %ld\n",
					(long)locals[i].offset );

			tree_t *tree = (tree_t*) vm_get_local( exec, (long)locals[i].offset );
			colm_tree_downref( prg, sp, tree );
		}
	}
}

static void downref_locals( program_t *prg, tree_t ***psp,
		Execution *exec, struct local_info *locals, long localsLen )
{
	long i;
	for ( i = localsLen-1; i >= 0; i-- ) {
		switch ( locals[i].type ) {
			case LI_Tree: {
				debug( prg, REALM_BYTECODE, "local tree downref: %ld\n",
						(long)locals[i].offset );
				tree_t *tree = (tree_t*) vm_get_local( exec, (long)locals[i].offset );
				colm_tree_downref( prg, *psp, tree );
				break;
			}
			case LI_Iter: {
				debug( prg, REALM_BYTECODE, "local iter downref: %ld\n",
						(long)locals[i].offset );
				tree_iter_t *iter = (tree_iter_t*) vm_get_plocal( exec, (long)locals[i].offset );
				colm_tree_iter_destroy( prg, psp, iter );
				break;
			}
			case LI_RevIter: {
				debug( prg, REALM_BYTECODE, "local rev iter downref: %ld\n",
						(long)locals[i].offset );
				rev_tree_iter_t *riter = (rev_tree_iter_t*) vm_get_plocal( exec,
						(long)locals[i].offset );
				colm_rev_tree_iter_destroy( prg, psp, riter );
				break;
			}
			case LI_UserIter: {
				debug( prg, REALM_BYTECODE, "local user iter downref: %ld\n",
						(long)locals[i].offset );
				user_iter_t *uiter = (user_iter_t*) vm_get_local( exec, locals[i].offset );
				colm_uiter_unwind( prg, psp, uiter );
				break;
			}
		}
	}
}

static tree_t *construct_arg0( program_t *prg, int argc, const char **argv )
{
	tree_t *arg0 = 0;
	if ( argc > 0 ) {
		head_t *head = colm_string_alloc_pointer( prg, argv[0], strlen(argv[0]) );
		arg0 = constructString( prg, head );
		colm_tree_upref( arg0 );
	}
	return arg0;
}

static list_t *construct_argv( program_t *prg, int argc, const char **argv )
{
	list_t *list = (list_t*)colm_construct_generic( prg, prg->rtd->argvGenericId );
	int i;
	for ( i = 1; i < argc; i++ ) {
		head_t *head = colm_string_alloc_pointer( prg, argv[i], strlen(argv[i]) );
		tree_t *arg = constructString( prg, head );
		colm_tree_upref( arg );

		struct_t *strct = colm_struct_new_size( prg, 16 );
		strct->id = prg->rtd->argvElId;
		colm_struct_set_field( strct, tree_t*, 0, arg );
		list_el_t *listEl = colm_struct_get_addr( strct, list_el_t*, 1 );
		colm_list_append( list, listEl );
	}
	
	return list;
}

/*
 * Execution environment
 */

void colm_rcode_downref_all( program_t *prg, tree_t **sp, struct rt_code_vect *rev )
{
	while ( rev->tabLen > 0 ) {
		/* Read the length */
		code_t *prcode = rev->data + rev->tabLen - SIZEOF_WORD;
		word_t len;
		read_word_p( len, prcode );

		/* Find the start of block. */
		long start = rev->tabLen - len - SIZEOF_WORD;
		prcode = rev->data + start;

		/* Execute it. */
		rcode_downref( prg, sp, prcode );

		/* Backup over it. */
		rev->tabLen -= len + SIZEOF_WORD;
	}
}

void colm_execute( program_t *prg, Execution *exec, code_t *code )
{
	tree_t **sp = prg->stackRoot;

	struct frame_info *fi = &prg->rtd->frameInfo[prg->rtd->rootFrameId];

	/* Set up the stack as if we have 
	 * called. We allow a return value. */

	long stretch = FR_AA + fi->frameSize;
	vm_contiguous( stretch );

	vm_push_tree( 0 );
	vm_push_tree( 0 ); 
	vm_push_tree( 0 );
	vm_push_tree( 0 );
	vm_push_tree( 0 );

	exec->framePtr = vm_ptop();
	vm_pushn( fi->frameSize );
	memset( vm_ptop(), 0, sizeof(word_t) * fi->frameSize );

	/* Execution loop. */
	sp = colm_execute_code( prg, exec, sp, code );

	downref_locals( prg, &sp, exec, fi->locals, fi->localsLen );
	vm_popn( fi->frameSize );

	vm_pop_ignore();
	vm_pop_ignore();
	colm_tree_downref( prg, sp, prg->returnVal );
	prg->returnVal = vm_pop_tree();
	vm_pop_ignore();

	prg->stackRoot = sp;
}

tree_t *colm_run_func( struct colm_program *prg, int frameId,
		const char **params, int paramCount )
{
	/* Make the arguments available to the program. */
	prg->argc = 0;
	prg->argv = 0;

	Execution execution;
	memset( &execution, 0, sizeof(execution) );

	tree_t **sp = prg->stackRoot;

	struct frame_info *fi = &prg->rtd->frameInfo[frameId];
	code_t *code = fi->codeWC;

	vm_pushn( paramCount );
	execution.callArgs = vm_ptop();
	memset( vm_ptop(), 0, sizeof(word_t) * paramCount );

	int p;
	for ( p = 0; p < paramCount; p++ ) {
		if ( params[p] == 0 ) {
			((value_t*)execution.callArgs)[p] = 0;
		}
		else {
			head_t *head = colm_string_alloc_pointer( prg, params[p], strlen(params[p]) );
			tree_t *tree = constructString( prg, head );
			colm_tree_upref( tree );
			((tree_t**)execution.callArgs)[p] = tree;
		}
	}

	long stretch = FR_AA + fi->frameSize;
	vm_contiguous( stretch );

	/* Set up the stack as if we have called. We allow a return value. */
	vm_push_tree( (tree_t*)execution.callArgs );
	vm_push_tree( 0 ); 
	vm_push_tree( 0 );
	vm_push_tree( 0 );
	vm_push_tree( 0 );

	execution.frameId = frameId;

	execution.framePtr = vm_ptop();
	vm_pushn( fi->frameSize );
	memset( vm_ptop(), 0, sizeof(word_t) * fi->frameSize );

	/* Execution loop. */
	sp = colm_execute_code( prg, &execution, sp, code );

	colm_tree_downref( prg, sp, prg->returnVal );
	prg->returnVal = execution.retVal;

	vm_popn( paramCount );
	
	assert( sp == prg->stackRoot );

	return prg->returnVal;
};


int colm_make_reverse_code( struct pda_run *pdaRun )
{
	struct rt_code_vect *reverseCode = &pdaRun->reverseCode;
	struct rt_code_vect *rcodeCollect = &pdaRun->rcodeCollect;

	/* Do we need to revert the left hand side? */

	/* Check if there was anything generated. */
	if ( rcodeCollect->tabLen == 0 )
		return false;

	if ( pdaRun->rcBlockCount == 0 ) {
		/* One reverse code run for the DECK terminator. */
		append_code_val( reverseCode, IN_PCR_END_DECK );
		append_code_val( reverseCode, IN_PCR_RET );
		append_word( reverseCode, 2 );
		pdaRun->rcBlockCount += 1;
		colm_increment_steps( pdaRun );
	}

	long startLength = reverseCode->tabLen;

	/* Go backwards, group by group, through the reverse code. Push each group
	 * to the global reverse code stack. */
	code_t *p = rcodeCollect->data + rcodeCollect->tabLen;
	while ( p != rcodeCollect->data ) {
		p--;
		long len = *p;
		p = p - len;
		append_code_vect( reverseCode, p, len );
	}

	/* Stop, then place a total length in the global stack. */
	append_code_val( reverseCode, IN_PCR_RET );
	long length = reverseCode->tabLen - startLength;
	append_word( reverseCode, length );

	/* Clear the revere code buffer. */
	rcodeCollect->tabLen = 0;

	pdaRun->rcBlockCount += 1;
	colm_increment_steps( pdaRun );

	return true;
}

void colm_transfer_reverse_code( struct pda_run *pdaRun, parse_tree_t *parseTree )
{
	if ( pdaRun->rcBlockCount > 0 ) {
		//debug( REALM_PARSE, "attaching reverse code to token\n" );
		parseTree->flags |= PF_HAS_RCODE;
		pdaRun->rcBlockCount = 0;
	}
}

static void rcode_unit_term( Execution *exec )
{
	append_code_val( &exec->parser->pdaRun->rcodeCollect, exec->rcodeUnitLen );
	exec->rcodeUnitLen = 0;
}

static void rcode_unit_start( Execution *exec )
{
	exec->rcodeUnitLen = 0;
}

static void rcode_code( Execution *exec, const code_t code )
{
	append_code_val( &exec->parser->pdaRun->rcodeCollect, code );
	exec->rcodeUnitLen += SIZEOF_CODE;
}

static void rcodeHalf( Execution *exec, const half_t half )
{
	append_half( &exec->parser->pdaRun->rcodeCollect, half );
	exec->rcodeUnitLen += SIZEOF_HALF;
}

static void rcode_word( Execution *exec, const word_t word )
{
	append_word( &exec->parser->pdaRun->rcodeCollect, word );
	exec->rcodeUnitLen += SIZEOF_WORD;
}

code_t *colm_pop_reverse_code( struct rt_code_vect *allRev )
{
	/* Read the length */
	code_t *prcode = allRev->data + allRev->tabLen - SIZEOF_WORD;
	word_t len;
	read_word_p( len, prcode );

	/* Find the start of block. */
	long start = allRev->tabLen - len - SIZEOF_WORD;
	prcode = allRev->data + start;

	/* Backup over it. */
	allRev->tabLen -= len + SIZEOF_WORD;
	return prcode;
}

tree_t **colm_execute_code( program_t *prg, Execution *exec, tree_t **sp, code_t *instr )
{
	/* When we exit we are going to verify that we did not eat up any stack
	 * space. */
	tree_t **root = sp;
	code_t c;

again:
	c = *instr++;
	//debug( REALM_BYTECODE, "--in 0x%x\n", c );

	switch ( c ) {
		case IN_RESTORE_LHS: {
			tree_t *restore;
			read_tree( restore );

			debug( prg, REALM_BYTECODE, "IN_RESTORE_LHS\n" );
			colm_tree_downref( prg, sp, exec->parser->pdaRun->parseInput->shadow->tree );
			exec->parser->pdaRun->parseInput->shadow->tree = restore;
			break;
		}
		case IN_LOAD_NIL: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_NIL\n" );
			vm_push_tree( 0 );
			break;
		}
		case IN_LOAD_TREE: {
			tree_t *tree;
			read_tree( tree );
			vm_push_tree( tree );
			debug( prg, REALM_BYTECODE, "IN_LOAD_TREE %p id: %d refs: %d\n",
					tree, tree->id, tree->refs );
			break;
		}
		case IN_LOAD_WORD: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_WORD\n" );
			word_t w;
			read_word( w );
			vm_push_type( word_t, w );
			break;
		}
		case IN_LOAD_TRUE: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_TRUE\n" );
			//colm_tree_upref( prg->trueVal );
			vm_push_tree( prg->trueVal );
			break;
		}
		case IN_LOAD_FALSE: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_FALSE\n" );
			//colm_tree_upref( prg->falseVal );
			vm_push_tree( prg->falseVal );
			break;
		}
		case IN_LOAD_INT: {
			word_t i;
			read_word( i );

			debug( prg, REALM_BYTECODE, "IN_LOAD_INT %d\n", i );

			value_t value = i;
			vm_push_value( value );
			break;
		}
		case IN_LOAD_STR: {
			word_t offset;
			read_word( offset );

			debug( prg, REALM_BYTECODE, "IN_LOAD_STR %d\n", offset );

			head_t *lit = makeLiteral( prg, offset );
			tree_t *tree = constructString( prg, lit );
			colm_tree_upref( tree );
			vm_push_tree( tree );
			break;
		}
		case IN_PRINT: {
			int n, i;
			read_byte( n );
			debug( prg, REALM_BYTECODE, "IN_PRINT %d\n", n );

			tree_t *arg[n];
			for ( i = n-1; i >= 0; i-- )
				arg[i] = vm_pop_tree();

			for ( i = 0; i < n; i++ )
				printTreeFile( prg, sp, stdout, arg[i], false );

			for ( i = 0; i < n; i++ )
				colm_tree_downref( prg, sp, arg[i] );
			break;
		}
		case IN_PRINT_STREAM: {
			int n, i;
			read_byte( n );
			debug( prg, REALM_BYTECODE, "IN_PRINT_STREAM %d\n", n );

			tree_t *arg[n];
			for ( i = n-1; i >= 0; i-- )
				arg[i] = vm_pop_tree();
			stream_t *stream = vm_pop_stream();
			struct stream_impl *si = streamToImpl( stream );

			for ( i = 0; i < n; i++ ) {
				if ( si->file != 0 )
					printTreeFile( prg, sp, si->file, arg[i], false );
				else
					printTreeFd( prg, sp, si->fd, arg[i], false );
			}

			for ( i = 0; i < n; i++ )
				colm_tree_downref( prg, sp, arg[i] );
			break;
		}
		case IN_PRINT_XML_AC: {
			int n, i;
			read_byte( n );

			debug( prg, REALM_BYTECODE, "IN_PRINT_XML_AC %d\n", n );

			tree_t *arg[n];
			for ( i = n-1; i >= 0; i-- )
				arg[i] = vm_pop_tree();

			for ( i = 0; i < n; i++ )
				printXmlStdout( prg, sp, arg[i], true, true );

			for ( i = 0; i < n; i++ )
				colm_tree_downref( prg, sp, arg[i] );
			break;
		}
		case IN_PRINT_XML: {
			int n, i;
			read_byte( n );
			debug( prg, REALM_BYTECODE, "IN_PRINT_XML %d", n );

			tree_t *arg[n];
			for ( i = n-1; i >= 0; i-- )
				arg[i] = vm_pop_tree();

			for ( i = 0; i < n; i++ )
				printXmlStdout( prg, sp, arg[i], false, true );

			for ( i = 0; i < n; i++ )
				colm_tree_downref( prg, sp, arg[i] );
			break;
		}

		/*
		 * LOAD_GLOBAL
		 */
		case IN_LOAD_GLOBAL_R: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_GLOBAL_R\n" );

			vm_push_struct( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_WV: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_GLOBAL_WV\n" );

			vm_push_struct( prg->global );

			/* Set up the reverse instruction. */
			rcode_unit_start( exec );
			rcode_code( exec, IN_LOAD_GLOBAL_BKT );
			break;
		}
		case IN_LOAD_GLOBAL_WC: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_GLOBAL_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			vm_push_struct( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_GLOBAL_BKT\n" );

			vm_push_struct( prg->global );
			break;
		}

		case IN_LOAD_INPUT_R: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_INPUT_R\n" );

			assert( exec->parser != 0 );
			vm_push_stream( exec->parser->input );
			break;
		}
		case IN_LOAD_INPUT_WV: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_INPUT_WV\n" );

			assert( exec->parser != 0 );
			vm_push_stream( exec->parser->input );

			/* Set up the reverse instruction. */
			rcode_unit_start( exec );
			rcode_code( exec, IN_LOAD_INPUT_BKT );
			rcode_word( exec, (word_t)exec->parser->input );
			break;
		}
		case IN_LOAD_INPUT_WC: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_INPUT_WC\n" );

			assert( exec->parser != 0 );
			vm_push_stream( exec->parser->input );
			break;
		}
		case IN_LOAD_INPUT_BKT: {
			tree_t *accumStream;
			read_tree( accumStream );

			debug( prg, REALM_BYTECODE, "IN_LOAD_INPUT_BKT\n" );

			colm_tree_upref( accumStream );
			vm_push_tree( accumStream );
			break;
		}

		case IN_LOAD_CONTEXT_R: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CONTEXT_R\n" );

			vm_push_type( struct_t*, exec->parser->pdaRun->context );
			break;
		}
		case IN_LOAD_CONTEXT_WV: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CONTEXT_WV\n" );

			vm_push_type( struct_t *, exec->parser->pdaRun->context );

			/* Set up the reverse instruction. */
			rcode_unit_start( exec );
			rcode_code( exec, IN_LOAD_CONTEXT_BKT );
			break;
		}
		case IN_LOAD_CONTEXT_WC: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CONTEXT_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			vm_push_type( struct_t *, exec->parser->pdaRun->context );
			break;
		}
		case IN_LOAD_CONTEXT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CONTEXT_BKT\n" );

			vm_push_type( struct_t *, exec->parser->pdaRun->context );
			break;
		}

		case IN_SET_PARSER_CONTEXT: {
			debug( prg, REALM_BYTECODE, "IN_SET_PARSER_CTX_WC\n" );

			struct_t *strct = vm_pop_struct();
			parser_t *parser = vm_pop_parser();

			colm_parser_set_context( prg, sp, parser, strct );

			vm_push_parser( parser );
			break;
		}

		case IN_INIT_CAPTURES: {
			consume_byte();

			debug( prg, REALM_BYTECODE, "IN_INIT_CAPTURES\n" );

			/* If there are captures (this is a translate block) then copy them into
			 * the local frame now. */
			struct lang_el_info *lelInfo = prg->rtd->lelInfo;
			char **mark = exec->parser->pdaRun->mark;

			int i;
			for ( i = 0; i < lelInfo[exec->parser->pdaRun->tokenId].numCaptureAttr; i++ ) {
				struct lang_el_info *lei = &lelInfo[exec->parser->pdaRun->tokenId];
				CaptureAttr *ca = &prg->rtd->captureAttr[lei->captureAttr + i];
				head_t *data = stringAllocFull( prg, mark[ca->mark_enter],
						mark[ca->mark_leave] - mark[ca->mark_enter] );
				tree_t *string = constructString( prg, data );
				colm_tree_upref( string );
				set_local( exec, -1 - i, string );
			}
			break;
		}
		case IN_INIT_RHS_EL: {
			half_t position;
			short field;
			read_half( position );
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_INIT_RHS_EL %hd\n", field );

			tree_t *val = getRhsEl( prg, exec->parser->pdaRun->redLel->shadow->tree, position );
			colm_tree_upref( val );
			vm_set_local(exec, field, val);
			break;
		}

		case IN_INIT_LHS_EL: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_INIT_LHS_EL %hd\n", field );

			/* We transfer it to to the local field. Possibly take a copy. */
			tree_t *val = exec->parser->pdaRun->redLel->shadow->tree;

			/* Save it. */
			colm_tree_upref( val );
			exec->parser->pdaRun->parsed = val;

			exec->parser->pdaRun->redLel->shadow->tree = 0;
			vm_set_local(exec, field, val);
			break;
		}
		case IN_STORE_LHS_EL: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_STORE_LHS_EL %hd\n", field );

			tree_t *val = vm_get_local(exec, field);
			vm_set_local(exec, field, 0);
			exec->parser->pdaRun->redLel->shadow->tree = val;
			break;
		}
		case IN_UITER_ADVANCE: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_ADVANCE\n" );

			/* Get the iterator. */
			user_iter_t *uiter = (user_iter_t*) vm_get_local(exec, field);

			long yieldSize = vm_ssize() - uiter->rootSize;
			assert( uiter->yieldSize == yieldSize );

			/* Fix the return instruction pointer. */
			uiter->stackRoot[-IFR_AA + IFR_RIN] = (SW)instr;

			instr = uiter->resume;
			exec->framePtr = uiter->frame;
			exec->iframePtr = &uiter->stackRoot[-IFR_AA];
			break;
		}
		case IN_UITER_GET_CUR_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_GET_CUR_R\n" );

			user_iter_t *uiter = (user_iter_t*) vm_get_local(exec, field);
			tree_t *val = uiter->ref.kid->tree;
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_UITER_GET_CUR_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_GET_CUR_WC\n" );

			user_iter_t *uiter = (user_iter_t*) vm_get_local(exec, field);
			splitRef( prg, &sp, &uiter->ref );
			tree_t *split = uiter->ref.kid->tree;
			colm_tree_upref( split );
			vm_push_tree( split );
			break;
		}
		case IN_UITER_SET_CUR_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_SET_CUR_WC\n" );

			tree_t *t = vm_pop_tree();
			user_iter_t *uiter = (user_iter_t*) vm_get_local(exec, field);
			splitRef( prg, &sp, &uiter->ref );
			tree_t *old = uiter->ref.kid->tree;
			setUiterCur( prg, uiter, t );
			colm_tree_downref( prg, sp, old );
			break;
		}
		case IN_GET_LOCAL_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_R %hd\n", field );

			tree_t *val = vm_get_local(exec, field);
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_GET_LOCAL_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_WC %hd\n", field );

			tree_t *split = get_local_split( prg, exec, field );
			colm_tree_upref( split );
			vm_push_tree( split );
			break;
		}
		case IN_SET_LOCAL_WC: {
			short field;
			read_half( field );
			debug( prg, REALM_BYTECODE, "IN_SET_LOCAL_WC %hd\n", field );

			tree_t *val = vm_pop_tree();
			colm_tree_downref( prg, sp, vm_get_local(exec, field) );
			set_local( exec, field, val );
			break;
		}
		case IN_GET_LOCAL_VAL_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_VAL_R %hd\n", field );

			tree_t *val = vm_get_local(exec, field);
			vm_push_tree( val );
			break;
		}
		case IN_SET_LOCAL_VAL_WC: {
			short field;
			read_half( field );
			debug( prg, REALM_BYTECODE, "IN_SET_LOCAL_VAL_WC %hd\n", field );

			tree_t *val = vm_pop_tree();
			vm_set_local(exec, field, val);
			break;
		}
		case IN_SAVE_RET: {
			debug( prg, REALM_BYTECODE, "IN_SAVE_RET\n" );

			value_t val = vm_pop_value();
			vm_set_local(exec, FR_RV, (tree_t*)val);
			break;
		}
		case IN_GET_LOCAL_REF_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_REF_R\n" );

			ref_t *ref = (ref_t*) vm_get_plocal(exec, field);
			tree_t *val = ref->kid->tree;
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_GET_LOCAL_REF_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_REF_WC\n" );

			ref_t *ref = (ref_t*) vm_get_plocal(exec, field);
			splitRef( prg, &sp, ref );
			tree_t *val = ref->kid->tree;
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_SET_LOCAL_REF_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_LOCAL_REF_WC\n" );

			tree_t *val = vm_pop_tree();
			ref_t *ref = (ref_t*) vm_get_plocal(exec, field);
			splitRef( prg, &sp, ref );
			refSetValue( prg, sp, ref, val );
			break;
		}
		case IN_GET_FIELD_TREE_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_TREE_R %d\n", field );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *val = colm_tree_get_field( obj, field );
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_GET_FIELD_TREE_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_TREE_WC %d\n", field );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *split = getFieldSplit( prg, obj, field );
			colm_tree_upref( split );
			vm_push_tree( split );
			break;
		}
		case IN_GET_FIELD_TREE_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_TREE_WV\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *split = getFieldSplit( prg, obj, field );
			colm_tree_upref( split );
			vm_push_tree( split );

			/* Set up the reverse instruction. */
			rcode_code( exec, IN_GET_FIELD_TREE_BKT );
			rcodeHalf( exec, field );
			break;
		}
		case IN_GET_FIELD_TREE_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_TREE_BKT\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *split = getFieldSplit( prg, obj, field );
			colm_tree_upref( split );
			vm_push_tree( split );
			break;
		}
		case IN_SET_FIELD_TREE_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_WC %d\n", field );

			tree_t *obj = vm_pop_tree();
			tree_t *val = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			/* Downref the old value. */
			tree_t *prev = colm_tree_get_field( obj, field );
			colm_tree_downref( prg, sp, prev );

			colm_tree_set_field( prg, obj, field, val );
			break;
		}
		case IN_SET_FIELD_TREE_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_WV %d\n", field );

			tree_t *obj = vm_pop_tree();
			tree_t *val = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			/* Save the old value, then set the field. */
			tree_t *prev = colm_tree_get_field( obj, field );
			colm_tree_set_field( prg, obj, field, val );

			/* Set up the reverse instruction. */
			rcode_code( exec, IN_SET_FIELD_TREE_BKT );
			rcodeHalf( exec, field );
			rcode_word( exec, (word_t)prev );
			rcode_unit_term( exec );
			break;
		}
		case IN_SET_FIELD_TREE_BKT: {
			short field;
			tree_t *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_BKT\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			/* Downref the old value. */
			tree_t *prev = colm_tree_get_field( obj, field );
			colm_tree_downref( prg, sp, prev );

			colm_tree_set_field( prg, obj, field, val );
			break;
		}
		case IN_SET_FIELD_TREE_LEAVE_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_LEAVE_WC\n" );

			/* Note that we don't downref the object here because we are
			 * leaving it on the stack. */
			tree_t *obj = vm_pop_tree();
			tree_t *val = vm_pop_tree();

			/* Downref the old value. */
			tree_t *prev = colm_tree_get_field( obj, field );
			colm_tree_downref( prg, sp, prev );

			/* Set the field. */
			colm_tree_set_field( prg, obj, field, val );

			/* Leave the object on the top of the stack. */
			vm_push_tree( obj );
			break;
		}
		case IN_GET_FIELD_VAL_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_VAL_R %d\n", field );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *pointer = colm_tree_get_field( obj, field );
			value_t value = 0;
			if ( pointer != 0 )
				value = colm_get_pointer_val( pointer );
			vm_push_value( value );
			break;
		}
		case IN_SET_FIELD_VAL_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_VAL_WC %d\n", field );

			tree_t *obj = vm_pop_tree();
			value_t value = vm_pop_value();
			colm_tree_downref( prg, sp, obj );

			/* Downref the old value. */
			tree_t *prev = colm_tree_get_field( obj, field );
			colm_tree_downref( prg, sp, prev );

			/* Make it into a pointer. */
			tree_t *pointer = colm_construct_pointer( prg, value );
			colm_tree_upref( pointer );

			colm_tree_set_field( prg, obj, field, pointer );
			break;
		}
		case IN_NEW_STRUCT: {
			short id;
			read_half( id );

			debug( prg, REALM_BYTECODE, "IN_NEW_STRUCT %hd\n", id );
			struct_t *item = colm_struct_new( prg, id );
			vm_push_type( struct_t*, item );
			break;
		}
		case IN_GET_STRUCT_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_R %d\n", field );

			tree_t *obj = vm_pop_tree();
			tree_t *val = colm_struct_get_field( obj, tree_t*, field );
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_GET_STRUCT_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_WC %d\n", field );

			tree_t *obj = vm_pop_tree();
			tree_t *val = colm_struct_get_field( obj, tree_t*, field );
			colm_tree_upref( val );
			vm_push_tree( val );

			break;
		}
		case IN_GET_STRUCT_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_WV\n" );

			tree_t *obj = vm_pop_tree();
			tree_t *val = colm_struct_get_field( obj, tree_t*, field );
			colm_tree_upref( val );
			vm_push_tree( val );

			/* Set up the reverse instruction. */
			rcode_code( exec, IN_GET_STRUCT_BKT );
			rcodeHalf( exec, field );
			break;
		}
		case IN_GET_STRUCT_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_BKT\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *split = getFieldSplit( prg, obj, field );
			colm_tree_upref( split );
			vm_push_tree( split );
			break;
		}
		case IN_SET_STRUCT_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_WC %d\n", field );

			tree_t *obj = vm_pop_tree();
			tree_t *val = vm_pop_tree();

			/* Downref the old value. */
			tree_t *prev = colm_struct_get_field( obj, tree_t*, field );
			colm_tree_downref( prg, sp, prev );
			colm_struct_set_field( obj, tree_t*, field, val );
			break;
		}
		case IN_SET_STRUCT_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_WV %d\n", field );

			struct_t *obj = vm_pop_struct();
			tree_t *val = vm_pop_tree();

			/* Save the old value, then set the field. */
			tree_t *prev = colm_struct_get_field( obj, tree_t*, field );
			colm_struct_set_field( obj, tree_t*, field, val );

			/* Set up the reverse instruction. */
			rcode_code( exec, IN_SET_STRUCT_BKT );
			rcodeHalf( exec, field );
			rcode_word( exec, (word_t)prev );
			rcode_unit_term( exec );
			break;
		}
		case IN_SET_STRUCT_BKT: {
			short field;
			tree_t *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_BKT\n" );

			tree_t *obj = vm_pop_tree();

			/* Downref the old value. */
			tree_t *prev = colm_struct_get_field( obj, tree_t*, field );
			colm_tree_downref( prg, sp, prev );

			colm_struct_set_field( obj, tree_t*, field, val );
			break;
		}
		case IN_GET_STRUCT_VAL_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_VAL_R %d\n", field );

			tree_t *obj = vm_pop_tree();
			tree_t *val = colm_struct_get_field( obj, tree_t*, field );
			vm_push_tree( val );
			break;
		}
		case IN_SET_STRUCT_VAL_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_VAL_WC %d\n", field );

			struct_t *strct = vm_pop_struct();
			tree_t *val = vm_pop_tree();

			colm_struct_set_field( strct, tree_t*, field, val );
			break;
		}
		case IN_SET_STRUCT_VAL_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_VAL_WV %d\n", field );

			struct_t *strct = vm_pop_struct();
			tree_t *val = vm_pop_tree();

			tree_t *prev = colm_struct_get_field( strct, tree_t*, field );
			colm_struct_set_field( strct, tree_t*, field, val );

			rcode_code( exec, IN_SET_STRUCT_VAL_BKT );
			rcodeHalf( exec, field );
			rcode_word( exec, (word_t)prev );
			rcode_unit_term( exec );
			break;
		}
		case IN_SET_STRUCT_VAL_BKT: {
			short field;
			tree_t *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_VAL_BKT\n" );

			tree_t *obj = vm_pop_tree();

			colm_struct_set_field( obj, tree_t*, field, val );
			break;
		}
		case IN_GET_RHS_VAL_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_RHS_VAL_R\n" );
			int i, done = 0;
			uchar len;

			tree_t *obj = vm_pop_tree(), *val = 0;
			colm_tree_downref( prg, sp, obj );

			read_byte( len );
			for ( i = 0; i < len; i++ ) {
				uchar prodNum, childNum;
				read_byte( prodNum );
				read_byte( childNum );
				if ( !done && obj->prod_num == prodNum ) {
					val = getRhsEl( prg, obj, childNum );
					done = 1;
				}
			}

			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_POP_TREE: {
			debug( prg, REALM_BYTECODE, "IN_POP_TREE\n" );

			tree_t *val = vm_pop_tree();
			colm_tree_downref( prg, sp, val );
			break;
		}
		case IN_POP_VAL: {
			debug( prg, REALM_BYTECODE, "IN_POP_VAL\n" );

			vm_pop_tree();
			break;
		}
		case IN_POP_N_WORDS: {
			short n;
			read_half( n );

			debug( prg, REALM_BYTECODE, "IN_POP_N_WORDS %hd\n", n );

			vm_popn( n );
			break;
		}
		case IN_SPRINTF: {
			debug( prg, REALM_BYTECODE, "IN_SPRINTF\n" );

			vm_pop_tree();
			value_t integer = vm_pop_value();
			str_t *format = vm_pop_string();
			head_t *res = stringSprintf( prg, format, (long)integer );
			str_t *str = (str_t*)constructString( prg, res );
			colm_tree_upref( (tree_t*)str );
			vm_push_string( str );
			colm_tree_downref( prg, sp, (tree_t*)format );
			break;
		}
		case IN_INT_TO_STR: {
			debug( prg, REALM_BYTECODE, "IN_INT_TO_STR\n" );

			value_t i = vm_pop_value();
			head_t *res = intToStr( prg, (long)i );
			tree_t *str = constructString( prg, res );
			colm_tree_upref( str );
			vm_push_tree( str );
			break;
		}
		case IN_TREE_TO_STR: {
			debug( prg, REALM_BYTECODE, "IN_TREE_TO_STR\n" );

			tree_t *tree = vm_pop_tree();
			head_t *res = tree_to_str( prg, sp, tree, false );
			tree_t *str = constructString( prg, res );
			colm_tree_upref( str );
			vm_push_tree( str );
			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_TREE_TO_STR_TRIM: {
			debug( prg, REALM_BYTECODE, "IN_TREE_TO_STR_TRIM\n" );

			tree_t *tree = vm_pop_tree();
			head_t *res = tree_to_str( prg, sp, tree, true );
			tree_t *str = constructString( prg, res );
			colm_tree_upref( str );
			vm_push_tree( str );
			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_TREE_TRIM: {
			debug( prg, REALM_BYTECODE, "IN_TREE_TRIM\n" );

			tree_t *tree = vm_pop_tree();
			tree_t *trimmed = treeTrim( prg, sp, tree );
			vm_push_tree( trimmed );
			break;
		}
		case IN_CONCAT_STR: {
			debug( prg, REALM_BYTECODE, "IN_CONCAT_STR\n" );

			str_t *s2 = vm_pop_string();
			str_t *s1 = vm_pop_string();
			head_t *res = concatStr( s1->value, s2->value );
			tree_t *str = constructString( prg, res );
			colm_tree_upref( str );
			colm_tree_downref( prg, sp, (tree_t*)s1 );
			colm_tree_downref( prg, sp, (tree_t*)s2 );
			vm_push_tree( str );
			break;
		}

		case IN_STR_LENGTH: {
			debug( prg, REALM_BYTECODE, "IN_STR_LENGTH\n" );

			str_t *str = vm_pop_string();
			long len = stringLength( str->value );
			value_t res = len;
			vm_push_value( res );
			colm_tree_downref( prg, sp, (tree_t*)str );
			break;
		}
		case IN_JMP_FALSE_TREE: {
			short dist;
			read_half( dist );

			debug( prg, REALM_BYTECODE, "IN_JMP_FALSE_TREE %d\n", dist );

			tree_t *tree = vm_pop_tree();
			if ( testFalse( prg, tree ) )
				instr += dist;
			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_JMP_TRUE_TREE: {
			short dist;
			read_half( dist );

			debug( prg, REALM_BYTECODE, "IN_JMP_TRUE_TREE %d\n", dist );

			tree_t *tree = vm_pop_tree();
			if ( !testFalse( prg, tree ) )
				instr += dist;
			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_JMP_FALSE_VAL: {
			short dist;
			read_half( dist );

			debug( prg, REALM_BYTECODE, "IN_JMP_FALSE_VAL %d\n", dist );

			tree_t *tree = vm_pop_tree();
			if ( tree == 0 )
				instr += dist;
			break;
		}
		case IN_JMP_TRUE_VAL: {
			short dist;
			read_half( dist );

			debug( prg, REALM_BYTECODE, "IN_JMP_TRUE_VAL %d\n", dist );

			tree_t *tree = vm_pop_tree();
			if ( tree != 0 )
				instr += dist;
			break;
		}
		case IN_JMP: {
			short dist;
			read_half( dist );

			debug( prg, REALM_BYTECODE, "IN_JMP\n" );

			instr += dist;
			break;
		}
		case IN_REJECT: {
			debug( prg, REALM_BYTECODE, "IN_REJECT\n" );
			exec->parser->pdaRun->reject = true;
			break;
		}

		/*
		 * Binary comparison operators.
		 */
		case IN_TST_EQL_TREE: {
			debug( prg, REALM_BYTECODE, "IN_TST_EQL_TREE\n" );

			tree_t *o2 = vm_pop_tree();
			tree_t *o1 = vm_pop_tree();
			long r = colm_cmp_tree( prg, o1, o2 );
			value_t val = r == 0 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			colm_tree_downref( prg, sp, o1 );
			colm_tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_EQL_VAL: {
			debug( prg, REALM_BYTECODE, "IN_TST_EQL_VAL\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			value_t val = o1 == o2 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			break;
		}
		case IN_TST_NOT_EQL_TREE: {
			debug( prg, REALM_BYTECODE, "IN_TST_NOT_EQL_TREE\n" );

			tree_t *o2 = vm_pop_tree();
			tree_t *o1 = vm_pop_tree();
			long r = colm_cmp_tree( prg, o1, o2 );
			value_t val = r != 0 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			colm_tree_downref( prg, sp, o1 );
			colm_tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_NOT_EQL_VAL: {
			debug( prg, REALM_BYTECODE, "IN_TST_NOT_EQL_VAL\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			value_t val = o1 != o2 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			break;
		}
		case IN_TST_LESS_VAL: {
			debug( prg, REALM_BYTECODE, "IN_TST_LESS_VAL\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			value_t res = (long)o1 < (long)o2 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( res );
			break;
		}
		case IN_TST_LESS_TREE: {
			debug( prg, REALM_BYTECODE, "IN_TST_LESS_TREE\n" );

			tree_t *o2 = vm_pop_tree();
			tree_t *o1 = vm_pop_tree();
			long r = colm_cmp_tree( prg, o1, o2 );
			value_t val = r < 0 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			colm_tree_downref( prg, sp, o1 );
			colm_tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_LESS_EQL_VAL: {
			debug( prg, REALM_BYTECODE, "IN_TST_LESS_EQL_VAL\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			value_t val = (long)o1 <= (long)o2 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			break;
		}
		case IN_TST_LESS_EQL_TREE: {
			debug( prg, REALM_BYTECODE, "IN_TST_LESS_EQL_TREE\n" );

			tree_t *o2 = vm_pop_tree();
			tree_t *o1 = vm_pop_tree();
			long r = colm_cmp_tree( prg, o1, o2 );
			value_t val = r <= 0 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			colm_tree_downref( prg, sp, o1 );
			colm_tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_GRTR_VAL: {
			debug( prg, REALM_BYTECODE, "IN_TST_GRTR_VAL\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			value_t val = (long)o1 > (long)o2 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			break;
		}
		case IN_TST_GRTR_TREE: {
			debug( prg, REALM_BYTECODE, "IN_TST_GRTR_TREE\n" );

			tree_t *o2 = vm_pop_tree();
			tree_t *o1 = vm_pop_tree();
			long r = colm_cmp_tree( prg, o1, o2 );
			value_t val = r > 0 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			colm_tree_downref( prg, sp, o1 );
			colm_tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_GRTR_EQL_VAL: {
			debug( prg, REALM_BYTECODE, "IN_TST_GRTR_EQL_VAL\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();

			value_t val = (long)o1 >= (long)o2 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			break;
		}
		case IN_TST_GRTR_EQL_TREE: {
			debug( prg, REALM_BYTECODE, "IN_TST_GRTR_EQL_TREE\n" );

			tree_t *o2 = vm_pop_tree();
			tree_t *o1 = vm_pop_tree();
			long r = colm_cmp_tree( prg, o1, o2 );
			value_t val = r >= 0 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			colm_tree_downref( prg, sp, o1 );
			colm_tree_downref( prg, sp, o2 );
			break;
		}
		case IN_TST_LOGICAL_AND: {
			debug( prg, REALM_BYTECODE, "IN_TST_LOGICAL_AND\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			value_t val = o1 && o2 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			break;
		}
		case IN_TST_LOGICAL_OR: {
			debug( prg, REALM_BYTECODE, "IN_TST_LOGICAL_OR\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			value_t val = o1 || o2 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			break;
		}

		case IN_TST_NZ_TREE: {
			debug( prg, REALM_BYTECODE, "IN_TST_NZ_TREE\n" );

			tree_t *tree = vm_pop_tree();
			long r = !testFalse( prg, tree );
			colm_tree_downref( prg, sp, tree );
			vm_push_value( r );
			break;
		}
		
		case IN_NOT_VAL: {
			debug( prg, REALM_BYTECODE, "IN_NOT_VAL\n" );

			value_t o1 = vm_pop_value();
			value_t val = o1 == 0 ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			break;
		}

		case IN_NOT_TREE: {
			debug( prg, REALM_BYTECODE, "IN_NOT_TREE\n" );

			tree_t *tree = vm_pop_tree();
			long r = testFalse( prg, tree );
			value_t val = r ? TRUE_VAL : FALSE_VAL;
			vm_push_value( val );
			colm_tree_downref( prg, sp, tree );
			break;
		}

		case IN_ADD_INT: {
			debug( prg, REALM_BYTECODE, "IN_ADD_INT\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			long r = (long)o1 + (long)o2;
			value_t val = r;
			vm_push_value( val );
			break;
		}
		case IN_MULT_INT: {
			debug( prg, REALM_BYTECODE, "IN_MULT_INT\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			long r = (long)o1 * (long)o2;
			value_t val = r;
			vm_push_value( val );
			break;
		}
		case IN_DIV_INT: {
			debug( prg, REALM_BYTECODE, "IN_DIV_INT\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			long r = (long)o1 / (long)o2;
			value_t val = r;
			vm_push_value( val );
			break;
		}
		case IN_SUB_INT: {
			debug( prg, REALM_BYTECODE, "IN_SUB_INT\n" );

			value_t o2 = vm_pop_value();
			value_t o1 = vm_pop_value();
			long r = (long)o1 - (long)o2;
			value_t val = r;
			vm_push_value( val );
			break;
		}
		case IN_DUP_VAL: {
			debug( prg, REALM_BYTECODE, "IN_DUP_VAL\n" );

			word_t val = (word_t)vm_top();
			vm_push_type( word_t, val );
			break;
		}
		case IN_DUP_TREE: {
			debug( prg, REALM_BYTECODE, "IN_DUP_TREE\n" );

			tree_t *val = vm_top();
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_TRITER_FROM_REF: {
			short field;
			half_t argSize;
			half_t searchTypeId;
			read_half( field );
			read_half( argSize );
			read_half( searchTypeId );

			debug( prg, REALM_BYTECODE, "IN_TRITER_FROM_REF "
					"%hd %hd %hd\n", field, argSize, searchTypeId );

			ref_t rootRef;
			rootRef.kid = vm_pop_kid();
			rootRef.next = vm_pop_ref();
			void *mem = vm_get_plocal(exec, field);

			tree_t **stackRoot = vm_ptop();
			long rootSize = vm_ssize();

			colm_init_tree_iter( (tree_iter_t*)mem, stackRoot,
					argSize, rootSize, &rootRef, searchTypeId );
			break;
		}
		case IN_TRITER_UNWIND:
		case IN_TRITER_DESTROY: {
			short field;
			read_half( field );

			tree_iter_t *iter = (tree_iter_t*) vm_get_plocal(exec, field);
			debug( prg, REALM_BYTECODE, "IN_TRITER_DESTROY %hd %d\n",
					field, iter->yieldSize );
			colm_tree_iter_destroy( prg, &sp, iter );
			break;
		}
		case IN_REV_TRITER_FROM_REF: {
			short field;
			half_t argSize;
			half_t searchTypeId;
			read_half( field );
			read_half( argSize );
			read_half( searchTypeId );

			debug( prg, REALM_BYTECODE, "IN_REV_TRITER_FROM_REF "
					"%hd %hd %hd\n", field, argSize, searchTypeId );

			ref_t rootRef;
			rootRef.kid = vm_pop_kid();
			rootRef.next = vm_pop_ref();

			tree_t **stackRoot = vm_ptop();
			long rootSize = vm_ssize();
			
			int children = 0;
			kid_t *kid = treeChild( prg, rootRef.kid->tree );
			while ( kid != 0 ) {
				vm_push_kid( kid );
				kid = kid->next;
				children++;
			}

			void *mem = vm_get_plocal(exec, field);
			colm_init_rev_tree_iter( (rev_tree_iter_t*)mem, stackRoot,
					argSize, rootSize, &rootRef, searchTypeId, children );
			break;
		}
		case IN_REV_TRITER_UNWIND:
		case IN_REV_TRITER_DESTROY: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REV_TRITER_DESTROY\n" );

			rev_tree_iter_t *iter = (rev_tree_iter_t*) vm_get_plocal(exec, field);
			colm_rev_tree_iter_destroy( prg, &sp, iter );
			break;
		}
		case IN_TREE_SEARCH: {
			word_t id;
			read_word( id );

			debug( prg, REALM_BYTECODE, "IN_TREE_SEARCH\n" );

			tree_t *tree = vm_pop_tree();
			tree_t *res = treeSearch( prg, tree, id );
			colm_tree_upref( res );
			vm_push_tree( res );
			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_TRITER_ADVANCE: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_ADVANCE\n" );

			tree_iter_t *iter = (tree_iter_t*) vm_get_plocal(exec, field);
			tree_t *res = treeIterAdvance( prg, &sp, iter );
			//colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_TRITER_NEXT_CHILD: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_NEXT_CHILD\n" );

			tree_iter_t *iter = (tree_iter_t*) vm_get_plocal(exec, field);
			tree_t *res = treeIterNextChild( prg, &sp, iter );
			//colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_REV_TRITER_PREV_CHILD: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REV_TRITER_PREV_CHILD\n" );

			rev_tree_iter_t *iter = (rev_tree_iter_t*) vm_get_plocal(exec, field);
			tree_t *res = treeRevIterPrevChild( prg, &sp, iter );
			//colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_TRITER_NEXT_REPEAT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_NEXT_REPEAT\n" );

			tree_iter_t *iter = (tree_iter_t*) vm_get_plocal(exec, field);
			tree_t *res = treeIterNextRepeat( prg, &sp, iter );
			//colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_TRITER_PREV_REPEAT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_PREV_REPEAT\n" );

			tree_iter_t *iter = (tree_iter_t*) vm_get_plocal(exec, field);
			tree_t *res = treeIterPrevRepeat( prg, &sp, iter );
			//colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_TRITER_GET_CUR_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_GET_CUR_R\n" );
			
			tree_iter_t *iter = (tree_iter_t*) vm_get_plocal(exec, field);
			tree_t *tree = treeIterDerefCur( iter );
			colm_tree_upref( tree );
			vm_push_tree( tree );
			break;
		}
		case IN_TRITER_GET_CUR_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_GET_CUR_WC\n" );
			
			tree_iter_t *iter = (tree_iter_t*) vm_get_plocal(exec, field);
			splitIterCur( prg, &sp, iter );
			tree_t *tree = treeIterDerefCur( iter );
			colm_tree_upref( tree );
			vm_push_tree( tree );
			break;
		}
		case IN_TRITER_SET_CUR_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_SET_CUR_WC\n" );

			tree_t *tree = vm_pop_tree();
			tree_iter_t *iter = (tree_iter_t*) vm_get_plocal(exec, field);
			splitIterCur( prg, &sp, iter );
			tree_t *old = treeIterDerefCur( iter );
			setTriterCur( prg, iter, tree );
			colm_tree_downref( prg, sp, old );
			break;
		}
		case IN_GEN_ITER_FROM_REF: {
			short field;
			half_t argSize;
			half_t genericId;
			read_half( field );
			read_half( argSize );
			read_half( genericId );

			debug( prg, REALM_BYTECODE, "IN_LIST_ITER_FROM_REF "
					"%hd %hd %hd\n", field, argSize, genericId );

			ref_t rootRef;
			rootRef.kid = vm_pop_kid();
			rootRef.next = vm_pop_ref();
			void *mem = vm_get_plocal(exec, field);

			tree_t **stackRoot = vm_ptop();
			long rootSize = vm_ssize();

			colm_init_list_iter( (generic_iter_t*)mem, stackRoot, argSize,
				rootSize, &rootRef, genericId );
			break;
		}
		case IN_GEN_ITER_UNWIND:
		case IN_GEN_ITER_DESTROY: {
			short field;
			read_half( field );

			generic_iter_t *iter = (generic_iter_t*) vm_get_plocal(exec, field);

			debug( prg, REALM_BYTECODE, "IN_LIST_ITER_DESTROY %d\n", iter->yieldSize );

			colm_list_iter_destroy( prg, &sp, iter );
			break;
		}
		case IN_LIST_ITER_ADVANCE: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_LIST_ITER_ADVANCE\n" );

			generic_iter_t *iter = (generic_iter_t*) vm_get_plocal(exec, field);
			tree_t *res = colm_list_iter_advance( prg, &sp, iter );
			//colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_MAP_ITER_ADVANCE: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_MAP_ITER_ADVANCE\n" );

			generic_iter_t *iter = (generic_iter_t*) vm_get_plocal(exec, field);
			tree_t *res = colm_map_iter_advance( prg, &sp, iter );
			//colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_GEN_ITER_GET_CUR_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GEN_ITER_GET_CUR_R\n" );
			
			generic_iter_t *iter = (generic_iter_t*) vm_get_plocal(exec, field);
			tree_t *tree = colm_list_iter_deref_cur( prg, iter );
			//colm_tree_upref( tree );
			vm_push_tree( tree );
			break;
		}
		case IN_GEN_VITER_GET_CUR_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GEN_VITER_GET_CUR_R\n" );
			
			generic_iter_t *iter = (generic_iter_t*) vm_get_plocal(exec, field);
			value_t value = colm_viter_deref_cur( prg, iter );
			vm_push_value( value );
			break;
		}
		case IN_MATCH: {
			half_t patternId;
			read_half( patternId );

			debug( prg, REALM_BYTECODE, "IN_MATCH\n" );

			tree_t *tree = vm_pop_tree();

			/* Run the match, push the result. */
			int rootNode = prg->rtd->patReplInfo[patternId].offset;

			/* Bindings are indexed starting at 1. Zero bindId to represent no
			 * binding. We make a space for it here rather than do math at
			 * access them. */
			long numBindings = prg->rtd->patReplInfo[patternId].numBindings;
			tree_t *bindings[1+numBindings];
			memset( bindings, 0, sizeof(tree_t*)*(1+numBindings) );

			kid_t kid;
			kid.tree = tree;
			kid.next = 0;
			int matched = matchPattern( bindings, prg, rootNode, &kid, false );

			if ( !matched )
				memset( bindings, 0, sizeof(tree_t*)*(1+numBindings) );
			else {
				int b;
				for ( b = 1; b <= numBindings; b++ )
					assert( bindings[b] != 0 );
			}

			tree_t *result = matched ? tree : 0;
			colm_tree_upref( result );
			vm_push_tree( result ? tree : 0 );
			int b;
			for ( b = 1; b <= numBindings; b++ ) {
				colm_tree_upref( bindings[b] );
				vm_push_tree( bindings[b] );
			}

			colm_tree_downref( prg, sp, tree );
			break;
		}

		case IN_PARSE_APPEND_WC: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_APPEND_WC\n" );

			tree_t *input = vm_pop_tree();
			parser_t *parser = vm_pop_parser();

			stream_append_tree( prg, sp, parser->input, input );

			vm_push_parser( parser );
			colm_tree_downref( prg, sp, input );
			break;
		}

		case IN_PARSE_APPEND_WV: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_APPEND_WV\n" );

			tree_t *input = vm_pop_tree();
			parser_t *parser = vm_pop_parser();

			word_t len = stream_append_tree( prg, sp, parser->input, input );

			vm_push_parser( parser );

			rcode_unit_start( exec );
			rcode_code( exec, IN_PARSE_APPEND_BKT );
			rcode_word( exec, (word_t) parser );
			rcode_word( exec, (word_t) input );
			rcode_word( exec, (word_t) len );
			rcode_unit_term( exec );
			break;
		}

		case IN_PARSE_APPEND_BKT: {
			tree_t *pptr;
			tree_t *input;
			word_t len;
			read_tree( pptr );
			read_tree( input );
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_PARSE_APPEND_BKT\n" );

			struct stream_impl *si = streamToImpl( ((parser_t*)pptr)->input );
			stream_undo_append( prg, sp, si, input, len );

			colm_tree_downref( prg, sp, input );
			break;
		}

		case IN_PARSE_APPEND_STREAM_WC: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_APPEND_STREAM_WC\n" );

			tree_t *input = vm_pop_tree();
			parser_t *parser = vm_pop_parser();

			vm_push_parser( parser );

			stream_append_stream( prg, sp, parser->input, input );
			break;
		}
		case IN_PARSE_APPEND_STREAM_WV: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_APPEND_STREAM_WV\n" );

			tree_t *input = vm_pop_tree();
			parser_t *parser = vm_pop_parser();

			word_t len = stream_append_stream( prg, sp, parser->input, input );

			vm_push_parser( parser );

			rcode_unit_start( exec );
			rcode_code( exec, IN_PARSE_APPEND_STREAM_BKT );
			rcode_word( exec, (word_t) parser );
			rcode_word( exec, (word_t) input );
			rcode_word( exec, (word_t) len );
			rcode_unit_term( exec );
			break;
		}

		case IN_PARSE_APPEND_STREAM_BKT: {
			tree_t *pptr;
			tree_t *input;
			word_t len;
			read_tree( pptr );
			read_tree( input );
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_PARSE_APPEND_STREAM_BKT\n" );

			struct stream_impl *si = streamToImpl( ((parser_t*)pptr)->input );
			stream_undo_append_stream( prg, sp, si, input, len );

			colm_tree_downref( prg, sp, input );
			break;
		}

		case IN_INPUT_CLOSE_WC: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_CLOSE_WC\n" );

			stream_t *stream = vm_pop_stream();
			struct stream_impl *si = stream->impl;

			if ( si->file != 0 ) {
				fclose( si->file );
				si->file = 0;
			}
			else if ( si->fd >= 0 ) {
				close( si->fd );
				si->fd = -1;
			}

			vm_push_stream( stream );
			break;
		}

		case IN_SET_ERROR: {
			debug( prg, REALM_BYTECODE, "IN_SET_ERROR\n" );

			tree_t *error = vm_pop_tree();
			colm_tree_downref( prg, sp, prg->error );
			prg->error = error;
			break;
		}

		case IN_GET_ERROR: {
			debug( prg, REALM_BYTECODE, "IN_GET_ERROR\n" );

			vm_pop_tree();
			colm_tree_upref( prg->error );
			vm_push_tree( prg->error );
			break;
		}

		case IN_PARSE_LOAD: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_LOAD\n" );

			parser_t *parser = vm_pop_parser();
			struct pda_run *pdaRun = parser->pdaRun;
			long steps = pdaRun->steps;

			vm_push_parser( exec->parser );
			vm_push_type( long, exec->pcr );
			vm_push_type( long, exec->steps );

			exec->parser = parser;
			exec->steps = steps;
			exec->pcr = PCR_START;
			break;
		}

		case IN_PARSE_INIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_INIT_BKT\n" );

			parser_t *parser;
			word_t pcr;
			word_t steps;

			read_parser( parser );
			read_word( pcr );
			read_word( steps );

			vm_push_parser( exec->parser );
			vm_push_value( exec->pcr );
			vm_push_value( exec->steps );

			exec->parser = parser;
			exec->steps = steps;
			exec->pcr = pcr;
			break;
		}

		case IN_PCR_CALL: {
			debug( prg, REALM_BYTECODE, "IN_PCR_CALL\n" );

			int frameSize = 0;
			if ( exec->parser->pdaRun->frameId >= 0 )  {
				struct frame_info *fi = &prg->rtd->frameInfo[exec->parser->pdaRun->frameId];
				frameSize = fi->frameSize;
			}

			vm_contiguous( 4 + frameSize );

			vm_push_type( tree_t**, exec->framePtr );
			vm_push_type( tree_t**, exec->iframePtr );
			vm_push_type( long, exec->frameId );

			/* Return location one instruction back. Depends on the size of of
			 * the frag/finish. */
			code_t *returnTo = instr - ( SIZEOF_CODE + SIZEOF_CODE + SIZEOF_HALF );
			vm_push_type( code_t*, returnTo );

			exec->framePtr = 0;
			exec->iframePtr = 0;
			exec->frameId = 0;

			instr = exec->parser->pdaRun->code;

			exec->frameId = exec->parser->pdaRun->frameId;

			if ( exec->parser->pdaRun->frameId >= 0 )  {
				struct frame_info *fi = &prg->rtd->frameInfo[exec->parser->pdaRun->frameId];

				exec->framePtr = vm_ptop();
				vm_pushn( fi->frameSize );
				memset( vm_ptop(), 0, sizeof(word_t) * fi->frameSize );
			}
			break;
		}

		case IN_LOAD_RETVAL: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_RETVAL\n" );
			vm_push_tree( exec->retVal );
			break;
		}

		case IN_PCR_RET: {
			debug( prg, REALM_BYTECODE, "IN_PCR_RET\n" );

			if ( exec->frameId >= 0 ) {
				struct frame_info *fi = &prg->rtd->frameInfo[exec->frameId];
				downref_local_trees( prg, sp, exec, fi->locals, fi->localsLen );
				debug( prg, REALM_BYTECODE, "RET: %d\n", fi->frameSize );

				vm_popn( fi->frameSize );
			}

			instr = vm_pop_type(code_t*);
			exec->frameId =   vm_pop_type(long);
			exec->iframePtr = vm_pop_type(tree_t**);
			exec->framePtr =  vm_pop_type(tree_t**);

			if ( instr == 0 ) {
				fflush( stdout );
				goto out;
			}
			break;
		}

		case IN_PCR_END_DECK: {
			debug( prg, REALM_BYTECODE, "IN_PCR_END_DECK\n" );
			exec->parser->pdaRun->onDeck = false;
			break;
		}

		case IN_PARSE_FRAG_WC: {
			half_t stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_WC %hd\n", stopId );

			exec->pcr = colm_parse_frag( prg, sp, exec->parser->pdaRun,
					exec->parser->input, stopId, exec->pcr );

			/* If done, jump to the terminating instruction, otherwise fall
			 * through to call some code, then jump back here. */
			if ( exec->pcr == PCR_DONE )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FRAG_EXIT_WC: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_EXIT_WC\n" );

			parser_t *parser = exec->parser;

			exec->steps = vm_pop_type(long);
			exec->pcr = vm_pop_type(long);
			exec->parser = vm_pop_parser();

			vm_push_parser( parser );

			if ( prg->induceExit )
				goto out;

			break;
		}

		case IN_PARSE_FRAG_WV: {
			half_t stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_WV %hd\n", stopId );

			exec->pcr = colm_parse_frag( prg, sp, exec->parser->pdaRun,
					exec->parser->input, stopId, exec->pcr );

			/* If done, jump to the terminating instruction, otherwise fall
			 * through to call some code, then jump back here. */
			if ( exec->pcr == PCR_DONE )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FRAG_EXIT_WV: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_EXIT_WV \n" );

			parser_t *parser = exec->parser;
			long steps = exec->steps;

			exec->steps = vm_pop_type(long);
			exec->pcr = vm_pop_type(long);
			exec->parser = vm_pop_parser();

			vm_push_parser( parser );

			rcode_unit_start( exec );
			rcode_code( exec, IN_PARSE_INIT_BKT );
			rcode_word( exec, (word_t) parser );
			rcode_word( exec, (word_t) PCR_START );
			rcode_word( exec, steps );
			rcode_code( exec, IN_PARSE_FRAG_BKT );
			rcodeHalf( exec, 0 );
			rcode_code( exec, IN_PCR_CALL );
			rcode_code( exec, IN_PARSE_FRAG_EXIT_BKT );
			rcode_unit_term( exec );

			if ( prg->induceExit )
				goto out;
			break;
		}

		case IN_PARSE_FRAG_BKT: {
			half_t stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_BKT %hd\n", stopId );

			exec->pcr = colm_parse_undo_frag( prg, sp, exec->parser->pdaRun,
					exec->parser->input, exec->steps, exec->pcr );

			if ( exec->pcr == PCR_DONE )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FRAG_EXIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_EXIT_BKT\n" );

			exec->steps = vm_pop_type(long);
			exec->pcr = vm_pop_type(long);
			exec->parser = vm_pop_parser();

			break;
		}

		case IN_PARSE_FINISH_WC: {
			half_t stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_WC %hd\n", stopId );

			tree_t *result = 0;
			exec->pcr = colm_parse_finish( &result, prg, sp,
					exec->parser->pdaRun, exec->parser->input, false, exec->pcr );

			exec->parser->result = result;

			/* If done, jump to the terminating instruction, otherwise fall
			 * through to call some code, then jump back here. */
			if ( exec->pcr == PCR_DONE )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FINISH_EXIT_WC: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_EXIT_WC\n" );

			parser_t *parser = exec->parser;

			exec->steps = vm_pop_type(long);
			exec->pcr = vm_pop_type(long);
			exec->parser = vm_pop_parser();

			vm_push_parser( parser );

			if ( prg->induceExit )
				goto out;

			break;
		}

		case IN_PARSE_FINISH_WV: {
			half_t stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_WV %hd\n", stopId );

			tree_t *result = 0;
			exec->pcr = colm_parse_finish( &result, prg, sp, exec->parser->pdaRun,
					exec->parser->input, true, exec->pcr );

			exec->parser->result = result;

			if ( exec->pcr == PCR_DONE )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FINISH_EXIT_WV: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_EXIT_WV\n" );

			parser_t *parser = exec->parser;
			long steps = exec->steps;

			exec->steps = vm_pop_type(long);
			exec->pcr = vm_pop_type(long);
			exec->parser = vm_pop_parser();

			vm_push_parser( parser );

			rcode_unit_start( exec );
			rcode_code( exec, IN_PARSE_INIT_BKT );
			rcode_word( exec, (word_t)parser );
			rcode_word( exec, (word_t)PCR_START );
			rcode_word( exec, steps );
			rcode_code( exec, IN_PARSE_FINISH_BKT );
			rcodeHalf( exec, 0 );
			rcode_code( exec, IN_PCR_CALL );
			rcode_code( exec, IN_PARSE_FINISH_EXIT_BKT );
			rcode_unit_term( exec );

			if ( prg->induceExit )
				goto out;

			break;
		}

		case IN_PARSE_FINISH_BKT: {
			half_t stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_BKT %hd\n", stopId );

			exec->pcr = colm_parse_undo_frag( prg, sp, exec->parser->pdaRun,
					exec->parser->input, exec->steps, exec->pcr );

			if ( exec->pcr == PCR_DONE )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FINISH_EXIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_EXIT_BKT\n" );

			parser_t *parser = exec->parser;

			exec->steps = vm_pop_type(long);
			exec->pcr = vm_pop_type(long);
			exec->parser = vm_pop_parser();

			struct stream_impl *si = streamToImpl( parser->input );
			si->funcs->unsetEof( si );
			break;
		}

		case IN_INPUT_PULL_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PULL_WV\n" );

			stream_t *stream = vm_pop_stream();
			tree_t *len = vm_pop_tree();
			struct pda_run *pdaRun = exec->parser != 0 ? exec->parser->pdaRun : 0;
			tree_t *string = stream_pull_bc( prg, sp, pdaRun, stream, len );
			colm_tree_upref( string );
			vm_push_tree( string );

			/* Single unit. */
			colm_tree_upref( string );
			rcode_code( exec, IN_INPUT_PULL_BKT );
			rcode_word( exec, (word_t) string );
			rcode_unit_term( exec );

			//colm_tree_downref( prg, sp, len );
			break;
		}

		case IN_INPUT_PULL_WC: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PULL_WC\n" );

			stream_t *stream = vm_pop_stream();
			tree_t *len = vm_pop_tree();
			struct pda_run *pdaRun = exec->parser != 0 ? exec->parser->pdaRun : 0;
			tree_t *string = stream_pull_bc( prg, sp, pdaRun, stream, len );
			colm_tree_upref( string );
			vm_push_tree( string );

			//colm_tree_downref( prg, sp, len );
			break;
		}
		case IN_INPUT_PULL_BKT: {
			tree_t *string;
			read_tree( string );

			stream_t *stream = vm_pop_stream();

			debug( prg, REALM_BYTECODE, "IN_INPUT_PULL_BKT\n" );

			undo_pull( prg, stream, string );
			colm_tree_downref( prg, sp, string );
			break;
		}
		case IN_INPUT_PUSH_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_WV\n" );

			stream_t *input = vm_pop_stream();
			tree_t *tree = vm_pop_tree();
			long len = stream_push( prg, sp, streamToImpl( input ), tree, false );
			vm_push_tree( 0 );

			/* Single unit. */
			rcode_code( exec, IN_INPUT_PUSH_BKT );
			rcode_word( exec, len );
			rcode_unit_term( exec );

			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_INPUT_PUSH_IGNORE_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_IGNORE_WV\n" );

			stream_t *input = vm_pop_stream();
			tree_t *tree = vm_pop_tree();
			long len = stream_push( prg, sp, streamToImpl( input ), tree, true );
			vm_push_tree( 0 );

			/* Single unit. */
			rcode_code( exec, IN_INPUT_PUSH_BKT );
			rcode_word( exec, len );
			rcode_unit_term( exec );

			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_INPUT_PUSH_BKT: {
			word_t len;
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_BKT %d\n", len );

			stream_t *input = vm_pop_stream();
			colm_undo_stream_push( prg, sp, streamToImpl( input ), len );
			break;
		}
		case IN_INPUT_PUSH_STREAM_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_STREAM_WV\n" );

			stream_t *input = vm_pop_stream();
			stream_t *toPush = vm_pop_stream();
			long len = stream_push_stream( prg, sp, streamToImpl( input ), toPush );
			vm_push_tree( 0 );

			/* Single unit. */
			rcode_code( exec, IN_INPUT_PUSH_BKT );
			rcode_word( exec, len );
			rcode_unit_term( exec );
			break;
		}
		case IN_INPUT_PUSH_STREAM_BKT: {
			word_t len;
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_STREAM_BKT %d\n", len );

			stream_t *input = vm_pop_stream();
			colm_undo_stream_push( prg, sp, streamToImpl( input ), len );
			break;
		}
		case IN_CONS_GENERIC: {
			half_t genericId;
			read_half( genericId );

			debug( prg, REALM_BYTECODE, "IN_CONS_GENERIC %hd\n", genericId );

			struct_t *gen = colm_construct_generic( prg, genericId );
			vm_push_struct( gen );
			break;
		}
		case IN_CONS_OBJECT: {
			half_t langElId;
			read_half( langElId );

			debug( prg, REALM_BYTECODE, "IN_CONS_OBJECT %hd\n", langElId );

			tree_t *replTree = colm_construct_object( prg, 0, 0, langElId );
			vm_push_tree( replTree );
			break;
		}
		case IN_CONSTRUCT: {
			half_t patternId;
			read_half( patternId );

			debug( prg, REALM_BYTECODE, "IN_CONSTRUCT\n" );

			//struct lang_el_info *lelInfo = prg->rtd->lelInfo;
			//struct pat_cons_node *nodes = prg->rtd->patReplNodes;
			int rootNode = prg->rtd->patReplInfo[patternId].offset;

			/* Note that bindIds are indexed at one. Add one spot for them. */
			int numBindings = prg->rtd->patReplInfo[patternId].numBindings;
			tree_t *bindings[1+numBindings];

			int b;
			for ( b = 1; b <= numBindings; b++ ) {
				bindings[b] = vm_pop_tree();
				assert( bindings[b] != 0 );
			}

			tree_t *replTree = colm_construct_tree( prg, 0, bindings, rootNode );

			vm_push_tree( replTree );
			break;
		}
		case IN_CONSTRUCT_TERM: {
			half_t tokenId;
			read_half( tokenId );

			debug( prg, REALM_BYTECODE, "IN_CONSTRUCT_TERM\n" );

			/* Pop the string we are constructing the token from. */
			str_t *str = vm_pop_string();
			tree_t *res = colm_construct_term( prg, tokenId, str->value );
			colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_MAKE_TOKEN: {
			uchar nargs;
			int i;
			read_byte( nargs );

			debug( prg, REALM_BYTECODE, "IN_MAKE_TOKEN\n" );

			tree_t *arg[nargs];
			for ( i = nargs-1; i >= 0; i-- )
				arg[i] = vm_pop_tree();

			tree_t *result = colm_construct_token( prg, arg, nargs );
			for ( i = 1; i < nargs; i++ )
				colm_tree_downref( prg, sp, arg[i] );
			vm_push_tree( result );
			break;
		}
		case IN_MAKE_TREE: {
			uchar nargs;
			int i;
			read_byte( nargs );

			debug( prg, REALM_BYTECODE, "IN_MAKE_TREE\n" );

			tree_t *arg[nargs];
			for ( i = nargs-1; i >= 0; i-- )
				arg[i] = vm_pop_tree();

			tree_t *result = makeTree( prg, arg, nargs );
			for ( i = 1; i < nargs; i++ )
				colm_tree_downref( prg, sp, arg[i] );

			vm_push_tree( result );
			break;
		}
		case IN_TREE_CAST: {
			half_t langElId;
			read_half( langElId );

			debug( prg, REALM_BYTECODE, "IN_TREE_CAST %hd\n", langElId );

			tree_t *tree = vm_pop_tree();
			tree_t *res = castTree( prg, langElId, tree );
			colm_tree_upref( res );
			colm_tree_downref( prg, sp, tree );
			vm_push_tree( res );
			break;
		}
		case IN_PTR_ACCESS_WV: {
			debug( prg, REALM_BYTECODE, "IN_PTR_ACCESS_WV\n" );

			struct_t *ptr = vm_pop_struct();
			vm_push_struct( ptr );

			/* This is an initial global load. Need to reverse execute it. */
			rcode_unit_start( exec );
			rcode_code( exec, IN_PTR_ACCESS_BKT );
			rcode_word( exec, (word_t) ptr );
			break;
		}
		case IN_PTR_ACCESS_BKT: {
			word_t p;
			read_word( p );

			debug( prg, REALM_BYTECODE, "IN_PTR_ACCESS_BKT\n" );

			struct_t *ptr = (struct_t*)p;
			vm_push_type( struct_t *, ptr );
			break;
		}
		case IN_REF_FROM_LOCAL: {
			short int field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REF_FROM_LOCAL %hd\n", field );

			/* First push the null next pointer, then the kid pointer. */
			kid_t *kid = (kid_t*)vm_get_plocal(exec, field);
			vm_contiguous( 2 );
			vm_push_ref( 0 );
			vm_push_kid( kid );
			break;
		}
		case IN_REF_FROM_REF: {
			short int field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REF_FROM_REF %hd\n", field );

			ref_t *ref = (ref_t*)vm_get_plocal(exec, field);
			vm_contiguous( 2 );
			vm_push_ref( ref );
			vm_push_kid( ref->kid );
			break;
		}
		case IN_REF_FROM_QUAL_REF: {
			short int back;
			short int field;
			read_half( back );
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REF_FROM_QUAL_REF\n" );

			ref_t *ref = (ref_t*)(sp + back);

			tree_t *obj = ref->kid->tree;
			kid_t *attr_kid = getFieldKid( obj, field );

			vm_contiguous( 2 );
			vm_push_ref( ref );
			vm_push_kid( attr_kid );
			break;
		}
		case IN_RHS_REF_FROM_QUAL_REF: {
			short int back;
			int i, done = 0;
			uchar len;

			read_half( back );

			debug( prg, REALM_BYTECODE, "IN_RHS_REF_FROM_QUAL_REF\n" );

			ref_t *ref = (ref_t*)(sp + back);

			tree_t *obj = ref->kid->tree;
			kid_t *attrKid = 0;

			read_byte( len );
			for ( i = 0; i < len; i++ ) {
				uchar prodNum, childNum;
				read_byte( prodNum );
				read_byte( childNum );
				if ( !done && obj->prod_num == prodNum ) {
					attrKid = getRhsElKid( prg, obj, childNum );
					done = 1;
				}
			}

			vm_contiguous( 2 );
			vm_push_ref( ref );
			vm_push_kid( attrKid );
			break;
		}
		case IN_REF_FROM_BACK: {
			short int back;
			read_half( back );

			debug( prg, REALM_BYTECODE, "IN_REF_FROM_BACK %hd\n", back );

			kid_t *ptr = (kid_t*)(sp + back);

			vm_contiguous( 2 );
			vm_push_ref( 0 );
			vm_push_kid( ptr );
			break;
		}
		case IN_TRITER_REF_FROM_CUR: {
			short int field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_REF_FROM_CUR\n" );

			/* Push the next pointer first, then the kid. */
			tree_iter_t *iter = (tree_iter_t*) vm_get_plocal(exec, field);
			ref_t *ref = &iter->ref;
			vm_contiguous( 2 );
			vm_push_ref( ref );
			vm_push_kid( iter->ref.kid );
			break;
		}
		case IN_UITER_REF_FROM_CUR: {
			short int field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_REF_FROM_CUR\n" );

			/* Push the next pointer first, then the kid. */
			user_iter_t *uiter = (user_iter_t*) vm_get_local(exec, field);
			vm_contiguous( 2 );
			vm_push_ref( uiter->ref.next );
			vm_push_kid( uiter->ref.kid );
			break;
		}
		case IN_GET_TOKEN_DATA_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_TOKEN_DATA_R\n" );

			tree_t *tree = vm_pop_tree();
			head_t *data = stringCopy( prg, tree->tokdata );
			tree_t *str = constructString( prg, data );
			colm_tree_upref( str );
			vm_push_tree( str );
			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_SET_TOKEN_DATA_WC: {
			debug( prg, REALM_BYTECODE, "IN_SET_TOKEN_DATA_WC\n" );

			tree_t *tree = vm_pop_tree();
			tree_t *val = vm_pop_tree();
			head_t *head = stringCopy( prg, ((str_t*)val)->value );
			stringFree( prg, tree->tokdata );
			tree->tokdata = head;

			colm_tree_downref( prg, sp, tree );
			colm_tree_downref( prg, sp, val );
			break;
		}
		case IN_SET_TOKEN_DATA_WV: {
			debug( prg, REALM_BYTECODE, "IN_SET_TOKEN_DATA_WV\n" );

			tree_t *tree = vm_pop_tree();
			tree_t *val = vm_pop_tree();

			head_t *oldval = tree->tokdata;
			head_t *head = stringCopy( prg, ((str_t*)val)->value );
			tree->tokdata = head;

			/* Set up reverse code. Needs no args. */
			rcode_code( exec, IN_SET_TOKEN_DATA_BKT );
			rcode_word( exec, (word_t)oldval );
			rcode_unit_term( exec );

			colm_tree_downref( prg, sp, tree );
			colm_tree_downref( prg, sp, val );
			break;
		}
		case IN_SET_TOKEN_DATA_BKT: {
			debug( prg, REALM_BYTECODE, "IN_SET_TOKEN_DATA_BKT \n" );

			word_t oldval;
			read_word( oldval );

			tree_t *tree = vm_pop_tree();
			head_t *head = (head_t*)oldval;
			stringFree( prg, tree->tokdata );
			tree->tokdata = head;
			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_GET_TOKEN_POS_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_TOKEN_POS_R\n" );

			tree_t *tree = vm_pop_tree();
			value_t integer = 0;
			if ( tree->tokdata->location )
				integer = tree->tokdata->location->byte;
			vm_push_value( integer );
			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_GET_TOKEN_LINE_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_TOKEN_LINE_R\n" );

			tree_t *tree = vm_pop_tree();
			value_t integer = 0;
			if ( tree->tokdata->location )
				integer = tree->tokdata->location->line;

			vm_push_value( integer );
			colm_tree_downref( prg, sp, tree );
			break;
		}
		case IN_GET_MATCH_LENGTH_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_MATCH_LENGTH_R\n" );

			value_t integer = stringLength(exec->parser->pdaRun->tokdata);
			vm_push_value( integer );
			break;
		}
		case IN_GET_MATCH_TEXT_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_MATCH_TEXT_R\n" );

			head_t *s = stringCopy( prg, exec->parser->pdaRun->tokdata );
			tree_t *tree = constructString( prg, s );
			colm_tree_upref( tree );
			vm_push_tree( tree );
			break;
		}
		case IN_LIST_LENGTH: {
			debug( prg, REALM_BYTECODE, "IN_LIST_LENGTH\n" );

			list_t *list = vm_pop_list();
			long len = colm_list_length( list );
			value_t res = len;
			colm_tree_downref( prg, sp, (tree_t*)list );
			vm_push_value( res );
			break;
		}
		case IN_GET_LIST_EL_MEM_R: {
			short genId, field;
			read_half( genId );
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_EL_MEM_R\n" );

			struct_t *s = vm_pop_struct();

			list_el_t *listEl = colm_struct_to_list_el( prg, s, genId );
			struct_t *val = colm_list_el_get( prg, listEl, genId, field );
			vm_push_struct( val );
			break;
		}
		case IN_GET_LIST_MEM_R: {
			short genId, field;
			read_half( genId );
			read_half( field );

			debug( prg, REALM_BYTECODE, 
					"IN_GET_LIST_MEM_R %hd %hd\n", genId, field );

			list_t *list = vm_pop_list();
			struct_t *val = colm_list_get( prg, list, genId, field );
			vm_push_struct( val );
			break;
		}
		case IN_GET_LIST_MEM_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_MEM_WC\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *val = getListMemSplit( prg, (list_t*)obj, field );
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_GET_LIST_MEM_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_MEM_WV\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *val = getListMemSplit( prg, (list_t*)obj, field );
			colm_tree_upref( val );
			vm_push_tree( val );

			/* Set up the reverse instruction. */
			rcode_code( exec, IN_GET_LIST_MEM_BKT );
			rcodeHalf( exec, field );
			break;
		}
		case IN_GET_LIST_MEM_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_MEM_BKT\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *res = getListMemSplit( prg, (list_t*)obj, field );
			colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_GET_VLIST_MEM_R: {
			short genId, field;
			read_half( genId );
			read_half( field );

			debug( prg, REALM_BYTECODE, 
					"IN_GET_VLIST_MEM_R %hd %hd\n", genId, field );

			list_t *list = vm_pop_list();
			struct_t *el = colm_list_get( prg, list, genId, field );

			value_t val = colm_struct_get_field( el, value_t, 0 );
			vm_push_value( val );
			break;
		}
		case IN_GET_VLIST_MEM_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_VLIST_MEM_WC\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *val = getListMemSplit( prg, (list_t*)obj, field );
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_GET_VLIST_MEM_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_VLIST_MEM_WV\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *val = getListMemSplit( prg, (list_t*)obj, field );
			colm_tree_upref( val );
			vm_push_tree( val );

			/* Set up the reverse instruction. */
			rcode_code( exec, IN_GET_LIST_MEM_BKT );
			rcodeHalf( exec, field );
			break;
		}
		case IN_GET_VLIST_MEM_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_VLIST_MEM_BKT\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *res = getListMemSplit( prg, (list_t*)obj, field );
			colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}
		case IN_GET_PARSER_MEM_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_PARSER_MEM_R %hd\n", field );

			tree_t *obj = vm_pop_tree();
			tree_t *val = getParserMem( (parser_t*)obj, field );
			colm_tree_upref( val );

			/* In at least one case we extract the result on a parser with ref
			 * one. Do it after. */
			colm_tree_downref( prg, sp, obj );
			vm_push_tree( val );
			break;
		}

		case IN_GET_MAP_EL_MEM_R: {
			short genId, field;
			read_half( genId );
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_MAP_EL_MEM_R\n" );

			struct_t *strct = vm_pop_struct();

			map_el_t *mapEl = colm_struct_to_map_el( prg, strct, genId );
			struct_t *val = colm_map_el_get( prg, mapEl, genId, field );
			vm_push_struct( val );
			break;
		}
		case IN_MAP_LENGTH: {
			debug( prg, REALM_BYTECODE, "IN_MAP_LENGTH\n" );

			tree_t *obj = vm_pop_tree();
			long len = mapLength( (map_t*)obj );
			value_t res = len;
			vm_push_value( res );

			colm_tree_downref( prg, sp, obj );
			break;
		}
		case IN_GET_MAP_MEM_R: {
			short genId, field;
			read_half( genId );
			read_half( field );

			debug( prg, REALM_BYTECODE, 
					"IN_GET_MAP_MEM_R %hd %hd\n", genId, field );

			map_t *map = vm_pop_map();
			struct_t *val = colm_map_get( prg, map, genId, field );
			vm_push_struct( val );
			break;
		}
		case IN_GET_MAP_MEM_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_MAP_MEM_WC\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *val = getListMemSplit( prg, (list_t*)obj, field );
			colm_tree_upref( val );
			vm_push_tree( val );
			break;
		}
		case IN_GET_MAP_MEM_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_MAP_MEM_WV\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *val = getListMemSplit( prg, (list_t*)obj, field );
			colm_tree_upref( val );
			vm_push_tree( val );

			/* Set up the reverse instruction. */
			rcode_code( exec, IN_GET_MAP_MEM_BKT );
			rcodeHalf( exec, field );
			break;
		}
		case IN_GET_MAP_MEM_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_MAP_MEM_BKT\n" );

			tree_t *obj = vm_pop_tree();
			colm_tree_downref( prg, sp, obj );

			tree_t *res = getListMemSplit( prg, (list_t*)obj, field );
			colm_tree_upref( res );
			vm_push_tree( res );
			break;
		}

		case IN_STASH_ARG: {
			half_t pos;
			half_t size;
			read_half( pos );
			read_half( size );

			debug( prg, REALM_BYTECODE, "IN_STASH_ARG %hd %hd\n", pos, size );

			while ( size > 0 ) {
				value_t v = vm_pop_value();
				((value_t*)exec->callArgs)[pos] = v;
				size -= 1;
				pos += 1;
			}

			break;
		}

		case IN_PREP_ARGS: {
			half_t size;
			read_half( size );

			debug( prg, REALM_BYTECODE, "IN_PREP_ARGS %hd\n", size );

			vm_push_type( tree_t**, exec->callArgs );
			vm_pushn( size );
			exec->callArgs = vm_ptop();
			memset( vm_ptop(), 0, sizeof(word_t) * size );
			break;
		}

		case IN_CLEAR_ARGS: {
			half_t size;
			read_half( size );

			debug( prg, REALM_BYTECODE, "IN_CLEAR_ARGS %hd\n", size );

			vm_popn( size );
			exec->callArgs = vm_pop_type( tree_t** );
			break;
		}

		case IN_HOST: {
			half_t funcId;
			read_half( funcId );

			debug( prg, REALM_BYTECODE, "IN_HOST %hd\n", funcId );

			sp = host_call( prg, funcId, sp );
			break;
		}
		case IN_CALL_WV: {
			half_t funcId;
			read_half( funcId );

			struct function_info *fi = &prg->rtd->functionInfo[funcId];
			struct frame_info *fr = &prg->rtd->frameInfo[fi->frameId];

			debug( prg, REALM_BYTECODE, "IN_CALL_WV %s\n", fr->name );

			vm_contiguous( FR_AA + fi->frameSize );

			vm_push_type( tree_t**, exec->callArgs );
			vm_push_value( 0 ); /* Return value. */
			vm_push_type( code_t*, instr );
			vm_push_type( tree_t**, exec->framePtr );
			vm_push_type( long, exec->frameId );

			instr = fr->codeWV;
			exec->frameId = fi->frameId;

			exec->framePtr = vm_ptop();
			vm_pushn( fr->frameSize );
			memset( vm_ptop(), 0, sizeof(word_t) * fr->frameSize );
			break;
		}
		case IN_CALL_WC: {
			half_t funcId;
			read_half( funcId );

			struct function_info *fi = &prg->rtd->functionInfo[funcId];
			struct frame_info *fr = &prg->rtd->frameInfo[fi->frameId];

			debug( prg, REALM_BYTECODE, "IN_CALL_WC %s %d\n", fr->name, fr->frameSize );

			vm_contiguous( FR_AA + fi->frameSize );

			vm_push_type( tree_t**, exec->callArgs );
			vm_push_value( 0 ); /* Return value. */
			vm_push_type( code_t*, instr );
			vm_push_type( tree_t**, exec->framePtr );
			vm_push_type( long, exec->frameId );

			instr = fr->codeWC;
			exec->frameId = fi->frameId;

			exec->framePtr = vm_ptop();
			vm_pushn( fr->frameSize );
			memset( vm_ptop(), 0, sizeof(word_t) * fr->frameSize );
			break;
		}
		case IN_YIELD: {
			debug( prg, REALM_BYTECODE, "IN_YIELD\n" );

			kid_t *kid = vm_pop_kid();
			ref_t *next = vm_pop_ref();
			user_iter_t *uiter = (user_iter_t*) vm_plocal_iframe( IFR_AA );

			if ( kid == 0 || kid->tree == 0 ||
					kid->tree->id == uiter->searchId || 
					uiter->searchId == prg->rtd->anyId )
			{
				/* Store the yeilded value. */
				uiter->ref.kid = kid;
				uiter->ref.next = next;
				uiter->yieldSize = vm_ssize() - uiter->rootSize;
				uiter->resume = instr;
				uiter->frame = exec->framePtr;

				/* Restore the instruction and frame pointer. */
				instr = (code_t*) vm_local_iframe(IFR_RIN);
				exec->framePtr = (tree_t**) vm_local_iframe(IFR_RFR);
				exec->iframePtr = (tree_t**) vm_local_iframe(IFR_RIF);

				/* Return the yield result on the top of the stack. */
				tree_t *result = uiter->ref.kid != 0 ? prg->trueVal : prg->falseVal;
				//colm_tree_upref( result );
				vm_push_tree( result );
			}
			break;
		}
		case IN_UITER_CREATE_WV: {
			short field;
			half_t funcId, searchId;
			read_half( field );
			read_half( funcId );
			read_half( searchId );

			debug( prg, REALM_BYTECODE, "IN_UITER_CREATE_WV\n" );

			struct function_info *fi = prg->rtd->functionInfo + funcId;

			vm_contiguous( (sizeof(user_iter_t) / sizeof(word_t)) + FR_AA + fi->frameSize );

			user_iter_t *uiter = colm_uiter_create( prg, &sp, fi, searchId );
			vm_set_local(exec, field, (SW) uiter);

			/* This is a setup similar to as a call, only the frame structure
			 * is slightly different for user iterators. We aren't going to do
			 * the call. We don't need to set up the return ip because the
			 * uiter advance will set it. The frame we need to do because it
			 * is set once for the lifetime of the iterator. */
			vm_push_type( tree_t**, exec->callArgs );
			vm_push_value( 0 );

			vm_push_type( code_t*, 0 );            /* Return instruction pointer,  */
			vm_push_type( tree_t**, exec->iframePtr ); /* Return iframe. */
			vm_push_type( tree_t**, exec->framePtr );  /* Return frame. */

			uiter->frame = vm_ptop();
			vm_pushn( fi->frameSize );
			memset( vm_ptop(), 0, sizeof(word_t) * fi->frameSize );

			uiterInit( prg, sp, uiter, fi, true );
			break;
		}
		case IN_UITER_CREATE_WC: {
			short field;
			half_t funcId, searchId;
			read_half( field );
			read_half( funcId );
			read_half( searchId );

			debug( prg, REALM_BYTECODE, "IN_UITER_CREATE_WC\n" );

			struct function_info *fi = prg->rtd->functionInfo + funcId;

			vm_contiguous( (sizeof(user_iter_t) / sizeof(word_t)) + FR_AA + fi->frameSize );

			user_iter_t *uiter = colm_uiter_create( prg, &sp, fi, searchId );
			vm_set_local(exec, field, (SW) uiter);

			/* This is a setup similar to as a call, only the frame structure
			 * is slightly different for user iterators. We aren't going to do
			 * the call. We don't need to set up the return ip because the
			 * uiter advance will set it. The frame we need to do because it
			 * is set once for the lifetime of the iterator. */
			vm_push_type( tree_t**, exec->callArgs );
			vm_push_value( 0 );

			vm_push_type( code_t*, 0 );            /* Return instruction pointer,  */
			vm_push_type( tree_t**, exec->iframePtr ); /* Return iframe. */
			vm_push_type( tree_t**, exec->framePtr );  /* Return frame. */

			uiter->frame = vm_ptop();
			vm_pushn( fi->frameSize );
			memset( vm_ptop(), 0, sizeof(word_t) * fi->frameSize );

			uiterInit( prg, sp, uiter, fi, false );
			break;
		}
		case IN_UITER_DESTROY: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_DESTROY %hd\n", field );

			user_iter_t *uiter = (user_iter_t*) vm_get_local(exec, field);
			colm_uiter_destroy( prg, &sp, uiter );
			break;
		}

		case IN_UITER_UNWIND: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_UNWIND %hd\n", field );

			user_iter_t *uiter = (user_iter_t*) vm_get_local(exec, field);
			colm_uiter_unwind( prg, &sp, uiter );
			break;
		}

		case IN_RET: {
			struct frame_info *fi = &prg->rtd->frameInfo[exec->frameId];
			downref_local_trees( prg, sp, exec, fi->locals, fi->localsLen );
			vm_popn( fi->frameSize );

			exec->frameId = vm_pop_type(long);
			exec->framePtr = vm_pop_type(tree_t**);
			instr = vm_pop_type(code_t*);
			exec->retVal = vm_pop_tree();
			vm_pop_value();
			//vm_popn( fi->argSize );

			fi = &prg->rtd->frameInfo[exec->frameId];
			debug( prg, REALM_BYTECODE, "IN_RET %s\n", fi->name );

			/* This if for direct calls of functions. */
			if ( instr == 0 ){
				//assert( sp == root );
				return sp;
			}

			/* Might be some unwind code. */
			{
				short unwind_len;
				read_half( unwind_len );
				if ( unwind_len > 0 ) {
					instr += unwind_len;
					debug( prg, REALM_BYTECODE,
							"skipping unwind code length: %hd\n", unwind_len );
				}
			}
				
			break;
		}
		case IN_TO_UPPER: {
			debug( prg, REALM_BYTECODE, "IN_TO_UPPER\n" );

			tree_t *in = vm_pop_tree();
			head_t *head = stringToUpper( in->tokdata );
			tree_t *upper = constructString( prg, head );
			colm_tree_upref( upper );
			vm_push_tree( upper );
			colm_tree_downref( prg, sp, in );
			break;
		}
		case IN_TO_LOWER: {
			debug( prg, REALM_BYTECODE, "IN_TO_LOWER\n" );

			tree_t *in = vm_pop_tree();
			head_t *head = stringToLower( in->tokdata );
			tree_t *lower = constructString( prg, head );
			colm_tree_upref( lower );
			vm_push_tree( lower );
			colm_tree_downref( prg, sp, in );
			break;
		}
		case IN_OPEN_FILE: {
			debug( prg, REALM_BYTECODE, "IN_OPEN_FILE\n" );

			tree_t *mode = vm_pop_tree();
			tree_t *name = vm_pop_tree();
			stream_t *res = colm_stream_open_file( prg, name, mode );
			vm_push_stream( res );
			colm_tree_downref( prg, sp, name );
			colm_tree_downref( prg, sp, mode );
			break;
		}
		case IN_GET_STDIN: {
			debug( prg, REALM_BYTECODE, "IN_GET_STDIN\n" );

			/* Pop the root object. */
			vm_pop_tree();
			if ( prg->stdinVal == 0 )
				prg->stdinVal = colm_stream_open_fd( prg, "<stdin>", 0 );

			vm_push_stream( prg->stdinVal );
			break;
		}
		case IN_GET_STDOUT: {
			debug( prg, REALM_BYTECODE, "IN_GET_STDOUT\n" );

			/* Pop the root object. */
			vm_pop_tree();
			if ( prg->stdoutVal == 0 )
				prg->stdoutVal = colm_stream_open_fd( prg, "<stdout>", 1 );

			vm_push_stream( prg->stdoutVal );
			break;
		}
		case IN_GET_STDERR: {
			debug( prg, REALM_BYTECODE, "IN_GET_STDERR\n" );

			/* Pop the root object. */
			vm_pop_tree();
			if ( prg->stderrVal == 0 )
				prg->stderrVal = colm_stream_open_fd( prg, "<stderr>", 2 );

			vm_push_stream( prg->stderrVal );
			break;
		}
		case IN_SYSTEM: {
			debug( prg, REALM_BYTECODE, "IN_SYSTEM\n" );

			vm_pop_tree();
			str_t *cmd = vm_pop_string();

			char *cmd0 = malloc( cmd->value->length + 1 );
			memcpy( cmd0, cmd->value->data, cmd->value->length );
			cmd0[cmd->value->length] = 0;

			int res = system( cmd0 );

			if ( WIFSIGNALED( res ) )
				raise( WTERMSIG( res ) );
			res = WEXITSTATUS( res );

			colm_tree_downref( prg, sp, (tree_t*)cmd );

			value_t val = res;
			vm_push_value( val );
			break;
		}

		case IN_DONE:
			return sp;

		case IN_FN: {
			c = *instr++;
			switch ( c ) {
			case IN_STR_ATOI: {
				debug( prg, REALM_BYTECODE, "IN_STR_ATOI\n" );

				str_t *str = vm_pop_string();
				word_t res = strAtoi( str->value );
				value_t integer = res;
				vm_push_value( integer );
				colm_tree_downref( prg, sp, (tree_t*)str );
				break;
			}
			case IN_STR_ATOO: {
				debug( prg, REALM_BYTECODE, "IN_STR_ATOO\n" );

				str_t *str = vm_pop_string();
				word_t res = strAtoo( str->value );
				value_t integer = res;
				vm_push_value( integer );
				colm_tree_downref( prg, sp, (tree_t*)str );
				break;
			}
			case IN_STR_UORD8: {
				debug( prg, REALM_BYTECODE, "IN_STR_UORD8\n" );

				str_t *str = vm_pop_string();
				word_t res = strUord8( str->value );
				value_t integer = res;
				vm_push_value( integer );
				colm_tree_downref( prg, sp, (tree_t*)str );
				break;
			}
			case IN_STR_UORD16: {
				debug( prg, REALM_BYTECODE, "IN_STR_UORD16\n" );

				str_t *str = vm_pop_string();
				word_t res = strUord16( str->value );
				value_t integer = res;
				vm_push_value( integer );
				colm_tree_downref( prg, sp, (tree_t*)str );
				break;
			}
			case IN_STR_PREFIX: {
				debug( prg, REALM_BYTECODE, "IN_STR_PREFIX\n" );

				str_t *str = vm_pop_string();
				value_t len = vm_pop_value();

				str_t *res = string_prefix( prg, str, (long) len );
				colm_tree_upref( (tree_t*) res );
				vm_push_string( res );
				colm_tree_downref( prg, sp, (tree_t*)str );
				break;
			}
			case IN_STR_SUFFIX: {
				debug( prg, REALM_BYTECODE, "IN_STR_SUFFIX\n" );

				str_t *str = vm_pop_string();
				value_t pos = vm_pop_value();

				str_t *res = string_suffix( prg, str, (long) pos );
				colm_tree_upref( (tree_t*) res );
				vm_push_string( res );
				colm_tree_downref( prg, sp, (tree_t*)str );
				break;
			}
			case IN_PREFIX: {
				debug( prg, REALM_BYTECODE, "IN_PREFIX\n" );

				value_t len = vm_pop_value();
				str_t *str = vm_pop_string();

				str_t *res = string_prefix( prg, str, (long) len );
				colm_tree_upref( (tree_t*) res );
				vm_push_string( res );
				colm_tree_downref( prg, sp, (tree_t*)str );
				break;
			}
			case IN_SUFFIX: {
				debug( prg, REALM_BYTECODE, "IN_SUFFIX\n" );

				value_t pos = vm_pop_value();
				str_t *str = vm_pop_string();

				str_t *res = string_suffix( prg, str, (long) pos );
				colm_tree_upref( (tree_t*) res );
				vm_push_string( res );
				colm_tree_downref( prg, sp, (tree_t*)str );
				break;
			}
			case IN_LOAD_ARG0: {
				half_t field;
				read_half( field );
				debug( prg, REALM_BYTECODE, "IN_LOAD_ARG0 %lu\n", field );

				/* tree_t comes back upreffed. */
				tree_t *tree = construct_arg0( prg, prg->argc, prg->argv );
				tree_t *prev = colm_struct_get_field( prg->global, tree_t*, field );
				colm_tree_downref( prg, sp, prev );
				colm_struct_set_field( prg->global, tree_t*, field, tree );
				break;
			}
			case IN_LOAD_ARGV: {
				half_t field;
				read_half( field );
				debug( prg, REALM_BYTECODE, "IN_LOAD_ARGV %lu\n", field );

				list_t *list = construct_argv( prg, prg->argc, prg->argv );
				colm_struct_set_field( prg->global, list_t*, field, list );
				break;
			}
			case IN_STOP: {
				debug( prg, REALM_BYTECODE, "IN_STOP\n" );

				fflush( stdout );
				goto out;
			}

			case IN_LIST_PUSH_HEAD_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_HEAD_WC\n" );

				list_t *list = vm_pop_list();
				struct_t *s = vm_pop_struct();

				list_el_t *listEl = colm_struct_to_list_el( prg, s, genId );
				colm_list_prepend( list, listEl );

				//colm_tree_upref( prg->trueVal );
				vm_push_tree( prg->trueVal );
				break;
			}
			case IN_LIST_PUSH_HEAD_WV: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_HEAD_WV\n" );

				list_t *list = vm_pop_list();
				struct_t *s = vm_pop_struct();

				list_el_t *listEl = colm_struct_to_list_el( prg, s, genId );
				colm_list_prepend( list, listEl );

				//colm_tree_upref( prg->trueVal );
				vm_push_tree( prg->trueVal );

				/* Set up reverse code. Needs no args. */
				rcode_code( exec, IN_FN );
				rcode_code( exec, IN_LIST_PUSH_HEAD_BKT );
				rcode_unit_term( exec );
				break;
			}
			case IN_LIST_PUSH_HEAD_BKT: {
				debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_HEAD_BKT\n" );

				list_t *list = vm_pop_list();
				colm_list_detach_head( list );
				break;
			}
			case IN_LIST_PUSH_TAIL_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_TAIL_WC\n" );

				list_t *list = vm_pop_list();
				struct_t *s = vm_pop_struct();

				list_el_t *listEl = colm_struct_to_list_el( prg, s, genId );
				colm_list_append( list, listEl );

				//colm_tree_upref( prg->trueVal );
				vm_push_tree( prg->trueVal );
				break;
			}
			case IN_LIST_PUSH_TAIL_WV: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_TAIL_WV\n" );

				list_t *list = vm_pop_list();
				struct_t *s = vm_pop_struct();

				list_el_t *listEl = colm_struct_to_list_el( prg, s, genId );
				colm_list_append( list, listEl );

				//colm_tree_upref( prg->trueVal );
				vm_push_tree( prg->trueVal );

				/* Set up reverse code. Needs no args. */
				rcode_code( exec, IN_FN );
				rcode_code( exec, IN_LIST_PUSH_TAIL_BKT );
				rcode_unit_term( exec );
				break;
			}
			case IN_LIST_PUSH_TAIL_BKT: {
				debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_TAIL_BKT\n" );

				list_t *list = vm_pop_list();
				colm_list_detach_tail( list );
				break;
			}
			case IN_LIST_POP_TAIL_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_LIST_POP_TAIL_WC\n" );

				list_t *list = vm_pop_list();

				list_el_t *tail = list->tail;
				colm_list_detach_tail( list );
				struct_t *s = colm_generic_el_container( prg, tail, genId );

				vm_push_struct( s );
				break;
			}
			case IN_LIST_POP_TAIL_WV: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_LIST_POP_TAIL_WV\n" );

				list_t *list = vm_pop_list();

				list_el_t *tail = list->tail;
				colm_list_detach_tail( list );
				struct_t *s = colm_generic_el_container( prg, tail, genId );

				vm_push_struct( s );

				/* Set up reverse. The result comes off the list downrefed.
				 * Need it up referenced for the reverse code too. */
				rcode_code( exec, IN_FN );
				rcode_code( exec, IN_LIST_POP_TAIL_BKT );
				rcodeHalf( exec, genId );
				rcode_word( exec, (word_t)s );
				rcode_unit_term( exec );
				break;
			}
			case IN_LIST_POP_TAIL_BKT: {
				short genId;
				tree_t *val;
				read_half( genId );
				read_tree( val );

				debug( prg, REALM_BYTECODE, "IN_LIST_POP_TAIL_BKT\n" );

				list_t *list = vm_pop_list();
				struct_t *s = (struct_t*) val;

				list_el_t *listEl = colm_struct_to_list_el( prg, s, genId );

				colm_list_append( list, listEl );
				break;
			}
			case IN_LIST_POP_HEAD_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_LIST_POP_HEAD_WC\n" );

				list_t *list = vm_pop_list();

				list_el_t *head = list->head;
				colm_list_detach_head( list );
				struct_t *s = colm_generic_el_container( prg, head, genId );

				vm_push_struct( s );
				break;
			}
			case IN_LIST_POP_HEAD_WV: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_LIST_POP_HEAD_WV\n" );

				list_t *list = vm_pop_list();

				list_el_t *head = list->head;
				colm_list_detach_head( list );
				struct_t *s = colm_generic_el_container( prg, head, genId );

				vm_push_struct( s );

				/* Set up reverse. The result comes off the list downrefed.
				 * Need it up referenced for the reverse code too. */
				rcode_code( exec, IN_FN );
				rcode_code( exec, IN_LIST_POP_HEAD_BKT );
				rcodeHalf( exec, genId );
				rcode_word( exec, (word_t)s );
				rcode_unit_term( exec );
				break;
			}
			case IN_LIST_POP_HEAD_BKT: {
				short genId;
				tree_t *val;
				read_half( genId );
				read_tree( val );

				debug( prg, REALM_BYTECODE, "IN_LIST_POP_HEAD_BKT\n" );

				list_t *list = vm_pop_list();
				struct_t *s = (struct_t*) val;

				list_el_t *listEl = colm_struct_to_list_el( prg, s, genId );

				colm_list_prepend( list, listEl );
				break;
			}
			case IN_MAP_FIND: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_MAP_FIND %hd\n", genId );

				map_t *map = vm_pop_map();
				tree_t *key = vm_pop_tree();

				map_el_t *mapEl = colm_map_find( prg, map, key );

				struct colm_struct *strct = mapEl != 0 ?
						colm_generic_el_container( prg, mapEl, genId ) : 0;

				vm_push_struct( strct );

				if ( map->genericInfo->keyType == TYPE_TREE )
					colm_tree_downref( prg, sp, key );
				break;
			}
			case IN_MAP_INSERT_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_MAP_INSERT_WC %hd\n", genId );

				map_t *map = vm_pop_map();
				struct_t *s = vm_pop_struct();

				map_el_t *mapEl = colm_struct_to_map_el( prg, s, genId );

				colm_map_insert( prg, map, mapEl );

				//colm_tree_upref( prg->trueVal );
				vm_push_tree( prg->trueVal );
				break;
			}
			case IN_MAP_INSERT_WV: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_MAP_INSERT_WV %hd\n", genId );

				map_t *map = vm_pop_map();
				struct_t *s = vm_pop_struct();

				map_el_t *mapEl = colm_struct_to_map_el( prg, s, genId );

				map_el_t *inserted = colm_map_insert( prg, map, mapEl );

				//colm_tree_upref( prg->trueVal );
				vm_push_tree( prg->trueVal );

				rcode_code( exec, IN_FN );
				rcode_code( exec, IN_MAP_INSERT_BKT );
				rcodeHalf( exec, genId );
				rcode_code( exec, inserted != 0 ? 1 : 0 );
				rcode_word( exec, (word_t)mapEl );
				rcode_unit_term( exec );
				break;
			}

			case IN_MAP_INSERT_BKT: {
				short genId;
				uchar inserted;
				word_t wmapEl;

				read_half( genId );
				read_byte( inserted );
				read_word( wmapEl );

				map_el_t *mapEl = (map_el_t*)wmapEl;

				debug( prg, REALM_BYTECODE, "IN_MAP_INSERT_BKT %d\n",
						(int)inserted );

				map_t *map = vm_pop_map();

				if ( inserted ) 
					colm_map_detach( prg, map, mapEl );
				break;
			}
			case IN_MAP_DETACH_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_MAP_DETACH_WC %hd\n", genId );

				map_t *map = vm_pop_map();
				struct_t *s = vm_pop_struct();

				map_el_t *mapEl = colm_struct_to_map_el( prg, s, genId );

				colm_map_detach( prg, map, mapEl );

				//colm_tree_upref( prg->trueVal );
				vm_push_tree( prg->trueVal );
				break;
			}
			case IN_MAP_DETACH_WV: {
				debug( prg, REALM_BYTECODE, "IN_MAP_DETACH_WV\n" );

				tree_t *obj = vm_pop_tree();
				tree_t *key = vm_pop_tree();
				struct tree_pair pair = mapRemove( prg, (map_t*)obj, key );

				colm_tree_upref( pair.val );
				vm_push_tree( pair.val );

				/* Reverse instruction. */
				rcode_code( exec, IN_FN );
				rcode_code( exec, IN_MAP_DETACH_BKT );
				rcode_word( exec, (word_t)pair.key );
				rcode_word( exec, (word_t)pair.val );
				rcode_unit_term( exec );

				colm_tree_downref( prg, sp, obj );
				colm_tree_downref( prg, sp, key );
				break;
			}
			case IN_MAP_DETACH_BKT: {
				tree_t *key, *val;
				read_tree( key );
				read_tree( val );

				debug( prg, REALM_BYTECODE, "IN_MAP_DETACH_BKT\n" );

				/* Either both or neither. */
				assert( ( key == 0 ) ^ ( val != 0 ) );

				tree_t *obj = vm_pop_tree();
				#if 0
				if ( key != 0 )
					mapUnremove( prg, (map_t*)obj, key, val );
				#endif

				colm_tree_downref( prg, sp, obj );
				break;
			}
			case IN_VMAP_INSERT_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_VMAP_INSERT_WC %hd\n", genId );

				map_t *map = vm_pop_map();
				struct_t *value = vm_pop_struct();
				struct_t *key = vm_pop_struct();

				colm_vmap_insert( prg, map, key, value );

				//colm_tree_upref( prg->trueVal );
				vm_push_tree( prg->trueVal );
				break;
			}
			case IN_VMAP_REMOVE_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_VMAP_REMOVE_WC %hd\n", genId );

				map_t *map = vm_pop_map();
				tree_t *key = vm_pop_tree();

				colm_vmap_remove( prg, map, key );

				//colm_tree_upref( prg->trueVal );
				vm_push_tree( prg->trueVal );
				break;
			}
			case IN_VMAP_FIND: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_VMAP_FIND %hd\n", genId );

				map_t *map = vm_pop_map();
				tree_t *key = vm_pop_tree();

				tree_t *result = colm_vmap_find( prg, map, key );
	
				vm_push_tree( result );

				if ( map->genericInfo->keyType == TYPE_TREE )
					colm_tree_downref( prg, sp, key );
				break;
			}
			case IN_VLIST_PUSH_TAIL_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_VLIST_PUSH_TAIL_WC %hd\n", genId );

				list_t *list = vm_pop_list();
				value_t value = vm_pop_value();

				colm_vlist_append( prg, list, value );

				vm_push_tree( prg->trueVal );
				break;
			}
			case IN_VLIST_PUSH_HEAD_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_VLIST_PUSH_HEAD_WC %hd\n", genId );

				list_t *list = vm_pop_list();
				value_t value = vm_pop_value();

				colm_vlist_prepend( prg, list, value );

				vm_push_tree( prg->trueVal );
				break;
			}
			case IN_VLIST_POP_HEAD_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_VLIST_POP_HEAD_WC %hd\n", genId );

				list_t *list = vm_pop_list();

				value_t result = colm_vlist_detach_head( prg, list );
				vm_push_value( result );
				break;
			}
			case IN_VLIST_POP_TAIL_WC: {
				short genId;
				read_half( genId );

				debug( prg, REALM_BYTECODE, "IN_VLIST_POP_TAIL_WC %hd\n", genId );

				list_t *list = vm_pop_list();

				value_t result = colm_vlist_detach_tail( prg, list );
				vm_push_value( result );
				break;
			}

			case IN_EXIT_HARD: {
				debug( prg, REALM_BYTECODE, "IN_EXIT\n" );

				vm_pop_tree();
				prg->exitStatus = vm_pop_type(long);
				prg->induceExit = 1;
				exit( prg->exitStatus );
			}
			case IN_EXIT: {
				/* The unwind code follows the exit call (exception, see
				 * synthesis). */
				short unwind_len;
				read_half( unwind_len );

				debug( prg, REALM_BYTECODE, "IN_EXIT, unwind len: %hd\n", unwind_len );

				vm_pop_tree();
				prg->exitStatus = vm_pop_type(long);
				prg->induceExit = 1;

				while ( true ) {
					/* We stop on the root, leaving the psuedo-call setup on the
					 * stack. Note we exclude the local data. */
					if ( exec->frameId == prg->rtd->rootFrameId )
						break;

					struct frame_info *fi = &prg->rtd->frameInfo[exec->frameId];

					debug( prg, REALM_BYTECODE, "IN_EXIT, popping frame %s, "
							"unwind-len %hd, arg-size %ld\n",
							( fi->name != 0 ? fi->name : "<no-name>" ),
							unwind_len, fi->argSize );

					if ( unwind_len > 0 )
						sp = colm_execute_code( prg, exec, sp, instr );

					downref_locals( prg, &sp, exec, fi->locals, fi->localsLen );
					vm_popn( fi->frameSize );

					/* Call layout. */
					exec->frameId = vm_pop_type(long);
					exec->framePtr = vm_pop_type(tree_t**);
					instr = vm_pop_type(code_t*);

					tree_t *retVal = vm_pop_tree();
					vm_pop_value();

					/* The IN_PREP_ARGS stack data. */
					vm_popn( fi->argSize );
					vm_pop_value();

					if ( fi->retTree ) {
						/* Problem here. */
						colm_tree_downref( prg, sp, retVal );
					}

					read_half( unwind_len );
				}

				goto out;
			}
			default: {
				fatal( "UNKNOWN FUNCTION: 0x%2x -- something is wrong\n", c );
				break;
			}}
			break;
		}



		/* Halt is a default instruction given by the compiler when it is
		 * asked to generate and instruction it doesn't have. It is deliberate
		 * and can represent "not implemented" or "compiler error" because a
		 * variable holding instructions was not properly initialize. */
		case IN_HALT: {
			fatal( "IN_HALT -- compiler did something wrong\n" );
			exit(1);
			break;
		}
		default: {
			fatal( "UNKNOWN INSTRUCTION: 0x%2x -- something is wrong\n", *(instr-1) );
			assert(false);
			break;
		}
	}
	goto again;

out:
	if ( ! prg->induceExit )
		assert( sp == root );
	return sp;
}

/*
 * Deleteing rcode required downreffing any trees held by it.
 */
static void rcode_downref( program_t *prg, tree_t **sp, code_t *instr )
{
again:
	switch ( *instr++ ) {
		case IN_PARSE_LOAD: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_LOAD\n" );
			break;
		}
		case IN_PARSE_INIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_INIT_BKT\n" );

			consume_word(); //( parser );
			consume_word(); //( pcr );
			consume_word(); //( steps );

			break;
		}

		case IN_LOAD_TREE: {
			tree_t *w;
			read_tree( w );
			debug( prg, REALM_BYTECODE, "IN_LOAD_TREE %p\n", w );
			colm_tree_downref( prg, sp, w );
			break;
		}
		case IN_LOAD_WORD: {
			consume_word();
			debug( prg, REALM_BYTECODE, "IN_LOAD_WORD\n" );
			break;
		}
		case IN_RESTORE_LHS: {
			tree_t *restore;
			read_tree( restore );
			debug( prg, REALM_BYTECODE, "IN_RESTORE_LHS\n" );
			colm_tree_downref( prg, sp, restore );
			break;
		}

		case IN_PARSE_FRAG_BKT: {
			half_t stopId;
			read_half( stopId );
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_BKT\n" );
			break;
		}
		case IN_PARSE_FRAG_EXIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_EXIT_BKT\n" );
			break;
		}
		case IN_PARSE_FINISH_BKT: {
			half_t stopId;
			read_half( stopId );
			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_BKT\n" );
			break;
		}
		case IN_PARSE_FINISH_EXIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_EXIT_BKT\n" );
			break;
		}
		case IN_PCR_CALL: {
			debug( prg, REALM_BYTECODE, "IN_PCR_CALL\n" );
			break;
		}
		case IN_PCR_RET: {
			debug( prg, REALM_BYTECODE, "IN_PCR_RET\n" );
			return;
		}
		case IN_PCR_END_DECK: {
			debug( prg, REALM_BYTECODE, "IN_PCR_END_DECK\n" );
			return;
		}
		case IN_PARSE_APPEND_BKT: {
			consume_word(); //( parser );
			consume_word(); //( input );
			consume_word(); //( len );

			debug( prg, REALM_BYTECODE, "IN_PARSE_APPEND_BKT\n" ); 

			break;
		}
		case IN_PARSE_APPEND_STREAM_BKT: {
			tree_t *input;

			consume_word(); //( sptr );
			read_tree( input );
			consume_word(); //( len );

			debug( prg, REALM_BYTECODE, "IN_PARSE_APPEND_STREAM_BKT\n" );

			colm_tree_downref( prg, sp, input );
			break;
		}

		case IN_INPUT_PULL_BKT: {
			tree_t *string;
			read_tree( string );

			debug( prg, REALM_BYTECODE, "IN_INPUT_PULL_BKT\n" );

			colm_tree_downref( prg, sp, string );
			break;
		}
		case IN_INPUT_PUSH_BKT: {
			consume_word(); //( len );

			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_BKT\n" );
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_GLOBAL_BKT\n" );
			break;
		}
		case IN_LOAD_CONTEXT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CONTEXT_BKT\n" );
			break;
		}
		case IN_LOAD_INPUT_BKT: {
			consume_word(); //( input );
			debug( prg, REALM_BYTECODE, "IN_LOAD_INPUT_BKT\n" );
			break;
		}
		case IN_GET_FIELD_TREE_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_TREE_BKT %hd\n", field );
			break;
		}
		case IN_SET_FIELD_TREE_BKT: {
			short field;
			tree_t *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_BKT %hd\n", field );

			colm_tree_downref( prg, sp, val );
			break;
		}
		case IN_SET_STRUCT_BKT: {
			short field;
			tree_t *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_BKT %hd\n", field );

			colm_tree_downref( prg, sp, val );
			break;
		}
		case IN_SET_STRUCT_VAL_BKT: {
			consume_half(); //( field );
			consume_word(); //( val );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_VAL_BKT\n" );
			break;
		}
		case IN_PTR_ACCESS_BKT: {
			consume_word(); //( ptr );

			debug( prg, REALM_BYTECODE, "IN_PTR_ACCESS_BKT\n" );
			break;
		}
		case IN_SET_TOKEN_DATA_BKT: {
			word_t oldval;
			read_word( oldval );

			debug( prg, REALM_BYTECODE, "IN_SET_TOKEN_DATA_BKT\n" );

			head_t *head = (head_t*)oldval;
			stringFree( prg, head );
			break;
		}
		case IN_GET_LIST_MEM_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_MEM_BKT %hd\n", field );
			break;
		}
		case IN_GET_MAP_MEM_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_MAP_MEM_BKT %hd\n", field );
			break;
		}
		case IN_STOP: {
			return;
		}
		case IN_FN: {
			switch ( *instr++ ) {
			case IN_LIST_PUSH_HEAD_BKT: {
				debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_HEAD_BKT\n" );
				break;
			}
			case IN_LIST_POP_HEAD_BKT: {
				consume_half(); //( genId );
				consume_word(); //( val );

				debug( prg, REALM_BYTECODE, "IN_LIST_POP_HEAD_BKT\n" );

				break;
			}
			case IN_LIST_PUSH_TAIL_BKT: {
				debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_TAIL_BKT\n" );
				break;
			}
			case IN_LIST_POP_TAIL_BKT: {
				consume_half(); //( genId );
				consume_word(); //( val );

				debug( prg, REALM_BYTECODE, "IN_LIST_POP_TAIL_BKT\n" );

				break;
			}
			case IN_MAP_INSERT_BKT: {
				uchar inserted;

				consume_half(); //( genId );
				read_byte( inserted );
				consume_word(); //( wmapEl );

				debug( prg, REALM_BYTECODE, "IN_MAP_INSERT_BKT %d\n",
						(int)inserted );
				break;
			}
			case IN_MAP_DETACH_BKT: {
				tree_t *key, *val;
				read_tree( key );
				read_tree( val );

				debug( prg, REALM_BYTECODE, "IN_MAP_DETACH_BKT\n" );

				colm_tree_downref( prg, sp, key );
				colm_tree_downref( prg, sp, val );
				break;
			}}
			break;
		}
		default: {
			fatal( "UNKNOWN INSTRUCTION 0x%2x: -- reverse code downref\n", *(instr-1));
			assert(false);
			break;
		}
	}
	goto again;
}


