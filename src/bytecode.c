/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
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

typedef struct colm_struct Struct;

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

#define true 1
#define false 0

#define read_byte( i ) do { \
	i = ((uchar) *instr++); \
} while(0)

#define consume_byte( ) do { \
	instr += 1; \
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

	#define consume_word( ) do { \
		instr += 4; \
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

	#define consume_word( ) do { \
		instr += 8; \
	} while(0)
#endif

#define read_half( i ) do { \
	i = ((Word) *instr++); \
	i |= ((Word) *instr++) << 8; \
} while(0)

void colm_parser_set_context( Program *prg, Tree **sp, Parser *parser, Struct *val )
{
	parser->pdaRun->context = val;
}

static Head *treeToStr( Program *prg, Tree **sp, Tree *tree, int trim )
{
	/* Collect the tree data. */
	StrCollect collect;
	initStrCollect( &collect );

	printTreeCollect( prg, sp, &collect, tree, trim );

	/* Set up the input stream. */
	Head *ret = stringAllocFull( prg, collect.data, collect.length );

	strCollectDestroy( &collect );

	return ret;
}

Word streamAppend( Program *prg, Tree **sp, Tree *input, StreamImpl *is )
{
	long length = 0;

	if ( input->id == LEL_ID_STR ) {
		/* Collect the tree data. */
		StrCollect collect;
		initStrCollect( &collect );
		printTreeCollect( prg, sp, &collect, input, false );

		/* Load it into the input. */
		is->funcs->appendData( is, collect.data, collect.length );
		length = collect.length;
		strCollectDestroy( &collect );
	}
	else if ( input->id == LEL_ID_PTR ) {
		treeUpref( input );
		is->funcs->appendStream( is, input );
	}
	else {
		treeUpref( input );
		is->funcs->appendTree( is, input );
	}

	return length;
}

Word streamAppendStream( Program *prg, Tree **sp, StreamImpl *dest, Tree *stream )
{
	long length = 0;

	dest->funcs->appendStream( dest, stream );

	return length;
}

void undoStreamAppend( Program *prg, Tree **sp, StreamImpl *is, Tree *input, long length )
{
	if ( input->id == LEL_ID_STR )
		is->funcs->undoAppendData( is, length );
	else if ( input->id == LEL_ID_PTR )
		is->funcs->undoAppendStream( is );
	else {
		Tree *tree = is->funcs->undoAppendTree( is );
		treeDownref( prg, sp, tree );
	}
}

void undoStreamAppendStream( Program *prg, Tree **sp, StreamImpl *is,
		Tree *input, long length )
{
	is->funcs->undoAppendStream( is );
}

long parseFrag( Program *prg, Tree **sp, PdaRun *pdaRun,
		Stream *input, long stopId, long entry )
{
switch ( entry ) {
case PcrStart:

	if ( ! pdaRun->parseError ) {
		pdaRun->stopTarget = stopId;

		long pcr = parseLoop( prg, sp, pdaRun, 
				streamToImpl( input ), entry );

		while ( pcr != PcrDone ) {

return pcr;
case PcrReduction:
case PcrGeneration:
case PcrPreEof:
case PcrReverse:

			pcr = parseLoop( prg, sp, pdaRun, 
					streamToImpl( input ), entry );
		}
	}

case PcrDone:
break; }

	return PcrDone;
}

long parseFinish( Tree **result, Program *prg, Tree **sp,
		PdaRun *pdaRun, Stream *input , int revertOn, long entry )
{
	StreamImpl *si;
switch ( entry ) {
case PcrStart:

	if ( pdaRun->stopTarget <= 0 ) {
		si = streamToImpl( input );
		si->funcs->setEof( si );

		if ( ! pdaRun->parseError ) {
			si = streamToImpl( input );
			long pcr = parseLoop( prg, sp, pdaRun, si, entry );

		 	while ( pcr != PcrDone ) {

return pcr;
case PcrReduction:
case PcrGeneration:
case PcrPreEof:
case PcrReverse:

				si = streamToImpl( input );
				pcr = parseLoop( prg, sp, pdaRun, si, entry );
			}
		}
	}

	/* FIXME: need something here to check that we are not stopped waiting for
	 * more data when we are actually expected to finish. This check doesn't
	 * work (at time of writing). */
	//assert( (pdaRun->stopTarget > 0 && pdaRun->stopParsing) || streamToImpl( input )->eofSent );

	if ( !revertOn )
		commitFull( prg, sp, pdaRun, 0 );
	
	Tree *tree = getParsedRoot( pdaRun, pdaRun->stopTarget > 0 );
	treeUpref( tree );

	*result = tree;

case PcrDone:
break; }

	return PcrDone;
}

long undoParseFrag( Program *prg, Tree **sp, PdaRun *pdaRun,
		Stream *input, long steps, long entry )
{
	debug( prg, REALM_PARSE,
			"undo parse frag, target steps: %ld, pdarun steps: %ld\n",
			steps, pdaRun->steps );

	resetToken( pdaRun );

switch ( entry ) {
case PcrStart:

	if ( steps < pdaRun->steps ) {
		/* Setup environment for going backwards until we reduced steps to
		 * what we want. */
		pdaRun->numRetry += 1;
		pdaRun->targetSteps = steps;
		pdaRun->triggerUndo = 1;

		/* The parse loop will recognise the situation. */
		long pcr = parseLoop( prg, sp, pdaRun, streamToImpl(input), entry );
		while ( pcr != PcrDone ) {

return pcr;
case PcrReduction:
case PcrGeneration:
case PcrPreEof:
case PcrReverse:

			pcr = parseLoop( prg, sp, pdaRun, streamToImpl(input), entry );
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

Tree *streamPullBc( Program *prg, Tree **sp, PdaRun *pdaRun, StreamImpl *in, Tree *length )
{
	long len = ((Int*)length)->value;
	Head *tokdata = streamPull( prg, sp, pdaRun, in, len );
	return constructString( prg, tokdata );
}

void undoPull( Program *prg, StreamImpl *in, Tree *str )
{
	const char *data = stringData( ( (Str*)str )->value );
	long length = stringLength( ( (Str*)str )->value );
	undoStreamPull( in, data, length );
}

static long stream_push( Program *prg, Tree **sp, StreamImpl *in, Tree *tree, int ignore )
{
	if ( tree->id == LEL_ID_STR ) {
		/* This should become a compile error. If it's text, it's up to the
		 * scanner to decide. Want to force it then send a token. */
		assert( !ignore );
			
		/* Collect the tree data. */
		StrCollect collect;
		initStrCollect( &collect );
		printTreeCollect( prg, sp, &collect, tree, false );

		streamPushText( in, collect.data, collect.length );
		long length = collect.length;
		strCollectDestroy( &collect );

		return length;
	}
	else if ( tree->id == LEL_ID_PTR ) {
		treeUpref( tree );
		streamPushStream( in, tree );
		return -1;
	}
	else {
		treeUpref( tree );
		streamPushTree( in, tree, ignore );
		return -1;
	}
}

static long stream_push_stream( Program *prg, Tree **sp,
		StreamImpl *in, Stream *stream )
{
	streamPushStream( in, (Tree*)stream );
	return -1;
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

static void downref_local_trees( Program *prg, Tree **sp,
		Tree **frame, LocalInfo *locals, long localsLen )
{
	long i;
	for ( i = localsLen-1; i >= 0; i-- ) {
		if ( locals[i].type == LI_Tree ) {
			debug( prg, REALM_BYTECODE, "local tree downref: %ld\n",
					(long)locals[i].offset );

			Tree *tree = (Tree*) frame[(long)locals[i].offset];
			treeDownref( prg, sp, tree );
		}
	}
}

static void downrefLocals( Program *prg, Tree ***psp,
		Tree **frame, LocalInfo *locals, long localsLen )
{
	long i;
	for ( i = localsLen-1; i >= 0; i-- ) {
		switch ( locals[i].type ) {
			case LI_Tree: {
				debug( prg, REALM_BYTECODE, "local tree downref: %ld\n",
						(long)locals[i].offset );
				Tree *tree = (Tree*) frame[(long)locals[i].offset];
				treeDownref( prg, *psp, tree );
				break;
			}
			case LI_Iter: {
				debug( prg, REALM_BYTECODE, "local iter downref: %ld\n",
						(long)locals[i].offset );
				TreeIter *iter = (TreeIter*) &frame[(long)locals[i].offset];
				treeIterDestroy( prg, psp, iter );
				break;
			}
			case LI_RevIter: {
				debug( prg, REALM_BYTECODE, "local rev iter downref: %ld\n",
						(long)locals[i].offset );
				RevTreeIter *riter = (RevTreeIter*) &frame[(long)locals[i].offset];
				revTreeIterDestroy( prg, psp, riter );
				break;
			}
			case LI_UserIter: {
				debug( prg, REALM_BYTECODE, "local user iter downref: %ld\n",
						(long)locals[i].offset );
				UserIter *uiter = (UserIter*) frame[locals[i].offset];
				userIterDestroy2( prg, psp, uiter );
				break;
			}
		}
	}
}

static Tree *construct_arg0( Program *prg, int argc, const char **argv )
{
	Tree *arg0 = 0;
	if ( argc > 0 ) {
		Head *head = stringAllocPointer( prg, argv[0], strlen(argv[0]) );
		arg0 = constructString( prg, head );
		treeUpref( arg0 );
	}
	return arg0;
}

static List *construct_argv( Program *prg, int argc, const char **argv )
{
	List *list = (List*)constructGeneric( prg, prg->rtd->argvGenericId );
	int i;
	for ( i = 1; i < argc; i++ ) {
		Head *head = stringAllocPointer( prg, argv[i], strlen(argv[i]) );
		Tree *arg = constructString( prg, head );
		treeUpref( arg );

		Struct *strct = colm_struct_new_size( prg, 16 );
		strct->id = prg->rtd->argvElId;
		colm_struct_set_field( strct, Tree*, 0, arg );
		ListEl *listEl = colm_struct_get_addr( strct, ListEl*, 1 );
		colm_list_append( list, listEl );
	}
	
	return list;
}

/*
 * Execution environment
 */

void colm_rcode_downref_all( Program *prg, Tree **sp, RtCodeVect *rev )
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

void colm_execute( Program *prg, Execution *exec, Code *code )
{
	Tree **sp = prg->stackRoot;

	FrameInfo *fi = &prg->rtd->frameInfo[prg->rtd->rootFrameId];
	long stretch = fi->argSize + 4 + fi->frameSize;
	vm_contiguous( stretch );

	/* Set up the stack as if we have called. We allow a return value. */
	vm_push( 0 ); 
	vm_push( 0 );
	vm_push( 0 );
	vm_push( 0 );

	/* Execution loop. */
	executeCode( prg, exec, sp, code );

	vm_pop_ignore();
	vm_pop_ignore();
	treeDownref( prg, sp, prg->returnVal );
	prg->returnVal = vm_pop();
	vm_pop_ignore();

	prg->stackRoot = sp;
}

int colm_make_reverse_code( PdaRun *pdaRun )
{
	RtCodeVect *reverseCode = &pdaRun->reverseCode;
	RtCodeVect *rcodeCollect = &pdaRun->rcodeCollect;

	/* Do we need to revert the left hand side? */

	/* Check if there was anything generated. */
	if ( rcodeCollect->tabLen == 0 )
		return false;

	if ( pdaRun->rcBlockCount == 0 ) {
		/* One reverse code run for the DECK terminator. */
		appendCode( reverseCode, IN_PCR_END_DECK );
		appendCode( reverseCode, IN_PCR_RET );
		appendWord( reverseCode, 2 );
		pdaRun->rcBlockCount += 1;
		incrementSteps( pdaRun );
	}

	long startLength = reverseCode->tabLen;

	/* Go backwards, group by group, through the reverse code. Push each group
	 * to the global reverse code stack. */
	Code *p = rcodeCollect->data + rcodeCollect->tabLen;
	while ( p != rcodeCollect->data ) {
		p--;
		long len = *p;
		p = p - len;
		appendCode2( reverseCode, p, len );
	}

	/* Stop, then place a total length in the global stack. */
	appendCode( reverseCode, IN_PCR_RET );
	long length = reverseCode->tabLen - startLength;
	appendWord( reverseCode, length );

	/* Clear the revere code buffer. */
	rcodeCollect->tabLen = 0;

	pdaRun->rcBlockCount += 1;
	incrementSteps( pdaRun );

	return true;
}

void colm_transfer_reverse_code( PdaRun *pdaRun, ParseTree *parseTree )
{
	if ( pdaRun->rcBlockCount > 0 ) {
		//debug( REALM_PARSE, "attaching reverse code to token\n" );
		parseTree->flags |= PF_HAS_RCODE;
		pdaRun->rcBlockCount = 0;
	}
}

void rcodeUnitTerm( Execution *exec )
{
	appendCode( &exec->parser->pdaRun->rcodeCollect, exec->rcodeUnitLen );
	exec->rcodeUnitLen = 0;
}

void rcodeUnitStart( Execution *exec )
{
	exec->rcodeUnitLen = 0;
}

void rcodeCode( Execution *exec, const Code code )
{
	appendCode( &exec->parser->pdaRun->rcodeCollect, code );
	exec->rcodeUnitLen += SIZEOF_CODE;
}

void rcodeHalf( Execution *exec, const Half half )
{
	appendHalf( &exec->parser->pdaRun->rcodeCollect, half );
	exec->rcodeUnitLen += SIZEOF_HALF;
}

void rcodeWord( Execution *exec, const Word word )
{
	appendWord( &exec->parser->pdaRun->rcodeCollect, word );
	exec->rcodeUnitLen += SIZEOF_WORD;
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
	return prcode;
}

Tree **executeCode( Program *prg, Execution *exec, Tree **sp, Code *instr )
{
	/* When we exit we are going to verify that we did not eat up any stack
	 * space. */
	Tree **root = sp;
	Code c;

again:
	c = *instr++;
	//debug( REALM_BYTECODE, "--in 0x%x\n", c );

	switch ( c ) {
		case IN_RESTORE_LHS: {
			Tree *restore;
			read_tree( restore );

			debug( prg, REALM_BYTECODE, "IN_RESTORE_LHS\n" );
			treeDownref( prg, sp, exec->parser->pdaRun->parseInput->shadow->tree );
			exec->parser->pdaRun->parseInput->shadow->tree = restore;
			break;
		}
		case IN_LOAD_NIL: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_NIL\n" );
			vm_push( 0 );
			break;
		}
		case IN_LOAD_TREE: {
			Tree *tree;
			read_tree( tree );
			vm_push( tree );
			debug( prg, REALM_BYTECODE, "IN_LOAD_TREE %p id: %d refs: %d\n",
					tree, tree->id, tree->refs );
			break;
		}
		case IN_LOAD_WORD: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_WORD\n" );
			Word w;
			read_word( w );
			vm_push( (SW)w );
			break;
		}
		case IN_LOAD_TRUE: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_TRUE\n" );
			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );
			break;
		}
		case IN_LOAD_FALSE: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_FALSE\n" );
			treeUpref( prg->falseVal );
			vm_push( prg->falseVal );
			break;
		}
		case IN_LOAD_INT: {
			Word i;
			read_word( i );

			debug( prg, REALM_BYTECODE, "IN_LOAD_INT %d\n", i );

			Tree *tree = constructInteger( prg, i );
			treeUpref( tree );
			vm_push( tree );
			break;
		}
		case IN_LOAD_STR: {
			Word offset;
			read_word( offset );

			debug( prg, REALM_BYTECODE, "IN_LOAD_STR %d\n", offset );

			Head *lit = makeLiteral( prg, offset );
			Tree *tree = constructString( prg, lit );
			treeUpref( tree );
			vm_push( tree );
			break;
		}
		case IN_PRINT: {
			int n, i;
			read_byte( n );
			debug( prg, REALM_BYTECODE, "IN_PRINT %d\n", n );

			Tree *arg[n];
			for ( i = n-1; i >= 0; i-- )
				arg[i] = vm_pop();

			for ( i = 0; i < n; i++ )
				printTreeFile( prg, sp, stdout, arg[i], false );

			for ( i = 0; i < n; i++ )
				treeDownref( prg, sp, arg[i] );
			break;
		}
		case IN_PRINT_STREAM: {
			int n, i;
			read_byte( n );
			debug( prg, REALM_BYTECODE, "IN_PRINT_STREAM %d\n", n );

			Tree *arg[n];
			for ( i = n-1; i >= 0; i-- )
				arg[i] = vm_pop();
			Stream *stream = vm_pop_stream();
			StreamImpl *si = streamToImpl( stream );

			for ( i = 0; i < n; i++ ) {
				if ( si->file != 0 )
					printTreeFile( prg, sp, si->file, arg[i], false );
				else
					printTreeFd( prg, sp, si->fd, arg[i], false );
			}

			for ( i = 0; i < n; i++ )
				treeDownref( prg, sp, arg[i] );
			break;
		}
		case IN_PRINT_XML_AC: {
			int n, i;
			read_byte( n );

			debug( prg, REALM_BYTECODE, "IN_PRINT_XML_AC %d\n", n );

			Tree *arg[n];
			for ( i = n-1; i >= 0; i-- )
				arg[i] = vm_pop();

			for ( i = 0; i < n; i++ )
				printXmlStdout( prg, sp, arg[i], true, true );

			for ( i = 0; i < n; i++ )
				treeDownref( prg, sp, arg[i] );
			break;
		}
		case IN_PRINT_XML: {
			int n, i;
			read_byte( n );
			debug( prg, REALM_BYTECODE, "IN_PRINT_XML %d", n );

			Tree *arg[n];
			for ( i = n-1; i >= 0; i-- )
				arg[i] = vm_pop();

			for ( i = 0; i < n; i++ )
				printXmlStdout( prg, sp, arg[i], false, true );

			for ( i = 0; i < n; i++ )
				treeDownref( prg, sp, arg[i] );
			break;
		}
		case IN_LOAD_CONTEXT_R: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CONTEXT_R\n" );

			vm_push_type( Struct*, exec->parser->pdaRun->context );
			break;
		}
		case IN_LOAD_CONTEXT_WV: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CONTEXT_WV\n" );

			vm_push_type( Struct *, exec->parser->pdaRun->context );

			/* Set up the reverse instruction. */
			rcodeUnitStart( exec );
			rcodeCode( exec, IN_LOAD_CONTEXT_BKT );
			break;
		}
		case IN_LOAD_CONTEXT_WC: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CONTEXT_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			vm_push_type( Struct *, exec->parser->pdaRun->context );
			break;
		}
		case IN_LOAD_CONTEXT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CONTEXT_BKT\n" );

			vm_push_type( Struct *, exec->parser->pdaRun->context );
			break;
		}

		/*
		 * LOAD_GLOBAL
		 */
		case IN_LOAD_GLOBAL_R: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_GLOBAL_R\n" );

			vm_push_val( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_WV: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_GLOBAL_WV\n" );

			vm_push_val( prg->global );

			/* Set up the reverse instruction. */
			rcodeUnitStart( exec );
			rcodeCode( exec, IN_LOAD_GLOBAL_BKT );
			break;
		}
		case IN_LOAD_GLOBAL_WC: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_GLOBAL_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			vm_push_val( prg->global );
			break;
		}
		case IN_LOAD_GLOBAL_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_GLOBAL_BKT\n" );

			vm_push_val( prg->global );
			break;
		}

		case IN_LOAD_PARSER_R: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_PARSER_R\n" );

			vm_push( (Tree*)exec->parser );
			assert( exec->parser != 0 );
			break;
		}
		case IN_LOAD_PARSER_WV: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_PARSER_WV\n" );

			vm_push( (Tree*)exec->parser );
			assert( exec->parser != 0 );

			/* Set up the reverse instruction. */
			rcodeUnitStart( exec );
			rcodeCode( exec, IN_LOAD_PARSER_BKT );
			rcodeWord( exec, (Word)exec->parser );
			break;
		}
		case IN_LOAD_PARSER_WC: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_PARSER_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			vm_push( (Tree*)exec->parser );
			assert( exec->parser != 0 );
			break;
		}
		case IN_LOAD_PARSER_BKT: {
			Tree *parser;
			read_tree( parser );

			debug( prg, REALM_BYTECODE, "IN_LOAD_PARSER_BKT\n" );

			vm_push( parser );
			break;
		}
		case IN_LOAD_INPUT_R: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_INPUT_R\n" );

			assert( exec->parser != 0 );
			vm_push( (Tree*)exec->parser->input );
			break;
		}
		case IN_LOAD_INPUT_WV: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_INPUT_WV\n" );

			assert( exec->parser != 0 );
			vm_push_stream( exec->parser->input );

			/* Set up the reverse instruction. */
			rcodeUnitStart( exec );
			rcodeCode( exec, IN_LOAD_INPUT_BKT );
			rcodeWord( exec, (Word)exec->parser->input );
			break;
		}
		case IN_LOAD_INPUT_WC: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_INPUT_WC\n" );

			assert( exec->parser != 0 );
			vm_push( (Tree*)exec->parser->input );
			break;
		}
		case IN_LOAD_INPUT_BKT: {
			Tree *accumStream;
			read_tree( accumStream );

			debug( prg, REALM_BYTECODE, "IN_LOAD_INPUT_BKT\n" );

			treeUpref( accumStream );
			vm_push( accumStream );
			break;
		}
		case IN_LOAD_CTX_R: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CTX_R\n" );

			vm_push_type( Struct *, exec->parser->pdaRun->context );
			break;
		}
		case IN_LOAD_CTX_WV: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CTX_WV\n" );

			vm_push_type( Struct*, exec->parser->pdaRun->context );

			/* Set up the reverse instruction. */
			rcodeUnitStart( exec );
			rcodeCode( exec, IN_LOAD_PARSER_BKT );
			rcodeWord( exec, (Word)exec->parser );
			break;
		}
		case IN_LOAD_CTX_WC: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CTX_WC\n" );

			/* This is identical to the _R version, but using it for writing
			 * would be confusing. */
			vm_push_type( Struct *, exec->parser->pdaRun->context );
			break;
		}
		case IN_LOAD_CTX_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LOAD_CTX_BKT\n" );

			vm_push_type( Struct *, exec->parser->pdaRun->context );
			break;
		}
		case IN_INIT_CAPTURES: {
			/* uchar ncaps; */
			consume_byte();

			debug( prg, REALM_BYTECODE, "IN_INIT_CAPTURES\n" );

			/* If there are captures (this is a translate block) then copy them into
			 * the local frame now. */
			LangElInfo *lelInfo = prg->rtd->lelInfo;
			char **mark = exec->parser->pdaRun->fsmRun->mark;

			int i;
			for ( i = 0; i < lelInfo[exec->parser->pdaRun->tokenId].numCaptureAttr; i++ ) {
				LangElInfo *lei = &lelInfo[exec->parser->pdaRun->tokenId];
				CaptureAttr *ca = &prg->rtd->captureAttr[lei->captureAttr + i];
				Head *data = stringAllocFull( prg, mark[ca->mark_enter],
						mark[ca->mark_leave] - mark[ca->mark_enter] );
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

			debug( prg, REALM_BYTECODE, "IN_INIT_RHS_EL %hd\n", field );

			Tree *val = getRhsEl( prg, exec->parser->pdaRun->redLel->shadow->tree, position );
			treeUpref( val );
			vm_local(field) = val;
			break;
		}

		case IN_INIT_LHS_EL: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_INIT_LHS_EL %hd\n", field );

			/* We transfer it to to the local field. Possibly take a copy. */
			Tree *val = exec->parser->pdaRun->redLel->shadow->tree;

			/* Save it. */
			treeUpref( val );
			exec->parser->pdaRun->parsed = val;

			exec->parser->pdaRun->redLel->shadow->tree = 0;
			vm_local(field) = val;
			break;
		}
		case IN_STORE_LHS_EL: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_STORE_LHS_EL %hd\n", field );

			Tree *val = vm_local(field);
			vm_local(field) = 0;
			exec->parser->pdaRun->redLel->shadow->tree = val;
			break;
		}
		case IN_UITER_ADVANCE: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_ADVANCE\n" );

			/* Get the iterator. */
			UserIter *uiter = (UserIter*) vm_local(field);

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

			UserIter *uiter = (UserIter*) vm_local(field);
			Tree *val = uiter->ref.kid->tree;
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_UITER_GET_CUR_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_GET_CUR_WC\n" );

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

			debug( prg, REALM_BYTECODE, "IN_UITER_SET_CUR_WC\n" );

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

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_R %hd\n", field );

			Tree *val = vm_local(field);
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_LOCAL_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_WC %hd\n", field );

			Tree *split = getLocalSplit( prg, exec->framePtr, field );
			treeUpref( split );
			vm_push( split );
			break;
		}
		case IN_SET_LOCAL_WC: {
			short field;
			read_half( field );
			debug( prg, REALM_BYTECODE, "IN_SET_LOCAL_WC %hd\n", field );

			Tree *val = vm_pop();
			treeDownref( prg, sp, vm_local(field) );
			setLocal( exec->framePtr, field, val );
			break;
		}
		case IN_GET_LOCAL_VAL_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_VAL_R %hd\n", field );

			Tree *val = vm_local(field);
			vm_push( val );
			break;
		}
		case IN_SET_LOCAL_VAL_WC: {
			short field;
			read_half( field );
			debug( prg, REALM_BYTECODE, "IN_SET_LOCAL_VAL_WC %hd\n", field );

			Tree *val = vm_pop();
			vm_local(field) = val;
			break;
		}
		case IN_SAVE_RET: {
			debug( prg, REALM_BYTECODE, "IN_SAVE_RET\n" );

			Tree *val = vm_pop();
			vm_local(FR_RV) = val;
			break;
		}
		case IN_GET_LOCAL_REF_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_REF_R\n" );

			Ref *ref = (Ref*) vm_plocal(field);
			Tree *val = ref->kid->tree;
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_LOCAL_REF_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LOCAL_REF_WC\n" );

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

			debug( prg, REALM_BYTECODE, "IN_SET_LOCAL_REF_WC\n" );

			Tree *val = vm_pop();
			Ref *ref = (Ref*) vm_plocal(field);
			splitRef( prg, &sp, ref );
			refSetValue( prg, sp, ref, val );
			break;
		}
		case IN_GET_FIELD_TREE_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_TREE_R %d\n", field );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = colm_tree_get_field( obj, field );
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_FIELD_TREE_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_TREE_WC %d\n", field );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
			vm_push( split );
			break;
		}
		case IN_GET_FIELD_TREE_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_TREE_WV\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
			vm_push( split );

			/* Set up the reverse instruction. */
			rcodeCode( exec, IN_GET_FIELD_TREE_BKT );
			rcodeHalf( exec, field );
			break;
		}
		case IN_GET_FIELD_TREE_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_TREE_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
			vm_push( split );
			break;
		}
		case IN_SET_FIELD_TREE_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_WC %d\n", field );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();
			treeDownref( prg, sp, obj );

			/* Downref the old value. */
			Tree *prev = colm_tree_get_field( obj, field );
			treeDownref( prg, sp, prev );

			colm_tree_set_field( prg, obj, field, val );
			break;
		}
		case IN_SET_FIELD_TREE_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_WV %d\n", field );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();
			treeDownref( prg, sp, obj );

			/* Save the old value, then set the field. */
			Tree *prev = colm_tree_get_field( obj, field );
			colm_tree_set_field( prg, obj, field, val );

			/* Set up the reverse instruction. */
			rcodeCode( exec, IN_SET_FIELD_TREE_BKT );
			rcodeHalf( exec, field );
			rcodeWord( exec, (Word)prev );
			rcodeUnitTerm( exec );
			break;
		}
		case IN_SET_FIELD_TREE_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			/* Downref the old value. */
			Tree *prev = colm_tree_get_field( obj, field );
			treeDownref( prg, sp, prev );

			colm_tree_set_field( prg, obj, field, val );
			break;
		}
		case IN_SET_FIELD_TREE_LEAVE_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_LEAVE_WC\n" );

			/* Note that we don't downref the object here because we are
			 * leaving it on the stack. */
			Tree *obj = vm_pop();
			Tree *val = vm_pop();

			/* Downref the old value. */
			Tree *prev = colm_tree_get_field( obj, field );
			treeDownref( prg, sp, prev );

			/* Set the field. */
			colm_tree_set_field( prg, obj, field, val );

			/* Leave the object on the top of the stack. */
			vm_push( obj );
			break;
		}
		case IN_GET_FIELD_VAL_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_FIELD_VAL_R %d\n", field );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *ptr = colm_tree_get_field( obj, field );
			Tree *dval = 0;
			if ( ptr )
				dval = getPtrVal( ptr );
			vm_push( dval );
			break;
		}
		case IN_SET_FIELD_VAL_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_VAL_WC %d\n", field );

			Tree *obj = vm_pop();
			Struct *strct = vm_pop_struct();
			treeDownref( prg, sp, obj );

			/* Downref the old value. */
			Tree *prev = colm_tree_get_field( obj, field );
			treeDownref( prg, sp, prev );

			/* Make it into a pointer. */
			Tree *pointer = 0;
			if ( strct != 0 ) {
				pointer = constructPointer( prg, (Tree*)strct );
				treeUpref( pointer );
			}

			colm_tree_set_field( prg, obj, field, pointer );
			break;
		}
		case IN_NEW_STRUCT: {
			short id;
			read_half( id );

			debug( prg, REALM_BYTECODE, "IN_NEW_STRUCT %hd\n", id );
			Struct *item = colm_struct_new( prg, id );
			vm_push_type( Struct*, item );
			break;
		}
		case IN_GET_STRUCT_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_R %d\n", field );

			Tree *obj = vm_pop();
			Tree *val = colm_struct_get_field( obj, Tree*, field );
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_STRUCT_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_WC %d\n", field );

			Tree *obj = vm_pop();
			Tree *val = colm_struct_get_field( obj, Tree*, field );
			treeUpref( val );
			vm_push( val );

			break;
		}
		case IN_GET_STRUCT_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_WV\n" );

			Tree *obj = vm_pop();
			Tree *val = colm_struct_get_field( obj, Tree*, field );
			treeUpref( val );
			vm_push( val );

			/* Set up the reverse instruction. */
			rcodeCode( exec, IN_GET_STRUCT_BKT );
			rcodeHalf( exec, field );
			break;
		}
		case IN_GET_STRUCT_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *split = getFieldSplit( prg, obj, field );
			treeUpref( split );
			vm_push( split );
			break;
		}
		case IN_SET_STRUCT_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_WC %d\n", field );

			Tree *obj = vm_pop();
			Tree *val = vm_pop();

			/* Downref the old value. */
			Tree *prev = colm_struct_get_field( obj, Tree*, field );
			treeDownref( prg, sp, prev );
			colm_struct_set_field( obj, Tree*, field, val );
			break;
		}
		case IN_SET_STRUCT_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_WV %d\n", field );

			Struct *obj = vm_pop_struct();
			Tree *val = vm_pop();

			/* Save the old value, then set the field. */
			Tree *prev = colm_struct_get_field( obj, Tree*, field );
			colm_struct_set_field( obj, Tree*, field, val );

			/* Set up the reverse instruction. */
			rcodeCode( exec, IN_SET_STRUCT_BKT );
			rcodeHalf( exec, field );
			rcodeWord( exec, (Word)prev );
			rcodeUnitTerm( exec );
			break;
		}
		case IN_SET_STRUCT_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_BKT\n" );

			Tree *obj = vm_pop();

			/* Downref the old value. */
			Tree *prev = colm_struct_get_field( obj, Tree*, field );
			treeDownref( prg, sp, prev );

			colm_struct_set_field( obj, Tree*, field, val );
			break;
		}
		case IN_GET_STRUCT_VAL_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_STRUCT_VAL_R %d\n", field );

			Tree *obj = vm_pop();
			Tree *val = colm_struct_get_field( obj, Tree*, field );
			vm_push( val );
			break;
		}
		case IN_SET_STRUCT_VAL_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_VAL_WC %d\n", field );

			Struct *strct = vm_pop_struct();
			Tree *val = vm_pop();

			colm_struct_set_field( strct, Tree*, field, val );
			break;
		}
		case IN_SET_STRUCT_VAL_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_VAL_WV %d\n", field );

			Struct *strct = vm_pop_struct();
			Tree *val = vm_pop();

			Tree *prev = colm_struct_get_field( strct, Tree*, field );
			colm_struct_set_field( strct, Tree*, field, val );

			rcodeCode( exec, IN_SET_STRUCT_VAL_BKT );
			rcodeHalf( exec, field );
			rcodeWord( exec, (Word)prev );
			rcodeUnitTerm( exec );
			break;
		}
		case IN_SET_STRUCT_VAL_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_VAL_BKT\n" );

			Tree *obj = vm_pop();

			colm_struct_set_field( obj, Tree*, field, val );
			break;
		}
		case IN_GET_RHS_VAL_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_RHS_VAL_R\n" );
			int i, done = 0;
			uchar len;

			Tree *obj = vm_pop(), *val = 0;
			treeDownref( prg, sp, obj );

			read_byte( len );
			for ( i = 0; i < len; i++ ) {
				uchar prodNum, childNum;
				read_byte( prodNum );
				read_byte( childNum );
				if ( !done && obj->prodNum == prodNum ) {
					val = getRhsEl( prg, obj, childNum );
					done = 1;
				}
			}

			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_POP: {
			debug( prg, REALM_BYTECODE, "IN_POP\n" );

			Tree *val = vm_pop();
			treeDownref( prg, sp, val );
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
		case IN_INT_TO_STR: {
			debug( prg, REALM_BYTECODE, "IN_INT_TO_STR\n" );

			Int *i = (Int*)vm_pop();
			Head *res = intToStr( prg, i->value );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			vm_push( str );
			treeDownref( prg, sp, (Tree*) i );
			break;
		}
		case IN_TREE_TO_STR: {
			debug( prg, REALM_BYTECODE, "IN_TREE_TO_STR\n" );

			Tree *tree = vm_pop();
			Head *res = treeToStr( prg, sp, tree, false );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			vm_push( str );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_TREE_TO_STR_TRIM: {
			debug( prg, REALM_BYTECODE, "IN_TREE_TO_STR_TRIM\n" );

			Tree *tree = vm_pop();
			Head *res = treeToStr( prg, sp, tree, true );
			Tree *str = constructString( prg, res );
			treeUpref( str );
			vm_push( str );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_TREE_TRIM: {
			debug( prg, REALM_BYTECODE, "IN_TREE_TRIM\n" );

			Tree *tree = vm_pop();
			Tree *trimmed = treeTrim( prg, sp, tree );
			vm_push( trimmed );
			break;
		}
		case IN_CONCAT_STR: {
			debug( prg, REALM_BYTECODE, "IN_CONCAT_STR\n" );

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

		case IN_STR_LENGTH: {
			debug( prg, REALM_BYTECODE, "IN_STR_LENGTH\n" );

			Str *str = (Str*)vm_pop();
			long len = stringLength( str->value );
			Tree *res = constructInteger( prg, len );
			treeUpref( res );
			vm_push( res );
			treeDownref( prg, sp, (Tree*)str );
			break;
		}
		case IN_JMP_FALSE_TREE: {
			short dist;
			read_half( dist );

			debug( prg, REALM_BYTECODE, "IN_JMP_FALSE_TREE %d\n", dist );

			Tree *tree = vm_pop();
			if ( testFalse( prg, tree ) )
				instr += dist;
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_JMP_TRUE_TREE: {
			short dist;
			read_half( dist );

			debug( prg, REALM_BYTECODE, "IN_JMP_TRUE_TREE %d\n", dist );

			Tree *tree = vm_pop();
			if ( !testFalse( prg, tree ) )
				instr += dist;
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_JMP_FALSE_VAL: {
			short dist;
			read_half( dist );

			debug( prg, REALM_BYTECODE, "IN_JMP_FALSE_VAL %d\n", dist );

			Tree *tree = vm_pop();
			if ( tree == 0 )
				instr += dist;
			break;
		}
		case IN_JMP_TRUE_VAL: {
			short dist;
			read_half( dist );

			debug( prg, REALM_BYTECODE, "IN_JMP_TRUE_VAL %d\n", dist );

			Tree *tree = vm_pop();
			if ( !tree != 0 )
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
		case IN_TST_EQL_VAL: {
			debug( prg, REALM_BYTECODE, "IN_TST_EQL_VAL\n" );

			Tree *o2 = vm_pop();
			Tree *o1 = vm_pop();
			Tree *val = o1 == o2 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_TST_NOT_EQL_TREE: {
			debug( prg, REALM_BYTECODE, "IN_TST_NOT_EQL_TREE\n" );

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
		case IN_TST_NOT_EQL_VAL: {
			debug( prg, REALM_BYTECODE, "IN_TST_NOT_EQL_VAL\n" );

			Tree *o2 = vm_pop();
			Tree *o1 = vm_pop();
			Tree *val = o1 != o2 ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_TST_LESS: {
			debug( prg, REALM_BYTECODE, "IN_TST_LESS\n" );

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
			debug( prg, REALM_BYTECODE, "IN_TST_LESS_EQL\n" );

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
			debug( prg, REALM_BYTECODE, "IN_TST_GRTR\n" );

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
			debug( prg, REALM_BYTECODE, "IN_TST_GRTR_EQL\n" );

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
			debug( prg, REALM_BYTECODE, "IN_TST_LOGICAL_AND\n" );

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
			debug( prg, REALM_BYTECODE, "IN_TST_LOGICAL_OR\n" );

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
			debug( prg, REALM_BYTECODE, "IN_NOT\n" );

			Tree *tree = (Tree*)vm_pop();
			long r = testFalse( prg, tree );
			Tree *val = r ? prg->trueVal : prg->falseVal;
			treeUpref( val );
			vm_push( val );
			treeDownref( prg, sp, tree );
			break;
		}

		case IN_ADD_INT: {
			debug( prg, REALM_BYTECODE, "IN_ADD_INT\n" );

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
			debug( prg, REALM_BYTECODE, "IN_MULT_INT\n" );

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
			debug( prg, REALM_BYTECODE, "IN_DIV_INT\n" );

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
			debug( prg, REALM_BYTECODE, "IN_SUB_INT\n" );

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
		case IN_TOP_SWAP: {
			debug( prg, REALM_BYTECODE, "IN_TOP_SWAP\n" );

			Tree *v1 = vm_pop();
			Tree *v2 = vm_pop();
			vm_push( v1 );
			vm_push( v2 );
			break;
		}
		case IN_DUP_VAL: {
			debug( prg, REALM_BYTECODE, "IN_DUP_VAL\n" );

			Word val = (Word)vm_top();
			vm_push_type( Word, val );
			break;
		}
		case IN_DUP_TREE: {
			debug( prg, REALM_BYTECODE, "IN_DUP_TREE\n" );

			Tree *val = vm_top();
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_TRITER_FROM_REF: {
			short field;
			Half argSize;
			Half searchTypeId;
			read_half( field );
			read_half( argSize );
			read_half( searchTypeId );

			debug( prg, REALM_BYTECODE, "IN_TRITER_FROM_REF "
					"%hd %hd %hd\n", field, argSize, searchTypeId );

			Ref rootRef;
			rootRef.kid = (Kid*)vm_pop();
			rootRef.next = (Ref*)vm_pop();
			void *mem = vm_plocal(field);

			Tree **stackRoot = vm_ptop();
			long rootSize = vm_ssize();

			initTreeIter( (TreeIter*)mem, stackRoot,
					argSize, rootSize, &rootRef, searchTypeId );
			break;
		}
		case IN_TRITER_DESTROY: {
			short field;
			read_half( field );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			debug( prg, REALM_BYTECODE, "IN_TRITER_DESTROY %d\n", iter->yieldSize );
			treeIterDestroy( prg, &sp, iter );
			break;
		}
		case IN_REV_TRITER_FROM_REF: {
			short field;
			Half argSize;
			Half searchTypeId;
			read_half( field );
			read_half( argSize );
			read_half( searchTypeId );

			debug( prg, REALM_BYTECODE, "IN_REV_TRITER_FROM_REF "
					"%hd %hd %hd\n", field, argSize, searchTypeId );

			Ref rootRef;
			rootRef.kid = (Kid*)vm_pop();
			rootRef.next = (Ref*)vm_pop();

			Tree **stackRoot = vm_ptop();
			long rootSize = vm_ssize();
			
			int children = 0;
			Kid *kid = treeChild( prg, rootRef.kid->tree );
			while ( kid != 0 ) {
				vm_push( (SW)kid );
				kid = kid->next;
				children++;
			}

			void *mem = vm_plocal(field);
			initRevTreeIter( (RevTreeIter*)mem, stackRoot,
					argSize, rootSize, &rootRef, searchTypeId, children );
			break;
		}
		case IN_REV_TRITER_DESTROY: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REV_TRITER_DESTROY\n" );

			RevTreeIter *iter = (RevTreeIter*) vm_plocal(field);
			revTreeIterDestroy( prg, &sp, iter );
			break;
		}
		case IN_TREE_SEARCH: {
			Word id;
			read_word( id );

			debug( prg, REALM_BYTECODE, "IN_TREE_SEARCH\n" );

			Tree *tree = vm_pop();
			Tree *res = treeSearch( prg, tree, id );
			treeUpref( res );
			vm_push( res );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_TRITER_ADVANCE: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_ADVANCE\n" );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *res = treeIterAdvance( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_TRITER_NEXT_CHILD: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_NEXT_CHILD\n" );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *res = treeIterNextChild( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_REV_TRITER_PREV_CHILD: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REV_TRITER_PREV_CHILD\n" );

			RevTreeIter *iter = (RevTreeIter*) vm_plocal(field);
			Tree *res = treeRevIterPrevChild( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_TRITER_NEXT_REPEAT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_NEXT_REPEAT\n" );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *res = treeIterNextRepeat( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_TRITER_PREV_REPEAT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_PREV_REPEAT\n" );

			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *res = treeIterPrevRepeat( prg, &sp, iter );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_TRITER_GET_CUR_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_GET_CUR_R\n" );
			
			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Tree *tree = treeIterDerefCur( iter );
			treeUpref( tree );
			vm_push( tree );
			break;
		}
		case IN_TRITER_GET_CUR_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_GET_CUR_WC\n" );
			
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

			debug( prg, REALM_BYTECODE, "IN_TRITER_SET_CUR_WC\n" );

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

			debug( prg, REALM_BYTECODE, "IN_MATCH\n" );

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

		case IN_GET_PARSER_CTX_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_PARSER_CTX_R\n" );

			Parser *parser = vm_pop_parser();
			Struct *ctx = parser->pdaRun->context;
			vm_push_type( Struct *, ctx );
			break;
		}

		case IN_SET_PARSER_CTX_WC: {
			debug( prg, REALM_BYTECODE, "IN_SET_PARSER_CTX_WC\n" );

			Parser *parser = vm_pop_parser();
			Struct *strct = vm_pop_struct();
			colm_parser_set_context( prg, sp, parser, strct );
			break;
		}

//		case IN_GET_PARSER_CTX_WC:
//		case IN_GET_PARSER_CTX_WV:
//		case IN_SET_PARSER_CTX_WC:
//		case IN_SET_PARSER_CTX_WV:
//			break;

		case IN_INPUT_APPEND_WC: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_APPEND_WC\n" );

			Pointer *sptr = (Pointer*) vm_pop();
			Tree *input = vm_pop();

			StreamImpl *si = streamToImpl( (Stream*)sptr );
			streamAppend( prg, sp, input, si );

			//vm_push( (Tree*)sptr );
			treeDownref( prg, sp, input );
			break;
		}
		case IN_INPUT_APPEND_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_APPEND_WV\n" );

			Pointer *sptr = (Pointer*) vm_pop();
			Tree *input = vm_pop();

			StreamImpl *si = streamToImpl( (Stream*)sptr );
			Word len = streamAppend( prg, sp, input, si );

//			treeUpref( (Tree*)sptr );
//			vm_push( (Tree*)sptr );

			rcodeUnitStart( exec );
			rcodeCode( exec, IN_INPUT_APPEND_BKT );
			rcodeWord( exec, (Word) sptr );
			rcodeWord( exec, (Word) input );
			rcodeWord( exec, (Word) len );
			rcodeUnitTerm( exec );
			break;
		}

		case IN_INPUT_APPEND_BKT: {
			Tree *sptr;
			Tree *input;
			Word len;
			read_tree( sptr );
			read_tree( input );
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_INPUT_APPEND_BKT\n" );

			StreamImpl *si = streamToImpl( (Stream*)sptr );
			undoStreamAppend( prg, sp, si, input, len );

			treeDownref( prg, sp, sptr );
			treeDownref( prg, sp, input );
			break;
		}

		case IN_INPUT_APPEND_STREAM_WC: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_APPEND_STREAM_WC\n" );

			Stream *sptr = vm_pop_stream();
			Tree *input = vm_pop();

			StreamImpl *si = streamToImpl( sptr );
			streamAppendStream( prg, sp, si, input );
			break;
		}
		case IN_INPUT_APPEND_STREAM_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_APPEND_STREAM_WV\n" );

			Stream *stream = vm_pop_stream();
			Tree *input = vm_pop();

			StreamImpl *si = streamToImpl( stream );
			Word len = streamAppendStream( prg, sp, si, input );

			rcodeUnitStart( exec );
			rcodeCode( exec, IN_INPUT_APPEND_STREAM_BKT );
			rcodeWord( exec, (Word) stream );
			rcodeWord( exec, (Word) input );
			rcodeWord( exec, (Word) len );
			rcodeUnitTerm( exec );
			break;
		}

		case IN_INPUT_APPEND_STREAM_BKT: {
			Tree *sptr;
			Tree *input;
			Word len;
			read_tree( sptr );
			read_tree( input );
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_INPUT_APPEND_STREAM_BKT\n" );

			StreamImpl *si = streamToImpl( (Stream*)sptr );
			undoStreamAppendStream( prg, sp, si, input, len );

			treeDownref( prg, sp, sptr );
			treeDownref( prg, sp, input );
			break;
		}

		case IN_INPUT_CLOSE_WC: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_CLOSE_WC\n" );

			Stream *stream = (Stream*) vm_pop();
			StreamImpl *si = stream->impl;

			if ( si->file != 0 ) {
				fclose( si->file );
				si->file = 0;
			}
			else if ( si->fd >= 0 ) {
				close( si->fd );
				si->fd = -1;
			}

			vm_push( (Tree*)stream );
			break;
		}

		case IN_SET_ERROR: {
			debug( prg, REALM_BYTECODE, "IN_SET_ERROR\n" );

			Tree *error = vm_pop();
			treeDownref( prg, sp, prg->error );
			prg->error = error;
			break;
		}

		case IN_GET_ERROR: {
			debug( prg, REALM_BYTECODE, "IN_GET_ERROR\n" );

			Tree *obj = vm_pop();
			treeUpref( (Tree*)prg->error );
			vm_push( (Tree*)prg->error );
			break;
		}

		case IN_PARSE_SAVE_STEPS: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_SAVE_STEPS\n" );

			Parser *parser = (Parser*)vm_pop();
			PdaRun *pdaRun = parser->pdaRun;
			long steps = pdaRun->steps;

			vm_push( (SW)exec->parser );
			vm_push( (SW)exec->pcr );
			vm_push( (SW)exec->steps );

			exec->parser = (Parser*)parser;
			exec->steps = steps;
			exec->pcr = PcrStart;
			break;
		}

		case IN_PARSE_INIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_INIT_BKT\n" );

			Tree *parser;
			Word pcr;
			Word steps;

			read_tree( parser );
			read_word( pcr );
			read_word( steps );

			vm_push( (SW)exec->parser );
			vm_push( (SW)exec->pcr );
			vm_push( (SW)exec->steps );

			exec->parser = (Parser*)parser;
			exec->steps = steps;
			exec->pcr = pcr;
			break;
		}

		case IN_PCR_CALL: {
			debug( prg, REALM_BYTECODE, "IN_PCR_CALL\n" );

			if ( exec->parser->pdaRun->frameId >= 0 )  {
				FrameInfo *fi = &prg->rtd->frameInfo[exec->parser->pdaRun->frameId];
				long stretch = fi->argSize + 4 + fi->frameSize;
				vm_contiguous( stretch );
			}

			vm_push( (SW)exec->framePtr );
			vm_push( (SW)exec->iframePtr );
			vm_push( (SW)exec->frameId );

			/* Return location one instruction back. Depends on the size of of
			 * the frag/finish. */
			Code *returnTo = instr - ( SIZEOF_CODE + SIZEOF_CODE + SIZEOF_HALF );
			vm_push( (SW)returnTo );

			exec->framePtr = 0;
			exec->iframePtr = 0;
			exec->frameId = 0;

			exec->frameId = exec->parser->pdaRun->frameId;

			instr = exec->parser->pdaRun->code;
			break;
		}

		case IN_PCR_RET: {
			debug( prg, REALM_BYTECODE, "IN_PCR_RET\n" );

			if ( exec->frameId >= 0 ) {
				FrameInfo *fi = &prg->rtd->frameInfo[exec->frameId];
				downref_local_trees( prg, sp, exec->framePtr, fi->locals, fi->localsLen );
				debug( prg, REALM_BYTECODE, "RET: %d\n", fi->frameSize );

				vm_popn( fi->frameSize );
			}

			instr = (Code*) vm_pop();
			exec->frameId = ( long ) vm_pop();
			exec->iframePtr = ( Tree ** ) vm_pop();
			exec->framePtr = ( Tree ** ) vm_pop();

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
			Half stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_WC %hd\n", stopId );

			exec->pcr = parseFrag( prg, sp, exec->parser->pdaRun,
					exec->parser->input, stopId, exec->pcr );

			/* If done, jump to the terminating instruction, otherwise fall
			 * through to call some code, then jump back here. */
			if ( exec->pcr == PcrDone )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FRAG_EXIT_WC: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_EXIT_WC\n" );

			Parser *parser = exec->parser;

			exec->steps = (long)vm_pop();
			exec->pcr = (long)vm_pop();
			exec->parser = (Parser*) vm_pop();

			if ( prg->induceExit )
				goto out;

			break;
		}

		case IN_PARSE_FRAG_WV: {
			Half stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_WV %hd\n", stopId );

			exec->pcr = parseFrag( prg, sp, exec->parser->pdaRun,
					exec->parser->input, stopId, exec->pcr );

			/* If done, jump to the terminating instruction, otherwise fall
			 * through to call some code, then jump back here. */
			if ( exec->pcr == PcrDone )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FRAG_EXIT_WV: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_EXIT_WV \n" );

			Parser *parser = exec->parser;
			long steps = exec->steps;

			exec->steps = (long)vm_pop();
			exec->pcr = (long)vm_pop();
			exec->parser = (Parser*)vm_pop();

			rcodeUnitStart( exec );
			rcodeCode( exec, IN_PARSE_INIT_BKT );
			rcodeWord( exec, (Word)parser );
			rcodeWord( exec, (Word)PcrStart );
			rcodeWord( exec, steps );
			rcodeCode( exec, IN_PARSE_FRAG_BKT );
			rcodeHalf( exec, 0 );
			rcodeCode( exec, IN_PCR_CALL );
			rcodeCode( exec, IN_PARSE_FRAG_EXIT_BKT );
			rcodeUnitTerm( exec );

			if ( prg->induceExit )
				goto out;
			break;
		}

		case IN_PARSE_FRAG_BKT: {
			Half stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_BKT %hd\n", stopId );

			exec->pcr = undoParseFrag( prg, sp, exec->parser->pdaRun,
					exec->parser->input, exec->steps, exec->pcr );

			if ( exec->pcr == PcrDone )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FRAG_EXIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_EXIT_BKT\n" );

			Parser *parser = exec->parser;

			exec->steps = (long)vm_pop();
			exec->pcr = (long)vm_pop();
			exec->parser = (Parser*)vm_pop();

			break;
		}

		case IN_PARSE_FINISH_WC: {
			Half stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_WC %hd\n", stopId );

			Tree *result = 0;
			exec->pcr = parseFinish( &result, prg, sp,
					exec->parser->pdaRun, exec->parser->input, false, exec->pcr );

			exec->parser->result = result;

			/* If done, jump to the terminating instruction, otherwise fall
			 * through to call some code, then jump back here. */
			if ( exec->pcr == PcrDone )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FINISH_EXIT_WC: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_EXIT_WC\n" );

			Parser *parser = exec->parser;

			exec->steps = (long)vm_pop();
			exec->pcr = (long)vm_pop();
			exec->parser = (Parser*)vm_pop();

			vm_push( parser->result );
			if ( prg->induceExit )
				goto out;

			break;
		}

		case IN_PARSE_FINISH_WV: {
			Half stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_WV %hd\n", stopId );

			Tree *result = 0;
			exec->pcr = parseFinish( &result, prg, sp, exec->parser->pdaRun,
					exec->parser->input, true, exec->pcr );

			exec->parser->result = result;

			if ( exec->pcr == PcrDone )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FINISH_EXIT_WV: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_EXIT_WV\n" );

			Parser *parser = exec->parser;
			long steps = exec->steps;

			exec->steps = (long)vm_pop();
			exec->pcr = (long)vm_pop();
			exec->parser = (Parser *) vm_pop();

			vm_push( parser->result );

			rcodeUnitStart( exec );
			rcodeCode( exec, IN_PARSE_INIT_BKT );
			rcodeWord( exec, (Word)parser );
			rcodeWord( exec, (Word)PcrStart );
			rcodeWord( exec, steps );
			rcodeCode( exec, IN_PARSE_FINISH_BKT );
			rcodeHalf( exec, 0 );
			rcodeCode( exec, IN_PCR_CALL );
			rcodeCode( exec, IN_PARSE_FINISH_EXIT_BKT );
			rcodeUnitTerm( exec );

			if ( prg->induceExit )
				goto out;

			break;
		}

		case IN_PARSE_FINISH_BKT: {
			Half stopId;
			read_half( stopId );

			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_BKT %hd\n", stopId );

			exec->pcr = undoParseFrag( prg, sp, exec->parser->pdaRun,
					exec->parser->input, exec->steps, exec->pcr );

			if ( exec->pcr == PcrDone )
				instr += SIZEOF_CODE;
			break;
		}

		case IN_PARSE_FINISH_EXIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FINISH_EXIT_BKT\n" );

			Parser *parser = exec->parser;

			exec->steps = (long)vm_pop();
			exec->pcr = (long)vm_pop();
			exec->parser = (Parser*)vm_pop();

			StreamImpl *si = streamToImpl( parser->input );
			si->funcs->unsetEof( si );
			break;
		}

		case IN_INPUT_PULL_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PULL_WV\n" );

			Stream *obj = vm_pop_stream();
			Tree *len = vm_pop();
			PdaRun *pdaRun = exec->parser != 0 ? exec->parser->pdaRun : 0;
			Tree *string = streamPullBc( prg, sp, pdaRun, streamToImpl( obj ), len );
			treeUpref( string );
			vm_push( string );

			/* Single unit. */
			treeUpref( string );
			rcodeCode( exec, IN_INPUT_PULL_BKT );
			rcodeWord( exec, (Word) string );
			rcodeUnitTerm( exec );

			treeDownref( prg, sp, len );
			break;
		}

		case IN_INPUT_PULL_WC: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PULL_WC\n" );

			Stream *obj = (Stream*)vm_pop();
			Tree *len = vm_pop();
			PdaRun *pdaRun = exec->parser != 0 ? exec->parser->pdaRun : 0;
			Tree *string = streamPullBc( prg, sp, pdaRun, streamToImpl( obj ), len );
			treeUpref( string );
			vm_push( string );

			treeDownref( prg, sp, len );
			break;
		}
		case IN_INPUT_PULL_BKT: {
			Tree *string;
			read_tree( string );

			Stream *obj = vm_pop_stream();

			debug( prg, REALM_BYTECODE, "IN_INPUT_PULL_BKT\n" );

			undoPull( prg, streamToImpl( obj ), string );
			treeDownref( prg, sp, string );
			break;
		}
		case IN_INPUT_PUSH_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_WV\n" );

			Stream *input = vm_pop_stream();
			Tree *tree = vm_pop();
			long len = stream_push( prg, sp, streamToImpl( input ), tree, false );
			vm_push( 0 );

			/* Single unit. */
			rcodeCode( exec, IN_INPUT_PUSH_BKT );
			rcodeWord( exec, len );
			rcodeUnitTerm( exec );

			treeDownref( prg, sp, tree );
			break;
		}
		case IN_INPUT_PUSH_IGNORE_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_IGNORE_WV\n" );

			Stream *input = (Stream*)vm_pop();
			Tree *tree = vm_pop();
			long len = stream_push( prg, sp, streamToImpl( input ), tree, true );
			vm_push( 0 );

			/* Single unit. */
			rcodeCode( exec, IN_INPUT_PUSH_BKT );
			rcodeWord( exec, len );
			rcodeUnitTerm( exec );

			treeDownref( prg, sp, tree );
			break;
		}
		case IN_INPUT_PUSH_BKT: {
			Word len;
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_BKT %d\n", len );

			Stream *input = vm_pop_stream();
			undoStreamPush( prg, sp, streamToImpl( input ), len );
			break;
		}
		case IN_INPUT_PUSH_STREAM_WV: {
			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_STREAM_WV\n" );

			Stream *input = vm_pop_stream();
			Stream *toPush = vm_pop_stream();
			long len = stream_push_stream( prg, sp, streamToImpl( input ), toPush );
			vm_push( 0 );

			/* Single unit. */
			rcodeCode( exec, IN_INPUT_PUSH_BKT );
			rcodeWord( exec, len );
			rcodeUnitTerm( exec );
			break;
		}
		case IN_INPUT_PUSH_STREAM_BKT: {
			Word len;
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_INPUT_PUSH_STREAM_BKT %d\n", len );

			Stream *input = vm_pop_stream();
			undoStreamPush( prg, sp, streamToImpl( input ), len );
			break;
		}
		case IN_CONS_GENERIC: {
			Half genericId;
			read_half( genericId );

			debug( prg, REALM_BYTECODE, "IN_CONS_GENERIC %hd\n", genericId );

			Tree *replTree = constructGeneric( prg, genericId );
			vm_push( replTree );
			break;
		}
		case IN_CONS_OBJECT: {
			Half langElId;
			read_half( langElId );

			debug( prg, REALM_BYTECODE, "IN_CONS_OBJECT %hd\n", langElId );

			Tree *replTree = constructObject( prg, 0, 0, langElId );
			vm_push( replTree );
			break;
		}
		case IN_CONSTRUCT: {
			Half patternId;
			read_half( patternId );

			debug( prg, REALM_BYTECODE, "IN_CONSTRUCT\n" );

			LangElInfo *lelInfo = prg->rtd->lelInfo;
			PatConsNode *nodes = prg->rtd->patReplNodes;
			int rootNode = prg->rtd->patReplInfo[patternId].offset;

			/* Note that bindIds are indexed at one. Add one spot for them. */
			int numBindings = prg->rtd->patReplInfo[patternId].numBindings;
			Tree *bindings[1+numBindings];

			int b;
			for ( b = 1; b <= numBindings; b++ ) {
				bindings[b] = vm_pop();
				assert( bindings[b] != 0 );
			}

			Tree *replTree = constructTree( prg, 0, bindings, rootNode );

			vm_push( replTree );
			break;
		}
		case IN_CONSTRUCT_INPUT: {
			debug( prg, REALM_BYTECODE, "IN_CONSTRUCT_INPUT\n" );

			Tree *input = (Tree*)colm_stream_new( prg );
			vm_push( input );
			break;
		}
		case IN_GET_INPUT: {
			debug( prg, REALM_BYTECODE, "IN_GET_INPUT\n" );

			Parser *parser = vm_pop_parser();
			Stream *stream = parser->input;
			vm_push_stream( stream );
			break;
		}
		case IN_SET_INPUT: {
			debug( prg, REALM_BYTECODE, "IN_SET_INPUT\n" );

			Parser *parser = vm_pop_parser();
			Stream *stream = vm_pop_stream();
			parser->input = stream;
			break;
		}
		case IN_CONSTRUCT_TERM: {
			Half tokenId;
			read_half( tokenId );

			debug( prg, REALM_BYTECODE, "IN_CONSTRUCT_TERM\n" );

			/* Pop the string we are constructing the token from. */
			Str *str = (Str*)vm_pop();
			Tree *res = constructTerm( prg, tokenId, str->value );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_MAKE_TOKEN: {
			uchar nargs;
			int i;
			read_byte( nargs );

			debug( prg, REALM_BYTECODE, "IN_MAKE_TOKEN\n" );

			Tree *arg[nargs];
			for ( i = nargs-1; i >= 0; i-- )
				arg[i] = vm_pop();

			Tree *result = constructToken( prg, arg, nargs );
			for ( i = 0; i < nargs; i++ )
				treeDownref( prg, sp, arg[i] );
			vm_push( result );
			break;
		}
		case IN_MAKE_TREE: {
			uchar nargs;
			int i;
			read_byte( nargs );

			debug( prg, REALM_BYTECODE, "IN_MAKE_TREE\n" );

			Tree *arg[nargs];
			for ( i = nargs-1; i >= 0; i-- )
				arg[i] = vm_pop();

			Tree *result = makeTree( prg, arg, nargs );
			for ( i = 0; i < nargs; i++ )
				treeDownref( prg, sp, arg[i] );

			vm_push( result );
			break;
		}
		case IN_TREE_CAST: {
			Half langElId;
			read_half( langElId );

			debug( prg, REALM_BYTECODE, "IN_TREE_CAST %hd\n", langElId );

			Tree *tree = vm_pop();
			Tree *res = castTree( prg, langElId, tree );
			treeUpref( res );
			treeDownref( prg, sp, tree );
			vm_push( res );
			break;
		}
		case IN_TREE_NEW: {
			debug( prg, REALM_BYTECODE, "IN_TREE_NEW \n" );

			Tree *tree = vm_pop();
			Tree *res = constructPointer( prg, tree );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_PTR_ACCESS_WV: {
			debug( prg, REALM_BYTECODE, "IN_PTR_ACCESS_WV\n" );

			Struct *ptr = vm_pop_struct();
			vm_push_struct( ptr );

			/* This is an initial global load. Need to reverse execute it. */
			rcodeUnitStart( exec );
			rcodeCode( exec, IN_PTR_ACCESS_BKT );
			rcodeWord( exec, (Word) ptr );
			break;
		}
		case IN_PTR_ACCESS_BKT: {
			Word p;
			read_word( p );

			debug( prg, REALM_BYTECODE, "IN_PTR_ACCESS_BKT\n" );

			Struct *ptr = (Struct*)p;
			vm_push_type( Struct *, ptr );
			break;
		}
		case IN_REF_FROM_LOCAL: {
			short int field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REF_FROM_LOCAL %hd\n", field );

			/* First push the null next pointer, then the kid pointer. */
			Tree **ptr = vm_plocal(field);
			vm_contiguous( 2 );
			vm_push( 0 );
			vm_push( (SW)ptr );
			break;
		}
		case IN_REF_FROM_REF: {
			short int field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REF_FROM_REF\n" );

			Ref *ref = (Ref*)vm_plocal(field);
			vm_contiguous( 2 );
			vm_push( (SW)ref );
			vm_push( (SW)ref->kid );
			break;
		}
		case IN_REF_FROM_QUAL_REF: {
			short int back;
			short int field;
			read_half( back );
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_REF_FROM_QUAL_REF\n" );

			Ref *ref = (Ref*)(sp + back);

			Tree *obj = ref->kid->tree;
			Kid *attr_kid = getFieldKid( obj, field );

			vm_contiguous( 2 );
			vm_push( (SW)ref );
			vm_push( (SW)attr_kid );
			break;
		}
		case IN_RHS_REF_FROM_QUAL_REF: {
			short int back;
			int i, done = 0;
			uchar len;

			read_half( back );

			debug( prg, REALM_BYTECODE, "IN_RHS_REF_FROM_QUAL_REF\n" );

			Ref *ref = (Ref*)(sp + back);

			Tree *obj = ref->kid->tree;
			Kid *attrKid = 0;

			read_byte( len );
			for ( i = 0; i < len; i++ ) {
				uchar prodNum, childNum;
				read_byte( prodNum );
				read_byte( childNum );
				if ( !done && obj->prodNum == prodNum ) {
					attrKid = getRhsElKid( prg, obj, childNum );
					done = 1;
				}
			}

			vm_contiguous( 2 );
			vm_push( (SW)ref );
			vm_push( (SW)attrKid );
			break;
		}
		case IN_REF_FROM_BACK: {
			short int back;
			read_half( back );

			debug( prg, REALM_BYTECODE, "IN_REF_FROM_BACK %hd\n", back );

			Tree **ptr = (Tree**)(sp + back);

			vm_contiguous( 2 );
			vm_push( 0 );
			vm_push( (SW)ptr );
			break;
		}
		case IN_TRITER_REF_FROM_CUR: {
			short int field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_TRITER_REF_FROM_CUR\n" );

			/* Push the next pointer first, then the kid. */
			TreeIter *iter = (TreeIter*) vm_plocal(field);
			Ref *ref = &iter->ref;
			vm_contiguous( 2 );
			vm_push( (SW)ref );
			vm_push( (SW)iter->ref.kid );
			break;
		}
		case IN_UITER_REF_FROM_CUR: {
			short int field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_UITER_REF_FROM_CUR\n" );

			/* Push the next pointer first, then the kid. */
			UserIter *uiter = (UserIter*) vm_local(field);
			vm_contiguous( 2 );
			vm_push( (SW)uiter->ref.next );
			vm_push( (SW)uiter->ref.kid );
			break;
		}
		case IN_GET_TOKEN_DATA_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_TOKEN_DATA_R\n" );

			Tree *tree = (Tree*) vm_pop();
			Head *data = stringCopy( prg, tree->tokdata );
			Tree *str = constructString( prg, data );
			treeUpref( str );
			vm_push( str );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_SET_TOKEN_DATA_WC: {
			debug( prg, REALM_BYTECODE, "IN_SET_TOKEN_DATA_WC\n" );

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
			debug( prg, REALM_BYTECODE, "IN_SET_TOKEN_DATA_WV\n" );

			Tree *tree = vm_pop();
			Tree *val = vm_pop();

			Head *oldval = tree->tokdata;
			Head *head = stringCopy( prg, ((Str*)val)->value );
			tree->tokdata = head;

			/* Set up reverse code. Needs no args. */
			rcodeCode( exec, IN_SET_TOKEN_DATA_BKT );
			rcodeWord( exec, (Word)oldval );
			rcodeUnitTerm( exec );

			treeDownref( prg, sp, tree );
			treeDownref( prg, sp, val );
			break;
		}
		case IN_SET_TOKEN_DATA_BKT: {
			debug( prg, REALM_BYTECODE, "IN_SET_TOKEN_DATA_BKT \n" );

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
			debug( prg, REALM_BYTECODE, "IN_GET_TOKEN_POS_R\n" );

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
		case IN_GET_TOKEN_LINE_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_TOKEN_LINE_R\n" );

			Tree *tree = (Tree*) vm_pop();
			Tree *integer = 0;
			if ( tree->tokdata->location ) {
				integer = constructInteger( prg, tree->tokdata->location->line );
				treeUpref( integer );
			}
			vm_push( integer );
			treeDownref( prg, sp, tree );
			break;
		}
		case IN_GET_MATCH_LENGTH_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_MATCH_LENGTH_R\n" );

			Tree *integer = constructInteger( prg, 
					stringLength(exec->parser->pdaRun->tokdata) );
			treeUpref( integer );
			vm_push( integer );
			break;
		}
		case IN_GET_MATCH_TEXT_R: {
			debug( prg, REALM_BYTECODE, "IN_GET_MATCH_TEXT_R\n" );

			Head *s = stringCopy( prg, exec->parser->pdaRun->tokdata );
			Tree *tree = constructString( prg, s );
			treeUpref( tree );
			vm_push( tree );
			break;
		}
		case IN_LIST_LENGTH: {
			debug( prg, REALM_BYTECODE, "IN_LIST_LENGTH\n" );

			List *list = vm_pop_list();
			long len = colm_list_length( list );
			Tree *res = constructInteger( prg, len );
			treeDownref( prg, sp, (Tree*)list );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_LIST_PUSH_HEAD_WC: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_HEAD_WC\n" );

			List *list = vm_pop_list();
			Struct *s = vm_pop_struct();

			ListEl *listEl = colm_struct_to_list_el( prg, s, genId );
			colm_list_prepend( list, listEl );

			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );
			break;
		}
		case IN_LIST_PUSH_HEAD_WV: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_HEAD_WV\n" );

			List *list = vm_pop_list();
			Struct *s = vm_pop_struct();

			ListEl *listEl = colm_struct_to_list_el( prg, s, genId );
			colm_list_prepend( list, listEl );

			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );

			/* Set up reverse code. Needs no args. */
			rcodeCode( exec, IN_LIST_PUSH_HEAD_BKT );
			rcodeUnitTerm( exec );
			break;
		}
		case IN_LIST_PUSH_HEAD_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_HEAD_BKT\n" );

			List *list = vm_pop_list();
			colm_list_detach_head( list );
			break;
		}
		case IN_LIST_PUSH_TAIL_WC: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_TAIL_WC\n" );

			List *list = vm_pop_list();
			Struct *s = vm_pop_struct();

			ListEl *listEl = colm_struct_to_list_el( prg, s, genId );
			colm_list_append( list, listEl );

			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );
			break;
		}
		case IN_LIST_PUSH_TAIL_WV: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_TAIL_WV\n" );

			List *list = vm_pop_list();
			Struct *s = vm_pop_struct();

			ListEl *listEl = colm_struct_to_list_el( prg, s, genId );
			colm_list_append( list, listEl );

			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );

			/* Set up reverse code. Needs no args. */
			rcodeCode( exec, IN_LIST_PUSH_TAIL_BKT );
			rcodeUnitTerm( exec );
			break;
		}
		case IN_LIST_PUSH_TAIL_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_TAIL_BKT\n" );

			List *list = vm_pop_list();
			colm_list_detach_tail( list );
			break;
		}
		case IN_GET_LIST_EL_MEM_R: {
			short genId, field;
			read_half( genId );
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_EL_MEM_R\n" );

			Struct *s = vm_pop_struct();

			ListEl *listEl = colm_struct_to_list_el( prg, s, genId );
			Struct *val = colm_list_el_get( prg, listEl, genId, field );
			vm_push_struct( val );
			break;
		}
		case IN_LIST_POP_TAIL_WC: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_LIST_POP_TAIL_WC\n" );

			List *list = vm_pop_list();

			ListEl *tail = list->tail;
			colm_list_detach_tail( list );
			Struct *s = colm_generic_el_container( prg, tail, genId );

			vm_push_struct( s );
			break;
		}
		case IN_LIST_POP_TAIL_WV: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_LIST_POP_TAIL_WV\n" );

			List *list = vm_pop_list();

			ListEl *tail = list->tail;
			colm_list_detach_tail( list );
			Struct *s = colm_generic_el_container( prg, tail, genId );

			vm_push_struct( s );

			/* Set up reverse. The result comes off the list downrefed.
			 * Need it up referenced for the reverse code too. */
			rcodeCode( exec, IN_LIST_POP_TAIL_BKT );
			rcodeHalf( exec, genId );
			rcodeWord( exec, (Word)s );
			rcodeUnitTerm( exec );
			break;
		}
		case IN_LIST_POP_TAIL_BKT: {
			short genId;
			Tree *val;
			read_half( genId );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_LIST_POP_TAIL_BKT\n" );

			List *list = vm_pop_list();
			Struct *s = (Struct*) val;

			ListEl *listEl = colm_struct_to_list_el( prg, s, genId );

			colm_list_append( list, listEl );
			break;
		}
		case IN_LIST_POP_HEAD_WC: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_LIST_POP_HEAD_WC\n" );

			List *list = vm_pop_list();

			ListEl *head = list->head;
			colm_list_detach_head( list );
			Struct *s = colm_generic_el_container( prg, head, genId );

			vm_push_struct( s );
			break;
		}
		case IN_LIST_POP_HEAD_WV: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_LIST_POP_HEAD_WV\n" );

			List *list = vm_pop_list();

			ListEl *head = list->head;
			colm_list_detach_head( list );
			Struct *s = colm_generic_el_container( prg, head, genId );

			vm_push_struct( s );

			/* Set up reverse. The result comes off the list downrefed.
			 * Need it up referenced for the reverse code too. */
			rcodeCode( exec, IN_LIST_POP_HEAD_BKT );
			rcodeHalf( exec, genId );
			rcodeWord( exec, (Word)s );
			rcodeUnitTerm( exec );
			break;
		}
		case IN_LIST_POP_HEAD_BKT: {
			short genId;
			Tree *val;
			read_half( genId );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_LIST_POP_HEAD_BKT\n" );

			List *list = vm_pop_list();
			Struct *s = (Struct*) val;

			ListEl *listEl = colm_struct_to_list_el( prg, s, genId );

			colm_list_prepend( list, listEl );
			break;
		}
		case IN_GET_LIST_MEM_R: {
			short genId, field;
			read_half( genId );
			read_half( field );

			debug( prg, REALM_BYTECODE, 
					"IN_GET_LIST_MEM_R %hd %hd\n", genId, field );

			List *list = vm_pop_list();
			Struct *val = colm_list_get( prg, list, genId, field );
			vm_push_struct( val );
			break;
		}
		case IN_GET_LIST_MEM_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_MEM_WC\n" );

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

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_MEM_WV\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = getListMemSplit( prg, (List*)obj, field );
			treeUpref( val );
			vm_push( val );

			/* Set up the reverse instruction. */
			rcodeCode( exec, IN_GET_LIST_MEM_BKT );
			rcodeHalf( exec, field );
			break;
		}
		case IN_GET_LIST_MEM_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_MEM_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *res = getListMemSplit( prg, (List*)obj, field );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_GET_PARSER_MEM_R: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_PARSER_MEM_R %hd\n", field );

			Tree *obj = vm_pop();
			Tree *val = getParserMem( (Parser*)obj, field );
			treeUpref( val );

			/* In at least one case we extract the result on a parser with ref
			 * one. Do it after. */
			treeDownref( prg, sp, obj );
			vm_push( val );
			break;
		}

		case IN_GET_MAP_EL_MEM_R: {
			short genId, field;
			read_half( genId );
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_MAP_EL_MEM_R\n" );

			Struct *strct = vm_pop_struct();

			MapEl *mapEl = colm_struct_to_map_el( prg, strct, genId );
			Struct *val = colm_map_el_get( prg, mapEl, genId, field );
			vm_push_struct( val );
			break;
		}
		case IN_MAP_INSERT_WC: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_MAP_INSERT_WC %hd\n", genId );

			Map *map = vm_pop_map();
			Struct *s = vm_pop_struct();

			MapEl *mapEl = colm_struct_to_map_el( prg, s, genId );

			colm_map_insert( prg, map, mapEl );

			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );
			break;
		}
		case IN_MAP_INSERT_WV: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_MAP_INSERT_WV %hd\n", genId );

			Map *map = vm_pop_map();
			Struct *s = vm_pop_struct();

			MapEl *mapEl = colm_struct_to_map_el( prg, s, genId );

			MapEl *inserted = colm_map_insert( prg, map, mapEl );

			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );

			rcodeCode( exec, IN_MAP_INSERT_BKT );
			rcodeHalf( exec, genId );
			rcodeCode( exec, inserted != 0 ? 1 : 0 );
			rcodeWord( exec, (Word)mapEl );
			rcodeUnitTerm( exec );
			break;
		}

		case IN_MAP_INSERT_BKT: {
			short genId;
			uchar inserted;
			Word wmapEl;

			read_half( genId );
			read_byte( inserted );
			read_word( wmapEl );

			MapEl *mapEl = (MapEl*)wmapEl;

			debug( prg, REALM_BYTECODE, "IN_MAP_INSERT_BKT %d\n", (int)inserted );

			Map *map = vm_pop_map();

			if ( inserted ) 
				colm_map_detach( prg, map, mapEl );
			break;
		}
		case IN_MAP_DETACH_WC: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_MAP_DETACH_WC %hd\n", genId );

			Map *map = vm_pop_map();
			Struct *s = vm_pop_struct();

			MapEl *mapEl = colm_struct_to_map_el( prg, s, genId );

			colm_map_detach( prg, map, mapEl );

			treeUpref( prg->trueVal );
			vm_push( prg->trueVal );
			break;
		}
		case IN_MAP_DETACH_WV: {
			debug( prg, REALM_BYTECODE, "IN_MAP_DETACH_WV\n" );

			Tree *obj = vm_pop();
			Tree *key = vm_pop();
			TreePair pair = mapRemove( prg, (Map*)obj, key );

			treeUpref( pair.val );
			vm_push( pair.val );

			/* Reverse instruction. */
			rcodeCode( exec, IN_MAP_DETACH_BKT );
			rcodeWord( exec, (Word)pair.key );
			rcodeWord( exec, (Word)pair.val );
			rcodeUnitTerm( exec );

			treeDownref( prg, sp, obj );
			treeDownref( prg, sp, key );
			break;
		}
		case IN_MAP_DETACH_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_MAP_DETACH_BKT\n" );

			/* Either both or neither. */
			assert( ( key == 0 ) ^ ( val != 0 ) );

			Tree *obj = vm_pop();
			if ( key != 0 )
				mapUnremove( prg, (Map*)obj, key, val );

			treeDownref( prg, sp, obj );
			break;
		}
		case IN_MAP_LENGTH: {
			debug( prg, REALM_BYTECODE, "IN_MAP_LENGTH\n" );

			Tree *obj = vm_pop();
			long len = mapLength( (Map*)obj );
			Tree *res = constructInteger( prg, len );
			treeUpref( res );
			vm_push( res );

			treeDownref( prg, sp, obj );
			break;
		}
		case IN_MAP_FIND: {
			short genId;
			read_half( genId );

			debug( prg, REALM_BYTECODE, "IN_MAP_FIND %hd\n", genId );

			Map *map = vm_pop_map();
			Tree *key = vm_pop();

			MapEl *mapEl = colm_map_find( prg, map, key );

			struct colm_struct *strct = mapEl != 0 ?
				colm_generic_el_container( prg, mapEl, genId ) : 0;

			vm_push_struct( strct );
			break;
		}
		case IN_GET_MAP_MEM_R: {
			short genId, field;
			read_half( genId );
			read_half( field );

			debug( prg, REALM_BYTECODE, 
					"IN_GET_MAP_MEM_R %hd %hd\n", genId, field );

			Map *map = vm_pop_map();
			Struct *val = colm_map_get( prg, map, genId, field );
			vm_push_struct( val );
			break;
		}
		case IN_GET_MAP_MEM_WC: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_MAP_MEM_WC\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = getListMemSplit( prg, (List*)obj, field );
			treeUpref( val );
			vm_push( val );
			break;
		}
		case IN_GET_MAP_MEM_WV: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_MAP_MEM_WV\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *val = getListMemSplit( prg, (List*)obj, field );
			treeUpref( val );
			vm_push( val );

			/* Set up the reverse instruction. */
			rcodeCode( exec, IN_GET_MAP_MEM_BKT );
			rcodeHalf( exec, field );
			break;
		}
		case IN_GET_MAP_MEM_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_MAP_MEM_BKT\n" );

			Tree *obj = vm_pop();
			treeDownref( prg, sp, obj );

			Tree *res = getListMemSplit( prg, (List*)obj, field );
			treeUpref( res );
			vm_push( res );
			break;
		}
		case IN_CONTIGUOUS: {
			Half size;
			read_half( size );
			debug( prg, REALM_BYTECODE, "IN_CONTIGUOUS %hd\n", size );
			vm_contiguous( size );
			vm_push( 0 );
			break;
		}
		case IN_CALL_WV: {
			Half funcId;
			read_half( funcId );

			FunctionInfo *fi = &prg->rtd->functionInfo[funcId];
			FrameInfo *fr = &prg->rtd->frameInfo[fi->frameId];

			debug( prg, REALM_BYTECODE, "IN_CALL_WV %s\n", fr->name );

			vm_push( 0 ); /* Return value. */
			vm_push( (SW)instr );
			vm_push( (SW)exec->framePtr );
			vm_push( (SW)exec->frameId );

			instr = fr->codeWV;
			exec->framePtr = vm_ptop();
			exec->frameId = fi->frameId;
			break;
		}
		case IN_CALL_WC: {
			Half funcId;
			read_half( funcId );

			FunctionInfo *fi = &prg->rtd->functionInfo[funcId];
			FrameInfo *fr = &prg->rtd->frameInfo[fi->frameId];

			debug( prg, REALM_BYTECODE, "IN_CALL_WC %s\n", fr->name );

			vm_push( 0 ); /* Return value. */
			vm_push( (SW)instr );
			vm_push( (SW)exec->framePtr );
			vm_push( (SW)exec->frameId );

			instr = fr->codeWC;
			exec->framePtr = vm_ptop();
			exec->frameId = fi->frameId;
			break;
		}
		case IN_YIELD: {
			debug( prg, REALM_BYTECODE, "IN_YIELD\n" );

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
				uiter->yieldSize = vm_ssize() - uiter->rootSize;
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

			debug( prg, REALM_BYTECODE, "IN_UITER_CREATE_WV\n" );

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

			debug( prg, REALM_BYTECODE, "IN_UITER_CREATE_WC\n" );

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

			debug( prg, REALM_BYTECODE, "IN_UITER_DESTROY\n" );

			UserIter *uiter = (UserIter*) vm_local(field);
			userIterDestroy( prg, &sp, uiter );
			break;
		}
		case IN_RET: {

			FrameInfo *fi = &prg->rtd->frameInfo[exec->frameId];
			downref_local_trees( prg, sp, exec->framePtr, fi->locals, fi->localsLen );
			vm_popn( fi->frameSize );

			exec->frameId = (long) vm_pop();
			exec->framePtr = (Tree**) vm_pop();
			instr = (Code*) vm_pop();
			Tree *retVal = vm_pop();
			vm_popn( fi->argSize );
			vm_push( retVal );

			fi = &prg->rtd->frameInfo[exec->frameId];
			debug( prg, REALM_BYTECODE, "IN_RET %s\n", fi->name );

			/* This if for direct calls of functions. */
			if ( instr == 0 ){
				//assert( sp == root );
				return sp;
			}
				
			break;
		}
		case IN_TO_UPPER: {
			debug( prg, REALM_BYTECODE, "IN_TO_UPPER\n" );

			Tree *in = vm_pop();
			Head *head = stringToUpper( in->tokdata );
			Tree *upper = constructString( prg, head );
			treeUpref( upper );
			vm_push( upper );
			treeDownref( prg, sp, in );
			break;
		}
		case IN_TO_LOWER: {
			debug( prg, REALM_BYTECODE, "IN_TO_LOWER\n" );

			Tree *in = vm_pop();
			Head *head = stringToLower( in->tokdata );
			Tree *lower = constructString( prg, head );
			treeUpref( lower );
			vm_push( lower );
			treeDownref( prg, sp, in );
			break;
		}
		case IN_OPEN_FILE: {
			debug( prg, REALM_BYTECODE, "IN_OPEN_FILE\n" );

			Tree *mode = vm_pop();
			Tree *name = vm_pop();
			Stream *res = openFile( prg, name, mode );
			vm_push_stream( res );
			treeDownref( prg, sp, name );
			treeDownref( prg, sp, mode );
			break;
		}
		case IN_GET_STDIN: {
			debug( prg, REALM_BYTECODE, "IN_GET_STDIN\n" );

			/* Pop the root object. */
			Tree *obj = vm_pop();
			if ( prg->stdinVal == 0 )
				prg->stdinVal = openStreamFd( prg, "<stdin>", 0 );

			vm_push_stream( prg->stdinVal );
			break;
		}
		case IN_GET_STDOUT: {
			debug( prg, REALM_BYTECODE, "IN_GET_STDOUT\n" );

			/* Pop the root object. */
			Tree *obj = vm_pop();
			if ( prg->stdoutVal == 0 )
				prg->stdoutVal = openStreamFd( prg, "<stdout>", 1 );

			vm_push_stream( prg->stdoutVal );
			break;
		}
		case IN_GET_STDERR: {
			debug( prg, REALM_BYTECODE, "IN_GET_STDERR\n" );

			/* Pop the root object. */
			Tree *obj = vm_pop();
			if ( prg->stderrVal == 0 )
				prg->stderrVal = openStreamFd( prg, "<stderr>", 2 );

			vm_push_stream( prg->stderrVal );
			break;
		}
		case IN_SYSTEM: {
			debug( prg, REALM_BYTECODE, "IN_SYSTEM\n" );

			Tree *global = vm_pop();
			Str *cmd = (Str*)vm_pop();

			char *cmd0 = malloc( cmd->value->length + 1 );
			memcpy( cmd0, cmd->value->data, cmd->value->length );
			cmd0[cmd->value->length] = 0;

			int r = system( cmd0 );

			treeDownref( prg, sp, global );
			treeDownref( prg, sp, (Tree*)cmd );

			Tree *result = constructInteger( prg, r );
			treeUpref( result );
			vm_push( result );
			break;
		}

		case IN_FN: {
			c = *instr++;
			switch ( c ) {
			case IN_STR_ATOI: {
				debug( prg, REALM_BYTECODE, "IN_STR_ATOI\n" );

				Str *str = (Str*)vm_pop();
				Word res = strAtoi( str->value );
				Tree *integer = constructInteger( prg, res );
				treeUpref( integer );
				vm_push( integer );
				treeDownref( prg, sp, (Tree*)str );
				break;
			}
			case IN_STR_UORD8: {
				debug( prg, REALM_BYTECODE, "IN_STR_UORD8\n" );

				Str *str = (Str*)vm_pop();
				Word res = strUord8( str->value );
				Tree *tree = constructInteger( prg, res );
				treeUpref( tree );
				vm_push( tree );
				treeDownref( prg, sp, (Tree*)str );
				break;
			}
			case IN_STR_UORD16: {
				debug( prg, REALM_BYTECODE, "IN_STR_UORD16\n" );

				Str *str = (Str*)vm_pop();
				Word res = strUord16( str->value );
				Tree *tree = constructInteger( prg, res );
				treeUpref( tree );
				vm_push( tree );
				treeDownref( prg, sp, (Tree*)str );
				break;
			}
			case IN_LOAD_ARG0: {
				Half field;
				read_half( field );
				debug( prg, REALM_BYTECODE, "IN_LOAD_ARG0 %lu\n", field );

				/* Tree comes back upreffed. */
				Tree *tree = construct_arg0( prg, prg->argc, prg->argv );
				Tree *prev = colm_struct_get_field( prg->global, Tree*, field );
				treeDownref( prg, sp, prev );
				colm_struct_set_field( prg->global, Tree*, field, tree );
				break;
			}
			case IN_LOAD_ARGV: {
				Half field;
				read_half( field );
				debug( prg, REALM_BYTECODE, "IN_LOAD_ARGV %lu\n", field );

				List *list = construct_argv( prg, prg->argc, prg->argv );
				colm_struct_set_field( prg->global, List*, field, list );
				break;
			}
			case IN_INIT_LOCALS: {
				Half size;
				read_half( size );

				debug( prg, REALM_BYTECODE, "IN_INIT_LOCALS %hd\n", size );

				exec->framePtr = vm_ptop();
				vm_pushn( size );
				memset( vm_ptop(), 0, sizeof(Word) * size );
				break;
			}
			case IN_STOP: {
				debug( prg, REALM_BYTECODE, "IN_STOP\n" );

				FrameInfo *fi = &prg->rtd->frameInfo[exec->frameId];
				downref_local_trees( prg, sp, exec->framePtr, fi->locals, fi->localsLen );
				vm_popn( fi->frameSize );

				fflush( stdout );
				goto out;
			}
			}
			break;
		}

		case IN_EXIT: {
			debug( prg, REALM_BYTECODE, "IN_EXIT\n" );

			Tree *global = vm_pop();
			Int *status = (Int*)vm_pop();
			prg->exitStatus = status->value;
			prg->induceExit = 1;
			treeDownref( prg, sp, (Tree*)status );

			while ( true ) {
				FrameInfo *fi = &prg->rtd->frameInfo[exec->frameId];
				int frameId = exec->frameId;

				downrefLocals( prg, &sp, exec->framePtr, fi->locals, fi->localsLen );

				vm_popn( fi->frameSize );

				/* We stop on the root, leaving the psuedo-call setup on the
				 * stack. Note we exclude the local data. */
				if ( frameId == prg->rtd->rootFrameId )
					break;

				/* Call layout. */
				exec->frameId = (long) vm_pop();
				exec->framePtr = (Tree**) vm_pop();
				instr = (Code*) vm_pop();
				Tree *retVal = vm_pop();
				vm_popn( fi->argSize );
				vm_pop();

				treeDownref( prg, sp, retVal );
			}

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
/*
 * Deleteing rcode required downreffing any trees held by it.
 */
void rcodeDownref( Program *prg, Tree **sp, Code *instr )
{
again:
	switch ( *instr++ ) {
		case IN_PARSE_SAVE_STEPS: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_SAVE_STEPS\n" );
			break;
		}
		case IN_PARSE_INIT_BKT: {
			Tree *parser;
			Word pcr;
			Word steps;

			debug( prg, REALM_BYTECODE, "IN_PARSE_INIT_BKT\n" );

			read_tree( parser );
			read_word( pcr );
			read_word( steps );

			break;
		}

		case IN_LOAD_TREE: {
			Word w;
			read_word( w );
			debug( prg, REALM_BYTECODE, "IN_LOAD_TREE %p\n", (Tree*)w );
			treeDownref( prg, sp, (Tree*)w );
			break;
		}
		case IN_LOAD_WORD: {
			Word w;
			read_word( w );
			debug( prg, REALM_BYTECODE, "IN_LOAD_WORD\n" );
			break;
		}
		case IN_RESTORE_LHS: {
			Tree *restore;
			read_tree( restore );
			debug( prg, REALM_BYTECODE, "IN_RESTORE_LHS\n" );
			treeDownref( prg, sp, restore );
			break;
		}

		case IN_PARSE_FRAG_BKT: {
			Half stopId;
			read_half( stopId );
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_BKT\n" );
			break;
		}
		case IN_PARSE_FRAG_EXIT_BKT: {
			debug( prg, REALM_BYTECODE, "IN_PARSE_FRAG_EXIT_BKT\n" );
			break;
		}
		case IN_PARSE_FINISH_BKT: {
			Half stopId;
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
		case IN_INPUT_APPEND_BKT: {
			Tree *parser;
			Tree *input;
			Word len;
			read_tree( parser );
			read_tree( input );
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_INPUT_APPEND_BKT\n" ); 

			break;
		}
		case IN_INPUT_APPEND_STREAM_BKT: {
			Tree *sptr;
			Tree *input;
			Word len;
			read_tree( sptr );
			read_tree( input );
			read_word( len );

			debug( prg, REALM_BYTECODE, "IN_INPUT_APPEND_STREAM_BKT\n" );

			treeDownref( prg, sp, input );
			break;
		}

		case IN_INPUT_PULL_BKT: {
			Tree *string;
			read_tree( string );

			debug( prg, REALM_BYTECODE, "IN_INPUT_PULL_BKT\n" );

			treeDownref( prg, sp, string );
			break;
		}
		case IN_INPUT_PUSH_BKT: {
			Word len;
			read_word( len );

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
		case IN_LOAD_PARSER_BKT: {
			/* Tree *parser; */
			consume_word();
			debug( prg, REALM_BYTECODE, "IN_LOAD_PARSER_BKT\n" );
			break;
		}
		case IN_LOAD_INPUT_BKT: {
			/* Tree *input; */
			consume_word();
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
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_FIELD_TREE_BKT %hd\n", field );

			treeDownref( prg, sp, val );
			break;
		}
		case IN_SET_STRUCT_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_BKT %hd\n", field );

			treeDownref( prg, sp, val );
			break;
		}
		case IN_SET_STRUCT_VAL_BKT: {
			short field;
			Tree *val;
			read_half( field );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_SET_STRUCT_VAL_BKT\n" );
			break;
		}
		case IN_PTR_ACCESS_BKT: {
			Tree *ptr;
			read_tree( ptr );

			debug( prg, REALM_BYTECODE, "IN_PTR_ACCESS_BKT\n" );
			break;
		}
		case IN_SET_TOKEN_DATA_BKT: {
			Word oldval;
			read_word( oldval );

			debug( prg, REALM_BYTECODE, "IN_SET_TOKEN_DATA_BKT\n" );

			Head *head = (Head*)oldval;
			stringFree( prg, head );
			break;
		}
		case IN_LIST_PUSH_HEAD_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_HEAD_BKT\n" );
			break;
		}
		case IN_LIST_POP_HEAD_BKT: {
			short genId;
			Tree *val;
			read_half( genId );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_LIST_POP_HEAD_BKT\n" );

			break;
		}
		case IN_LIST_PUSH_TAIL_BKT: {
			debug( prg, REALM_BYTECODE, "IN_LIST_PUSH_TAIL_BKT\n" );
			break;
		}
		case IN_LIST_POP_TAIL_BKT: {
			short genId;
			Tree *val;
			read_half( genId );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_LIST_POP_TAIL_BKT\n" );

			break;
		}
		case IN_GET_LIST_MEM_BKT: {
			short field;
			read_half( field );

			debug( prg, REALM_BYTECODE, "IN_GET_LIST_MEM_BKT %hd\n", field );
			break;
		}
		case IN_MAP_INSERT_BKT: {
			short genId;
			uchar inserted;
			Word wmapEl;

			read_half( genId );
			read_byte( inserted );
			read_word( wmapEl );

			debug( prg, REALM_BYTECODE, "IN_MAP_INSERT_BKT %d\n", (int)inserted );
			break;
		}
		case IN_MAP_DETACH_BKT: {
			Tree *key, *val;
			read_tree( key );
			read_tree( val );

			debug( prg, REALM_BYTECODE, "IN_MAP_DETACH_BKT\n" );

			treeDownref( prg, sp, key );
			treeDownref( prg, sp, val );
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
		default: {
			fatal( "UNKNOWN INSTRUCTION 0x%2x: -- reverse code downref\n", *(instr-1));
			assert(false);
			break;
		}
	}
	goto again;
}


