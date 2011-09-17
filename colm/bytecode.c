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

#include <alloca.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "pdarun.h"
#include "fsmrun.h"
#include "tree.h"
#include "bytecode.h"
#include "pool.h"
#include "debug.h"
#include "config.h"

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

#undef COLM_LOG
#undef COLM_LOG_BYTECODE
#undef COLM_LOG_PARSE
#undef COLM_LOG_MATCH
#undef COLM_LOG_COMPILE

#define true 1
#define false 0

#define push(i) (*(--sp) = (i))
#define pop() (*sp++)
#define top() (*sp)
#define top_off(n) (sp[n])
#define ptop() (sp)
#define popn(n) (sp += (n))
#define pushn(n) (sp -= (n))
#define local(o) (exec->frame[o])
#define plocal(o) (&exec->frame[o])
#define local_iframe(o) (exec->iframe[o])
#define plocal_iframe(o) (&exec->iframe[o])

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

Tree *prepParseTree( Program *prg, Tree **sp, Tree *tree )
{
	/* Seems like we need to always copy here. If it isn't a parse tree it
	 * needs to be made into one. If it is then we need to make a new one in
	 * case the old one is still in use by some parsing routine. The case were
	 * we might be able to avoid a copy would be that it's a parse tree
	 * already, but it's owning parser is completely finished with it. */

	#ifdef COLM_LOG_BYTECODE
	if ( colm_log_bytecode ) {
		cerr << "copying tree in send function" << endl;
	}
	#endif
	Kid *unused = 0;
	tree = copyRealTree( prg, tree, 0, &unused, true );
	return tree;
}

void sendTreeFrag( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, 
		InputStream *inputStream,
		Tree *tree, int ignore )
{
	tree = prepParseTree( prg, sp, tree );

	if ( tree->id >= prg->rtd->firstNonTermId )
		tree->id = prg->rtd->lelInfo[tree->id].termDupId;

	tree->flags |= AF_ARTIFICIAL;

	treeUpref( tree );
		
	/* FIXME: Do we need to remove the ignore tokens 
	 * at this point? Will it cause a leak? */

	Kid *send = kidAllocate( prg );
	send->tree = tree;

	LangElInfo *lelInfo = pdaRun->prg->rtd->lelInfo;

	/* Must clear next, since the parsing algorithm uses it. */
	if ( lelInfo[send->tree->id].ignore ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "ignoring queued item: " << 
					pdaRun->tables->rtd->lelInfo[send->tree->id].name << endl;
		}
		#endif

		incrementConsumed( pdaRun );

		ignoreTree( pdaRun, send->tree );
		kidFree( pdaRun->prg, send );
	}
	else {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "sending queue item: " << 
					pdaRun->tables->rtd->lelInfo[send->tree->id].name << endl;
		}
		#endif

		incrementConsumed( pdaRun );

		sendHandleError( sp, pdaRun, fsmRun, inputStream, send );
	}
}

void parserSetContext( Tree **sp, Program *prg, Accum *accum, Tree *val )
{
	accum->pdaRun->context = splitTree( prg, val );
}

Head *treeToStr( Tree **sp, Program *prg, Tree *tree )
{
	/* Collect the tree data. */
	StrCollect collect;
	initStrCollect( &collect );

	printTree( &collect, sp, prg, tree );

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

void parseStream( Tree **sp, Program *prg, Tree *input, Accum *accum, long stopId )
{
	accum->pdaRun->stopTarget = stopId;

	Stream *stream = (Stream*)input;
	accum->fsmRun->curStream = input;
	parseLoop( sp, accum->pdaRun, accum->fsmRun, stream->in );
}

Word streamAppend( Tree **sp, Program *prg, Tree *input, Stream *stream )
{
	if ( input->id == LEL_ID_STR ) {
		//assert(false);
		/* Collect the tree data. */
		StrCollect collect;
		initStrCollect( &collect );
		printTree( &collect, sp, prg, input );

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

void parseFrag( Tree **sp, Program *prg, Tree *input, Accum *accum, long stopId )
{
	accum->pdaRun->stopTarget = stopId;
	Stream *stream = (Stream*) extractInput( prg, accum );

	if ( input->id == LEL_ID_STR ) {
		//assert(false);
		/* Collect the tree data. */
		StrCollect collect;
		initStrCollect( &collect );
		printTree( &collect, sp, prg, input );

		/* Load it into the input. */
		stream->in->funcs->appendData( stream->in, collect.data, collect.length );

		/* Parse. */
		parseLoop( sp, accum->pdaRun, accum->fsmRun, stream->in );

		strCollectDestroy( &collect );
	}
	else {
		//assert(false);
		/* Cause a flush */

		input = prepParseTree( prg, sp, input );

		if ( input->id >= prg->rtd->firstNonTermId )
			input->id = prg->rtd->lelInfo[input->id].termDupId;

		input->flags |= AF_ARTIFICIAL;

		treeUpref( input );
		stream->in->funcs->appendTree( stream->in, input );

		parseLoop( sp, accum->pdaRun, accum->fsmRun, stream->in );

//		stream->in->flush = true;
//		parseLoop( sp, accum->pdaRun, accum->fsmRun, stream->in );

//		sendTreeFrag( prg, sp, accum->pdaRun, accum->fsmRun, stream->in, input, false );
	}
}

void undoParseStream( Tree **sp, Program *prg, Stream *input, Accum *accum, long consumed )
{
	if ( consumed < accum->pdaRun->consumed ) {
		accum->pdaRun->numRetry += 1;
		accum->pdaRun->targetConsumed = consumed;
		parseToken( sp, accum->pdaRun, accum->fsmRun, input->in, 0 );
		accum->pdaRun->targetConsumed = -1;
		accum->pdaRun->numRetry -= 1;

		accum->fsmRun->region = pdaRunGetNextRegion( accum->pdaRun, 0 );
		accum->fsmRun->cs = accum->fsmRun->tables->entryByRegion[accum->fsmRun->region];
	}
}

Tree *parseFinish( Tree **sp, Program *prg, Accum *accum, int revertOn )
{
	Stream *stream = (Stream*)extractInput( prg, accum );

	if ( accum->pdaRun->stopTarget > 0 ) {

	}
	else {
		stream->in->eof = true;
		stream->in->later = false;
		parseLoop( sp, accum->pdaRun, accum->fsmRun, stream->in );
	}

	if ( !revertOn )
		commitFull( sp, accum->pdaRun, 0 );
	
	Tree *tree = getParsedRoot( accum->pdaRun, accum->pdaRun->stopTarget > 0 );
	treeUpref( tree );

	if ( !revertOn )
		cleanParser( sp, accum->pdaRun );

	/* Indicate that this tree came out of a parser. */
	tree->flags |= AF_PARSED;

	return tree;
}

Tree *streamPull2( Program *prg, FsmRun *fsmRun, Stream *stream, Tree *length )
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
		printTree( &collect, sp, prg, tree );

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
		#ifdef COLM_LOG_BYTECODE
		if ( colm_log_bytecode ) {
			cerr << "local tree downref: " << (long)trees[i] << endl;
		}
		#endif

		treeDownref( prg, sp, frame[((long)trees[i])] );
	}
}

UserIter *uiterCreate( Tree ***psp, Program *prg, FunctionInfo *fi, long searchId )
{
	Tree **sp = *psp;
	pushn( sizeof(UserIter) / sizeof(Word) );
	void *mem = ptop();

	UserIter *uiter = mem;
	initUserIter( uiter, ptop(), fi->argSize, searchId );
	*psp = sp;
	return uiter;
}

void uiterInit( Program *prg, Tree **sp, UserIter *uiter, 
		FunctionInfo *fi, int revertOn )
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

void treeIterDestroy( Tree ***psp, TreeIter *iter )
{
	Tree **sp = *psp;
	long curStackSize = iter->stackRoot - ptop();
	assert( iter->stackSize == curStackSize );
	popn( iter->stackSize );
	*psp = sp;
}

void userIterDestroy( Tree ***psp, UserIter *uiter )
{
	Tree **sp = *psp;

	/* We should always be coming from a yield. The current stack size will be
	 * nonzero and the stack size in the iterator will be correct. */
	long curStackSize = uiter->stackRoot - ptop();
	assert( uiter->stackSize == curStackSize );

	long argSize = uiter->argSize;

	popn( uiter->stackRoot - ptop() );
	popn( sizeof(UserIter) / sizeof(Word) );
	popn( argSize );

	*psp = sp;
}

Tree *constructArgv( Program *prg, int argc, char **argv )
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

void initProgram( Program *prg, int argc, char **argv, int ctxDepParsing, 
		RuntimeData *rtd )
{
	prg->argc = argc;
	prg->argv = argv;
	prg->ctxDepParsing = ctxDepParsing;
	prg->rtd = rtd;
	prg->global = 0;
	prg->heap = 0;
	prg->stdinVal = 0;
	prg->stdoutVal = 0;
	prg->stderrVal = 0;

	initPoolAlloc( &prg->kidPool, sizeof(Kid) );
	initPoolAlloc( &prg->treePool, sizeof(Tree) );
	initPoolAlloc( &prg->parseTreePool, sizeof(ParseTree) );
	initPoolAlloc( &prg->listElPool, sizeof(ListEl) );
	initPoolAlloc( &prg->mapElPool, sizeof(MapEl) );
	initPoolAlloc( &prg->headPool, sizeof(Head) );
	initPoolAlloc( &prg->locationPool, sizeof(Location) );
	initPoolAlloc( &prg->ignoreListPool, sizeof(IgnoreList) );

	Int *trueInt = (Int*) treeAllocate( prg );
	trueInt->id = LEL_ID_BOOL;
	trueInt->refs = 1;
	trueInt->value = 1;

	Int *falseInt = (Int*) treeAllocate( prg );
	falseInt->id = LEL_ID_BOOL;
	falseInt->refs = 1;
	falseInt->value = 0;

	prg->trueVal = (Tree*)trueInt;
	prg->falseVal = (Tree*)falseInt;
}

void clearGlobal( Program *prg, Tree **sp )
{
	/* Downref all the fields in the global object. */
	int g;
	for ( g = 0; g < prg->rtd->globalSize; g++ ) {
		//assert( getAttr( global, g )->refs == 1 );
		treeDownref( prg, sp, getAttr( prg->global, g ) );
	}

	/* Free the global object. */
	if ( prg->rtd->globalSize > 0 )
		freeAttrs( prg, prg->global->child );
	treeFree( prg, prg->global );
}

void clearProgram( Program *prg, Tree **vm_stack, Tree **sp )
{
	#ifdef COLM_LOG_BYTECODE
	if ( colm_log_bytecode ) {
		cerr << "clearing the prg" << endl;
	}
	#endif

	clearGlobal( prg, sp );

	/* Clear the heap. */
	Kid *a = prg->heap;
	while ( a != 0 ) {
		Kid *next = a->next;
		treeDownref( prg, sp, a->tree );
		kidFree( prg, a );
		a = next;
	}

	//assert( trueVal->refs == 1 );
	//assert( falseVal->refs == 1 );
	treeDownref( prg, sp, prg->trueVal );
	treeDownref( prg, sp, prg->falseVal );

	treeDownref( prg, sp, (Tree*)prg->stdinVal );
	treeDownref( prg, sp, (Tree*)prg->stdoutVal );
	treeDownref( prg, sp, (Tree*)prg->stderrVal );

	long kidLost = kidNumLost( prg );
	if ( kidLost )
		message( "warning: lost kids: %ld\n", kidLost );

	long treeLost = treeNumLost( prg );
	if ( treeLost )
		message( "warning: lost trees: %ld\n", treeLost );

	long parseTreeLost = parseTreeNumLost( prg );
	if ( parseTreeLost )
		message( "warning: lost parse trees: %ld\n", parseTreeLost );

	long listLost = listElNumLost( prg );
	if ( listLost )
		message( "warning: lost listEls: %ld\n", listLost );

	long mapLost = mapElNumLost( prg );
	if ( mapLost )
		message( "warning: lost mapEls: %ld\n", mapLost );

	long headLost = headNumLost( prg );
	if ( headLost )
		message( "warning: lost heads: %ld\n", headLost );

	long locationLost = locationNumLost( prg );
	if ( locationLost )
		message( "warning: lost locations: %ld\n", locationLost );

	long ignoreListList = ignoreListNumLost( prg );
	if ( ignoreListNumLost )
		message( "warning: lost ignore lists: %ld\n", ignoreListNumLost );

	kidClear( prg );
	treeClear( prg );
	parseTreeClear( prg );
	listElClear( prg );
	mapElClear( prg );
	locationClear( prg );

	//exec->reverseCode->empty();
	//memset( vm_stack, 0, sizeof(Tree*) * VM_STACK_SIZE);
}

void allocGlobal( Program *prg )
{
	/* Alloc the global. */
	Tree *tree = treeAllocate( prg );
	tree->child = allocAttrs( prg, prg->rtd->globalSize );
	tree->refs = 1;
	prg->global = tree;
}

Tree **stackAlloc()
{
	//return new Tree*[VM_STACK_SIZE];

	return (Tree**)mmap( 0, sizeof(Tree*)*VM_STACK_SIZE,
		PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0 );
}

void runProgram( Program *prg )
{
	assert( sizeof(Int)      <= sizeof(Tree) );
	assert( sizeof(Str)      <= sizeof(Tree) );
	assert( sizeof(Pointer)  <= sizeof(Tree) );
	assert( sizeof(Map)      <= sizeof(MapEl) );
	assert( sizeof(List)     <= sizeof(MapEl) );
	assert( sizeof(Stream)   <= sizeof(MapEl) );
	assert( sizeof(Accum)    <= sizeof(MapEl) );

	/* Allocate the global variable. */
	allocGlobal( prg );

	/* 
	 * Allocate the VM stack.
	 */

	Tree **vm_stack = stackAlloc();
	Tree **root = &vm_stack[VM_STACK_SIZE];

	/*
	 * Execute
	 */

	if ( prg->rtd->rootCodeLen > 0 ) {
		RtCodeVect reverseCode;
		Execution execution;
		initRtCodeVect( &reverseCode );
		initExecution( &execution, prg, &reverseCode, 0, 0, prg->rtd->rootCode, 0, 0, 0, 0 );
		execute( &execution, root );

		/* Pull out the reverse code and free it. */
		#ifdef COLM_LOG_BYTECODE
		if ( colm_log_bytecode ) {
			cerr << "freeing the root reverse code" << endl;
		}
		#endif

		/* The root code should all be commit code and reverseCode
		 * should be empty. */
		assert( reverseCode.tabLen == 0 );
	}

	/* Clear */
	clearProgram( prg, vm_stack, root );
}

void initExecution( Execution *exec, Program *prg, RtCodeVect *reverseCode,
		PdaRun *pdaRun, FsmRun *fsmRun, Code *code, Tree *lhs,
		long genId, Head *matchText, char **captures )
{
	exec->prg = prg;
	exec->pdaRun = pdaRun;
	exec->fsmRun = fsmRun;
	exec->code = code;
	exec->frame = 0;
	exec->iframe = 0;
	exec->lhs = lhs;
	exec->parsed = 0;
	exec->genId = genId;
	exec->matchText = matchText;
	exec->reject = false;
	exec->reverseCode = reverseCode;
	exec->rcodeUnitLen = 0;
	exec->captures = captures;

	assert( lhs == 0 || lhs->refs == 1 );
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
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_RESTORE_LHS" << endl;
			}
			#endif
			treeDownref( prg, sp, lhs );
			break;
		}

		case IN_EXTRACT_INPUT_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode )
				cerr << "IN_EXTRACT_INPUT_BKT" << endl;
			#endif
			break;
		}
		case IN_STREAM_APPEND_BKT: {
			Tree *stream;
			Tree *input;
			Word len;
			read_tree( stream );
			read_tree( input );
			read_word( len );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_APPEND_BKT" << endl;
			}
			#endif

			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, input );
			break;
		}
		case IN_PARSE_FRAG_BKT: {
			Tree *accum;
			Tree *input;
			long consumed;
			read_tree( accum );
			read_tree( input );
			read_word( consumed );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode )
				cerr << "IN_PARSE_FRAG_BKT " << consumed << endl;
			#endif
			
			treeDownref( prg, sp, accum );
			treeDownref( prg, sp, input );
			break;
		}
		case IN_PARSE_FINISH_BKT: {
			Tree *accumTree;
			Tree *tree;
			long consumed;

			read_tree( accumTree );
			read_tree( tree );
			read_word( consumed );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_FINISH_BKT " << consumed << endl;
			}
			#endif

			treeDownref( prg, sp, accumTree );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_STREAM_PULL_BKT: {
			Tree *string;
			read_tree( string );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PULL_BKT" << endl;
			}
			#endif

			treeDownref( prg, sp, string );
			break;
		}
		case IN_STREAM_PUSH_BKT: {
			Word len;
			read_word( len );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PUSH_BKT" << endl;
			}
			#endif
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
		case IN_LOAD_CONTEXT_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_CONTEXT_BKT" << endl;
			}
			#endif
			break;
		}
		case IN_LOAD_INPUT_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_INPUT_BKT" << endl;
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

			treeDownref( prg, sp, val );
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

			treeDownref( prg, sp, ptr );
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
			stringFree( prg, head );
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

			treeDownref( prg, sp, val );
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

			treeDownref( prg, sp, val );
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
			
			treeDownref( prg, sp, key );
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

			treeDownref( prg, sp, key );
			treeDownref( prg, sp, val );
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

			treeDownref( prg, sp, key );
			treeDownref( prg, sp, val );
			break;
		}
		case IN_STOP: {
			return;
		}
		default: {
			fatal( "UNKNOWN INSTRUCTION: -- reverse code downref\n" );
			assert(false);
			break;
		}
	}
	goto again;
}

void execute( Execution *exec, Tree **sp )
{
	/* If we have a lhs push it to the stack. */
	int haveLhs = exec->lhs != 0;
	if ( haveLhs )
		push( exec->lhs );

	/* Execution loop. */
	executeCode( exec, sp, exec->code );

	/* Take the lhs off the stack. */
	if ( haveLhs )
		exec->lhs = (Tree*) pop();
}

int makeReverseCode( RtCodeVect *all, RtCodeVect *reverseCode )
{
	/* Do we need to revert the left hand side? */

	/* Check if there was anything generated. */
	if ( reverseCode->tabLen == 0 )
		return false;

	long prevAllLength = all->tabLen;

	/* Go backwards, group by group, through the reverse code. Push each group
	 * to the global reverse code stack. */
	Code *p = reverseCode->data + reverseCode->tabLen;
	while ( p != reverseCode->data ) {
		p--;
		long len = *p;
		p = p - len;
		append2( all, p, len );
	}

	/* Stop, then place a total length in the global stack. */
	append( all, IN_STOP );
	long length = all->tabLen - prevAllLength;
	appendWord( all, length );

	/* Clear the revere code buffer. */
	reverseCode->tabLen = 0;

	return true;
}

void rexecute( Execution *exec, Tree **root, RtCodeVect *allRev )
{
	/* Read the length */
	Code *prcode = allRev->data + allRev->tabLen - SIZEOF_WORD;
	Word len;
	read_word_p( len, prcode );

	/* Find the start of block. */
	long start = allRev->tabLen - len - SIZEOF_WORD;
	prcode = allRev->data + start;

	/* Execute it. */
	Tree **sp = root;
	executeCode( exec, sp, prcode );
	assert( sp == root );

	/* Backup over it. */
	allRev->tabLen -= len + SIZEOF_WORD;
}

void executeCode( Execution *exec, Tree **sp, Code *instr )
{
	/* When we exit we are going to verify that we did not eat up any stack
	 * space. */
	Tree **root = sp;
	Program *prg = exec->prg;

again:
	switch ( *instr++ ) {
		case IN_SAVE_LHS: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SAVE_LHS" << endl;
			}
			#endif

			assert( exec->lhs != 0 );

			/* Save and upref before writing. We don't generate a restore
			 * here. Instead, in the parser we will check if it actually
			 * changed and insert the instruction then. The presence of this
			 * instruction here is just a conservative approximation.  */
			exec->parsed = exec->lhs;
			//treeUpref( parsed );
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
			assert( exec->lhs == 0 );
			exec->lhs = restore;
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

			treeUpref( prg->trueVal );
			push( prg->trueVal );
			break;
		}
		case IN_LOAD_FALSE: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_FALSE" << endl;
			}
			#endif

			treeUpref( prg->falseVal );
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

			Tree *tree = constructInteger( prg, i );
			treeUpref( tree );
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

			Head *lit = makeLiteral( prg, offset );
			Tree *tree = constructString( prg, lit );
			treeUpref( tree );
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
				printTree2( stdout, sp, prg, tree );
				treeDownref( prg, sp, tree );
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
				printXmlTree( sp, prg, tree, true );
				treeDownref( prg, sp, tree );
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
				printXmlTree( sp, prg, tree, false );
				treeDownref( prg, sp, tree );
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
				printTree2( stream->file, sp, prg, tree );
				treeDownref( prg, sp, tree );
			}
			treeDownref( prg, sp, (Tree*)stream );
			break;
		}
		case IN_LOAD_CONTEXT_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_CONTEXT_R" << endl;
			}
			#endif

			treeUpref( exec->pdaRun->context );
			push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_CONTEXT_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_CONTEXT_WV" << endl;
			}
			#endif

			treeUpref( exec->pdaRun->context );
			push( exec->pdaRun->context );

			/* Set up the reverse instruction. */
			append( exec->reverseCode, IN_LOAD_CONTEXT_BKT );
			exec->rcodeUnitLen = SIZEOF_CODE;
			break;
		}
		case IN_LOAD_CONTEXT_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_CONTEXT_WC" << endl;
			}
			#endif

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			treeUpref( exec->pdaRun->context );
			push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_CONTEXT_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_CONTEXT_BKT" << endl;
			}
			#endif

			treeUpref( exec->pdaRun->context );
			push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_GLOBAL_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_GLOBAL_R" << endl;
			}
			#endif

			treeUpref( prg->global );
			push( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_GLOBAL_WV" << endl;
			}
			#endif

			treeUpref( prg->global );
			push( prg->global );

			/* Set up the reverse instruction. */
			append( exec->reverseCode, IN_LOAD_GLOBAL_BKT );
			exec->rcodeUnitLen = SIZEOF_CODE;
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
			treeUpref( prg->global );
			push( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_GLOBAL_BKT" << endl;
			}
			#endif

			treeUpref( prg->global );
			push( prg->global );
			break;
		}
		case IN_LOAD_INPUT_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_INPUT_R" << endl;
			}
			#endif

			treeUpref( exec->fsmRun->curStream );
			push( exec->fsmRun->curStream );
			break;
		}
		case IN_LOAD_INPUT_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_INPUT_WV" << endl;
			}
			#endif

			treeUpref( exec->fsmRun->curStream );
			push( exec->fsmRun->curStream );

			/* Set up the reverse instruction. */
			append( exec->reverseCode, IN_LOAD_INPUT_BKT );
			exec->rcodeUnitLen = SIZEOF_CODE;
			break;
		}
		case IN_LOAD_INPUT_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_INPUT_WC" << endl;
			}
			#endif

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			treeUpref( exec->fsmRun->curStream );
			push( exec->fsmRun->curStream );
			break;
		}
		case IN_LOAD_INPUT_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_INPUT_BKT" << endl;
			}
			#endif

			treeUpref( exec->fsmRun->curStream );
			push( exec->fsmRun->curStream );
			break;
		}
		case IN_LOAD_CTX_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_CTX_R" << endl;
			}
			#endif

			treeUpref( exec->pdaRun->context );
			push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_CTX_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_CTX_WV" << endl;
			}
			#endif

			treeUpref( exec->pdaRun->context );
			push( exec->pdaRun->context );

			/* Set up the reverse instruction. */
			append( exec->reverseCode, IN_LOAD_INPUT_BKT );
			exec->rcodeUnitLen = SIZEOF_CODE;
			break;
		}
		case IN_LOAD_CTX_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_CTX_WC" << endl;
			}
			#endif

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			treeUpref( exec->pdaRun->context );
			push( exec->pdaRun->context );
			break;
		}
		case IN_LOAD_CTX_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LOAD_CTX_BKT" << endl;
			}
			#endif

			treeUpref( exec->pdaRun->context );
			push( exec->pdaRun->context );
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
			char **mark = exec->captures;

			int i;
			for ( i = 0; i < lelInfo[exec->genId].numCaptureAttr; i++ ) {
				CaptureAttr *ca = &prg->rtd->captureAttr[lelInfo[exec->genId].captureAttr + i];
				Head *data = stringAllocFull( prg, 
						mark[ca->mark_enter], mark[ca->mark_leave] - mark[ca->mark_enter] );
				Tree *string = constructString( prg, data );
				treeUpref( string );
				setLocal( exec->frame, -1 - i, string );
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

			Tree *val = getRhsEl( prg, exec->lhs, position );
			treeUpref( val );
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
			exec->frame = uiter->frame;
			exec->iframe = &uiter->stackRoot[-IFR_AA];
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
			treeUpref( val );
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
			splitRef( &sp, prg, &uiter->ref );
			Tree *split = uiter->ref.kid->tree;
			treeUpref( split );
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
			splitRef( &sp, prg, &uiter->ref );
			Tree *old = uiter->ref.kid->tree;
			uiter->ref.kid->tree = t;
			treeDownref( prg, sp, old );
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
			treeUpref( val );
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

			Tree *split = getLocalSplit( prg, exec->frame, field );
			treeUpref( split );
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
			treeDownref( prg, sp, local(field) );
			setLocal( exec->frame, field, val );
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
			treeUpref( val );
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
			splitRef( &sp, prg, ref );
			Tree *val = ref->kid->tree;
			treeUpref( val );
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
			splitRef( &sp, prg, ref );
			refSetValue( ref, val );
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
			treeDownref( prg, sp, obj );

			Tree *val = getField( obj, field );
			treeUpref( val );
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
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
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
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
			push( split );

			/* Set up the reverse instruction. */
			append( exec->reverseCode, IN_GET_FIELD_BKT );
			appendHalf( exec->reverseCode, field );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_HALF;
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
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
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

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_FIELD_WV " << field << endl;
			}
			#endif

			Tree *obj = pop();
			Tree *val = pop();
			treeDownref( prg, sp, obj );

			/* Save the old value, then set the field. */
			Tree *prev = getField( obj, field );
			setField( prg, obj, field, val );

			/* Set up the reverse instruction. */
			append( exec->reverseCode, IN_SET_FIELD_BKT );
			appendHalf( exec->reverseCode, field );
			appendWord( exec->reverseCode, (Word)prev );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_HALF + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );
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
			Tree *prev = getField( obj, field );
			treeDownref( prg, sp, prev );

			/* Set the field. */
			setField( prg, obj, field, val );

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
			treeDownref( prg, sp, val );
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
		case IN_SPRINTF: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SPRINTF" << endl;
			}
			#endif

			Tree *f = pop();
			f++;
			Tree *integer = pop();
			Tree *format = pop();
			Head *res = stringSprintf( prg, (Str*)format, (Int*)integer );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			push( str );
			treeDownref( prg, sp, integer );
			treeDownref( prg, sp, format );
			break;
		}
		case IN_STR_ATOI: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STR_ATOI" << endl;
			}
			#endif

			Str *str = (Str*)pop();
			Word res = strAtoi( str->value );
			Tree *integer = constructInteger( prg, res );
			treeUpref( integer );
			push( integer );
			treeDownref( prg, sp, (Tree*)str );
			break;
		}
		case IN_INT_TO_STR: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_INT_TO_STR" << endl;
			}
			#endif

			Int *i = (Int*)pop();
			Head *res = intToStr( prg, i->value );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			push( str );
			treeDownref( prg, sp, (Tree*) i );
			break;
		}
		case IN_TREE_TO_STR: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TREE_TO_STR" << endl;
			}
			#endif

			Tree *tree = pop();
			Head *res = treeToStr( sp, prg, tree );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			push( str );
			treeDownref( prg, sp, tree );
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
			Head *res = concatStr( s1->value, s2->value );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			treeDownref( prg, sp, (Tree*)s1 );
			treeDownref( prg, sp, (Tree*)s2 );
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
			Word res = strUord8( str->value );
			Tree *tree = constructInteger( prg, res );
			treeUpref( tree );
			push( tree );
			treeDownref( prg, sp, (Tree*)str );
			break;
		}
		case IN_STR_UORD16: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STR_UORD16" << endl;
			}
			#endif

			Str *str = (Str*)pop();
			Word res = strUord16( str->value );
			Tree *tree = constructInteger( prg, res );
			treeUpref( tree );
			push( tree );
			treeDownref( prg, sp, (Tree*)str );
			break;
		}

		case IN_STR_LENGTH: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STR_LENGTH" << endl;
			}
			#endif

			Str *str = (Str*)pop();
			long len = stringLength( str->value );
			Tree *res = constructInteger( prg, len );
			treeUpref( res );
			push( res );
			treeDownref( prg, sp, (Tree*)str );
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
			if ( testFalse( prg, tree ) )
				instr += dist;
			treeDownref( prg, sp, tree );
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
			if ( !testFalse( prg, tree ) )
				instr += dist;
			treeDownref( prg, sp, tree );
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
			exec->reject = true;
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
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r ? prg->falseVal : prg->trueVal;
			treeUpref( val );
			push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
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
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
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
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r < 0 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
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
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r <= 0 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
		}
		case IN_TST_GRTR: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TST_GRTR" << endl;
			}
			#endif

			Tree *o2 = pop();
			Tree *o1 = pop();
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r > 0 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
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
			long r = cmpTree( prg, o1, o2 );
			Tree *val = r >= 0 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
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
			long v2 = !testFalse( prg, o2 );
			long v1 = !testFalse( prg, o1 );
			Word r = v1 && v2;
			Tree *val = r ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
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
			long v2 = !testFalse( prg, o2 );
			long v1 = !testFalse( prg, o1 );
			Word r = v1 || v2;
			Tree *val = r ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			push( val );
			treeDownref( prg, sp, o1 );
			treeDownref( prg, sp, o2 );
			break;
		}
		case IN_NOT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_NOT" << endl;
			}
			#endif

			Tree *tree = (Tree*)pop();
			long r = testFalse( prg, tree );
			Tree *val = r ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			push( val );
			treeDownref( prg, sp, tree );
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
			Tree *tree = constructInteger( prg, r );
			treeUpref( tree );
			push( tree );
			treeDownref( prg, sp, (Tree*)o1 );
			treeDownref( prg, sp, (Tree*)o2 );
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
			Tree *tree = constructInteger( prg, r );
			treeUpref( tree );
			push( tree );
			treeDownref( prg, sp, (Tree*)o1 );
			treeDownref( prg, sp, (Tree*)o2 );
			break;
		}
		case IN_DIV_INT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_DIV_INT" << endl;
			}
			#endif

			Int *o2 = (Int*)pop();
			Int *o1 = (Int*)pop();
			long r = o1->value / o2->value;
			Tree *tree = constructInteger( prg, r );
			treeUpref( tree );
			push( tree );
			treeDownref( prg, sp, (Tree*)o1 );
			treeDownref( prg, sp, (Tree*)o2 );
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
			Tree *tree = constructInteger( prg, r );
			treeUpref( tree );
			push( tree );
			treeDownref( prg, sp, (Tree*)o1 );
			treeDownref( prg, sp, (Tree*)o2 );
			break;
		}
		case IN_DUP_TOP_OFF: {
			short off;
			read_half( off );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_DUP_N " << off << endl;
			}
			#endif

			Tree *val = top_off(off);
			treeUpref( val );
			push( val );
			break;
		}
		case IN_DUP_TOP: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_DUP_TOP" << endl;
			}
			#endif

			Tree *val = top();
			treeUpref( val );
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
			initTreeIter( (TreeIter*)mem, &rootRef, searchTypeId, ptop() );
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
			treeIterDestroy( &sp, iter );
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
			Kid *kid = treeChild( prg, rootRef.kid->tree );
			while ( kid != 0 ) {
				children++;
				push( (SW) kid );
				kid = kid->next;
			}

			void *mem = plocal(field);
			initRevTreeIter( (RevTreeIter*)mem, &rootRef, searchTypeId, stackRoot, children );
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
			Tree *res = treeSearch2( prg, tree, id );
			treeUpref( res );
			push( res );
			treeDownref( prg, sp, tree );
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
			Tree *res = treeIterAdvance( prg, &sp, iter );
			treeUpref( res );
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
			Tree *res = treeIterNextChild( prg, &sp, iter );
			treeUpref( res );
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
			Tree *res = treeRevIterPrevChild( prg, &sp, iter );
			treeUpref( res );
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
			Tree *res = treeIterNextRepeat( prg, &sp, iter );
			treeUpref( res );
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
			Tree *res = treeIterPrevRepeat( prg, &sp, iter );
			treeUpref( res );
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
			Tree *tree = treeIterDerefCur( iter );
			treeUpref( tree );
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
			splitIterCur( &sp, prg, iter );
			Tree *tree = treeIterDerefCur( iter );
			treeUpref( tree );
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
			splitIterCur( &sp, prg, iter );
			Tree *old = treeIterDerefCur( iter );
			setTriterCur( iter, tree );
			treeDownref( prg, sp, old );
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
			int matched = matchPattern( bindings, prg, rootNode, &kid, false );

			if ( !matched )
				memset( bindings, 0, sizeof(Tree*)*(1+numBindings) );
			else {
				int b;
				for ( b = 1; b <= numBindings; b++ )
					assert( bindings[b] != 0 );
			}

			#ifdef COLM_LOG_MATCH
			if ( colm_log_match ) {
				cerr << "match result: " << matched << endl;
			}
			#endif

			Tree *result = matched ? tree : 0;
			treeUpref( result );
			push( result ? tree : 0 );
			int b;
			for ( b = 1; b <= numBindings; b++ ) {
				treeUpref( bindings[b] );
				push( bindings[b] );
			}

			treeDownref( prg, sp, tree );
			break;
		}

		case IN_GET_ACCUM_CTX_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_ACCUM_CTX_R" << endl;
			}
			#endif

			Tree *obj = pop();
			Tree *ctx = ((Accum*)obj)->pdaRun->context;
			treeUpref( ctx );
			push( ctx );
			treeDownref( prg, sp, obj );
			break;
		}

		case IN_SET_ACCUM_CTX_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_ACCUM_CTX_WC" << endl;
			}
			#endif

			Tree *obj = pop();
			Tree *val = pop();
			parserSetContext( sp, prg, (Accum*)obj, val );
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
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_EXTRACT_INPUT_WC " << endl;
			}
			#endif

			Tree *accum = pop();
			Tree *input = extractInput( prg, (Accum*)accum );
			treeUpref( input );
			push( input );
			treeDownref( prg, sp, accum );
			break;
		}

		case IN_EXTRACT_INPUT_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_EXTRACT_INPUT_WV " << endl;
			}
			#endif

			Tree *accum = pop();
			Tree *input = extractInput( prg, (Accum*)accum );
			treeUpref( input );
			push( input );
			treeDownref( prg, sp, accum );
//
//			append( exec->reverseCode, IN_EXTRACT_INPUT_BKT );
//			exec->rcodeUnitLen += SIZEOF_CODE;
			break;
		}

		case IN_EXTRACT_INPUT_BKT: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode )
				cerr << "IN_EXTRACT_INPUT_BKT" << endl;
			#endif
			break;
		}

		case IN_SET_INPUT_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_SET_INPUT_WC " << endl;
			}
			#endif

			Tree *accum = pop();
			Tree *input = pop();
			setInput( prg, sp, (Accum*)accum, (Stream*)input );
			treeDownref( prg, sp, accum );
			treeDownref( prg, sp, input );
			break;
		}

		case IN_STREAM_APPEND_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_APPEND_WC " << endl;
			}
			#endif

			Tree *stream = pop();
			Tree *input = pop();
			streamAppend( sp, prg, input, (Stream*)stream );

			treeDownref( prg, sp, input );
			push( stream );
			break;
		}
		case IN_STREAM_APPEND_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_APPEND_WV " << endl;
			}
			#endif

			Tree *stream = pop();
			Tree *input = pop();
			Word len = streamAppend( sp, prg, input, (Stream*)stream );

			treeUpref( stream );
			push( stream );

			append( exec->reverseCode, IN_STREAM_APPEND_BKT );
			appendWord( exec->reverseCode, (Word) stream );
			appendWord( exec->reverseCode, (Word) input );
			appendWord( exec->reverseCode, (Word) len );
			append( exec->reverseCode, SIZEOF_CODE + 3 * SIZEOF_WORD );
			break;
		}
		case IN_STREAM_APPEND_BKT: {
			Tree *stream;
			Tree *input;
			Word len;
			read_tree( stream );
			read_tree( input );
			read_word( len );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_APPEND_BKT" << endl;
			}
			#endif

			undoStreamAppend( prg, sp, ((Stream*)stream)->in, len );
			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, input );
			break;
		}

		case IN_PARSE_FRAG_WC: {
			Half stopId;
			read_half( stopId );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode )
				cerr << "IN_PARSE_FRAG_WC " << endl;
			#endif

			Tree *accum = pop();
			Tree *stream = pop();

			parseStream( sp, prg, stream, (Accum*)accum, stopId );

			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, accum );
			break;
		}

		case IN_PARSE_FRAG_WV: {
			Half stopId;
			read_half( stopId );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_FRAG_WV " << endl;
			}
			#endif

			Tree *accum = pop();
			Tree *stream = pop();

			long consumed = ((Accum*)accum)->pdaRun->consumed;
			parseStream( sp, prg, stream, (Accum*)accum, stopId );

			//treeDownref( prg, sp, stream );
			//treeDownref( prg, sp, accum );

			append( exec->reverseCode, IN_PARSE_FRAG_BKT );
			appendWord( exec->reverseCode, (Word) accum );
			appendWord( exec->reverseCode, (Word) stream );
			appendWord( exec->reverseCode, consumed );
			append( exec->reverseCode, SIZEOF_CODE + 3 * SIZEOF_WORD );
			break;
		}

		case IN_PARSE_FRAG_BKT: {
			Tree *accum;
			Tree *input;
			long consumed;
			read_tree( accum );
			read_tree( input );
			read_word( consumed );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode )
				cerr << "IN_PARSE_FRAG_BKT " << consumed << endl;
			#endif

			undoParseStream( sp, prg, (Stream*)input, (Accum*)accum, consumed );

			treeDownref( prg, sp, accum );
			treeDownref( prg, sp, input );
			break;
		}

		case IN_PARSE_FINISH_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_FINISH_WC " << endl;
			}
			#endif

			Tree *accum = pop();
			Tree *result = parseFinish( sp, prg, (Accum*)accum, false );
			push( result );
			treeDownref( prg, sp, accum );
			break;
		}
		case IN_PARSE_FINISH_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_PARSE_FINISH_WV " << endl;
			}
			#endif

			Tree *accum = pop();
			long consumed = ((Accum*)accum)->pdaRun->consumed;
			Tree *result = parseFinish( sp, prg, (Accum*)accum, true );
			push( result );

			treeUpref( result );
			append( exec->reverseCode, IN_PARSE_FINISH_BKT );
			appendWord( exec->reverseCode, (Word) accum );
			appendWord( exec->reverseCode, (Word) result );
			appendWord( exec->reverseCode, (Word) consumed );
			append( exec->reverseCode, SIZEOF_CODE + 3*SIZEOF_WORD );
			break;
		}
		case IN_PARSE_FINISH_BKT: {
			Tree *parser;
			Tree *tree;
			Word consumed;

			read_tree( parser );
			read_tree( tree );
			read_word( consumed );

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode )
				cerr << "IN_PARSE_FINISH_BKT " << consumed << endl;
			#endif

			undoParseStream( sp, prg, ((Accum*)parser)->stream, (Accum*)parser, consumed );
			((Accum*)parser)->stream->in->eof = false;

			/* This needs an implementation. */
			treeDownref( prg, sp, parser );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_STREAM_PULL: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PULL" << endl;
			}
			#endif
			Tree *stream = pop();
			Tree *len = pop();
			Tree *string = streamPull2( prg, exec->fsmRun, (Stream*)stream, len );
			treeUpref( string );
			push( string );

			/* Single unit. */
			treeUpref( string );
			append( exec->reverseCode, IN_STREAM_PULL_BKT );
			appendWord( exec->reverseCode, (Word) string );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );

			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, len );
			break;
		}
		case IN_STREAM_PULL_BKT: {
			Tree *string;
			read_tree( string );

			Tree *stream = pop();

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PULL_BKT" << endl;
			}
			#endif

			undoPull( prg, exec->fsmRun, (Stream*)stream, string );
			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, string );
			break;
		}
		case IN_STREAM_PUSH_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PUSH_WV" << endl;
			}
			#endif
			Tree *stream = pop();
			Tree *tree = pop();
			Word len = streamPush( prg, sp, ((Stream*)stream), tree, false );
			push( 0 );

			/* Single unit. */
			append( exec->reverseCode, IN_STREAM_PUSH_BKT );
			appendWord( exec->reverseCode, len );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );

			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_STREAM_PUSH_IGNORE_WV: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PUSH_IGNORE_WV" << endl;
			}
			#endif
			Tree *stream = pop();
			Tree *tree = pop();
			Word len = streamPush( prg, sp, ((Stream*)stream), tree, true );
			push( 0 );

			/* Single unit. */
			append( exec->reverseCode, IN_STREAM_PUSH_BKT );
			appendWord( exec->reverseCode, len );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );

			treeDownref( prg, sp, stream );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_STREAM_PUSH_BKT: {
			Word len;
			read_word( len );

			Tree *stream = pop();

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STREAM_PUSH_BKT" << endl;
			}
			#endif

			undoStreamPush( prg, sp, ((Stream*)stream)->in, len );
			treeDownref( prg, sp, stream );
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

			int b;
			for ( b = 1; b <= numBindings; b++ ) {
				bindings[b] = pop();
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
				replTree = constructReplacementTree( bindings, 
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
			Tree *res = constructTerm( prg, tokenId, str->value );
			treeUpref( res );
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

			Tree *result = makeToken2( sp, prg, nargs );
			long i;
			for ( i = 0; i < nargs; i++ ) {
				Tree *arg = pop();
				treeDownref( prg, sp, arg );
			}
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

			Tree *result = makeTree( sp, prg, nargs );
			long i;
			for ( i = 0; i < nargs; i++ ) {
				Tree *arg = pop();
				treeDownref( prg, sp, arg );
			}
			push( result );
			break;
		}
		case IN_TREE_NEW: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TREE_NEW " << endl;
			}
			#endif

			Tree *tree = pop();
			Tree *res = constructPointer( prg, tree );
			treeUpref( res );
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
			treeDownref( prg, sp, (Tree*)ptr );

			Tree *dval = getPtrVal( ptr );
			treeUpref( dval );
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
			treeDownref( prg, sp, (Tree*)ptr );

			Tree *dval = getPtrValSplit( prg, ptr );
			treeUpref( dval );
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

			Tree *dval = getPtrValSplit( prg, ptr );
			treeUpref( dval );
			push( dval );

			/* This is an initial global load. Need to reverse execute it. */
			append( exec->reverseCode, IN_PTR_DEREF_BKT );
			appendWord( exec->reverseCode, (Word) ptr );
			exec->rcodeUnitLen = SIZEOF_CODE + SIZEOF_WORD;
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

			Tree *dval = getPtrValSplit( prg, ptr );
			treeUpref( dval );
			push( dval );

			treeDownref( prg, sp, (Tree*)ptr );
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
			Kid *attr_kid = getFieldKid( obj, field );

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
			Head *data = stringCopy( prg, tree->tokdata );
			Tree *str = constructString( prg, data );
			treeUpref( str );
			push( str );
			treeDownref( prg, sp, tree );
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
			Head *head = stringCopy( prg, ((Str*)val)->value );
			stringFree( prg, tree->tokdata );
			tree->tokdata = head;

			treeDownref( prg, sp, tree );
			treeDownref( prg, sp, val );
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
			Head *head = stringCopy( prg, ((Str*)val)->value );
			tree->tokdata = head;

			/* Set up reverse code. Needs no args. */
			append( exec->reverseCode, IN_SET_TOKEN_DATA_BKT );
			appendWord( exec->reverseCode, (Word)oldval );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );

			treeDownref( prg, sp, tree );
			treeDownref( prg, sp, val );
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
			stringFree( prg, tree->tokdata );
			tree->tokdata = head;
			treeDownref( prg, sp, tree );
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
				integer = constructInteger( prg, tree->tokdata->location->byte );
				treeUpref( integer );
			}
			push( integer );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_GET_MATCH_LENGTH_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_MATCH_LENGTH_R" << endl;
			}
			#endif
			Tree *integer = constructInteger( prg, stringLength(exec->matchText) );
			treeUpref( integer );
			push( integer );
			break;
		}
		case IN_GET_MATCH_TEXT_R: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_MATCH_TEXT_R" << endl;
			}
			#endif
			Head *s = stringCopy( prg, exec->matchText );
			Tree *tree = constructString( prg, s );
			treeUpref( tree );
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
			long len = listLength( list );
			Tree *res = constructInteger( prg, len );
			treeUpref( res );
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

			treeDownref( prg, sp, obj );

			listAppend2( prg, (List*)obj, val );
			treeUpref( prg->trueVal );
			push( prg->trueVal );

			/* Set up reverse code. Needs no args. */
			append( exec->reverseCode, IN_LIST_APPEND_BKT );
			exec->rcodeUnitLen += SIZEOF_CODE;
			append( exec->reverseCode, exec->rcodeUnitLen );
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

			treeDownref( prg, sp, obj );

			listAppend2( prg, (List*)obj, val );
			treeUpref( prg->trueVal );
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
			treeDownref( prg, sp, obj );

			Tree *tree = listRemoveEnd( prg, (List*)obj );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_LIST_REMOVE_END_WC: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_LIST_REMOVE_END_WC" << endl;
			}
			#endif

			Tree *obj = pop();
			treeDownref( prg, sp, obj );

			Tree *end = listRemoveEnd( prg, (List*)obj );
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
			treeDownref( prg, sp, obj );

			Tree *end = listRemoveEnd( prg, (List*)obj );
			push( end );

			/* Set up reverse. The result comes off the list downrefed.
			 * Need it up referenced for the reverse code too. */
			treeUpref( end );
			append( exec->reverseCode, IN_LIST_REMOVE_END_BKT );
			appendWord( exec->reverseCode, (Word)end );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );
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
			treeDownref( prg, sp, obj );

			listAppend2( prg, (List*)obj, val );
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
			treeDownref( prg, sp, obj );

			Tree *val = getListMem( (List*)obj, field );
			treeUpref( val );
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
			treeDownref( prg, sp, obj );

			Tree *val = getListMemSplit( prg, (List*)obj, field );
			treeUpref( val );
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
			treeDownref( prg, sp, obj );

			Tree *val = getListMemSplit( prg, (List*)obj, field );
			treeUpref( val );
			push( val );

			/* Set up the reverse instruction. */
			append( exec->reverseCode, IN_GET_LIST_MEM_BKT );
			appendHalf( exec->reverseCode, field );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_HALF;
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
			treeDownref( prg, sp, obj );

			Tree *res = getListMemSplit( prg, (List*)obj, field );
			treeUpref( res );
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
			treeDownref( prg, sp, obj );

			Tree *val = pop();
			Tree *existing = setListMem( (List*)obj, field, val );
			treeDownref( prg, sp, existing );
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
			treeDownref( prg, sp, obj );

			Tree *val = pop();
			Tree *existing = setListMem( (List*)obj, field, val );

			/* Set up the reverse instruction. */
			append( exec->reverseCode, IN_SET_LIST_MEM_BKT );
			appendHalf( exec->reverseCode, field );
			appendWord( exec->reverseCode, (Word)existing );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_HALF + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );
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
			treeDownref( prg, sp, obj );

			Tree *undid = setListMem( (List*)obj, field, val );
			treeDownref( prg, sp, undid );
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

			treeDownref( prg, sp, obj );

			int inserted = mapInsert( prg, (Map*)obj, key, val );
			Tree *result = inserted ? prg->trueVal : prg->falseVal;
			treeUpref( result );
			push( result );

			/* Set up the reverse instruction. If the insert fails still need
			 * to pop the loaded map object. Just use the reverse instruction
			 * since it's nice to see it in the logs. */

			/* Need to upref key for storage in reverse code. */
			treeUpref( key );
			append( exec->reverseCode, IN_MAP_INSERT_BKT );
			append( exec->reverseCode, inserted );
			appendWord( exec->reverseCode, (Word)key );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_CODE + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );

			if ( ! inserted ) {
				treeDownref( prg, sp, key );
				treeDownref( prg, sp, val );
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

			treeDownref( prg, sp, obj );

			int inserted = mapInsert( prg, (Map*)obj, key, val );
			Tree *result = inserted ? prg->trueVal : prg->falseVal;
			treeUpref( result );
			push( result );

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

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_INSERT_BKT" << endl;
			}
			#endif
			
			Tree *obj = pop();
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
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_STORE_WC" << endl;
			}
			#endif

			Tree *obj = pop();
			Tree *element = pop();
			Tree *key = pop();

			Tree *existing = mapStore( prg, (Map*)obj, key, element );
			Tree *result = existing == 0 ? prg->trueVal : prg->falseVal;
			treeUpref( result );
			push( result );

			treeDownref( prg, sp, obj );
			if ( existing != 0 ) {
				treeDownref( prg, sp, key );
				treeDownref( prg, sp, existing );
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

			Tree *existing = mapStore( prg, (Map*)obj, key, element );
			Tree *result = existing == 0 ? prg->trueVal : prg->falseVal;
			treeUpref( result );
			push( result );

			/* Set up the reverse instruction. */
			treeUpref( key );
			treeUpref( existing );
			append( exec->reverseCode, IN_MAP_STORE_BKT );
			appendWord( exec->reverseCode, (Word)key );
			appendWord( exec->reverseCode, (Word)existing );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );
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

			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_STORE_BKT" << endl;
			}
			#endif

			Tree *obj = pop();
			Tree *stored = mapUnstore( prg, (Map*)obj, key, val );

			treeDownref( prg, sp, stored );
			if ( val == 0 )
				treeDownref( prg, sp, key );

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
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
			TreePair pair = mapRemove( prg, (Map*)obj, key );

			push( pair.val );

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
			treeDownref( prg, sp, pair.key );
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
			TreePair pair = mapRemove( prg, (Map*)obj, key );

			treeUpref( pair.val );
			push( pair.val );

			/* Reverse instruction. */
			append( exec->reverseCode, IN_MAP_REMOVE_BKT );
			appendWord( exec->reverseCode, (Word)pair.key );
			appendWord( exec->reverseCode, (Word)pair.val );
			exec->rcodeUnitLen += SIZEOF_CODE + SIZEOF_WORD + SIZEOF_WORD;
			append( exec->reverseCode, exec->rcodeUnitLen );

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
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
			assert( ( key == 0 ) ^ ( val != 0 ) );

			Tree *obj = pop();
			if ( key != 0 )
				mapUnremove( prg, (Map*)obj, key, val );

			treeDownref( prg, sp, obj );
			break;
		}
		case IN_MAP_LENGTH: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_MAP_LENGTH" << endl;
			}
			#endif

			Tree *obj = pop();
			long len = mapLength( (Map*)obj );
			Tree *res = constructInteger( prg, len );
			treeUpref( res );
			push( res );

			treeDownref( prg, sp, obj );
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
			Tree *result = mapFind( prg, (Map*)obj, key );
			treeUpref( result );
			push( result );

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
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

			exec->frame = ptop();
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
			downrefLocalTrees( prg, sp, exec->frame, fi->trees, fi->treesLen );
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
			push( (SW)exec->frame );

			instr = prg->rtd->frameInfo[fi->frameId].codeWV;
			exec->frame = ptop();
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
			push( (SW)exec->frame );

			instr = prg->rtd->frameInfo[fi->frameId].codeWC;
			exec->frame = ptop();
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
				uiter->frame = exec->frame;

				/* Restore the instruction and frame pointer. */
				instr = (Code*) local_iframe(IFR_RIN);
				exec->frame = (Tree**) local_iframe(IFR_RFR);
				exec->iframe = (Tree**) local_iframe(IFR_RIF);

				/* Return the yield result on the top of the stack. */
				Tree *result = uiter->ref.kid != 0 ? prg->trueVal : prg->falseVal;
				treeUpref( result );
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
			UserIter *uiter = uiterCreate( &sp, prg, fi, searchId );
			local(field) = (SW) uiter;

			/* This is a setup similar to as a call, only the frame structure
			 * is slightly different for user iterators. We aren't going to do
			 * the call. We don't need to set up the return ip because the
			 * uiter advance will set it. The frame we need to do because it
			 * is set once for the lifetime of the iterator. */
			push( 0 );            /* Return instruction pointer,  */
			push( (SW)exec->iframe ); /* Return iframe. */
			push( (SW)exec->frame );  /* Return frame. */

			uiterInit( prg, sp, uiter, fi, true );
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
			UserIter *uiter = uiterCreate( &sp, prg, fi, searchId );
			local(field) = (SW) uiter;

			/* This is a setup similar to as a call, only the frame structure
			 * is slightly different for user iterators. We aren't going to do
			 * the call. We don't need to set up the return ip because the
			 * uiter advance will set it. The frame we need to do because it
			 * is set once for the lifetime of the iterator. */
			push( 0 );            /* Return instruction pointer,  */
			push( (SW)exec->iframe ); /* Return iframe. */
			push( (SW)exec->frame );  /* Return frame. */

			uiterInit( prg, sp, uiter, fi, false );
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
			userIterDestroy( &sp, uiter );
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
			downrefLocalTrees( prg, sp, exec->frame, fi->trees, fi->treesLen );

			popn( fui->frameSize );
			exec->frame = (Tree**) pop();
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
			Head *head = stringToUpper( in->tokdata );
			Tree *upper = constructString( prg, head );
			treeUpref( upper );
			push( upper );
			treeDownref( prg, sp, in );
			break;
		}
		case IN_TO_LOWER: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_TO_LOWER" << endl;
			}
			#endif
			Tree *in = pop();
			Head *head = stringToLower( in->tokdata );
			Tree *lower = constructString( prg, head );
			treeUpref( lower );
			push( lower );
			treeDownref( prg, sp, in );
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
			Tree *res = (Tree*)openFile( prg, name, mode );
			treeUpref( res );
			push( res );
			treeDownref( prg, sp, name );
			treeDownref( prg, sp, mode );
			break;
		}
		case IN_GET_STDIN: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_GET_STDIN" << endl;
			}
			#endif

			/* Pop the root object. */
			Tree *obj = pop();
			treeDownref( prg, sp, obj );
			if ( prg->stdinVal == 0 ) {
				prg->stdinVal = openStreamFd( prg, 0 );
				treeUpref( (Tree*)prg->stdinVal );
			}

			treeUpref( (Tree*)prg->stdinVal );
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
			Tree *tree = constructArgv( prg, prg->argc, prg->argv );
			setField( prg, prg->global, field, tree );
			break;
		}
		case IN_STOP: {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode ) {
				cerr << "IN_STOP" << endl;
			}
			#endif

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
			fatal( "UNKNOWN INSTRUCTION: -- something is wrong\n" );
			assert(false);
			break;
		}
	}
	goto again;

out:
	assert( sp == root );
}

void parseError( InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, int tokId, Tree *tree )
{
	fprintf( stderr, "error: %ld:\n", inputStream->line );
//	cerr << "error:" << inputStream->line << ": at token ";
//	if ( tokId < 128 )
//		cerr << "\"" << pdaRun->tables->rtd->lelInfo[tokId].name << "\"";
////	else 
//		cerr << pdaRun->tables->rtd->lelInfo[tokId].name;
//	if ( stringLength( tree->tokdata ) > 0 ) {
//		cerr << " with data \"";
//		cerr.write( stringData( tree->tokdata ), 
//				stringLength( tree->tokdata ) );
//		cerr << "\"";
//	}
//	cerr << ": ";
}
