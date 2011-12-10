/*
 *  Copyright 2006-2011 Adrian Thurston <thurston@complang.org>
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

//#define COLM_LOG

#include <colm/pdarun.h>
#include <colm/fsmrun.h>
#include <colm/tree.h>
#include <colm/bytecode.h>
#include <colm/pool.h>
#include <colm/debug.h>
#include <colm/config.h>

#include <alloca.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

#define true 1
#define false 0

/* More common macros are in bytecode.h. */
#define vm_top_off(n) (sp[n])
#define vm_popn(n) (sp += (n))
#define vm_pushn(n) (sp -= (n))
#define vm_local(o) (exec->framePtr[o])
#define vm_plocal(o) (&exec->framePtr[o])
#define vm_local_iframe(o) (exec->iframePtr[o])
#define vm_plocal_iframe(o) (&exec->iframePtr[o])

#define read_byte( i ) do { \
	i = ((uchar) *instr++); \
} while(0)

#define read_word_p( i, p ) do { \
	i = ((Word)  p[0]); \
	i |= ((Word) p[1]) << 8; \
	i |= ((Word) p[2]) << 16; \
	i |= ((Word) p[3]) << 24; \
} while(0)

/* There are better ways. */
#if SIZEOF_LONG == 4
	#define read_word( i ) do { \
		i = ((Word) *instr++); \
		i |= ((Word) *instr++) << 8; \
		i |= ((Word) *instr++) << 16; \
		i |= ((Word) *instr++) << 24; \
	} while(0)
#else
	#define read_word( i ) do { \
		i = ((Word) *instr++); \
		i |= ((Word) *instr++) << 8; \
		i |= ((Word) *instr++) << 16; \
		i |= ((Word) *instr++) << 24; \
		i |= ((Word) *instr++) << 32; \
		i |= ((Word) *instr++) << 40; \
		i |= ((Word) *instr++) << 48; \
		i |= ((Word) *instr++) << 56; \
	} while(0)
#endif

/* There are better ways. */
#if SIZEOF_LONG == 4
	#define read_tree( i ) do { \
		Word w; \
		w = ((Word) *instr++); \
		w |= ((Word) *instr++) << 8; \
		w |= ((Word) *instr++) << 16; \
		w |= ((Word) *instr++) << 24; \
		i = (Tree*) w; \
	} while(0)

	#define read_word_type( Type, i ) do { \
		Word w; \
		w = ((Word) *instr++); \
		w |= ((Word) *instr++) << 8; \
		w |= ((Word) *instr++) << 16; \
		w |= ((Word) *instr++) << 24; \
		i = (Type) w; \
	} while(0)
#else
	#define read_tree( i ) do { \
		Word w; \
		w = ((Word) *instr++); \
		w |= ((Word) *instr++) << 8; \
		w |= ((Word) *instr++) << 16; \
		w |= ((Word) *instr++) << 24; \
		w |= ((Word) *instr++) << 32; \
		w |= ((Word) *instr++) << 40; \
		w |= ((Word) *instr++) << 48; \
		w |= ((Word) *instr++) << 56; \
		i = (Tree*) w; \
	} while(0)

	#define read_word_type( Type, i ) do { \
		Word w; \
		w = ((Word) *instr++); \
		w |= ((Word) *instr++) << 8; \
		w |= ((Word) *instr++) << 16; \
		w |= ((Word) *instr++) << 24; \
		w |= ((Word) *instr++) << 32; \
		w |= ((Word) *instr++) << 40; \
		w |= ((Word) *instr++) << 48; \
		w |= ((Word) *instr++) << 56; \
		i = (Type) w; \
	} while(0)
#endif

#define read_half( i ) do { \
	i = ((Word) *instr++); \
	i |= ((Word) *instr++) << 8; \
} while(0)

int colm_log_bytecode = 0;
int colm_log_parse = 0;
int colm_log_match = 0;
int colm_log_compile = 0;
int colm_log_conds = 0;

void vm_grow( Program *prg )
{
	debug( REALM_BYTECODE, "growing stack\n" );
}

Tree *prepParseTree( Program *prg, Tree **sp, Tree *tree )
{
	/* Seems like we need to always copy here. If it isn't a parse tree it
	 * needs to be made into one. If it is then we need to make a new one in
	 * case the old one is still in use by some parsing routine. The case were
	 * we might be able to avoid a copy would be that it's a parse tree
	 * already, but it's owning parser is completely finished with it. */

	debug( REALM_BYTECODE, "copying tree in send function\n" );

	Kid *unused = 0;
	tree = copyRealTree( prg, tree, 0, &unused, true );
	return tree;
}

void parserSetContext( Program *prg, Tree **sp, Accum *accum, Tree *val )
{
	accum->pdaRun->context = splitTree( prg, val );
}

Head *treeToStr( Program *prg, Tree **sp, Tree *tree )
{
	/* Collect the tree data. */
	StrCollect collect;
	initStrCollect( &collect );

	printTreeCollect( prg, sp, &collect, tree );

	/* Set up the input stream. */
	Head *ret = stringAllocFull( prg, collect.data, collect.length );

	strCollectDestroy( &collect );

	return ret;
}

Tree *extractInput( Program *prg, Accum *accum )
{
	if ( accum->stream == 0 ) {
		Stream *res = (Stream*)mapElAllocate( prg );
		res->id = LEL_ID_STREAM;
		res->in = newInputStreamAccum();
		treeUpref( (Tree*)res );
		accum->stream = res;
	}

	return (Tree*)accum->stream;
}

void setInput( Program *prg, Tree **sp, Accum *accum, Stream *stream )
{
	if ( accum->stream != 0 )
		treeDownref( prg, sp, (Tree*)accum->stream );

	accum->stream = stream;
	treeUpref( (Tree*)accum->stream );
}

Word streamAppend( Program *prg, Tree **sp, Tree *input, Stream *stream )
{
	if ( input->id == LEL_ID_STR ) {
		//assert(false);
		/* Collect the tree data. */
		StrCollect collect;
		initStrCollect( &collect );
		printTreeCollect( prg, sp, &collect, input );

		/* Load it into the input. */
		stream->in->funcs->appendData( stream->in, collect.data, collect.length );

		long length = collect.length;
		strCollectDestroy( &collect );

		return length;
	}
	else {
		input = prepParseTree( prg, sp, input );

		if ( input->id >= prg->rtd->firstNonTermId )
			input->id = prg->rtd->lelInfo[input->id].termDupId;

		input->flags |= AF_ARTIFICIAL;

		treeUpref( input );
		stream->in->funcs->appendTree( stream->in, input );
		return 0;
	}
}

long parseFrag( Program *prg, Tree **sp, Accum *accum, long stopId, long entry )
{
	Stream *stream = accum->stream;

switch ( entry ) {
case PcrStart:

	if ( ! accum->pdaRun->parseError ) {
		accum->pdaRun->stopTarget = stopId;
		accum->fsmRun->curStream = (Tree*)stream;

		long pcr = parseLoop( prg, sp, accum->pdaRun, accum->fsmRun, stream->in, entry );

		while ( pcr != PcrDone ) {

return pcr;
case PcrReduction:
case PcrGeneration:
case PcrPreEof:
case PcrRevIgnore:
case PcrRevIgnore2:
case PcrRevToken:
case PcrRevToken2:
case PcrRevReduction:
case PcrRevReduction2:

			pcr = parseLoop( prg, sp, accum->pdaRun, accum->fsmRun, stream->in, entry );
		}
	}

case PcrDone:
break; }

	return PcrDone;
}

long parseFinish( Tree **result, Program *prg, Tree **sp,
		Accum *accum, int revertOn, long entry )
{
	Stream *stream = accum->stream;

switch ( entry ) {
case PcrStart:

	if ( accum->pdaRun->stopTarget <= 0 ) {
		stream->in->eof = true;
		stream->in->later = false;

		if ( ! accum->pdaRun->parseError ) {
			long pcr = parseLoop( prg, sp, accum->pdaRun, accum->fsmRun, stream->in, entry );

			while ( pcr != PcrDone ) {

return pcr;
case PcrReduction:
case PcrGeneration:
case PcrPreEof:
case PcrRevIgnore:
case PcrRevIgnore2:
case PcrRevToken:
case PcrRevToken2:
case PcrRevReduction:
case PcrRevReduction2:

				pcr = parseLoop( prg, sp, accum->pdaRun, accum->fsmRun, stream->in, entry );
			}
		}
	}

	if ( !revertOn )
		commitFull( prg, sp, accum->pdaRun, 0 );
	
	Tree *tree = getParsedRoot( accum->pdaRun, accum->pdaRun->stopTarget > 0 );
	treeUpref( tree );

	/* Indicate that this tree came out of a parser. */
	if ( tree != 0 )
		tree->flags |= AF_PARSED;

	*result = tree;

case PcrDone:
break; }

	return PcrDone;
}

long undoParseFrag( Program *prg, Tree **sp, Accum *accum, long steps, long entry )
{
	Stream *stream = accum->stream;
	InputStream *inputStream = stream->in;
	FsmRun *fsmRun = accum->fsmRun;
	PdaRun *pdaRun = accum->pdaRun;

switch ( entry ) {
case PcrStart:

	if ( steps < pdaRun->steps ) {
		/* Setup environment for going backwards until we reduced steps to
		 * what we want. */
		pdaRun->numRetry += 1;
		pdaRun->targetSteps = steps;
		pdaRun->triggerUndo = 1;

		/* The parse loop will recognise the situation. */
		long pcr = parseLoop( prg, sp, pdaRun, fsmRun, inputStream, entry );
		while ( pcr != PcrDone ) {

return pcr;
case PcrReduction:
case PcrGeneration:
case PcrPreEof:
case PcrRevIgnore:
case PcrRevIgnore2:
case PcrRevToken:
case PcrRevToken2:
case PcrRevReduction:
case PcrRevReduction2:

			pcr = parseLoop( prg, sp, pdaRun, fsmRun, inputStream, entry );
		}

		/* Reset environment. */
		pdaRun->triggerUndo = 0;
		pdaRun->targetSteps = -1;
		pdaRun->numRetry -= 1;
	}

case PcrDone:
break; }

	return PcrDone;
}

Tree *streamPullBc( Program *prg, FsmRun *fsmRun, Stream *stream, Tree *length )
{
	long len = ((Int*)length)->value;
	Head *tokdata = streamPull( prg, fsmRun, stream->in, len );
	return constructString( prg, tokdata );
}


void undoPull( Program *prg, FsmRun *fsmRun, Stream *stream, Tree *str )
{
	const char *data = stringData( ( (Str*)str )->value );
	long length = stringLength( ( (Str*)str )->value );
	undoStreamPull( fsmRun, stream->in, data, length );
}

Word streamPush( Program *prg, Tree **sp, Stream *stream, Tree *tree, int ignore )
{
	if ( tree->id == LEL_ID_STR ) {
		/* This should become a compile error. If it's text, it's up to the
		 * scanner to decide. Want to force it then send a token. */
		assert( !ignore );
			
		/* Collect the tree data. */
		StrCollect collect;
		initStrCollect( &collect );
		printTreeCollect( prg, sp, &collect, tree );

		streamPushText( stream->in, collect.data, collect.length );
		long length = collect.length;
		strCollectDestroy( &collect );

		return length;
	}
	else {
		tree = prepParseTree( prg, sp, tree );

		if ( tree->id >= prg->rtd->firstNonTermId )
			tree->id = prg->rtd->lelInfo[tree->id].termDupId;

		tree->flags |= AF_ARTIFICIAL;
		treeUpref( tree );
		streamPushTree( stream->in, tree, ignore );
		return 0;
	}
}

void setLocal( Tree **frame, long field, Tree *tree )
{
	if ( tree != 0 )
		assert( tree->refs >= 1 );
	frame[field] = tree;
}

Tree *getLocalSplit( Program *prg, Tree **frame, long field )
{
	Tree *val = frame[field];
	Tree *split = splitTree( prg, val );
	frame[field] = split;
	return split;
}

void downrefLocalTrees( Program *prg, Tree **sp, Tree **frame, char *trees, long treesLen )
{
	long i;
	for ( i = 0; i < treesLen; i++ ) {
		debug( REALM_BYTECODE, "local tree downref: %ld\n", (long)trees[i] );

		treeDownref( prg, sp, frame[((long)trees[i])] );
	}
}

UserIter *uiterCreate( Program *prg, Tree ***psp, FunctionInfo *fi, long searchId )
{
	Tree **sp = *psp;
	vm_pushn( sizeof(UserIter) / sizeof(Word) );
	void *mem = vm_ptop();

	UserIter *uiter = mem;
	initUserIter( uiter, vm_ptop(), fi->argSize, searchId );
	*psp = sp;
	return uiter;
}

void uiterInit( Program *prg, Tree **sp, UserIter *uiter, 
		FunctionInfo *fi, int revertOn )
{
	/* Set up the first yeild so when we resume it starts at the beginning. */
	uiter->ref.kid = 0;
	uiter->stackSize = uiter->stackRoot - vm_ptop();
	uiter->frame = &uiter->stackRoot[-IFR_AA];

	if ( revertOn )
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWV;
	else
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWC;
}

void treeIterDestroy( Tree ***psp, TreeIter *iter )
{
	Tree **sp = *psp;
	long curStackSize = iter->stackRoot - vm_ptop();
	assert( iter->stackSize == curStackSize );
	vm_popn( iter->stackSize );
	*psp = sp;
}

void userIterDestroy( Tree ***psp, UserIter *uiter )
{
	Tree **sp = *psp;

	/* We should always be coming from a yield. The current stack size will be
	 * nonzero and the stack size in the iterator will be correct. */
	long curStackSize = uiter->stackRoot - vm_ptop();
	assert( uiter->stackSize == curStackSize );

	long argSize = uiter->argSize;

	vm_popn( uiter->stackRoot - vm_ptop() );
	vm_popn( sizeof(UserIter) / sizeof(Word) );
	vm_popn( argSize );

	*psp = sp;
}

Tree *constructArgv( Program *prg, int argc, const char **argv )
{
	Tree *list = createGeneric( prg, prg->rtd->argvGenericId );
	treeUpref( list );
	int i;
	for ( i = 0; i < argc; i++ ) {
		Head *head = stringAllocPointer( prg, argv[i], strlen(argv[i]) );
		Tree *arg = constructString( prg, head );
		treeUpref( arg );
		listAppend2( prg, (List*)list, arg );
	}
	return list;
}


/*
 * Execution environment
 */

void initExecution( Execution *exec, Program *prg, RtCodeVect *rcodeCollect,
		PdaRun *pdaRun, FsmRun *fsmRun, int frameId )
{
	exec->prg = prg;
	exec->pdaRun = pdaRun;
	exec->fsmRun = fsmRun;
	exec->framePtr = 0;
	exec->iframePtr = 0;
	exec->frameId = frameId;
	exec->rcodeCollect = rcodeCollect;
	exec->rcodeUnitLen = 0;
}

void rcodeDownrefAll( Program *prg, Tree **sp, RtCodeVect *rev )
{
	while ( rev->tabLen > 0 ) {
		/* Read the length */
		Code *prcode = rev->data + rev->tabLen - SIZEOF_WORD;
		Word len;
		read_word_p( len, prcode );

		/* Find the start of block. */
		long start = rev->tabLen - len - SIZEOF_WORD;
		prcode = rev->data + start;

		/* Execute it. */
		rcodeDownref( prg, sp, prcode );

		/* Backup over it. */
		rev->tabLen -= len + SIZEOF_WORD;
	}
}

void rcodeDownref( Program *prg, Tree **sp, Code *instr )
{
again:
	switch ( *instr++ ) {
		case IN_RESTORE_LHS: {
			Tree *lhs;
			read_tree( lhs );
			debug( REALM_BYTECODE, "IN_RESTORE_LHS\n" );
			treeDownref( prg, sp, lhs );
			break;
		}

		case IN_EXTRACT_INPUT_BKT: {
			debug( REALM_BYTECODE, "IN_EXTRACT_INPUT_BKT\n" );
			break;
		}
		case IN_STREAM_APPEND_BKT: {
			Tree *stream;
			Tree *input;
			Word len;
			read_tree( stream );
			read_tree( input );
			read_word( len );

			debug( REALM_BYTECODE, "IN_STREAM_APPEND_BKT\n" ); 

			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, input );
			break;
		}
		case IN_PARSE_FRAG_BKT: {
			Tree *accum;
			long steps;
			read_tree( accum );
			read_word( steps );

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_BKT %ld\n", steps );
			
			treeDownref( prg, sp, accum );
			break;
		}
		case IN_PARSE_FRAG_BKT2: {
			debug( REALM_BYTECODE, "IN_PARSE_FRAG_BKT2\n" );
			break;
		}
		case IN_PARSE_FRAG_BKT3: {
			debug( REALM_BYTECODE, "IN_PARSE_FRAG_BKT3\n" );
			break;
		}
		case IN_PARSE_FINISH_BKT: {
			Tree *accumTree;
			long steps;

			read_tree( accumTree );
			read_word( steps );

			debug( REALM_BYTECODE, "IN_PARSE_FINISH_BKT %ld\n", steps );

			treeDownref( prg, sp, accumTree );
			break;
		}
		case IN_PARSE_FINISH_BKT2: {
			debug( REALM_BYTECODE, "IN_PARSE_FINISH_BKT2\n" );
			break;
		}
		case IN_PARSE_FINISH_BKT3: {
			debug( REALM_BYTECODE, "IN_PARSE_FINISH_BKT3\n" );
			break;
		}
		case IN_PCR_RET: {
			debug( REALM_BYTECODE, "IN_PCR_RET\n" );
			return;
		}
		case IN_PCR_END_DECK: {
			debug( REALM_BYTECODE, "IN_PCR_END_DECK\n" );
			return;
		}
		case IN_STREAM_PULL_BKT: {
			Tree *string;
			read_tree( string );

			debug( REALM_BYTECODE, "IN_STREAM_PULL_BKT\n" );

			treeDownref( prg, sp, string );
			break;
		}
		case IN_STREAM_PUSH_BKT: {
			Word len;
			read_word( len );

			debug( REALM_BYTECODE, "IN_STREAM_PUSH_BKT\n" );
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			debug( REALM_BYTECODE, "IN_LOAD_GLOBAL_BKT\n" );
			break;
		}
		case IN_LOAD_CONTEXT_BKT: {
			debug( REALM_BYTECODE, "IN_LOAD_CONTEXT_BKT\n" );
			break;
		}
		case IN_LOAD_INPUT_BKT: {
			debug( REALM_BYTECODE, "IN_LOAD_INPUT_BKT\n" );
			break;
		}
		case IN_GET_FIELD_BKT: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_FIELD_BKT %hd\n", field );
			break;
		}
		case IN_SET_FIELD_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( REALM_BYTECODE, "IN_SET_FIELD_BKT %hd\n", field );

			treeDownref( prg, sp, val );
			break;
		}
		case IN_PTR_DEREF_BKT: {
			Tree *ptr;
			read_tree( ptr );

			debug( REALM_BYTECODE, "IN_PTR_DEREF_BKT\n" );

			treeDownref( prg, sp, ptr );
			break;
		}
		case IN_SET_TOKEN_DATA_BKT: {
			Word oldval;
			read_word( oldval );

			debug( REALM_BYTECODE, "IN_SET_TOKEN_DATA_BKT\n" );

			Head *head = (Head*)oldval;
			stringFree( prg, head );
			break;
		}
		case IN_LIST_APPEND_BKT: {
			debug( REALM_BYTECODE, "IN_LIST_APPEND_BKT\n" );
			break;
		}
		case IN_LIST_REMOVE_END_BKT: {
			Tree *val;
			read_tree( val );

			debug( REALM_BYTECODE, "IN_LIST_REMOVE_END_BKT\n" );

			treeDownref( prg, sp, val );
			break;
		}
		case IN_GET_LIST_MEM_BKT: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_LIST_MEM_BKT %hd\n", field );
			break;
		}
		case IN_SET_LIST_MEM_BKT: {
			Half field;
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( REALM_BYTECODE, "IN_SET_LIST_MEM_BKT %hd\n", field );

			treeDownref( prg, sp, val );
			break;
		}
		case IN_MAP_INSERT_BKT: {
			uchar inserted;
			Tree *key;
			read_byte( inserted );
			read_tree( key );

			debug( REALM_BYTECODE, "IN_MAP_INSERT_BKT\n" );
			
			treeDownref( prg, sp, key );
			break;
		}
		case IN_MAP_STORE_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			debug( REALM_BYTECODE,"IN_MAP_STORE_BKT\n" );

			treeDownref( prg, sp, key );
			treeDownref( prg, sp, val );
			break;
		}
		case IN_MAP_REMOVE_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			debug( REALM_BYTECODE, "IN_MAP_REMOVE_BKT\n" );

			treeDownref( prg, sp, key );
			treeDownref( prg, sp, val );
			break;
		}
		case IN_STOP: {
			return;
		}
		default: {
			fatal( "UNKNOWN INSTRUCTION 0x%2x: -- reverse code downref\n", *(instr-1));
			assert(false);
			break;
		}
	}
	goto again;
}


void mainExecution( Execution *exec, Code *code )
{
	Program *prg = exec->prg;
	Tree **sp = prg->vm_root;

	/* Set up the stack as if we have called. We allow a return value. */
	vm_push( 0 ); 
	vm_push( 0 );
	vm_push( 0 );
	vm_push( 0 );

	/* Execution loop. */
	executeCode( exec, sp, code );

	vm_pop_ignore();
	vm_pop_ignore();
	prg->returnVal = vm_pop();

	/* The root code should all be commit code and reverse code
	 * should be empty. */
	assert( exec->rcodeCollect->tabLen == 0 );
}

int makeReverseCode( PdaRun *pdaRun )
{
	RtCodeVect *reverseCode = &pdaRun->reverseCode;
	RtCodeVect *rcodeCollect = &pdaRun->rcodeCollect;

	/* Do we need to revert the left hand side? */

	/* Check if there was anything generated. */
	if ( rcodeCollect->tabLen == 0 )
		return false;

	/* One reverse code run for the DECK terminator. */
	append( reverseCode, IN_PCR_END_DECK );
	appendWord( reverseCode, 1 );

	long startLength = reverseCode->tabLen;

	/* Go backwards, group by group, through the reverse code. Push each group
	 * to the global reverse code stack. */
	Code *p = rcodeCollect->data + rcodeCollect->tabLen;
	while ( p != rcodeCollect->data ) {
		p--;
		long len = *p;
		p = p - len;
		append2( reverseCode, p, len );
	}

	/* Stop, then place a total length in the global stack. */
	append( reverseCode, IN_PCR_RET );
	long length = reverseCode->tabLen - startLength;
	appendWord( reverseCode, length );

	/* Clear the revere code buffer. */
	rcodeCollect->tabLen = 0;

	return true;
}

Code *popReverseCode( RtCodeVect *allRev )
{
	/* Read the length */
	Code *prcode = allRev->data + allRev->tabLen - SIZEOF_WORD;
	Word len;
	read_word_p( len, prcode );

	/* Find the start of block. */
	long start = allRev->tabLen - len - SIZEOF_WORD;
	prcode = allRev->data + start;

	/* Backup over it. */
	allRev->tabLen -= len + SIZEOF_WORD;

//	/* Do it again for the terminator. */
//	Code *prcode2 = allRev->data + allRev->tabLen - SIZEOF_WORD;
//	read_word_p( len, prcode2 );
//	start = allRev->tabLen - len - SIZEOF_WORD;
//	prcode2 = allRev->data + start;
//	allRev->tabLen -= len + SIZEOF_WORD;

	return prcode;
}

void callParseBlock( Code **pinstr, Tree ***psp, long pcr, Program *prg,
		Execution *exec, Accum *accum )
{
	Tree **sp = *psp;
	PdaRun *pdaRun = accum->pdaRun;
	FsmRun *fsmRun = accum->fsmRun;

	switch ( pcr ) {
		case PcrReduction: {
			/* Execution environment for the reduction code. */
			initExecution( pdaRun->exec, prg, &pdaRun->rcodeCollect, pdaRun, fsmRun, pdaRun->frameId );

			/* Call execution. */
			*pinstr = pdaRun->fi->codeWV;
			break;
		}

		case PcrGeneration: {
			/* 
			 * Not supported:
			 *  -invoke failure (the backtracker)
			 */
			initExecution( pdaRun->exec, prg, &pdaRun->rcodeCollect, pdaRun, fsmRun, pdaRun->frameId );

			/* Call execution. */
			*pinstr = pdaRun->fi->codeWV;
			break;
		}

		case PcrPreEof: {
			/* Execute the translation. */
			initExecution( pdaRun->exec, prg, &pdaRun->rcodeCollect, pdaRun, fsmRun, pdaRun->frameId );

			/* Call execution. */
			*pinstr = pdaRun->fi->codeWV;
			break;
		}

		case PcrRevIgnore:
		case PcrRevIgnore2:
		{
			initExecution( pdaRun->exec, prg, &pdaRun->rcodeCollect, pdaRun, fsmRun, -1 );

			*pinstr = popReverseCode( &pdaRun->reverseCode );
			break;
		}

		case PcrRevReduction:
		case PcrRevReduction2:
		{
			initExecution( pdaRun->exec, prg, &pdaRun->rcodeCollect, pdaRun, fsmRun, -1 );

			*pinstr = popReverseCode( &pdaRun->reverseCode );
			break;
		}

		case PcrRevToken:
		case PcrRevToken2:
		{
			initExecution( pdaRun->exec, prg, &pdaRun->rcodeCollect, pdaRun, fsmRun, -1 );

			*pinstr = popReverseCode( &pdaRun->reverseCode );
			break;
		}

		default: {
			fatal( "unknown parsing co-routine stop point -- something is wrong\n" );
		}
	}

	*psp = sp;
}

Tree **executeCode( Execution *exec, Tree **sp, Code *instr )
{
	/* When we exit we are going to verify that we did not eat up any stack
	 * space. */
	Tree **root = sp;
	Program *prg = exec->prg;
	Code c;

again:
	c = *instr++;
	//debug( REALM_BYTECODE, "--in 0x%x\n", c );

	switch ( c ) {
		case IN_RESTORE_LHS: {
			Tree *restore;
			read_tree( restore );

			debug( REALM_BYTECODE, "IN_RESTORE_LHS\n" );
			//assert( exec->lhs == 0 );
			exec->pdaRun->lhs = restore;
			break;
		}
		case IN_LOAD_NIL: {
			debug( REALM_BYTECODE, "IN_LOAD_NIL\n" );
			vm_push( 0 );
			break;
		}
		case IN_LOAD_TRUE: {
			debug( REALM_BYTECODE, "IN_LOAD_TRUE\n" );
			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );
			break;
		}
		case IN_LOAD_FALSE: {
			debug( REALM_BYTECODE, "IN_LOAD_FALSE\n" );
			treeUpref( prg->falseVal );
			vm_push( prg->falseVal );
			break;
		}
		case IN_LOAD_INT: {
			Word i;
			read_word( i );

			debug( REALM_BYTECODE, "IN_LOAD_INT %d\n", i );

			Tree *tree = constructInteger( prg, i );
			treeUpref( tree );
			vm_push( tree );
			break;
		}
		case IN_LOAD_STR: {
			Word offset;
			read_word( offset );

			debug( REALM_BYTECODE, "IN_LOAD_STR %d\n", offset );

			Head *lit = makeLiteral( prg, offset );
			Tree *tree = constructString( prg, lit );
			treeUpref( tree );
			vm_push( tree );
			break;
		}
		case IN_PRINT: {
			int n;
			read_byte( n );
			debug( REALM_BYTECODE, "IN_PRINT %d\n", n );

			while ( n-- > 0 ) {
				Tree *tree = vm_pop();
				printTreeFile( prg, sp, stdout, tree );
				treeDownref( prg, sp, tree );
			}
			break;
		}
		case IN_PRINT_XML_AC: {
			int n;
			read_byte( n );

			debug( REALM_BYTECODE, "IN_PRINT_XML_AC %d\n", n );

			while ( n-- > 0 ) {
				Tree *tree = vm_pop();
				printXmlStdout( prg, sp, tree, true );
				treeDownref( prg, sp, tree );
			}
			break;
		}
		case IN_PRINT_XML: {
			int n;
			read_byte( n );
			debug( REALM_BYTECODE, "IN_PRINT_XML %d", n );

			while ( n-- > 0 ) {
				Tree *tree = vm_pop();
				printXmlStdout( prg, sp, tree, false );
				treeDownref( prg, sp, tree );
			}
			break;
		}
		case IN_PRINT_STREAM: {
			int n;
			read_byte( n );
			debug( REALM_BYTECODE, "IN_PRINT_STREAM\n" );

			Stream *stream = (Stream*)vm_pop();
			while ( n-- > 0 ) {
				Tree *tree = vm_pop();
				printTreeFile( prg, sp, stream->file, tree );
				treeDownref( prg, sp, tree );
			}
			treeDownref( prg, sp, (Tree*)stream );
			break;
		}
		case IN_LOAD_CONTEXT_R: {
			debug( REALM_BYTECODE, "IN_LOAD_CONTEXT_R\n" );

			treeUpref( exec->pdaRun->context );
			vm_push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_CONTEXT_WV: {
			debug( REALM_BYTECODE, "IN_LOAD_CONTEXT_WV\n" );

			treeUpref( exec->pdaRun->context );
			vm_push( exec->pdaRun->context );

			/* Set up the reverse instruction. */
			append( exec->rcodeCollect, IN_LOAD_CONTEXT_BKT );
			exec->rcodeUnitLen = SIZEOF_CODE;
			break;
		}
		case IN_LOAD_CONTEXT_WC: {
			debug( REALM_BYTECODE, "IN_LOAD_CONTEXT_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			treeUpref( exec->pdaRun->context );
			vm_push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_CONTEXT_BKT: {
			debug( REALM_BYTECODE, "IN_LOAD_CONTEXT_BKT\n" );

			treeUpref( exec->pdaRun->context );
			vm_push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_GLOBAL_R: {
			debug( REALM_BYTECODE, "IN_LOAD_GLOBAL_R\n" );

			treeUpref( prg->global );
			vm_push( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_WV: {
			debug( REALM_BYTECODE, "IN_LOAD_GLOBAL_WV\n" );

			treeUpref( prg->global );
			vm_push( prg->global );

			/* Set up the reverse instruction. */
			append( exec->rcodeCollect, IN_LOAD_GLOBAL_BKT );
			exec->rcodeUnitLen = SIZEOF_CODE;
			break;
		}
		case IN_LOAD_GLOBAL_WC: {
			debug( REALM_BYTECODE, "IN_LOAD_GLOBAL_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			treeUpref( prg->global );
			vm_push( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			debug( REALM_BYTECODE, "IN_LOAD_GLOBAL_BKT\n" );

			treeUpref( prg->global );
			vm_push( prg->global );
			break;
		}
		case IN_LOAD_INPUT_R: {
			debug( REALM_BYTECODE, "IN_LOAD_INPUT_R\n" );

			treeUpref( exec->fsmRun->curStream );
			vm_push( exec->fsmRun->curStream );
			break;
		}
		case IN_LOAD_INPUT_WV: {
			debug( REALM_BYTECODE, "IN_LOAD_INPUT_WV\n" );

			treeUpref( exec->fsmRun->curStream );
			vm_push( exec->fsmRun->curStream );

			/* Set up the reverse instruction. */
			append( exec->rcodeCollect, IN_LOAD_INPUT_BKT );
			exec->rcodeUnitLen = SIZEOF_CODE;
			break;
		}
		case IN_LOAD_INPUT_WC: {
			debug( REALM_BYTECODE, "IN_LOAD_INPUT_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			treeUpref( exec->fsmRun->curStream );
			vm_push( exec->fsmRun->curStream );
			break;
		}
		case IN_LOAD_INPUT_BKT: {
			debug( REALM_BYTECODE, "IN_LOAD_INPUT_BKT\n" );

			treeUpref( exec->fsmRun->curStream );
			vm_push( exec->fsmRun->curStream );
			break;
		}
		case IN_LOAD_CTX_R: {
			debug( REALM_BYTECODE, "IN_LOAD_CTX_R\n" );

			treeUpref( exec->pdaRun->context );
			vm_push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_CTX_WV: {
			debug( REALM_BYTECODE, "IN_LOAD_CTX_WV\n" );

			treeUpref( exec->pdaRun->context );
			vm_push( exec->pdaRun->context );

			/* Set up the reverse instruction. */
			append( exec->rcodeCollect, IN_LOAD_INPUT_BKT );
			exec->rcodeUnitLen = SIZEOF_CODE;
			break;
		}
		case IN_LOAD_CTX_WC: {
			debug( REALM_BYTECODE, "IN_LOAD_CTX_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			treeUpref( exec->pdaRun->context );
			vm_push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_CTX_BKT: {
			debug( REALM_BYTECODE, "IN_LOAD_CTX_BKT\n" );

			treeUpref( exec->pdaRun->context );
			vm_push( exec->pdaRun->context );
			break;
		}
		case IN_INIT_CAPTURES: {
			uchar ncaps;
			read_byte( ncaps );

			debug( REALM_BYTECODE, "IN_INIT_CAPTURES\n" );

			/* If there are captures (this is a translate block) then copy them into
			 * the local frame now. */
			LangElInfo *lelInfo = prg->rtd->lelInfo;
			char **mark = exec->fsmRun->mark;

			int i;
			for ( i = 0; i < lelInfo[exec->pdaRun->tokenId].numCaptureAttr; i++ ) {
				CaptureAttr *ca = &prg->rtd->captureAttr[lelInfo[exec->pdaRun->tokenId].captureAttr + i];
				Head *data = stringAllocFull( prg, 
						mark[ca->mark_enter], mark[ca->mark_leave] - mark[ca->mark_enter] );
				Tree *string = constructString( prg, data );
				treeUpref( string );
				setLocal( exec->framePtr, -1 - i, string );
			}
			break;
		}
		case IN_INIT_RHS_EL: {
			Half position;
			short field;
			read_half( position );
			read_half( field );

			debug( REALM_BYTECODE, "IN_INIT_RHS_EL %hd\n", field );

			Tree *val = getRhsEl( prg, exec->pdaRun->lhs, position );
			treeUpref( val );
			vm_local(field) = val;
			break;
		}
		case IN_INIT_LHS_EL: {
			short field;
			uchar save;
			read_half( field );
			read_byte( save );

			debug( REALM_BYTECODE, "IN_INIT_LHS_EL %hd %hhu\n", field, save );

			Tree *val = exec->pdaRun->lhs;
			if ( save )
				exec->pdaRun->parsed = exec->pdaRun->lhs;
			exec->pdaRun->lhs = 0;
			vm_local(field) = val;
			break;
		}
		case IN_STORE_LHS_EL: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_STORE_LHS_EL %hd\n", field );

			Tree *val = vm_local(field);
			vm_local(field) = 0;
			exec->pdaRun->lhs = val;
			break;
		}
		case IN_UITER_ADVANCE: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_UITER_ADVANCE\n" );

			/* Get the iterator. */
			UserIter *uiter = (UserIter*) vm_local(field);

			long stackSize = uiter->stackRoot - vm_ptop();
			assert( uiter->stackSize == stackSize );

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

			debug( REALM_BYTECODE, "IN_UITER_GET_CUR_R\n" );

			UserIter *uiter = (UserIter*) vm_local(field);
			Tree *val = uiter->ref.kid->tree;
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_UITER_GET_CUR_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_UITER_GET_CUR_WC\n" );

			UserIter *uiter = (UserIter*) vm_local(field);
			splitRef( prg, &sp, &uiter->ref );
			Tree *split = uiter->ref.kid->tree;
			treeUpref( split );
			vm_push( split );
			break;
		}
		case IN_UITER_SET_CUR_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_UITER_SET_CUR_WC\n" );

			Tree *t = vm_pop();
			UserIter *uiter = (UserIter*) vm_local(field);
			splitRef( prg, &sp, &uiter->ref );
			Tree *old = uiter->ref.kid->tree;
			setUiterCur( prg, uiter, t );
			treeDownref( prg, sp, old );
			break;
		}
		case IN_GET_LOCAL_R: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_LOCAL_R\n" );

			Tree *val = vm_local(field);
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_LOCAL_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_LOCAL_WC\n" );

			Tree *split = getLocalSplit( prg, exec->framePtr, field );
			treeUpref( split );
			vm_push( split );
			break;
		}
		case IN_SET_LOCAL_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_SET_LOCAL_WC %d\n", field );

			Tree *val = vm_pop();
			treeDownref( prg, sp, vm_local(field) );
			setLocal( exec->framePtr, field, val );
			break;
		}
		case IN_SAVE_RET: {
			debug( REALM_BYTECODE, "IN_SAVE_RET\n" );

			Tree *val = vm_pop();
			vm_local(FR_RV) = val;
			break;
		}
		case IN_GET_LOCAL_REF_R: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_LOCAL_REF_R\n" );

			Ref *ref = (Ref*) vm_plocal(field);
			Tree *val = ref->kid->tree;
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_LOCAL_REF_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_LOCAL_REF_WC\n" );

			Ref *ref = (Ref*) vm_plocal(field);
			splitRef( prg, &sp, ref );
			Tree *val = ref->kid->tree;
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_SET_LOCAL_REF_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_SET_LOCAL_REF_WC\n" );

			Tree *val = vm_pop();
			Ref *ref = (Ref*) vm_plocal(field);
			splitRef( prg, &sp, ref );
			refSetValue( ref, val );
			break;
		}
		case IN_GET_FIELD_R: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_FIELD_R %d\n", field );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = getField( obj, field );
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_FIELD_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_FIELD_WC %d\n", field );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
			vm_push( split );
			break;
		}
		case IN_GET_FIELD_WV: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_FIELD_WV\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
			vm_push( split );

			/* Set up the reverse instruction. */
			append( exec->rcodeCollect, IN_GET_FIELD_BKT );
			appendHalf( exec->rcodeCollect, field );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_HALF;
			break;
		}
		case IN_GET_FIELD_BKT: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_FIELD_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
			vm_push( split );
			break;
		}
		case IN_SET_FIELD_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_SET_FIELD_WC %d\n", field );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();
			treeDownref( prg, sp, obj );

			/* Downref the old value. */
			Tree *prev = getField( obj, field );
			treeDownref( prg, sp, prev );

			setField( prg, obj, field, val );
			break;
		}
		case IN_SET_FIELD_WV: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_SET_FIELD_WV %d\n", field );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();
			treeDownref( prg, sp, obj );

			/* Save the old value, then set the field. */
			Tree *prev = getField( obj, field );
			setField( prg, obj, field, val );

			/* Set up the reverse instruction. */
			append( exec->rcodeCollect, IN_SET_FIELD_BKT );
			appendHalf( exec->rcodeCollect, field );
			appendWord( exec->rcodeCollect, (Word)prev );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_HALF + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );
			/* FLUSH */
			break;
		}
		case IN_SET_FIELD_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( REALM_BYTECODE, "IN_SET_FIELD_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			/* Downref the old value. */
			Tree *prev = getField( obj, field );
			treeDownref( prg, sp, prev );

			setField( prg, obj, field, val );
			break;
		}
		case IN_SET_FIELD_LEAVE_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_SET_FIELD_LEAVE_WC\n" );

			/* Note that we don't downref the object here because we are
			 * leaving it on the stack. */
			Tree *obj = vm_pop();
			Tree *val = vm_pop();

			/* Downref the old value. */
			Tree *prev = getField( obj, field );
			treeDownref( prg, sp, prev );

			/* Set the field. */
			setField( prg, obj, field, val );

			/* Leave the object on the top of the stack. */
			vm_push( obj );
			break;
		}
		case IN_GET_RHS_VAL_R: {
			int i, done = 0;
			uchar len;
			Tree *val, *res = 0;

			val = vm_pop();

			read_byte( len );
			for ( i = 0; i < len; i++ ) {
				uchar prodNum, childNum;
				read_byte( prodNum );
				read_byte( childNum );
				if ( !done && val->prodNum == prodNum ) {
					res = getRhsEl( prg, val, childNum );
					done = 1;
				}
			}

			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_POP: {
			debug( REALM_BYTECODE, "IN_POP\n" );

			Tree *val = vm_pop();
			treeDownref( prg, sp, val );
			break;
		}
		case IN_POP_N_WORDS: {
			short n;
			read_half( n );

			debug( REALM_BYTECODE, "IN_POP_N_WORDS\n" );

			vm_popn( n );
			break;
		}
		case IN_SPRINTF: {
			debug( REALM_BYTECODE, "IN_SPRINTF\n" );

			Tree *f = vm_pop();
			f++;
			Tree *integer = vm_pop();
			Tree *format = vm_pop();
			Head *res = stringSprintf( prg, (Str*)format, (Int*)integer );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			vm_push( str );
			treeDownref( prg, sp, integer );
			treeDownref( prg, sp, format );
			break;
		}
		case IN_STR_ATOI: {
			debug( REALM_BYTECODE, "IN_STR_ATOI\n" );

			Str *str = (Str*)vm_pop();
			Word res = strAtoi( str->value );
			Tree *integer = constructInteger( prg, res );
			treeUpref( integer );
			vm_push( integer );
			treeDownref( prg, sp, (Tree*)str );
			break;
		}
		case IN_INT_TO_STR: {
			debug( REALM_BYTECODE, "IN_INT_TO_STR\n" );

			Int *i = (Int*)vm_pop();
			Head *res = intToStr( prg, i->value );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			vm_push( str );
			treeDownref( prg, sp, (Tree*) i );
			break;
		}
		case IN_TREE_TO_STR: {
			debug( REALM_BYTECODE, "IN_TREE_TO_STR\n" );

			Tree *tree = vm_pop();
			Head *res = treeToStr( prg, sp, tree );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			vm_push( str );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_CONCAT_STR: {
			debug( REALM_BYTECODE, "IN_CONCAT_STR\n" );

			Str *s2 = (Str*)vm_pop();
			Str *s1 = (Str*)vm_pop();
			Head *res = concatStr( s1->value, s2->value );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			treeDownref( prg, sp, (Tree*)s1 );
			treeDownref( prg, sp, (Tree*)s2 );
			vm_push( str );
			break;
		}
		case IN_STR_UORD8: {
			debug( REALM_BYTECODE, "IN_STR_UORD8\n" );

			Str *str = (Str*)vm_pop();
			Word res = strUord8( str->value );
			Tree *tree = constructInteger( prg, res );
			treeUpref( tree );
			vm_push( tree );
			treeDownref( prg, sp, (Tree*)str );
			break;
		}
		case IN_STR_UORD16: {
			debug( REALM_BYTECODE, "IN_STR_UORD16\n" );

			Str *str = (Str*)vm_pop();
			Word res = strUord16( str->value );
			Tree *tree = constructInteger( prg, res );
			treeUpref( tree );
			vm_push( tree );
			treeDownref( prg, sp, (Tree*)str );
			break;
		}

		case IN_STR_LENGTH: {
			debug( REALM_BYTECODE, "IN_STR_LENGTH\n" );

			Str *str = (Str*)vm_pop();
			long len = stringLength( str->value );
			Tree *res = constructInteger( prg, len );
			treeUpref( res );
			vm_push( res );
			treeDownref( prg, sp, (Tree*)str );
			break;
		}
		case IN_JMP_FALSE: {
			short dist;
			read_half( dist );

			debug( REALM_BYTECODE, "IN_JMP_FALSE %d\n", dist );

			Tree *tree = vm_pop();
			if ( testFalse( prg, tree ) )
				instr += dist;
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_JMP_TRUE: {
			short dist;
			read_half( dist );

			debug( REALM_BYTECODE, "IN_JMP_TRUE %d\n", dist );

			Tree *tree = vm_pop();
			if ( !testFalse( prg, tree ) )
				instr += dist;
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_JMP: {
			short dist;
			read_half( dist );

			debug( REALM_BYTECODE, "IN_JMP\n" );

			instr += dist;
			break;
		}
		case IN_REJECT: {
			debug( REALM_BYTECODE, "IN_REJECT\n" );
			exec->pdaRun->reject = true;
			break;
		}

		/*
		 * Binary comparison operators.
		 */
		case IN_TST_EQL: {
			debug( REALM_BYTECODE, "IN_TST_EQL\n" );

			Tree *o2 = vm_pop();
			Tree *o1 = vm_pop();
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r ? prg->falseVal : prg->trueVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
			break;
		}
		case IN_TST_NOT_EQL: {
			debug( REALM_BYTECODE, "IN_TST_NOT_EQL\n" );

			Tree *o2 = vm_pop();
			Tree *o1 = vm_pop();
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
			break;
		}
		case IN_TST_LESS: {
			debug( REALM_BYTECODE, "IN_TST_LESS\n" );

			Tree *o2 = vm_pop();
			Tree *o1 = vm_pop();
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r < 0 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
			break;
		}
		case IN_TST_LESS_EQL: {
			debug( REALM_BYTECODE, "IN_TST_LESS_EQL\n" );

			Tree *o2 = vm_pop();
			Tree *o1 = vm_pop();
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r <= 0 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
		}
		case IN_TST_GRTR: {
			debug( REALM_BYTECODE, "IN_TST_GRTR\n" );

			Tree *o2 = vm_pop();
			Tree *o1 = vm_pop();
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r > 0 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
			break;
		}
		case IN_TST_GRTR_EQL: {
			debug( REALM_BYTECODE, "IN_TST_GRTR_EQL\n" );

			Tree *o2 = (Tree*)vm_pop();
			Tree *o1 = (Tree*)vm_pop();
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r >= 0 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
			break;
		}
		case IN_TST_LOGICAL_AND: {
			debug( REALM_BYTECODE, "IN_TST_LOGICAL_AND\n" );

			Tree *o2 = vm_pop();
			Tree *o1 = vm_pop();
			long v2 = !testFalse( prg, o2 );
			long v1 = !testFalse( prg, o1 );
			Word r = v1 && v2;
			Tree *val = r ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
			break;
		}
		case IN_TST_LOGICAL_OR: {
			debug( REALM_BYTECODE, "IN_TST_LOGICAL_OR\n" );

			Tree *o2 = vm_pop();
			Tree *o1 = vm_pop();
			long v2 = !testFalse( prg, o2 );
			long v1 = !testFalse( prg, o1 );
			Word r = v1 || v2;
			Tree *val = r ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
			break;
		}
		case IN_NOT: {
			debug( REALM_BYTECODE, "IN_NOT\n" );

			Tree *tree = (Tree*)vm_pop();
			long r = testFalse( prg, tree );
			Tree *val = r ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, tree );
			break;
		}

		case IN_ADD_INT: {
			debug( REALM_BYTECODE, "IN_ADD_INT\n" );

			Int *o2 = (Int*)vm_pop();
			Int *o1 = (Int*)vm_pop();
			long r = o1->value + o2->value;
			Tree *tree = constructInteger( prg, r );
			treeUpref( tree );
			vm_push( tree );
			treeDownref( prg, sp, (Tree*)o1 );
			treeDownref( prg, sp, (Tree*)o2 );
			break;
		}
		case IN_MULT_INT: {
			debug( REALM_BYTECODE, "IN_MULT_INT\n" );

			Int *o2 = (Int*)vm_pop();
			Int *o1 = (Int*)vm_pop();
			long r = o1->value * o2->value;
			Tree *tree = constructInteger( prg, r );
			treeUpref( tree );
			vm_push( tree );
			treeDownref( prg, sp, (Tree*)o1 );
			treeDownref( prg, sp, (Tree*)o2 );
			break;
		}
		case IN_DIV_INT: {
			debug( REALM_BYTECODE, "IN_DIV_INT\n" );

			Int *o2 = (Int*)vm_pop();
			Int *o1 = (Int*)vm_pop();
			long r = o1->value / o2->value;
			Tree *tree = constructInteger( prg, r );
			treeUpref( tree );
			vm_push( tree );
			treeDownref( prg, sp, (Tree*)o1 );
			treeDownref( prg, sp, (Tree*)o2 );
			break;
		}
		case IN_SUB_INT: {
			debug( REALM_BYTECODE, "IN_SUB_INT\n" );

			Int *o2 = (Int*)vm_pop();
			Int *o1 = (Int*)vm_pop();
			long r = o1->value - o2->value;
			Tree *tree = constructInteger( prg, r );
			treeUpref( tree );
			vm_push( tree );
			treeDownref( prg, sp, (Tree*)o1 );
			treeDownref( prg, sp, (Tree*)o2 );
			break;
		}
		case IN_DUP_TOP_OFF: {
			short off;
			read_half( off );

			debug( REALM_BYTECODE, "IN_DUP_N\n" );

			Tree *val = vm_top_off(off);
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_DUP_TOP: {
			debug( REALM_BYTECODE, "IN_DUP_TOP\n" );

			Tree *val = vm_top();
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_TRITER_FROM_REF: {
			short field;
			Half searchTypeId;
			read_half( field );
			read_half( searchTypeId );

			debug( REALM_BYTECODE, "IN_TRITER_FROM_REF\n" );

			Ref rootRef;
			rootRef.kid = (Kid*)vm_pop();
			rootRef.next = (Ref*)vm_pop();
			void *mem = vm_plocal(field);
			initTreeIter( (TreeIter*)mem, &rootRef, searchTypeId, vm_ptop() );
			break;
		}
		case IN_TRITER_DESTROY: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_TRITER_DESTROY\n" );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			treeIterDestroy( &sp, iter );
			break;
		}
		case IN_REV_TRITER_FROM_REF: {
			short field;
			Half searchTypeId;
			read_half( field );
			read_half( searchTypeId );

			debug( REALM_BYTECODE, "IN_REV_TRITER_FROM_REF\n" );

			Ref rootRef;
			rootRef.kid = (Kid*)vm_pop();
			rootRef.next = (Ref*)vm_pop();

			Tree **stackRoot = vm_ptop();

			int children = 0;
			Kid *kid = treeChild( prg, rootRef.kid->tree );
			while ( kid != 0 ) {
				children++;
				vm_push( (SW) kid );
				kid = kid->next;
			}

			void *mem = vm_plocal(field);
			initRevTreeIter( (RevTreeIter*)mem, &rootRef, searchTypeId, stackRoot, children );
			break;
		}
		case IN_REV_TRITER_DESTROY: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_REV_TRITER_DESTROY\n" );

			RevTreeIter *iter = (RevTreeIter*) vm_plocal(field);
			long curStackSize = iter->stackRoot - vm_ptop();
			assert( iter->stackSize == curStackSize );
			vm_popn( iter->stackSize );
			break;
		}
		case IN_TREE_SEARCH: {
			Word id;
			read_word( id );

			debug( REALM_BYTECODE, "IN_TREE_SEARCH\n" );

			Tree *tree = vm_pop();
			Tree *res = treeSearch2( prg, tree, id );
			treeUpref( res );
			vm_push( res );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_TRITER_ADVANCE: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_TRITER_ADVANCE\n" );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *res = treeIterAdvance( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_TRITER_NEXT_CHILD: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_TRITER_NEXT_CHILD\n" );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *res = treeIterNextChild( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_REV_TRITER_PREV_CHILD: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_REV_TRITER_PREV_CHILD\n" );

			RevTreeIter *iter = (RevTreeIter*) vm_plocal(field);
			Tree *res = treeRevIterPrevChild( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_TRITER_NEXT_REPEAT: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_TRITER_NEXT_REPEAT\n" );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *res = treeIterNextRepeat( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_TRITER_PREV_REPEAT: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_TRITER_PREV_REPEAT\n" );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *res = treeIterPrevRepeat( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_TRITER_GET_CUR_R: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_TRITER_GET_CUR_R\n" );
			
			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *tree = treeIterDerefCur( iter );
			treeUpref( tree );
			vm_push( tree );
			break;
		}
		case IN_TRITER_GET_CUR_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_TRITER_GET_CUR_WC\n" );
			
			TreeIter *iter = (TreeIter*) vm_plocal(field);
			splitIterCur( prg, &sp, iter );
			Tree *tree = treeIterDerefCur( iter );
			treeUpref( tree );
			vm_push( tree );
			break;
		}
		case IN_TRITER_SET_CUR_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_TRITER_SET_CUR_WC\n" );

			Tree *tree = vm_pop();
			TreeIter *iter = (TreeIter*) vm_plocal(field);
			splitIterCur( prg, &sp, iter );
			Tree *old = treeIterDerefCur( iter );
			setTriterCur( prg, iter, tree );
			treeDownref( prg, sp, old );
			break;
		}
		case IN_MATCH: {
			Half patternId;
			read_half( patternId );

			debug( REALM_BYTECODE, "IN_MATCH\n" );

			Tree *tree = vm_pop();

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
			int matched = matchPattern( bindings, prg, rootNode, &kid, false );

			if ( !matched )
				memset( bindings, 0, sizeof(Tree*)*(1+numBindings) );
			else {
				int b;
				for ( b = 1; b <= numBindings; b++ )
					assert( bindings[b] != 0 );
			}

			Tree *result = matched ? tree : 0;
			treeUpref( result );
			vm_push( result ? tree : 0 );
			int b;
			for ( b = 1; b <= numBindings; b++ ) {
				treeUpref( bindings[b] );
				vm_push( bindings[b] );
			}

			treeDownref( prg, sp, tree );
			break;
		}

		case IN_GET_ACCUM_CTX_R: {
			debug( REALM_BYTECODE, "IN_GET_ACCUM_CTX_R\n" );

			Tree *obj = vm_pop();
			Tree *ctx = ((Accum*)obj)->pdaRun->context;
			treeUpref( ctx );
			vm_push( ctx );
			treeDownref( prg, sp, obj );
			break;
		}

		case IN_SET_ACCUM_CTX_WC: {
			debug( REALM_BYTECODE, "IN_SET_ACCUM_CTX_WC\n" );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();
			parserSetContext( prg, sp, (Accum*)obj, val );
			treeDownref( prg, sp, obj );
			//treeDownref( prg, sp, val );
			break;
		}

//		case IN_GET_ACCUM_CTX_WC:
//		case IN_GET_ACCUM_CTX_WV:
//		case IN_SET_ACCUM_CTX_WC:
//		case IN_SET_ACCUM_CTX_WV:
//			break;

		case IN_EXTRACT_INPUT_WC: {
			debug( REALM_BYTECODE, "IN_EXTRACT_INPUT_WC \n" );

			Tree *accum = vm_pop();
			Tree *input = extractInput( prg, (Accum*)accum );
			treeUpref( input );
			vm_push( input );
			treeDownref( prg, sp, accum );
			break;
		}

		case IN_EXTRACT_INPUT_WV: {
			debug( REALM_BYTECODE, "IN_EXTRACT_INPUT_WV \n" );

			Tree *accum = vm_pop();
			Tree *input = extractInput( prg, (Accum*)accum );
			treeUpref( input );
			vm_push( input );
			treeDownref( prg, sp, accum );
//
//			append( exec->rcodeCollect, IN_EXTRACT_INPUT_BKT );
//			exec->rcodeUnitLen += SIZEOF_CODE;
			break;
		}

		case IN_EXTRACT_INPUT_BKT: {
			debug( REALM_BYTECODE, "IN_EXTRACT_INPUT_BKT\n" );
			break;
		}

		case IN_SET_INPUT_WC: {
			debug( REALM_BYTECODE, "IN_SET_INPUT_WC \n" );

			Tree *accum = vm_pop();
			Tree *input = vm_pop();
			setInput( prg, sp, (Accum*)accum, (Stream*)input );
			treeDownref( prg, sp, accum );
			treeDownref( prg, sp, input );
			break;
		}

		case IN_STREAM_APPEND_WC: {
			debug( REALM_BYTECODE, "IN_STREAM_APPEND_WC \n" );

			Tree *stream = vm_pop();
			Tree *input = vm_pop();
			streamAppend( prg, sp, input, (Stream*)stream );

			treeDownref( prg, sp, input );
			treeDownref( prg, sp, stream );
			break;
		}
		case IN_STREAM_APPEND_WV: {
			debug( REALM_BYTECODE, "IN_STREAM_APPEND_WV \n" );

			Tree *stream = vm_pop();
			Tree *input = vm_pop();
			Word len = streamAppend( prg, sp, input, (Stream*)stream );

			append( exec->rcodeCollect, IN_STREAM_APPEND_BKT );
			appendWord( exec->rcodeCollect, (Word) stream );
			appendWord( exec->rcodeCollect, (Word) input );
			appendWord( exec->rcodeCollect, (Word) len );
			append( exec->rcodeCollect, SIZEOF_CODE + 3 * SIZEOF_WORD );
			break;
		}
		case IN_STREAM_APPEND_BKT: {
			Tree *stream;
			Tree *input;
			Word len;
			read_tree( stream );
			read_tree( input );
			read_word( len );

			debug( REALM_BYTECODE, "IN_STREAM_APPEND_BKT\n" );

			undoStreamAppend( prg, sp, ((Stream*)stream)->in, len );
			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, input );
			break;
		}

		case IN_PARSE_FRAG_WC: {
			Half stopId;
			read_half( stopId );

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_WC %d\n", stopId );

			Accum *accum = (Accum*)vm_pop();

			long pcr = parseFrag( prg, sp, accum, stopId, PcrStart );

			vm_push( (SW)pcr );
			vm_push( (Tree*)accum );
			break;
		}

		case IN_PARSE_FRAG_WC2: {
			Half stopId;
			read_half( stopId );

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_WC2 %d\n", stopId );

			Accum *accum = (Accum*)vm_pop();
			long pcr = (long)vm_pop();

			if ( pcr != PcrDone ) {
				/* Push the execution. */
				vm_push( (SW)exec->prg );
				vm_push( (SW)exec->pdaRun );
				vm_push( (SW)exec->fsmRun );
				vm_push( (SW)exec->framePtr );
				vm_push( (SW)exec->iframePtr );
				vm_push( (SW)exec->frameId );
				vm_push( (SW)exec->rcodeCollect );
				vm_push( (SW)exec->rcodeUnitLen );

				accum->pdaRun->exec = exec;

				vm_push( (SW)pcr );
				vm_push( (SW)accum );
				vm_push( (SW)instr );

				callParseBlock( &instr, &sp, pcr, prg, exec, accum );
			}
			else {
				treeDownref( prg, sp, (Tree*)accum );

				instr += SIZEOF_CODE + SIZEOF_HALF;
				if ( prg->induceExit )
					goto out;
			}
			break;
		}

		case IN_PARSE_FRAG_WC3: {
			Half stopId;
			read_half( stopId );

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_WC3 %d\n", stopId );

			Accum *accum = (Accum*)vm_pop();
			long pcr = (long)vm_pop();

			pcr = parseFrag( prg, sp, accum, stopId, pcr );

			/* Pop the saved execution. */
			exec->rcodeUnitLen = ( long ) vm_pop();
			exec->rcodeCollect = ( RtCodeVect * ) vm_pop();
			exec->frameId = ( long ) vm_pop();
			exec->iframePtr = ( Tree ** ) vm_pop();
			exec->framePtr = ( Tree ** ) vm_pop();
			exec->fsmRun = ( FsmRun * ) vm_pop();
			exec->pdaRun = ( PdaRun * ) vm_pop();
			exec->prg = ( struct ColmProgram * ) vm_pop();

			vm_push( (SW)pcr );
			vm_push( (Tree*)accum );

			/* Back up to the frag 2. */
			instr -= SIZEOF_CODE + SIZEOF_HALF;
			instr -= SIZEOF_CODE + SIZEOF_HALF;
			break;
		}

		case IN_PARSE_FRAG_WV: {
			Half stopId;
			read_half( stopId );

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_WV\n" );

			Accum *accum = (Accum*)vm_pop();

			long steps = accum->pdaRun->steps;
			long pcr = parseFrag( prg, sp, accum, stopId, PcrStart );

			vm_push( (SW)pcr );
			vm_push( (SW)steps );
			vm_push( (Tree*)accum );
			break;
		}

		case IN_PARSE_FRAG_WV2: {
			Half stopId;
			read_half( stopId );

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_WV2\n" );

			Accum *accum = (Accum*)vm_pop();
			long steps = (long)vm_pop();
			long pcr = (long)vm_pop();

			if ( pcr != PcrDone ) {
				/* Push the execution. */
				vm_push( (SW)exec->prg );
				vm_push( (SW)exec->pdaRun );
				vm_push( (SW)exec->fsmRun );
				vm_push( (SW)exec->framePtr );
				vm_push( (SW)exec->iframePtr );
				vm_push( (SW)exec->frameId );
				vm_push( (SW)exec->rcodeCollect );
				vm_push( (SW)exec->rcodeUnitLen );

				accum->pdaRun->exec = exec;

				vm_push( (SW)pcr );
				vm_push( (SW)steps );
				vm_push( (SW)accum );
				vm_push( (SW)instr );

				callParseBlock( &instr, &sp, pcr, prg, exec, accum );
			}
			else {
				instr += SIZEOF_CODE + SIZEOF_HALF;

				append( exec->rcodeCollect, IN_PARSE_FRAG_BKT );
				appendWord( exec->rcodeCollect, (Word) accum );
				appendWord( exec->rcodeCollect, steps );
				append( exec->rcodeCollect, IN_PARSE_FRAG_BKT2 );
				append( exec->rcodeCollect, IN_PARSE_FRAG_BKT3 );
				append( exec->rcodeCollect, 3 * SIZEOF_CODE + 2 * SIZEOF_WORD );
				if ( prg->induceExit )
					goto out;
			}
			break;
		}

		case IN_PARSE_FRAG_WV3: {
			Half stopId;
			read_half( stopId );

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_WV3 \n" );

			Accum *accum = (Accum*)vm_pop();
			long steps = (long)vm_pop();
			long pcr = (long)vm_pop();

			pcr = parseFrag( prg, sp, accum, stopId, pcr );

			/* Pop the saved execution. */
			exec->rcodeUnitLen = ( long ) vm_pop();
			exec->rcodeCollect = ( RtCodeVect * ) vm_pop();
			exec->frameId = ( long ) vm_pop();
			exec->iframePtr = ( Tree ** ) vm_pop();
			exec->framePtr = ( Tree ** ) vm_pop();
			exec->fsmRun = ( FsmRun * ) vm_pop();
			exec->pdaRun = ( PdaRun * ) vm_pop();
			exec->prg = ( struct ColmProgram * ) vm_pop();

			vm_push( (SW)pcr );
			vm_push( (SW)steps );
			vm_push( (SW)accum );

			/* Back up to the frag 2. */
			instr -= SIZEOF_CODE + SIZEOF_HALF;
			instr -= SIZEOF_CODE + SIZEOF_HALF;
			break;
		}

		case IN_PARSE_FRAG_BKT: {
			Accum *accum;
			long steps;
			read_word_type( Accum*, accum );
			read_word( steps );

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_BKT %ld", steps );

			long pcr = undoParseFrag( prg, sp, accum, steps, PcrStart );

			vm_push( (SW)pcr );
			vm_push( (SW)steps );
			vm_push( (SW)accum );
			break;
		}

		case IN_PARSE_FRAG_BKT2: {
			Accum *accum = (Accum*)vm_pop();
			long steps = (long)vm_pop();
			long pcr = (long)vm_pop();

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_BKT2 %ld", steps );

			if ( pcr != PcrDone ) {
				/* Push the execution. */
				vm_push( (SW)exec->prg );
				vm_push( (SW)exec->pdaRun );
				vm_push( (SW)exec->fsmRun );
				vm_push( (SW)exec->framePtr );
				vm_push( (SW)exec->iframePtr );
				vm_push( (SW)exec->frameId );
				vm_push( (SW)exec->rcodeCollect );
				vm_push( (SW)exec->rcodeUnitLen );

				accum->pdaRun->exec = exec;

				vm_push( (SW)pcr );
				vm_push( (SW)steps );
				vm_push( (SW)accum );
				vm_push( (SW)instr );

				callParseBlock( &instr, &sp, pcr, prg, exec, accum );
			}
			else {
				instr += SIZEOF_CODE;

				treeDownref( prg, sp, (Tree*)accum );
			}
			break;
		}

		case IN_PARSE_FRAG_BKT3: {
			Accum *accum = (Accum*)vm_pop();
			long steps = (long)vm_pop();
			long pcr = (long)vm_pop();

			debug( REALM_BYTECODE, "IN_PARSE_FRAG_BKT3 %ld", steps );

			pcr = undoParseFrag( prg, sp, accum, steps, pcr );

			/* Pop the saved execution. */
			exec->rcodeUnitLen = ( long ) vm_pop();
			exec->rcodeCollect = ( RtCodeVect * ) vm_pop();
			exec->frameId = ( long ) vm_pop();
			exec->iframePtr = ( Tree ** ) vm_pop();
			exec->framePtr = ( Tree ** ) vm_pop();
			exec->fsmRun = ( FsmRun * ) vm_pop();
			exec->pdaRun = ( PdaRun * ) vm_pop();
			exec->prg = ( struct ColmProgram * ) vm_pop();

			vm_push( (SW)pcr );
			vm_push( (SW)steps );
			vm_push( (SW)accum );

			/* Back up to the frag 2. */
			instr -= SIZEOF_CODE;
			instr -= SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FINISH_WC: {
			debug( REALM_BYTECODE, "IN_PARSE_FINISH_WC\n" );

			Accum *accum = (Accum*)vm_pop();

			extractInput( prg, accum );

			Tree *result = 0;
			long pcr = parseFinish( &result, prg, sp, accum, false, PcrStart );

			vm_push( (SW)pcr );
			vm_push( result );
			vm_push( (SW)accum );
			break;
		}

		case IN_PARSE_FINISH_WC2: {
			debug( REALM_BYTECODE, "IN_PARSE_FINISH_WC2\n" );

			Accum *accum = (Accum*)vm_pop();
			Tree *result = vm_pop();
			long pcr = (long)vm_pop();

			if ( pcr != PcrDone ) {
				/* Push the execution. */
				vm_push( (SW)exec->prg );
				vm_push( (SW)exec->pdaRun );
				vm_push( (SW)exec->fsmRun );
				vm_push( (SW)exec->framePtr );
				vm_push( (SW)exec->iframePtr );
				vm_push( (SW)exec->frameId );
				vm_push( (SW)exec->rcodeCollect );
				vm_push( (SW)exec->rcodeUnitLen );

				accum->pdaRun->exec = exec;

				vm_push( (SW)pcr );
				vm_push( result );
				vm_push( (SW)accum );
				vm_push( (SW)instr );

				callParseBlock( &instr, &sp, pcr, prg, exec, accum );
			}
			else {
				instr += SIZEOF_CODE;

				vm_push( result );
				treeDownref( prg, sp, (Tree*)accum );
				if ( prg->induceExit )
					goto out;
			}
			break;
		}

		case IN_PARSE_FINISH_WC3: {
			debug( REALM_BYTECODE, "IN_PARSE_FINISH_WC3\n" );

			Accum *accum = (Accum*)vm_pop();
			Tree *result = vm_pop();
			long pcr = (long)vm_pop();

			pcr = parseFinish( &result, prg, sp, accum, false, pcr );

			/* Pop the saved execution. */
			exec->rcodeUnitLen = ( long ) vm_pop();
			exec->rcodeCollect = ( RtCodeVect * ) vm_pop();
			exec->frameId = ( long ) vm_pop();
			exec->iframePtr = ( Tree ** ) vm_pop();
			exec->framePtr = ( Tree ** ) vm_pop();
			exec->fsmRun = ( FsmRun * ) vm_pop();
			exec->pdaRun = ( PdaRun * ) vm_pop();
			exec->prg = ( struct ColmProgram * ) vm_pop();

			vm_push( (SW)pcr );
			vm_push( result );
			vm_push( (SW)accum );

			instr -= SIZEOF_CODE;
			instr -= SIZEOF_CODE;

			break;
		}

		case IN_PARSE_FINISH_WV: {
			debug( REALM_BYTECODE, "IN_PARSE_FINISH_WV\n" );

			Accum *accum = (Accum*)vm_pop();

			long steps = accum->pdaRun->steps;
			extractInput( prg, accum );
			Tree *result = 0;
			long pcr = parseFinish( &result, prg, sp, accum, true, PcrStart );

			vm_push( (SW)pcr );
			vm_push( (SW)steps );
			vm_push( result );
			vm_push( (SW)accum );
			break;
		}

		case IN_PARSE_FINISH_WV2: {
			debug( REALM_BYTECODE, "IN_PARSE_FINISH_WV2\n" );

			Accum *accum = (Accum*)vm_pop();
			Tree *result = vm_pop();
			long steps = (long)vm_pop();
			long pcr = (long)vm_pop();

			if ( pcr != PcrDone ) {
				/* Push the execution. */
				vm_push( (SW)exec->prg );
				vm_push( (SW)exec->pdaRun );
				vm_push( (SW)exec->fsmRun );
				vm_push( (SW)exec->framePtr );
				vm_push( (SW)exec->iframePtr );
				vm_push( (SW)exec->frameId );
				vm_push( (SW)exec->rcodeCollect );
				vm_push( (SW)exec->rcodeUnitLen );

				accum->pdaRun->exec = exec;

				vm_push( (SW)pcr );
				vm_push( (SW)steps );
				vm_push( result );
				vm_push( (SW)accum );
				vm_push( (SW)instr );

				callParseBlock( &instr, &sp, pcr, prg, exec, accum );
			}
			else {
				instr += SIZEOF_CODE;

				vm_push( result );

				append( exec->rcodeCollect, IN_PARSE_FINISH_BKT );
				appendWord( exec->rcodeCollect, (Word) accum );
				appendWord( exec->rcodeCollect, (Word) steps );
				append( exec->rcodeCollect, IN_PARSE_FINISH_BKT2 );
				append( exec->rcodeCollect, IN_PARSE_FINISH_BKT3 );
				append( exec->rcodeCollect, 3 * SIZEOF_CODE + 2 * SIZEOF_WORD );
				if ( prg->induceExit )
					goto out;
			}
			break;
		}

		case IN_PARSE_FINISH_WV3: {
			debug( REALM_BYTECODE, "IN_PARSE_FINISH_WV3\n" );

			Accum *accum = (Accum*)vm_pop();
			Tree *result = vm_pop();
			long steps = (long)vm_pop();
			long pcr = (long)vm_pop();

			pcr = parseFinish( &result, prg, sp, accum, true, pcr );

			/* Pop the saved execution. */
			exec->rcodeUnitLen = ( long ) vm_pop();
			exec->rcodeCollect = ( RtCodeVect * ) vm_pop();
			exec->frameId = ( long ) vm_pop();
			exec->iframePtr = ( Tree ** ) vm_pop();
			exec->framePtr = ( Tree ** ) vm_pop();
			exec->fsmRun = ( FsmRun * ) vm_pop();
			exec->pdaRun = ( PdaRun * ) vm_pop();
			exec->prg = ( struct ColmProgram * ) vm_pop();

			vm_push( (SW)pcr );
			vm_push( (SW)steps );
			vm_push( result );
			vm_push( (SW)accum );

			instr -= SIZEOF_CODE;
			instr -= SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FINISH_BKT: {
			Accum *accum;
			Word steps;

			read_word_type( Accum*, accum );
			read_word( steps );

			debug( REALM_BYTECODE, "IN_PARSE_FINISH_BKT\n" );

			long pcr = undoParseFrag( prg, sp, accum, steps, PcrStart );

			vm_push( (SW)pcr );
			vm_push( (SW)steps );
			vm_push( (SW)accum );
			break;
		}

		case IN_PARSE_FINISH_BKT2: {
			Accum *accum = (Accum*)vm_pop();
			long steps = (long)vm_pop();
			long pcr = (long)vm_pop();

			debug( REALM_BYTECODE, "IN_PARSE_FINISH_BKT2\n" );

			if ( pcr != PcrDone ) {
				/* Push the execution. */
				vm_push( (SW)exec->prg );
				vm_push( (SW)exec->pdaRun );
				vm_push( (SW)exec->fsmRun );
				vm_push( (SW)exec->framePtr );
				vm_push( (SW)exec->iframePtr );
				vm_push( (SW)exec->frameId );
				vm_push( (SW)exec->rcodeCollect );
				vm_push( (SW)exec->rcodeUnitLen );

				accum->pdaRun->exec = exec;

				vm_push( (SW)pcr );
				vm_push( (SW)steps );
				vm_push( (SW)accum );
				vm_push( (SW)instr );

				callParseBlock( &instr, &sp, pcr, prg, exec, accum );
			}
			else {
				instr += SIZEOF_CODE;

				accum->stream->in->eof = false;

				/* This needs an implementation. */
				treeDownref( prg, sp, (Tree*)accum );
			}
			break;
		}

		case IN_PARSE_FINISH_BKT3: {
			Accum *accum = (Accum*)vm_pop();
			long steps = (long)vm_pop();
			long pcr = (long)vm_pop();

			debug( REALM_BYTECODE, "IN_PARSE_FINISH_BKT3\n" );

			pcr = undoParseFrag( prg, sp, accum, steps, pcr );

			/* Pop the saved execution. */
			exec->rcodeUnitLen = ( long ) vm_pop();
			exec->rcodeCollect = ( RtCodeVect * ) vm_pop();
			exec->frameId = ( long ) vm_pop();
			exec->iframePtr = ( Tree ** ) vm_pop();
			exec->framePtr = ( Tree ** ) vm_pop();
			exec->fsmRun = ( FsmRun * ) vm_pop();
			exec->pdaRun = ( PdaRun * ) vm_pop();
			exec->prg = ( struct ColmProgram * ) vm_pop();

			vm_push( (SW)pcr );
			vm_push( (SW)steps );
			vm_push( (SW)accum );

			/* Back up to the frag 2. */
			instr -= SIZEOF_CODE;
			instr -= SIZEOF_CODE;
			break;
		}

		case IN_PCR_RET: {
			debug( REALM_BYTECODE, "IN_PCR_RET\n" );

			instr = (Code*) vm_pop();

			if ( instr == 0 ) {
				fflush( stdout );
				goto out;
			}
			break;
		}

		case IN_PCR_END_DECK: {
			debug( REALM_BYTECODE, "IN_PCR_END_DECK\n" );
			exec->pdaRun->onDeck = false;

			instr = (Code*) vm_pop();

			if ( instr == 0 ) {
				fflush( stdout );
				goto out;
			}

			break;
		}

		case IN_STREAM_PULL: {
			debug( REALM_BYTECODE, "IN_STREAM_PULL\n" );

			Tree *stream = vm_pop();
			Tree *len = vm_pop();
			Tree *string = streamPullBc( prg, exec->fsmRun, (Stream*)stream, len );
			treeUpref( string );
			vm_push( string );

			/* Single unit. */
			treeUpref( string );
			append( exec->rcodeCollect, IN_STREAM_PULL_BKT );
			appendWord( exec->rcodeCollect, (Word) string );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );

			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, len );
			break;
		}
		case IN_STREAM_PULL_BKT: {
			Tree *string;
			read_tree( string );

			Tree *stream = vm_pop();

			debug( REALM_BYTECODE, "IN_STREAM_PULL_BKT\n" );

			undoPull( prg, exec->fsmRun, (Stream*)stream, string );
			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, string );
			break;
		}
		case IN_STREAM_PUSH_WV: {
			debug( REALM_BYTECODE, "IN_STREAM_PUSH_WV\n" );

			Tree *stream = vm_pop();
			Tree *tree = vm_pop();
			Word len = streamPush( prg, sp, ((Stream*)stream), tree, false );
			vm_push( 0 );

			/* Single unit. */
			append( exec->rcodeCollect, IN_STREAM_PUSH_BKT );
			appendWord( exec->rcodeCollect, len );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );

			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_STREAM_PUSH_IGNORE_WV: {
			debug( REALM_BYTECODE, "IN_STREAM_PUSH_IGNORE_WV\n" );

			Tree *stream = vm_pop();
			Tree *tree = vm_pop();
			Word len = streamPush( prg, sp, ((Stream*)stream), tree, true );
			vm_push( 0 );

			/* Single unit. */
			append( exec->rcodeCollect, IN_STREAM_PUSH_BKT );
			appendWord( exec->rcodeCollect, len );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );

			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_STREAM_PUSH_BKT: {
			Word len;
			read_word( len );

			Tree *stream = vm_pop();

			debug( REALM_BYTECODE, "IN_STREAM_PUSH_BKT\n" );

			undoStreamPush( prg, sp, ((Stream*)stream)->in, len );
			treeDownref( prg, sp, stream );
			break;
		}
		case IN_CONSTRUCT: {
			Half patternId;
			read_half( patternId );

			debug( REALM_BYTECODE, "IN_CONSTRUCT\n" );

			int rootNode = prg->rtd->patReplInfo[patternId].offset;

			/* Note that bindIds are indexed at one. Add one spot for them. */
			int numBindings = prg->rtd->patReplInfo[patternId].numBindings;
			Tree *bindings[1+numBindings];

			int b;
			for ( b = 1; b <= numBindings; b++ ) {
				bindings[b] = vm_pop();
				assert( bindings[b] != 0 );
			}

			Tree *replTree = 0;
			PatReplNode *nodes = prg->rtd->patReplNodes;
			LangElInfo *lelInfo = prg->rtd->lelInfo;
			long genericId = lelInfo[nodes[rootNode].id].genericId;
			if ( genericId > 0 ) {
				replTree = createGeneric( prg, genericId );
				treeUpref( replTree );
			}
			else {
				replTree = constructReplacementTree( 0, bindings, 
						prg, rootNode );
			}

			vm_push( replTree );
			break;
		}
		case IN_CONSTRUCT_TERM: {
			Half tokenId;
			read_half( tokenId );

			debug( REALM_BYTECODE, "IN_CONSTRUCT_TERM\n" );

			/* Pop the string we are constructing the token from. */
			Str *str = (Str*)vm_pop();
			Tree *res = constructTerm( prg, tokenId, str->value );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_MAKE_TOKEN: {
			uchar nargs;
			read_byte( nargs );

			debug( REALM_BYTECODE, "IN_MAKE_TOKEN\n" );

			Tree *result = constructToken( prg, sp, nargs );
			long i;
			for ( i = 0; i < nargs; i++ ) {
				Tree *arg = vm_pop();
				treeDownref( prg, sp, arg );
			}
			vm_push( result );
			break;
		}
		case IN_MAKE_TREE: {
			uchar nargs;
			read_byte( nargs );

			debug( REALM_BYTECODE, "IN_MAKE_TREE\n" );

			Tree *result = makeTree( prg, sp, nargs );
			long i;
			for ( i = 0; i < nargs; i++ ) {
				Tree *arg = vm_pop();
				treeDownref( prg, sp, arg );
			}
			vm_push( result );
			break;
		}
		case IN_TREE_NEW: {
			debug( REALM_BYTECODE, "IN_TREE_NEW \n" );

			Tree *tree = vm_pop();
			Tree *res = constructPointer( prg, tree );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_PTR_DEREF_R: {
			debug( REALM_BYTECODE, "IN_PTR_DEREF_R\n" );

			Pointer *ptr = (Pointer*)vm_pop();
			treeDownref( prg, sp, (Tree*)ptr );

			Tree *dval = getPtrVal( ptr );
			treeUpref( dval );
			vm_push( dval );
			break;
		}
		case IN_PTR_DEREF_WC: {
			debug( REALM_BYTECODE, "IN_PTR_DEREF_WC\n" );

			Pointer *ptr = (Pointer*)vm_pop();
			treeDownref( prg, sp, (Tree*)ptr );

			Tree *dval = getPtrValSplit( prg, ptr );
			treeUpref( dval );
			vm_push( dval );
			break;
		}
		case IN_PTR_DEREF_WV: {
			debug( REALM_BYTECODE, "IN_PTR_DEREF_WV\n" );

			Pointer *ptr = (Pointer*)vm_pop();
			/* Don't downref the pointer since it is going into the reverse
			 * instruction. */

			Tree *dval = getPtrValSplit( prg, ptr );
			treeUpref( dval );
			vm_push( dval );

			/* This is an initial global load. Need to reverse execute it. */
			append( exec->rcodeCollect, IN_PTR_DEREF_BKT );
			appendWord( exec->rcodeCollect, (Word) ptr );
			exec->rcodeUnitLen = SIZEOF_CODE + SIZEOF_WORD;
			break;
		}
		case IN_PTR_DEREF_BKT: {
			Word p;
			read_word( p );

			debug( REALM_BYTECODE, "IN_PTR_DEREF_BKT\n" );

			Pointer *ptr = (Pointer*)p;

			Tree *dval = getPtrValSplit( prg, ptr );
			treeUpref( dval );
			vm_push( dval );

			treeDownref( prg, sp, (Tree*)ptr );
			break;
		}
		case IN_REF_FROM_LOCAL: {
			short int field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_REF_FROM_LOCAL\n" );

			/* First push the null next pointer, then the kid pointer. */
			Tree **ptr = vm_plocal(field);
			vm_push( 0 );
			vm_push( (SW)ptr );
			break;
		}
		case IN_REF_FROM_REF: {
			short int field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_REF_FROM_REF\n" );

			Ref *ref = (Ref*)vm_plocal(field);
			vm_push( (SW)ref );
			vm_push( (SW)ref->kid );
			break;
		}
		case IN_REF_FROM_QUAL_REF: {
			short int back;
			short int field;
			read_half( back );
			read_half( field );

			debug( REALM_BYTECODE, "IN_REF_FROM_QUAL_REF\n" );

			Ref *ref = (Ref*)(sp + back);

			Tree *obj = ref->kid->tree;
			Kid *attr_kid = getFieldKid( obj, field );

			vm_push( (SW)ref );
			vm_push( (SW)attr_kid );
			break;
		}
		case IN_TRITER_REF_FROM_CUR: {
			short int field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_TRITER_REF_FROM_CUR\n" );

			/* Push the next pointer first, then the kid. */
			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Ref *ref = &iter->ref;
			vm_push( (SW)ref );
			vm_push( (SW)iter->ref.kid );
			break;
		}
		case IN_UITER_REF_FROM_CUR: {
			short int field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_UITER_REF_FROM_CUR\n" );

			/* Push the next pointer first, then the kid. */
			UserIter *uiter = (UserIter*) vm_local(field);
			vm_push( (SW)uiter->ref.next );
			vm_push( (SW)uiter->ref.kid );
			break;
		}
		case IN_GET_TOKEN_DATA_R: {
			debug( REALM_BYTECODE, "IN_GET_TOKEN_DATA_R\n" );

			Tree *tree = (Tree*) vm_pop();
			Head *data = stringCopy( prg, tree->tokdata );
			Tree *str = constructString( prg, data );
			treeUpref( str );
			vm_push( str );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_SET_TOKEN_DATA_WC: {
			debug( REALM_BYTECODE, "IN_SET_TOKEN_DATA_WC\n" );

			Tree *tree = vm_pop();
			Tree *val = vm_pop();
			Head *head = stringCopy( prg, ((Str*)val)->value );
			stringFree( prg, tree->tokdata );
			tree->tokdata = head;

			treeDownref( prg, sp, tree );
			treeDownref( prg, sp, val );
			break;
		}
		case IN_SET_TOKEN_DATA_WV: {
			debug( REALM_BYTECODE, "IN_SET_TOKEN_DATA_WV\n" );

			Tree *tree = vm_pop();
			Tree *val = vm_pop();

			Head *oldval = tree->tokdata;
			Head *head = stringCopy( prg, ((Str*)val)->value );
			tree->tokdata = head;

			/* Set up reverse code. Needs no args. */
			append( exec->rcodeCollect, IN_SET_TOKEN_DATA_BKT );
			appendWord( exec->rcodeCollect, (Word)oldval );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );

			treeDownref( prg, sp, tree );
			treeDownref( prg, sp, val );
			break;
		}
		case IN_SET_TOKEN_DATA_BKT: {
			debug( REALM_BYTECODE, "IN_SET_TOKEN_DATA_BKT \n" );

			Word oldval;
			read_word( oldval );

			Tree *tree = vm_pop();
			Head *head = (Head*)oldval;
			stringFree( prg, tree->tokdata );
			tree->tokdata = head;
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_GET_TOKEN_POS_R: {
			debug( REALM_BYTECODE, "IN_GET_TOKEN_POS_R\n" );

			Tree *tree = (Tree*) vm_pop();
			Tree *integer = 0;
			if ( tree->tokdata->location ) {
				integer = constructInteger( prg, tree->tokdata->location->byte );
				treeUpref( integer );
			}
			vm_push( integer );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_GET_MATCH_LENGTH_R: {
			debug( REALM_BYTECODE, "IN_GET_MATCH_LENGTH_R\n" );

			Tree *integer = constructInteger( prg, stringLength(exec->pdaRun->tokdata) );
			treeUpref( integer );
			vm_push( integer );
			break;
		}
		case IN_GET_MATCH_TEXT_R: {
			debug( REALM_BYTECODE, "IN_GET_MATCH_TEXT_R\n" );

			Head *s = stringCopy( prg, exec->pdaRun->tokdata );
			Tree *tree = constructString( prg, s );
			treeUpref( tree );
			vm_push( tree );
			break;
		}
		case IN_LIST_LENGTH: {
			debug( REALM_BYTECODE, "IN_LIST_LENGTH\n" );

			List *list = (List*) vm_pop();
			long len = listLength( list );
			Tree *res = constructInteger( prg, len );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_LIST_APPEND_WV: {
			debug( REALM_BYTECODE, "IN_LIST_APPEND_WV\n" );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();

			treeDownref( prg, sp, obj );

			listAppend2( prg, (List*)obj, val );
			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );

			/* Set up reverse code. Needs no args. */
			append( exec->rcodeCollect, IN_LIST_APPEND_BKT );
			exec->rcodeUnitLen += SIZEOF_CODE;
			append( exec->rcodeCollect, exec->rcodeUnitLen );
			/* FLUSH */
			break;
		}
		case IN_LIST_APPEND_WC: {
			debug( REALM_BYTECODE, "IN_LIST_APPEND_WC\n" );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();

			treeDownref( prg, sp, obj );

			listAppend2( prg, (List*)obj, val );
			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );
			break;
		}
		case IN_LIST_APPEND_BKT: {
			debug( REALM_BYTECODE, "IN_LIST_APPEND_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *tree = listRemoveEnd( prg, (List*)obj );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_LIST_REMOVE_END_WC: {
			debug( REALM_BYTECODE, "IN_LIST_REMOVE_END_WC\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *end = listRemoveEnd( prg, (List*)obj );
			vm_push( end );
			break;
		}
		case IN_LIST_REMOVE_END_WV: {
			debug( REALM_BYTECODE, "IN_LIST_REMOVE_END_WV\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *end = listRemoveEnd( prg, (List*)obj );
			vm_push( end );

			/* Set up reverse. The result comes off the list downrefed.
			 * Need it up referenced for the reverse code too. */
			treeUpref( end );
			append( exec->rcodeCollect, IN_LIST_REMOVE_END_BKT );
			appendWord( exec->rcodeCollect, (Word)end );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );
			/* FLUSH */
			break;
		}
		case IN_LIST_REMOVE_END_BKT: {
			debug( REALM_BYTECODE, "IN_LIST_REMOVE_END_BKT\n" );

			Tree *val;
			read_tree( val );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			listAppend2( prg, (List*)obj, val );
			break;
		}
		case IN_GET_LIST_MEM_R: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_LIST_MEM_R\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = getListMem( (List*)obj, field );
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_LIST_MEM_WC: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_LIST_MEM_WC\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = getListMemSplit( prg, (List*)obj, field );
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_LIST_MEM_WV: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_LIST_MEM_WV\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = getListMemSplit( prg, (List*)obj, field );
			treeUpref( val );
			vm_push( val );

			/* Set up the reverse instruction. */
			append( exec->rcodeCollect, IN_GET_LIST_MEM_BKT );
			appendHalf( exec->rcodeCollect, field );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_HALF;
			break;
		}
		case IN_GET_LIST_MEM_BKT: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_GET_LIST_MEM_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *res = getListMemSplit( prg, (List*)obj, field );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_SET_LIST_MEM_WC: {
			Half field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_SET_LIST_MEM_WC\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = vm_pop();
			Tree *existing = setListMem( (List*)obj, field, val );
			treeDownref( prg, sp, existing );
			break;
		}
		case IN_SET_LIST_MEM_WV: {
			Half field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_SET_LIST_MEM_WV\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = vm_pop();
			Tree *existing = setListMem( (List*)obj, field, val );

			/* Set up the reverse instruction. */
			append( exec->rcodeCollect, IN_SET_LIST_MEM_BKT );
			appendHalf( exec->rcodeCollect, field );
			appendWord( exec->rcodeCollect, (Word)existing );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_HALF + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );
			/* FLUSH */
			break;
		}
		case IN_SET_LIST_MEM_BKT: {
			Half field;
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( REALM_BYTECODE, "IN_SET_LIST_MEM_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *undid = setListMem( (List*)obj, field, val );
			treeDownref( prg, sp, undid );
			break;
		}
		case IN_MAP_INSERT_WV: {
			debug( REALM_BYTECODE, "IN_MAP_INSERT_WV\n" );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();
			Tree *key = vm_pop();

			treeDownref( prg, sp, obj );

			int inserted = mapInsert( prg, (Map*)obj, key, val );
			Tree *result = inserted ? prg->trueVal : prg->falseVal;
			treeUpref( result );
			vm_push( result );

			/* Set up the reverse instruction. If the insert fails still need
			 * to pop the loaded map object. Just use the reverse instruction
			 * since it's nice to see it in the logs. */

			/* Need to upref key for storage in reverse code. */
			treeUpref( key );
			append( exec->rcodeCollect, IN_MAP_INSERT_BKT );
			append( exec->rcodeCollect, inserted );
			appendWord( exec->rcodeCollect, (Word)key );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_CODE + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );

			if ( ! inserted ) {
				treeDownref( prg, sp, key );
				treeDownref( prg, sp, val );
			}
			break;
		}
		case IN_MAP_INSERT_WC: {
			debug( REALM_BYTECODE, "IN_MAP_INSERT_WC\n" );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();
			Tree *key = vm_pop();

			treeDownref( prg, sp, obj );

			int inserted = mapInsert( prg, (Map*)obj, key, val );
			Tree *result = inserted ? prg->trueVal : prg->falseVal;
			treeUpref( result );
			vm_push( result );

			if ( ! inserted ) {
				treeDownref( prg, sp, key );
				treeDownref( prg, sp, val );
			}
			break;
		}
		case IN_MAP_INSERT_BKT: {
			uchar inserted;
			Tree *key;
			read_byte( inserted );
			read_tree( key );

			debug( REALM_BYTECODE, "IN_MAP_INSERT_BKT\n" );
			
			Tree *obj = vm_pop();
			if ( inserted ) {
				Tree *val = mapUninsert( prg, (Map*)obj, key );
				treeDownref( prg, sp, key );
				treeDownref( prg, sp, val );
			}

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
			break;
		}
		case IN_MAP_STORE_WC: {
			debug( REALM_BYTECODE, "IN_MAP_STORE_WC\n" );

			Tree *obj = vm_pop();
			Tree *element = vm_pop();
			Tree *key = vm_pop();

			Tree *existing = mapStore( prg, (Map*)obj, key, element );
			Tree *result = existing == 0 ? prg->trueVal : prg->falseVal;
			treeUpref( result );
			vm_push( result );

			treeDownref( prg, sp, obj );
			if ( existing != 0 ) {
				treeDownref( prg, sp, key );
				treeDownref( prg, sp, existing );
			}
			break;
		}
		case IN_MAP_STORE_WV: {
			debug( REALM_BYTECODE, "IN_MAP_STORE_WV\n" );

			Tree *obj = vm_pop();
			Tree *element = vm_pop();
			Tree *key = vm_pop();

			Tree *existing = mapStore( prg, (Map*)obj, key, element );
			Tree *result = existing == 0 ? prg->trueVal : prg->falseVal;
			treeUpref( result );
			vm_push( result );

			/* Set up the reverse instruction. */
			treeUpref( key );
			treeUpref( existing );
			append( exec->rcodeCollect, IN_MAP_STORE_BKT );
			appendWord( exec->rcodeCollect, (Word)key );
			appendWord( exec->rcodeCollect, (Word)existing );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );
			/* FLUSH */

			treeDownref( prg, sp, obj );
			if ( existing != 0 ) {
				treeDownref( prg, sp, key );
				treeDownref( prg, sp, existing );
			}
			break;
		}
		case IN_MAP_STORE_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			debug( REALM_BYTECODE, "IN_MAP_STORE_BKT\n" );

			Tree *obj = vm_pop();
			Tree *stored = mapUnstore( prg, (Map*)obj, key, val );

			treeDownref( prg, sp, stored );
			if ( val == 0 )
				treeDownref( prg, sp, key );

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
			break;
		}
		case IN_MAP_REMOVE_WC: {
			debug( REALM_BYTECODE, "IN_MAP_REMOVE_WC\n" );

			Tree *obj = vm_pop();
			Tree *key = vm_pop();
			TreePair pair = mapRemove( prg, (Map*)obj, key );

			vm_push( pair.val );

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
			treeDownref( prg, sp, pair.key );
			break;
		}
		case IN_MAP_REMOVE_WV: {
			debug( REALM_BYTECODE, "IN_MAP_REMOVE_WV\n" );

			Tree *obj = vm_pop();
			Tree *key = vm_pop();
			TreePair pair = mapRemove( prg, (Map*)obj, key );

			treeUpref( pair.val );
			vm_push( pair.val );

			/* Reverse instruction. */
			append( exec->rcodeCollect, IN_MAP_REMOVE_BKT );
			appendWord( exec->rcodeCollect, (Word)pair.key );
			appendWord( exec->rcodeCollect, (Word)pair.val );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD + SIZEOF_WORD;
			append( exec->rcodeCollect, exec->rcodeUnitLen );

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
			break;
		}
		case IN_MAP_REMOVE_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			debug( REALM_BYTECODE, "IN_MAP_REMOVE_BKT\n" );

			/* Either both or neither. */
			assert( ( key == 0 ) ^ ( val != 0 ) );

			Tree *obj = vm_pop();
			if ( key != 0 )
				mapUnremove( prg, (Map*)obj, key, val );

			treeDownref( prg, sp, obj );
			break;
		}
		case IN_MAP_LENGTH: {
			debug( REALM_BYTECODE, "IN_MAP_LENGTH\n" );

			Tree *obj = vm_pop();
			long len = mapLength( (Map*)obj );
			Tree *res = constructInteger( prg, len );
			treeUpref( res );
			vm_push( res );

			treeDownref( prg, sp, obj );
			break;
		}
		case IN_MAP_FIND: {
			debug( REALM_BYTECODE, "IN_MAP_FIND\n" );

			Tree *obj = vm_pop();
			Tree *key = vm_pop();
			Tree *result = mapFind( prg, (Map*)obj, key );
			treeUpref( result );
			vm_push( result );

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
			break;
		}
		case IN_INIT_LOCALS: {
			Half size;
			read_half( size );

			debug( REALM_BYTECODE, "IN_INIT_LOCALS\n" );

			exec->framePtr = vm_ptop();
			vm_pushn( size );
			memset( vm_ptop(), 0, sizeof(Word) * size );
			break;
		}
		case IN_POP_LOCALS: {
			Half frameId, size;
			read_half( frameId );
			read_half( size );

			debug( REALM_BYTECODE, "IN_POP_LOCALS\n" );

			FrameInfo *fi = &prg->rtd->frameInfo[frameId];
			downrefLocalTrees( prg, sp, exec->framePtr, fi->trees, fi->treesLen );
			vm_popn( size );
			break;
		}
		case IN_CALL_WV: {
			Half funcId;
			read_half( funcId );

			FunctionInfo *fi = &prg->rtd->functionInfo[funcId];

			debug( REALM_BYTECODE, "IN_CALL_WV %ld\n", fi->name );

			vm_push( 0 ); /* Return value. */
			vm_push( (SW)instr );
			vm_push( (SW)exec->framePtr );
			vm_push( (SW)exec->frameId );

			instr = prg->rtd->frameInfo[fi->frameId].codeWV;
			exec->framePtr = vm_ptop();
			break;
		}
		case IN_CALL_WC: {
			Half funcId;
			read_half( funcId );

			FunctionInfo *fi = &prg->rtd->functionInfo[funcId];

			debug( REALM_BYTECODE, "IN_CALL_WC %ld\n", fi->name );

			vm_push( 0 ); /* Return value. */
			vm_push( (SW)instr );
			vm_push( (SW)exec->framePtr );
			vm_push( (SW)exec->frameId );

			instr = prg->rtd->frameInfo[fi->frameId].codeWC;
			exec->framePtr = vm_ptop();
			break;
		}
		case IN_YIELD: {
			debug( REALM_BYTECODE, "IN_YIELD\n" );

			Kid *kid = (Kid*)vm_pop();
			Ref *next = (Ref*)vm_pop();
			UserIter *uiter = (UserIter*) vm_plocal_iframe( IFR_AA );

			if ( kid == 0 || kid->tree == 0 ||
					kid->tree->id == uiter->searchId || 
					uiter->searchId == prg->rtd->anyId )
			{
				/* Store the yeilded value. */
				uiter->ref.kid = kid;
				uiter->ref.next = next;
				uiter->stackSize = uiter->stackRoot - vm_ptop();
				uiter->resume = instr;
				uiter->frame = exec->framePtr;

				/* Restore the instruction and frame pointer. */
				instr = (Code*) vm_local_iframe(IFR_RIN);
				exec->framePtr = (Tree**) vm_local_iframe(IFR_RFR);
				exec->iframePtr = (Tree**) vm_local_iframe(IFR_RIF);

				/* Return the yield result on the top of the stack. */
				Tree *result = uiter->ref.kid != 0 ? prg->trueVal : prg->falseVal;
				treeUpref( result );
				vm_push( result );
			}
			break;
		}
		case IN_UITER_CREATE_WV: {
			short field;
			Half funcId, searchId;
			read_half( field );
			read_half( funcId );
			read_half( searchId );

			debug( REALM_BYTECODE, "IN_UITER_CREATE_WV\n" );

			FunctionInfo *fi = prg->rtd->functionInfo + funcId;
			UserIter *uiter = uiterCreate( prg, &sp, fi, searchId );
			vm_local(field) = (SW) uiter;

			/* This is a setup similar to as a call, only the frame structure
			 * is slightly different for user iterators. We aren't going to do
			 * the call. We don't need to set up the return ip because the
			 * uiter advance will set it. The frame we need to do because it
			 * is set once for the lifetime of the iterator. */
			vm_push( 0 );            /* Return instruction pointer,  */
			vm_push( (SW)exec->iframePtr ); /* Return iframe. */
			vm_push( (SW)exec->framePtr );  /* Return frame. */

			uiterInit( prg, sp, uiter, fi, true );
			break;
		}
		case IN_UITER_CREATE_WC: {
			short field;
			Half funcId, searchId;
			read_half( field );
			read_half( funcId );
			read_half( searchId );

			debug( REALM_BYTECODE, "IN_UITER_CREATE_WC\n" );

			FunctionInfo *fi = prg->rtd->functionInfo + funcId;
			UserIter *uiter = uiterCreate( prg, &sp, fi, searchId );
			vm_local(field) = (SW) uiter;

			/* This is a setup similar to as a call, only the frame structure
			 * is slightly different for user iterators. We aren't going to do
			 * the call. We don't need to set up the return ip because the
			 * uiter advance will set it. The frame we need to do because it
			 * is set once for the lifetime of the iterator. */
			vm_push( 0 );            /* Return instruction pointer,  */
			vm_push( (SW)exec->iframePtr ); /* Return iframe. */
			vm_push( (SW)exec->framePtr );  /* Return frame. */

			uiterInit( prg, sp, uiter, fi, false );
			break;
		}
		case IN_UITER_DESTROY: {
			short field;
			read_half( field );

			debug( REALM_BYTECODE, "IN_UITER_DESTROY\n" );

			UserIter *uiter = (UserIter*) vm_local(field);
			userIterDestroy( &sp, uiter );
			break;
		}
		case IN_RET: {
			Half funcId;
			read_half( funcId );

			FunctionInfo *fui = &prg->rtd->functionInfo[funcId];

			debug( REALM_BYTECODE, "IN_RET %ld\n", fui->name );

			FrameInfo *fi = &prg->rtd->frameInfo[fui->frameId];
			downrefLocalTrees( prg, sp, exec->framePtr, fi->trees, fi->treesLen );

			vm_popn( fui->frameSize );
			exec->frameId = (long) vm_pop();
			exec->framePtr = (Tree**) vm_pop();
			instr = (Code*) vm_pop();
			Tree *retVal = vm_pop();
			vm_popn( fui->argSize );
			vm_push( retVal );
			break;
		}
		case IN_TO_UPPER: {
			debug( REALM_BYTECODE, "IN_TO_UPPER\n" );

			Tree *in = vm_pop();
			Head *head = stringToUpper( in->tokdata );
			Tree *upper = constructString( prg, head );
			treeUpref( upper );
			vm_push( upper );
			treeDownref( prg, sp, in );
			break;
		}
		case IN_TO_LOWER: {
			debug( REALM_BYTECODE, "IN_TO_LOWER\n" );

			Tree *in = vm_pop();
			Head *head = stringToLower( in->tokdata );
			Tree *lower = constructString( prg, head );
			treeUpref( lower );
			vm_push( lower );
			treeDownref( prg, sp, in );
			break;
		}
		case IN_EXIT: {
			debug( REALM_BYTECODE, "IN_EXIT\n" );

			Tree *global = vm_pop();
			Int *status = (Int*)vm_pop();
			prg->exitStatus = status->value;
			prg->induceExit = 1;
			treeDownref( prg, sp, global );
			goto out;
		}
		case IN_ERROR: {
			debug( REALM_BYTECODE, "IN_ERROR\n" );

			/* Pop the global. */
			Tree *global = vm_pop();
			treeDownref( prg, sp, global );
			treeUpref( prg->lastParseError );
			vm_push( prg->lastParseError );
			break;
		}
		case IN_OPEN_FILE: {
			debug( REALM_BYTECODE, "IN_OPEN_FILE\n" );

			Tree *mode = vm_pop();
			Tree *name = vm_pop();
			Tree *res = (Tree*)openFile( prg, name, mode );
			treeUpref( res );
			vm_push( res );
			treeDownref( prg, sp, name );
			treeDownref( prg, sp, mode );
			break;
		}
		case IN_GET_STDIN: {
			debug( REALM_BYTECODE, "IN_GET_STDIN\n" );

			/* Pop the root object. */
			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );
			if ( prg->stdinVal == 0 ) {
				prg->stdinVal = openStreamFd( prg, 0 );
				treeUpref( (Tree*)prg->stdinVal );
			}

			treeUpref( (Tree*)prg->stdinVal );
			vm_push( (Tree*)prg->stdinVal );
			break;
		}
		case IN_LOAD_ARGV: {
			Half field;
			read_half( field );
			debug( REALM_BYTECODE, "IN_LOAD_ARGV %lu\n", field );

			/* Tree comes back upreffed. */
			Tree *tree = constructArgv( prg, prg->argc, prg->argv );
			setField( prg, prg->global, field, tree );
			break;
		}

		case IN_STOP: {
			debug( REALM_BYTECODE, "IN_STOP\n" );
			fflush( stdout );
			goto out;
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

