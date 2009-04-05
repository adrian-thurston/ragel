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

#include <string.h>
#include <iostream>
#include <stdlib.h>

#include "config.h"
#include "fsmrun.h"
#include "redfsm.h"
#include "parsedata.h"
#include "parsetree.h"
#include "pdarun.h"
#include "colm.h"

using std::cerr;
using std::endl;

exit_object endp;

void operator<<( ostream &out, exit_object & )
{
	out << endl;
	exit(1);
}

FsmRun::FsmRun( Program *prg ) :
	prg(prg),
	tables(prg->rtd->fsmTables)
{
}

FsmRun::~FsmRun()
{
//	RunBuf *rb = runBuf;
//	while ( rb != 0 ) {
//		RunBuf *next = rb->next;
//		delete rb;
//		rb = next;
//	}
}

void FsmRun::undoStreamPush( long length )
{
	long remainder = pe - p;
	memmove( runBuf->buf, runBuf->buf + length, remainder );
	pe -= length;
}

void FsmRun::streamPush( const char *data, long length )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "readying fake push" << endl;
	}
	#endif

	if ( p == runBuf->buf ) {
		cerr << "case 1" << endl;
		assert(false);
	}
	else if ( p == (runBuf->buf + runBuf->length) ) {
		cerr << "case 2" << endl;
		assert(false);
	}
	else {
		/* Send back the second half of the current run buffer. */
		RunBuf *dup = new RunBuf;
		memcpy( dup, runBuf, sizeof(RunBuf) );

		/* Need to fix the offset. */
		dup->length = pe - runBuf->buf;
		dup->offset = p - runBuf->buf;

		/* Send it back. */
		inputStream->pushBack( dup );

		/* Since the second half is gone the current buffer now ends at p. */
		pe = p;
		runBuf->length = p - runBuf->buf;

		/* Create a new buffer for the data. This is the easy implementation.
		 * Something better is needed here. It puts a max on the amount of
		 * data that can be pushed back to the stream. */
		assert( length < FSM_BUFSIZE );
		RunBuf *newBuf = new RunBuf;
		newBuf->next = runBuf;
		newBuf->offset = 0;
		newBuf->length = length;
		memcpy( newBuf->buf, data, length );

		p = newBuf->buf;
		pe = newBuf->buf + newBuf->length;
		runBuf = newBuf;
	}
}

/* Keep the position up to date after consuming text. */
void update_position( FsmRun *fsmRun, const char *data, long length )
{
	if ( !fsmRun->inputStream->handlesLine ) {
		for ( int i = 0; i < length; i++ ) {
			if ( data[i] == '\n' )
				fsmRun->inputStream->line += 1;
		}
	}

	fsmRun->inputStream->position += length;
}

/* Keep the position up to date after sending back text. */
void undo_position( FsmRun *fsmRun, const char *data, long length )
{
	if ( !fsmRun->inputStream->handlesLine ) {
		for ( int i = 0; i < length; i++ ) {
			if ( data[i] == '\n' )
				fsmRun->inputStream->line -= 1;
		}
	}

	fsmRun->inputStream->position -= length;
}

/* Should only be sending back whole tokens/ignores, therefore the send back
 * should never cross a buffer boundary. Either we slide back p, or we move to
 * a previous buffer and slide back p. */
void FsmRun::sendBackText( const char *data, long length )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "push back of " << length << " characters" << endl;
	}
	#endif

	if ( length == 0 )
		return;

	if ( p == runBuf->buf ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "pushing back runbuf" << endl;
		}
		#endif

		/* Move to the next run buffer. */
		RunBuf *back = runBuf;
		runBuf = runBuf->next;
		
		/* Flush out the input buffer. */
		back->length = pe-p;
		back->offset = 0;
		inputStream->pushBack( back );

		/* Set p and pe. */
		assert( runBuf != 0 );
		p = pe = runBuf->buf + runBuf->length;
	}

	/* If there is data in the current buffer then the whole send back
	 * should be in this buffer. */
	assert( (p - runBuf->buf) >= length );

	/* slide p back. */
	p -= length;

	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		if ( memcmp( data, p, length ) != 0 )
			cerr << "mismatch of pushed back text" << endl;
	}
	#endif

	assert( memcmp( data, p, length ) == 0 );
		
	undo_position( this, data, length );
}

void FsmRun::queueBack( PdaRun *parser, Kid *input )
{
	if ( input->tree->flags & AF_GROUP_MEM ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
			cerr << "queuing back: " << lelInfo[input->tree->id].name << endl;
		}
		#endif

		if ( parser->queue == 0 )
			parser->queue = parser->queueLast = input;
		else {
			parser->queueLast->next = input;
			parser->queueLast = input;
		}
	}
	else {
		/* If there are queued items send them back starting at the tail
		 * (newest). */
		if ( parser->queue != 0 ) {
			/* Reverse the list. */
			Kid *kid = parser->queue, *last = 0;
			while ( kid != 0 ) {
				Kid *next = kid->next;
				kid->next = last;
				last = kid;
				kid = next;
			}

			/* Send them back. */
			while ( last != 0 ) {
				Kid *next = last->next;
				sendBack( parser, last );
				last = next;
			}

			parser->queue = 0;
		}

		/* Now that the queue is flushed, can send back the original item. */
		sendBack( parser, input );
	}
}

void FsmRun::sendBackIgnore( PdaRun *parser, Kid *ignore )
{
	/* Ignore tokens are queued in reverse order. */
	while ( tree_is_ignore( prg, ignore ) ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
			cerr << "sending back: " << lelInfo[ignore->tree->id].name;
			if ( ignore->tree->flags & AF_ARTIFICIAL )
				cerr << " (artificial)";
			cerr << endl;
		}
		#endif

		Head *head = ignore->tree->tokdata;
		bool artificial = ignore->tree->flags & AF_ARTIFICIAL;

		if ( head != 0 && !artificial )
			sendBackText( string_data( head ), head->length );

		/* Check for reverse code. */
		if ( ignore->tree->flags & AF_HAS_RCODE ) {
			Execution execution( prg, parser->reverseCode, 
					parser, 0, 0, 0, 0 );

			/* Do the reverse exeuction. */
			execution.rexecute( parser->root, parser->allReverseCode );
			ignore->tree->flags &= ~AF_HAS_RCODE;
		}

		ignore = ignore->next;
	}
}

void FsmRun::sendBack( PdaRun *parser, Kid *input )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
		cerr << "sending back: " << lelInfo[input->tree->id].name;
		if ( input->tree->flags & AF_ARTIFICIAL )
			cerr << " (artificial)";
		cerr << endl;
	}
	#endif

	if ( input->tree->flags & AF_NAMED ) {
		/* Send back anything that is in the buffer. */
		inputStream->pushBack( p, pe-p );
		p = pe = runBuf->buf;

		/* Send the named lang el back first, then send back any leading
		 * whitespace. */
		inputStream->pushBackNamed();
	}

	if ( !(input->tree->flags & AF_ARTIFICIAL) ) {
		/* Push back the token data. */
		sendBackText( string_data( input->tree->tokdata ), 
				string_length( input->tree->tokdata ) );
	}

	/* Check for reverse code. */
	if ( input->tree->flags & AF_HAS_RCODE ) {
		Execution execution( prg, parser->reverseCode, 
				parser, 0, 0, 0, 0 );

		/* Do the reverse exeuction. */
		execution.rexecute( parser->root, parser->allReverseCode );
		input->tree->flags &= ~AF_HAS_RCODE;
	}

	/* Always push back the ignore text. */
	sendBackIgnore( parser, tree_ignore( prg, input->tree ) );

	/* If eof was just sent back remember that it needs to be sent again. */
	if ( input->tree->id == parser->tables->rtd->eofLelIds[parser->parserId] )
		eofSent = false;

	/* If the item is bound then store remove it from the bindings array. */
	Tree *lastBound = parser->bindings.top();
	if ( lastBound == input->tree ) {
		parser->bindings.pop();
		tree_downref( prg, parser->root, input->tree );
	}

	/* Downref the tree that was sent back and free the kid. */
	tree_downref( prg, parser->root, input->tree );
	prg->kidPool.free( input );
}

/* If no token was generated but there is reverse code then we must generate
 * a fake token so we can attach the reverse code to it. */
void add_notoken( Program *prg, PdaRun *parser )
{
	/* Check if there was anything generated. */
	if ( parser->queue == 0 && parser->reverseCode.length() > 0 ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "found reverse code but no token, sending _notoken" << endl;
		}
		#endif

		Tree *tree = (Tree*)prg->parseTreePool.allocate();
		tree->flags |= AF_PARSE_TREE;
		tree->refs = 1;
		tree->id = prg->rtd->noTokenId;
		tree->tokdata = 0;

		parser->queue = prg->kidPool.allocate();
		parser->queue->tree = tree;
		parser->queue->next = 0;
	}
}

/* Sets the AF_GROUP_MEM so the backtracker can tell which tokens were sent
 * generated from a single action. */
void set_AF_GROUP_MEM( PdaRun *parser )
{
	LangElInfo *lelInfo = parser->prg->rtd->lelInfo;

	long sendCount = 0;
	Kid *queued = parser->queue;
	while ( queued != 0 ) {
		/* Only bother with non-ignore tokens. */
		if ( !lelInfo[queued->tree->id].ignore ) {
			if ( sendCount > 0 )
				queued->tree->flags |= AF_GROUP_MEM;
			sendCount += 1;
		}
		queued = queued->next;
	}
}

void send_queued_tokens( FsmRun *fsmRun, PdaRun *parser )
{
	LangElInfo *lelInfo = fsmRun->prg->rtd->lelInfo;

	while ( parser->queue != 0 ) {
		/* Pull an item to send off the queue. */
		Kid *send = parser->queue;
		parser->queue = parser->queue->next;

		/* Must clear next, since the parsing algorithm uses it. */
		send->next = 0;
		if ( lelInfo[send->tree->id].ignore ) {
			#ifdef COLM_LOG_PARSE
			if ( colm_log_parse ) {
				cerr << "ignoring queued item: " << 
						parser->tables->rtd->lelInfo[send->tree->id].name << endl;
			}
			#endif
			
			parser->ignore( send->tree );
			fsmRun->prg->kidPool.free( send );
		}
		else {
			#ifdef COLM_LOG_PARSE
			if ( colm_log_parse ) {
				cerr << "sending queue item: " << 
						parser->tables->rtd->lelInfo[send->tree->id].name << endl;
			}
			#endif

			send_handle_error( fsmRun, parser, send );
		}
	}
}

Kid *make_token( FsmRun *fsmRun, PdaRun *parser, int id, Head *tokdata, bool
		namedLangEl, int bindId )
{
	/* Make the token object. */
	long objectLength = parser->tables->rtd->lelInfo[id].objectLength;
	Kid *attrs = alloc_attrs( fsmRun->prg, objectLength );

	Kid *input = 0;
	input = fsmRun->prg->kidPool.allocate();
	input->tree = (Tree*)fsmRun->prg->parseTreePool.allocate();
	input->tree->flags |= AF_PARSE_TREE;

	if ( namedLangEl )
		input->tree->flags |= AF_NAMED;

	input->tree->refs = 1;
	input->tree->id = id;
	input->tree->tokdata = tokdata;

	/* No children and ignores get added later. */
	input->tree->child = attrs;

	LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
	if ( lelInfo[id].numCaptureAttr > 0 ) {
		for ( int i = 0; i < lelInfo[id].numCaptureAttr; i++ ) {
			CaptureAttr *ca = &parser->tables->rtd->captureAttr[lelInfo[id].captureAttr + i];
			Head *data = string_alloc_new( fsmRun->prg, 
					fsmRun->mark[ca->mark_enter], fsmRun->mark[ca->mark_leave]
					- fsmRun->mark[ca->mark_enter] );
			Tree *string = construct_string( fsmRun->prg, data );
			tree_upref( string );
			set_attr( input->tree, ca->offset, string );
		}
	}
	
	/* If the item is bound then store it in the bindings array. */
	if ( bindId > 0 ) {
		parser->bindings.push( input->tree );
		tree_upref( input->tree );
	}

	return input;
}

void send_named_lang_el( FsmRun *fsmRun, PdaRun *parser )
{
	/* All three set by getLangEl. */
	long bindId;
	char *data;
	long length;

	KlangEl *klangEl = fsmRun->inputStream->getLangEl( bindId, data, length );
	if ( klangEl->termDup != 0 )
		klangEl = klangEl->termDup;
	
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "named langEl: " << parser->tables->rtd->lelInfo[klangEl->id].name << endl;
	}
	#endif

	/* Copy the token data. */
	Head *tokdata = 0;
	if ( data != 0 )
		tokdata = string_alloc_new( fsmRun->prg, data, length );

	Kid *input = make_token( fsmRun, parser, klangEl->id, tokdata, true, bindId );
	send_handle_error( fsmRun, parser, input );
}

void execute_generation_action( Program *prg, PdaRun *parser, Code *code, long id, Head *tokdata )
{
	/* Execute the translation. */
	Execution execution( prg, parser->reverseCode, parser, code, 0, id, tokdata );
	execution.execute( parser->root );

	/* If there is revese code but nothing generated we need a noToken. */
	add_notoken( prg, parser );

	/* If there is reverse code then add_notoken will guarantee that the
	 * queue is not empty. Pull the reverse code out and store in the
	 * token. */
	Tree *tree = parser->queue->tree;
	bool hasrcode = make_reverse_code( parser->allReverseCode, parser->reverseCode );
	if ( hasrcode )
		tree->flags |= AF_HAS_RCODE;

	/* Mark generated tokens as belonging to a group. */
	set_AF_GROUP_MEM( parser );
}

/* 
 * Not supported:
 *  -invoke failure (the backtracker)
 */

void generation_action( FsmRun *fsmRun, PdaRun *parser, int id, Head *tokdata,
		bool namedLangEl, int bindId )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "generation action: " << 
				parser->tables->rtd->lelInfo[id].name << endl;
	}
	#endif

	/* Find the code. */
	Code *code = parser->tables->rtd->frameInfo[
			parser->tables->rtd->lelInfo[id].frameId].codeWV;

	/* Execute the action and process the queue. */
	execute_generation_action( fsmRun->prg, parser, code, id, tokdata );

	/* Finished with the match text. */
	string_free( fsmRun->prg, tokdata );

	/* Send the queued tokens. */
	send_queued_tokens( fsmRun, parser );
}

/* Send back the accumulated ignore tokens. */
void send_back_queued_ignore( PdaRun *parser )
{
	Kid *ignore = parser->extractIgnore();
	parser->fsmRun->sendBackIgnore( parser, ignore );
	while ( ignore != 0 ) {
		Kid *next = ignore->next;
		tree_downref( parser->prg, parser->root, ignore->tree );
		parser->prg->kidPool.free( ignore );
		ignore = next;
	}
}

Kid *PdaRun::extractIgnore()
{
	Kid *ignore = accumIgnore;
	accumIgnore = 0;
	return ignore;
}

void PdaRun::send( Kid *input )
{
	/* Pull the ignore tokens out and store in the token. */
	Kid *ignore = extractIgnore();
	if ( ignore != 0 ) {
		Kid *child = input->tree->child;
		input->tree->child = ignore;
		while ( ignore->next != 0 )
			ignore = ignore->next;
		ignore->next = child;
	}
		
	parseToken( input );
}

void send_handle_error( FsmRun *fsmRun, PdaRun *parser, Kid *input )
{
	long id = input->tree->id;

	/* Send the token to the parser. */
	parser->send( input );
		
	/* Check the result. */
	if ( parser->errCount > 0 ) {
		/* Error occured in the top-level parser. */
		parser->parse_error(id, input->tree) << "parse error" << endp;
	}
	else {
		if ( parser->isParserStopFinished() ) {
			#ifdef COLM_LOG_PARSE
			if ( colm_log_parse ) {
				cerr << "stopping the parse" << endl;
			}
			#endif
			parser->stopParsing = true;
		}
	}
}

void PdaRun::ignore( Tree *tree )
{
	/* Add the ignore string to the head of the ignore list. */
	Kid *ignore = prg->kidPool.allocate();
	ignore->tree = tree;

	/* Prepend it to the list of ignore tokens. */
	ignore->next = accumIgnore;
	accumIgnore = ignore;
}

void exec_gen( FsmRun *fsmRun, PdaRun *parser, long id )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "token gen action: " << parser->tables->rtd->lelInfo[id].name << endl;
	}
	#endif

	/* Make the token data. */
	Head *tokdata = fsmRun->extractMatch();

	/* Note that we don't update the position now. It is done when the token
	 * data is pulled from the stream. */

	fsmRun->p = fsmRun->tokstart;

	generation_action( fsmRun, parser, id, tokdata, false, 0 );
}

void send_ignore( FsmRun *fsmRun, PdaRun *parser, long id )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "ignoring: " << parser->tables->rtd->lelInfo[id].name << endl;
	}
	#endif

	/* Make the ignore string. */
	Head *ignoreStr = fsmRun->extractMatch();
	update_position( fsmRun, fsmRun->tokstart, ignoreStr->length );
	
	Tree *tree = fsmRun->prg->treePool.allocate();
	tree->refs = 1;
	tree->id = id;
	tree->tokdata = ignoreStr;

	/* Send it to the parser. */
	parser->ignore( tree );
}

Head *FsmRun::extractMatch()
{
	long length = p - tokstart;
	return string_alloc_const( prg, tokstart, length );
}

void send_token( FsmRun *fsmRun, PdaRun *parser, long id )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "token: " << parser->tables->rtd->lelInfo[id].name << endl;
	}
	#endif

	/* Make the token data. */
	Head *tokdata = fsmRun->extractMatch();
	update_position( fsmRun, fsmRun->tokstart, tokdata->length );

	Kid *input = make_token( fsmRun, parser, id, tokdata, false, 0 );
	send_handle_error( fsmRun, parser, input );
}

/* Load up a token, starting from tokstart if it is set. If not set then
 * start it at p. */
Head *FsmRun::extractPrefix( PdaRun *parser, long length )
{
	/* How much do we have already? Tokstart may or may not be set. */
	assert( tokstart == 0 );

	/* The generated token length has been stuffed into tokdata. */
	if ( p + length > pe ) {
		p = pe = runBuf->buf;
		peof = 0;

		long space = runBuf->buf + FSM_BUFSIZE - pe;
			
		if ( space == 0 )
			cerr << "OUT OF BUFFER SPACE" << endp;
			
		long len = inputStream->getData( p, space );
		pe = p + len;
	}

	if ( p + length > pe )
		cerr << "NOT ENOUGH DATA TO FETCH TOKEN" << endp;

	Head *tokdata = string_alloc_const( prg, p, length );
	update_position( this, p, length );
	p += length;

	return tokdata;
}

void send_eof( FsmRun *fsmRun, PdaRun *parser )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "token: _EOF" << endl;
	}
	#endif

	Kid *input = fsmRun->prg->kidPool.allocate();
	input->tree = (Tree*)fsmRun->prg->parseTreePool.allocate();
	input->tree->flags |= AF_PARSE_TREE;

	input->tree->refs = 1;
	input->tree->id = parser->tables->rtd->eofLelIds[parser->parserId];

	bool ctxDepParsing = fsmRun->prg->ctxDepParsing;
	long frameId = parser->tables->rtd->regionInfo[fsmRun->region].eofFrameId;
	if ( ctxDepParsing && frameId >= 0 ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "HAVE PRE_EOF BLOCK" << endl;
		}
		#endif

		/* Get the code for the pre-eof block. */
		Code *code = parser->tables->rtd->frameInfo[frameId].codeWV;

		/* Execute the action and process the queue. */
		execute_generation_action( fsmRun->prg, parser, code, input->tree->id, 0 );

		/* Send the generated tokens. */
		send_queued_tokens( fsmRun, parser );
	}

	parser->send( input );

	if ( parser->errCount > 0 ) {
		parser->parse_error( input->tree->id, input->tree ) << 
				"parse error" << endp;
	}
}


void FsmRun::attachInputStream( InputStream *in )
{
	/* Run buffers need to stick around because 
	 * token strings point into them. */
	runBuf = new RunBuf;
	runBuf->next = 0;

	inputStream = in;
	p = pe = runBuf->buf;
	peof = 0;
	eofSent = false;
	inputStream->position = 0;
}

long PdaRun::undoParse( Tree *tree, CodeVect *rev )
{
	/* PDA must be init first to set next region. */
	init();
	Kid *top = prg->kidPool.allocate();
	top->next = stackTop;
	top->tree = tree;
	stackTop = top;
	numRetry += 1;
	allReverseCode = rev;

//	PdaRun *prevParser = fsmRun->parser;
//	fsmRun->parser = this;

	parseToken( 0 );

//	fsmRun->parser = prevParser;

	assert( stackTop->next == 0 );

	tree_downref( prg, root, stackTop->tree );
	prg->kidPool.free( stackTop );
	return 0;
}

#define SCAN_ERROR    -3
#define SCAN_LANG_EL  -2
#define SCAN_EOF      -1

void scanner_error( FsmRun *fsmRun, PdaRun *parser )
{
	if ( parser->getNextRegion( 1 ) != 0 ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "scanner failed, trying next region" << endl;
		}
		#endif

		/* May have accumulated ignore tokens from a previous region.
		 * need to rescan them since we won't be sending tokens from
		 * this region. */
		send_back_queued_ignore( parser );
		parser->nextRegionInd += 1;
	}
	else if ( parser->numRetry > 0 ) {
		/* Invoke the parser's error handling. */
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "invoking parse error from the scanner" << endl;
		}
		#endif

		send_back_queued_ignore( parser );
		parser->parseToken( 0 );

		if ( parser->errCount > 0 ) {
			/* Error occured in the top-level parser. */
			cerr << "PARSE ERROR" << endp;
		}
	}
	else {
		/* There are no alternative scanning regions to try, nor are there any
		 * alternatives stored in the current parse tree. No choice but to
		 * kill the parse. */
		cerr << "error:" << fsmRun->inputStream->line << ": scanner error" << endp;
	}
}

void parse( PdaRun *parser )
{
	parser->init();

	while ( true ) {
		
		/* Pull the current scanner from the parser. This can change during
		 * parsing due to stream pushes, usually for the purpose of includes.
		 * */
		FsmRun *fsmRun = parser->fsmRun;

		int tokenId = fsmRun->scanToken( parser );

		/* Check for EOF. */
		if ( tokenId == SCAN_EOF ) {
			fsmRun->eofSent = true;
			send_eof( fsmRun, parser );
			if ( fsmRun->eofSent )
				break;
			continue;
		}

		/* Check for a named language element. */
		if ( tokenId == SCAN_LANG_EL ) {
			send_named_lang_el( fsmRun, parser );
			continue;
		}

		/* Check for error. */
		if ( tokenId == SCAN_ERROR ) {
			scanner_error( fsmRun, parser );
			continue;
		}

		bool ctxDepParsing = fsmRun->prg->ctxDepParsing;
		LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
		if ( ctxDepParsing && lelInfo[tokenId].frameId >= 0 )
			exec_gen( fsmRun, parser, tokenId );
		else if ( lelInfo[tokenId].ignore )
			send_ignore( fsmRun, parser, tokenId );
		else
			send_token( fsmRun, parser, tokenId );

		/* Fall through here either when the input buffer has been exhausted
		 * or the scanner is in an error state. Otherwise we must continue. */
		if ( parser->stopParsing ) {
			#ifdef COLM_LOG_PARSE
			if ( colm_log_parse ) {
				cerr << "scanner has been stopped" << endl;
			}
			#endif
			break;
		}
	}
}

long FsmRun::scanToken( PdaRun *parser )
{
	/* Init the scanner vars. */
	act = 0;
	tokstart = 0;
	tokend = 0;
	matchedToken = 0;

	/* Set the state using the state of the parser. */
	region = parser->getNextRegion();
	cs = tables->entryByRegion[region];

	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "scanning using token region: " << 
				parser->tables->rtd->regionInfo[region].name << endl;
	}
	#endif

	/* Clear the mark array. */
	memset( mark, 0, sizeof(mark) );

	while ( true ) {
		execute();

		/* First check if scanning stopped because we have a token. */
		if ( matchedToken > 0 ) {
			/* If the token has a marker indicating the end (due to trailing
			 * context) then adjust p now. */
			LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
			if ( lelInfo[matchedToken].markId >= 0 )
				p = mark[lelInfo[matchedToken].markId];

			return matchedToken;
		}

		/* Check for error. */
		if ( cs == tables->errorState ) {
			/* If a token was started, but not finished (tokstart != 0) then
			 * restore p to the beginning of that token. */
			if ( tokstart != 0 )
				p = tokstart;

			/* Check for a default token in the region. If one is there
			 * then send it and continue with the processing loop. */
			if ( parser->tables->rtd->regionInfo[region].defaultToken >= 0 ) {
				tokstart = tokend = p;
				return parser->tables->rtd->regionInfo[region].defaultToken;
			}

			return SCAN_ERROR;
		}

		/* Got here because the state machine didn't match a token or
		 * encounter an error. Must be because we got to the end of the buffer
		 * data. */
		assert( p == pe );

		/* Check for a named language element. Note that we can do this only
		 * when p == pe otherwise we get ahead of what's already in the
		 * buffer. */
		if ( inputStream->isLangEl() )
			return SCAN_LANG_EL;

		/* Maybe need eof. */
 		if ( inputStream->isEOF() ) {
			if ( tokstart != 0 ) {
				/* If a token has been started, but not finshed 
				 * this is an error. */
				cs = tables->errorState;
				return SCAN_ERROR;
			}
			else {
				return SCAN_EOF;
			}
		}

		/* There may be space left in the current buffer. If not then we need
		 * to make some. */
		long space = runBuf->buf + FSM_BUFSIZE - pe;
		if ( space == 0 ) {
			/* Create a new run buf. */
			RunBuf *newBuf = new RunBuf;

			/* If partway through a token then preserve the prefix. */
			long have = 0;

			if ( tokstart == 0 ) {
				/* No prefix. We filled the previous buffer. */
				runBuf->length = FSM_BUFSIZE;
			}
			else {
				if ( tokstart == runBuf->buf ) {
					/* A token is started and it is already at the beginning
					 * of the current buffer. This means buffer is full and it
					 * must be grown. Probably need to do this sooner. */
					cerr << "OUT OF BUFFER SPACE" << endp;
				}

				/* There is data that needs to be shifted over. */
				have = pe - tokstart;
				memcpy( newBuf->buf, tokstart, have );

				/* Compute the length of the previous buffer. */
				runBuf->length = FSM_BUFSIZE - have;

				/* Compute tokstart and tokend. */
				long dist = tokstart - newBuf->buf;

				tokend -= dist;
				tokstart = newBuf->buf;

				/* Shift any markers. */
				for ( int i = 0; i < MARK_SLOTS; i++ ) {
					if ( mark[i] != 0 )
						mark[i] -= dist;
				}
			}

			p = pe = newBuf->buf + have;
			peof = 0;

			newBuf->next = runBuf;
			runBuf = newBuf;
		}

		/* We don't have any data. What is next in the input stream? */
		space = runBuf->buf + FSM_BUFSIZE - pe;
		assert( space > 0 );
			
		/* Get more data. */
		int len = inputStream->getData( p, space );
		pe = p + len;
		if ( inputStream->needFlush() )
			peof = pe;
	}

	/* Should not be reached. */
	return SCAN_ERROR;
}
