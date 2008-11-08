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
	tables(prg->rtd->fsmTables),
	parser(0),
	line(1),
	position(0)
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
	cerr << "readying fake push" << endl;
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
	for ( int i = 0; i < length; i++ ) {
		if ( data[i] == '\n' )
			fsmRun->line += 1;
	}

	fsmRun->position += length;
}

/* Keep the position up to date after sending back text. */
void undo_position( FsmRun *fsmRun, const char *data, long length )
{
	for ( int i = 0; i < length; i++ ) {
		if ( data[i] == '\n' )
			fsmRun->line -= 1;
	}

	fsmRun->position -= length;
}

/* Should only be sending back whole tokens/ignores, therefore the send back
 * should never cross a buffer boundary. Either we slide back p, or we move to
 * a previous buffer and slide back p. */
void FsmRun::sendBackText( const char *data, long length )
{
	#ifdef COLM_LOG_PARSE
	cerr << "push back of " << length << " characters" << endl;
	#endif

	if ( length == 0 )
		return;

	if ( p == runBuf->buf ) {
		#ifdef COLM_LOG_PARSE
		cerr << "pushing back runbuf" << endl;
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
	if ( memcmp( data, p, length ) != 0 )
		cerr << "mismatch of pushed back text" << endl;
	#endif

	assert( memcmp( data, p, length ) == 0 );
		
	undo_position( this, data, length );

	/* We are adjusting p so this must be reset. */
	tokstart = 0;
}

void FsmRun::queueBack( Kid *input )
{
	Alg *alg = input->tree->alg;

	if ( alg->flags & AF_GROUP_MEM ) {
		#ifdef COLM_LOG_PARSE
		LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
		cerr << "queuing back: " << lelInfo[input->tree->id].name << endl;
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
				sendBack( last );
				last = next;
			}

			parser->queue = 0;
		}

		/* Now that the queue is flushed, can send back the original item. */
		sendBack( input );
	}
}

void FsmRun::sendBackIgnore( Kid *ignore )
{
	/* Ignore tokens are queued in reverse order. */
	while ( tree_is_ignore( prg, ignore ) ) {
		#ifdef COLM_LOG_PARSE
		LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
		cerr << "sending back: " << lelInfo[ignore->tree->id].name;
		if ( ignore->tree->alg != 0 && ignore->tree->alg->flags & AF_ARTIFICIAL )
			cerr << " (artificial)";
		cerr << endl;
		#endif

		Head *head = ignore->tree->tokdata;
		bool artificial = ignore->tree->alg != 0 && 
				ignore->tree->alg->flags & AF_ARTIFICIAL;

		if ( head != 0 && !artificial )
			sendBackText( string_data( head ), head->length );

		/* Check for reverse code. */
		Alg *alg = ignore->tree->alg;
		if ( alg != 0 && alg->flags & AF_HAS_RCODE ) {
			Execution execution( prg, parser->reverseCode, 
					parser, 0, 0, 0 );

			/* Do the reverse exeuction. */
			execution.rexecute( parser->root, parser->allReverseCode );
			alg->flags &= ~AF_HAS_RCODE;
		}

		ignore = ignore->next;
	}
}

void FsmRun::sendBack( Kid *input )
{
	#ifdef COLM_LOG_PARSE
	LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
	cerr << "sending back: " << lelInfo[input->tree->id].name;
	if ( input->tree->alg->flags & AF_ARTIFICIAL )
		cerr << " (artificial)";
	cerr << endl;
	#endif

	Alg *alg = input->tree->alg;
	if ( alg->flags & AF_NAMED ) {
		/* Send back anything that is in the buffer. */
		inputStream->pushBack( p, pe-p );
		p = pe = runBuf->buf;

		/* Send the named lang el back first, then send back any leading
		 * whitespace. */
		inputStream->pushBackNamed();
	}

	if ( !(alg->flags & AF_ARTIFICIAL) ) {
		/* Push back the token data. */
		sendBackText( string_data( input->tree->tokdata ), 
				string_length( input->tree->tokdata ) );
	}

	/* Check for reverse code. */
	if ( alg->flags & AF_HAS_RCODE ) {
		Execution execution( prg, parser->reverseCode, 
				parser, 0, 0, 0 );

		/* Do the reverse exeuction. */
		execution.rexecute( parser->root, parser->allReverseCode );
		alg->flags &= ~AF_HAS_RCODE;
	}

	/* Always push back the ignore text. */
	sendBackIgnore( tree_ignore( prg, input->tree ) );

	/* If eof was just sent back remember that it needs to be sent again. */
	if ( input->tree->id == parser->tables->rtd->eofId )
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
		cerr << "found reverse code but no token, sending _notoken" << endl;
		#endif

		Tree *tree = prg->treePool.allocate();
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
				queued->tree->alg->flags |= AF_GROUP_MEM;
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
			cerr << "ignoring queued item: " << 
					parser->tables->rtd->lelInfo[send->tree->id].name << endl;
			#endif
			
			parser->ignore( send->tree );
			fsmRun->prg->kidPool.free( send );
		}
		else {
			#ifdef COLM_LOG_PARSE
			cerr << "sending queue item: " << 
					parser->tables->rtd->lelInfo[send->tree->id].name << endl;
			#endif

			send_handle_error( fsmRun, parser, send );
		}
	}
}

void FsmRun::sendToken( long id )
{
	#ifdef COLM_LOG_PARSE
	cerr << "token: " << parser->tables->rtd->lelInfo[id].name << endl;
	#endif

	bool ctxDepParsing = prg->ctxDepParsing;
	LangElInfo *lelInfo = parser->tables->rtd->lelInfo;

	/* Make the token data. */
	long length = p-tokstart;
	Head *tokdata = string_alloc_const( prg, tokstart, length );
	update_position( this, tokstart, length );

	if ( ctxDepParsing && lelInfo[id].frameId >= 0 ) {
		/* We don't want the generation actions to automatically consume text
		 * so reset p since the scanner leaves it at tokend. */
		p = tokstart;
		tokstart = 0;

		generationAction( id, tokdata, false, 0 );
	}
	else {
		/* By default the match is consumed and this is what we need. Just
		 * need to reset tokstart. */
		tokstart = 0;

		Kid *input = makeToken( id, tokdata, false, 0 );
		send_handle_error( this, parser, input );
	}

	memset( mark_leave, 0, sizeof(mark_leave) );
}

void FsmRun::sendNamedLangEl()
{
	/* All three set by getLangEl. */
	long bindId;
	char *data;
	long length;

	KlangEl *klangEl = inputStream->getLangEl( bindId, data, length );
	if ( klangEl->termDup != 0 )
		klangEl = klangEl->termDup;
	
	#ifdef COLM_LOG_PARSE
	cerr << "named langEl: " << parser->tables->rtd->lelInfo[klangEl->id].name << endl;
	#endif

	/* Copy the token data. */
	Head *tokdata = 0;
	if ( data != 0 )
		tokdata = string_alloc_new( prg, data, length );

	Kid *input = makeToken( klangEl->id, tokdata, true, bindId );
	send_handle_error( this, parser, input );
}

void execute_generation_action( Program *prg, PdaRun *parser, Code *code, Head *tokdata )
{
	/* Execute the translation. */
	Execution execution( prg, parser->reverseCode, parser, code, 0, tokdata );
	execution.execute( parser->root );

	/* If there is revese code but nothing generated we need a noToken. */
	add_notoken( prg, parser );

	/* If there is reverse code then add_notoken will guarantee that the
	 * queue is not empty. Pull the reverse code out and store in the
	 * token. */
	Tree *tree = parser->queue->tree;
	bool hasrcode = make_reverse_code( parser->allReverseCode, parser->reverseCode );
	if ( hasrcode ) {
		if ( tree->alg == 0 )
			tree->alg = prg->algPool.allocate();
		tree->alg->flags |= AF_HAS_RCODE;
	}

	/* Mark generated tokens as belonging to a group. */
	set_AF_GROUP_MEM( parser );
}

/* 
 * Not supported:
 *  -invoke failure (the backtracker)
 */

void FsmRun::generationAction( int id, Head *tokdata, bool namedLangEl, int bindId )
{
	#ifdef COLM_LOG_PARSE
	cerr << "generation action: " << 
			parser->tables->rtd->lelInfo[id].name << endl;
	#endif

	/* Find the code. */
	Code *code = parser->tables->rtd->frameInfo[
			parser->tables->rtd->lelInfo[id].frameId].codeWV;

	/* Execute the action and process the queue. */
	execute_generation_action( prg, parser, code, tokdata );

	/* Finished with the match text. */
	string_free( prg, tokdata );

	/* Send the queued tokens. */
	send_queued_tokens( this, parser );
}

Kid *FsmRun::makeToken( int id, Head *tokdata, bool namedLangEl, int bindId )
{
	/* Make the token object. */
	long objectLength = parser->tables->rtd->lelInfo[id].objectLength;
	Kid *attrs = alloc_attrs( prg, objectLength );

	Kid *input = 0;
	input = prg->kidPool.allocate();
	input->tree = prg->treePool.allocate();
	input->tree->alg = prg->algPool.allocate();

	if ( namedLangEl )
		input->tree->alg->flags |= AF_NAMED;

	input->tree->refs = 1;
	input->tree->id = id;
	input->tree->tokdata = tokdata;

	/* No children and ignores get added later. */
	input->tree->child = attrs;

	/* Set attributes for the labelled components. */
	for ( int i = 0; i < 32; i++ ) {
		if ( mark_leave[i] != 0 ) {
			Head *data = string_alloc_new( prg, 
					mark_enter[i], mark_leave[i] - mark_enter[i] );
			set_attr( input->tree, i, construct_string( prg, data ) );
			tree_upref( get_attr( input->tree, i ) );
		}
	}
	
	/* If the item is bound then store it in the bindings array. */
	if ( bindId > 0 ) {
		parser->bindings.push( input->tree );
		tree_upref( input->tree );
	}

	return input;
}

/* Send back the accumulated ignore tokens. */
void PdaRun::sendBackIgnore()
{
	Kid *ignore = extractIgnore();
	fsmRun->sendBackIgnore( ignore );
	while ( ignore != 0 ) {
		Kid *next = ignore->next;
		tree_downref( prg, root, ignore->tree );
		prg->kidPool.free( ignore );
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
		/* Set the current state from the next region. */
		fsmRun->region = parser->getNextRegion();
		fsmRun->cs = fsmRun->tables->entryByRegion[fsmRun->region];

		if ( parser->isParserStopFinished() ) {
			#ifdef COLM_LOG_PARSE
			cerr << "stopping the parse" << endl;
			#endif
			fsmRun->cs = fsmRun->tables->errorState;
			parser->stopParsing = true;
		}
	}

	#ifdef COLM_LOG_PARSE
	cerr << "new token region: " << 
			parser->tables->rtd->regionInfo[fsmRun->region].name << endl;
	#endif
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

void FsmRun::sendIgnore( long id )
{
	#ifdef COLM_LOG_PARSE
	cerr << "ignoring: " << parser->tables->rtd->lelInfo[id].name << endl;
	#endif

	/* Make the ignore string. */
	int length = p - tokstart;
	Head *ignoreStr = string_alloc_const( prg, tokstart, length );
	update_position( this, tokstart, length );
	tokstart = 0;
	
	Tree *tree = prg->treePool.allocate();
	tree->refs = 1;
	tree->id = id;
	tree->tokdata = ignoreStr;

	/* Send it to the parser. */
	parser->ignore( tree );

	/* Prepare for more scanning. */
	position += length;
	region = parser->getNextRegion();
	cs = tables->entryByRegion[region];

	memset( mark_leave, 0, sizeof(mark_leave) );
}

void FsmRun::emitToken( KlangEl *token )
{
	if ( token->ignore )
		sendIgnore( token->id );
	else
		sendToken( token->id );
}

/* Load up a token, starting from tokstart if it is set. If not set then
 * start it at p. */
Head *FsmRun::extractToken( long length )
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

void FsmRun::sendEOF( )
{
	#ifdef COLM_LOG_PARSE
	cerr << "token: _EOF" << endl;
	#endif

	Kid *input = prg->kidPool.allocate();
	input->tree = prg->treePool.allocate();
	input->tree->alg = prg->algPool.allocate();

	input->tree->refs = 1;
	input->tree->id = parser->tables->rtd->eofId;

	bool ctxDepParsing = prg->ctxDepParsing;
	long frameId = parser->tables->rtd->regionInfo[region].eofFrameId;
	if ( ctxDepParsing && frameId >= 0 ) {
		#ifdef COLM_LOG_PARSE
		cerr << "HAVE PRE_EOF BLOCK" << endl;
		#endif

		/* Get the code for the pre-eof block. */
		Code *code = parser->tables->rtd->frameInfo[frameId].codeWV;

		/* Execute the action and process the queue. */
		execute_generation_action( prg, parser, code, 0 );

		/* Send the generated tokens. */
		send_queued_tokens( this, parser );
	}

	parser->send( input );

	if ( parser->errCount > 0 ) {
		parser->parse_error( parser->tables->rtd->eofId, input->tree ) << 
				"parse error" << endp;
	}

	tokstart = 0;
	region = parser->getNextRegion();
	cs = tables->entryByRegion[region];
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
	position = 0;
}

long PdaRun::run()
{
	/* PDA must be init first to set next region. */
	init();
	return fsmRun->run( this );
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

	PdaRun *prevParser = fsmRun->parser;
	fsmRun->parser = this;

	parseToken( 0 );

	fsmRun->parser = prevParser;

	assert( stackTop->next == 0 );

	prg->algPool.free( stackTop->tree->alg );
	prg->treePool.free( stackTop->tree );
	prg->kidPool.free( stackTop );
	return 0;
}

long FsmRun::run( PdaRun *destParser )
{
	long space, prevState = cs;

	PdaRun *prevParser = parser;
	parser = destParser;

	act = 0;
	tokstart = 0;
	tokend = 0;
	region = parser->getNextRegion();
	cs = tables->entryByRegion[region];
	memset( mark_leave, 0, sizeof(mark_leave) );

	/* Start with the EOF test. The pattern and replacement input sources can
	 * be EOF from the start. */

	while ( true ) {
		/* Check for eof. */
 		if ( p == pe && inputStream->isEOF() ) {
			if ( tokstart != 0 ) {
				/* If a token has been started, but not finshed 
				 * this is an error. */
				cs = tables->errorState;
			}
			else {
				eofSent = true;
				sendEOF();
				if ( !eofSent )
					continue;
				break;
			}
		}

		if ( p == pe ) {
			/* We don't have any data. What is next in the input stream? */
			if ( inputStream->isLangEl() )
				sendNamedLangEl( );
			else {
				space = runBuf->buf + FSM_BUFSIZE - pe;
			
				if ( space == 0 )
					cerr << "OUT OF BUFFER SPACE" << endp;
			
				int len = inputStream->getData( p, space );
				pe = p + len;
				if ( inputStream->needFlush() )
					peof = pe;
			}
		}

		execute();

		/* Fall through here either when the input buffer has been exhausted
		 * or the scanner is in an error state. Otherwise we must continue. */

		if ( cs == tables->errorState && parser->stopParsing ) {
			#ifdef COLM_LOG_PARSE
			cerr << "scanner has been stopped" << endl;
			#endif
			goto done;
		}

		/* First thing check for error. */
		if ( cs == tables->errorState ) {
			/* If a token was started, but not finished (tokstart != 0) then
			 * restore p to the beginning of that token. */
			if ( tokstart != 0 )
				p = tokstart;

			/* Check for a default token in the region. If one is there
			 * then send it and continue with the processing loop. */
			if ( parser->tables->rtd->regionInfo[region].defaultToken >= 0 ) {
				tokstart = tokend = p;
				sendToken( parser->tables->rtd->regionInfo[region].defaultToken );
				continue;
			}

			if ( parser->getNextRegion( 1 ) != 0 ) {
				#ifdef COLM_LOG_PARSE
				cerr << "scanner failed, trying next region" << endl;
				#endif

				/* May have accumulated ignore tokens from a previous region.
				 * need to rescan them since we won't be sending tokens from
				 * this region. */
				parser->sendBackIgnore();

				parser->nextRegionInd += 1;
				region = parser->getNextRegion();
				cs = tables->entryByRegion[region];
				#ifdef COLM_LOG_PARSE
				cerr << "new token region: " << 
						parser->tables->rtd->regionInfo[region].name << endl;
				#endif
				continue;
			}

			if ( parser->numRetry > 0 ) {
				/* Invoke the parser's error handling. */
				#ifdef COLM_LOG_PARSE
				cerr << "invoking parse error from the scanner" << endl;
				#endif

				parser->sendBackIgnore();
				parser->parseToken( 0 );

				if ( parser->errCount > 0 ) {
					/* Error occured in the top-level parser. */
					cerr << "PARSE ERROR" << endp;
				}
				else {
					region = parser->getNextRegion();
					cs = tables->entryByRegion[region];
					#ifdef COLM_LOG_PARSE
					cerr << "new token region: " << 
							parser->tables->rtd->regionInfo[region].name << endl;
					#endif
					continue;
				}
			}

			/* Machine failed before finding a token. */
			cerr << "SCANNER ERROR" << endp;
		}

		space = runBuf->buf + FSM_BUFSIZE - pe;
		if ( space == 0 ) {
			/* Create a new run buf. */
			RunBuf *buf = new RunBuf;
			buf->next = runBuf;
			runBuf = buf;

			/* If partway through a token then preserve the prefix. */
			long have = 0;

			if ( tokstart == 0 ) {
				/* No prefix, the previous buffer was filled. */
				runBuf->next->length = FSM_BUFSIZE;
			}
			else {
				/* There is data that needs to be shifted over. */
				have = pe - tokstart;
				memcpy( runBuf->buf, tokstart, have );

				/* Compute the length of the previous buffer. */
				runBuf->next->length = FSM_BUFSIZE - have;

				/* Compute tokstart and tokend. */
				tokend = runBuf->buf + (tokend - tokstart);
				tokstart = runBuf->buf;
			}
			p = pe = runBuf->buf + have;
			peof = 0;
		}
	}

done:
	parser = prevParser;
	cs = prevState;
	return 0;
}
