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
		cerr << "case 3" << endl;

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
		
	position -= length;

	/* We are adjusting p so this must be reset. */
	tokstart = 0;
}

void FsmRun::queueBack( Kid *input )
{
	Alg *alg = input->tree->alg;

	if ( alg->flags & AF_GROUP_MEM ) {
		#ifdef COLM_LOG_PARSE
		LangElInfo *lelInfo = parser->tables->gbl->lelInfo;
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
		LangElInfo *lelInfo = parser->tables->gbl->lelInfo;
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
			execution.rexecute( parser->root, 0, parser->allReverseCode );
			alg->flags &= ~AF_HAS_RCODE;
		}

		ignore = ignore->next;
	}
}

void FsmRun::sendBack( Kid *input )
{
	#ifdef COLM_LOG_PARSE
	LangElInfo *lelInfo = parser->tables->gbl->lelInfo;
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
		execution.rexecute( parser->root, 0, parser->allReverseCode );
		alg->flags &= ~AF_HAS_RCODE;
	}

	/* Always push back the ignore text. */
	sendBackIgnore( tree_ignore( prg, input->tree ) );

	/* If eof was just sent back remember that it needs to be sent again. */
	if ( input->tree->id == parser->tables->gbl->eofId )
		eofSent = false;

	/* If the item is bound then store remove it from the bindings array. */
	Tree *lastBound = parser->bindings.top();
	if ( lastBound == input->tree ) {
		parser->bindings.pop();
		tree_downref( prg, input->tree );
	}

	/* Downref the tree that was sent back and free the kid. */
	tree_downref( prg, input->tree );
	prg->kidPool.free( input );
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
	input->tree->id = parser->tables->gbl->eofId;

	bool ctxDepParsing = prg->ctxDepParsing;
	long frameId = parser->tables->gbl->regionInfo[region].eofFrameId;
	if ( ctxDepParsing && frameId >= 0 ) {
		#ifdef COLM_LOG_PARSE
		cerr << "HAVE PRE_EOF BLOCK" << endl;
		#endif

		Code *code = parser->tables->gbl->frameInfo[frameId].code;
	
		/* Execute the translation. */
		Execution execution( prg, parser->reverseCode, 
				parser, code, 0, 0 );
		execution.execute( parser->root );

		set_AF_GROUP_MEM();

		sendQueuedTokens();
	}

	parser->send( input );

	if ( parser->errCount > 0 ) {
		parser->parse_error( parser->tables->gbl->eofId, input->tree ) << 
				"parse error" << endp;
	}

	tokstart = 0;
	region = parser->getNextRegion();
	cs = tables->entryByRegion[region];
}

void FsmRun::sendQueuedTokens()
{
	while ( parser->queue != 0 ) {
		/* Pull an item to send off the queue. */
		Kid *send = parser->queue;
		parser->queue = parser->queue->next;

		/* Must clear next, since the parsing algorithm uses it. */
		send->next = 0;
		if ( send->tree->alg->flags & AF_IGNORE ) {
			#ifdef COLM_LOG_PARSE
			cerr << "ignoring queued item: " << 
					parser->tables->gbl->lelInfo[send->tree->id].name << endl;
			#endif
			
			parser->ignore( send->tree );
			prg->kidPool.free( send );
		}
		else {
			#ifdef COLM_LOG_PARSE
			cerr << "sending queue item: " << 
					parser->tables->gbl->lelInfo[send->tree->id].name << endl;
			#endif
			sendLangEl( send );
		}
	}
}

void FsmRun::sendToken( long id )
{
	#ifdef COLM_LOG_PARSE
	cerr << "token: " << parser->tables->gbl->lelInfo[id].name << endl;
	#endif

	bool ctxDepParsing = prg->ctxDepParsing;
	LangElInfo *lelInfo = parser->tables->gbl->lelInfo;

	/* Copy the token data. */
	long length = p-tokstart;
	Head *tokdata = string_alloc_const( prg, tokstart, length );

	if ( ctxDepParsing && lelInfo[id].frameId >= 0 ) {
		translateLangEl( id, tokdata, false, 0 );
		sendQueuedTokens();
	}
	else {
		makeToken( id, tokdata, false, 0 );
		assert( parser->queue == 0 );
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
	cerr << "named langEl: " << parser->tables->gbl->lelInfo[klangEl->id].name << endl;
	#endif

	/* Copy the token data. */
	Head *tokdata = 0;
	if ( data != 0 )
		tokdata = string_alloc_new( prg, data, length );

	makeToken( klangEl->id, tokdata, true, bindId );
}

void FsmRun::set_AF_GROUP_MEM()
{
	/* Set AF_GROUP_MEM now. */
	long sendCount = 0;
	Kid *queued = parser->queue;
	while ( queued != 0 ) {
		if ( !(queued->tree->alg->flags & AF_IGNORE) ) {
			if ( sendCount > 0 )
				queued->tree->alg->flags |= AF_GROUP_MEM;
			sendCount += 1;
		}
		queued = queued->next;
	}
}

/* 
 * Implmented:
 *  -shorten the match (possibly to zero length)
 *  -change the token to a new identifier 
 *  -change global state (it can, but it isn't reverted during backtracking).
 *
 * Not implemented:
 *  -invoke failure (and hence the backtracker)
 */

void FsmRun::translateLangEl( int id, Head *tokdata, bool namedLangEl, int bindId )
{
	#ifdef COLM_LOG_PARSE
	cerr << "translating: " << 
			parser->tables->gbl->lelInfo[id].name << endl;
	#endif

	Code *code = parser->tables->gbl->frameInfo[
			parser->tables->gbl->lelInfo[id].frameId].code;
	
	p = tokstart;

	/* Execute the translation. */
	Execution execution( prg, parser->reverseCode, 
			parser, code, 0, tokdata );
	execution.execute( parser->root );

	string_free( prg, tokdata );

	set_AF_GROUP_MEM();
}

void FsmRun::makeToken( int id, Head *tokdata, bool namedLangEl, int bindId )
{
	/* Make the token object. */
	long objectLength = parser->tables->gbl->lelInfo[id].objectLength;
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

	sendLangEl( input );
}

/* Send back the accumulated ignore tokens. */
void PdaRun::sendBackIgnore()
{
	Kid *ignore = extractIgnore();
	fsmRun->sendBackIgnore( ignore );
	while ( ignore != 0 ) {
		Kid *next = ignore->next;
		tree_downref( prg, ignore->tree );
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
	long length = string_length( input->tree->tokdata );
	//input->tree->pos = fsmRun->position;
	fsmRun->position += length;

	/* Pull the ignore tokens out and store in the token. */
	Kid *ignore = extractIgnore();
	if ( ignore != 0 ) {
		Kid *child = input->tree->child;
		input->tree->child = ignore;
		while ( ignore->next != 0 )
			ignore = ignore->next;
		ignore->next = child;
	}
		
	/* Pull the reverse code out and store in the token. */
	bool hasrcode = makeReverseCode( allReverseCode, reverseCode );
	if ( hasrcode )
		input->tree->alg->flags |= AF_HAS_RCODE;

	parseToken( input );
}

void FsmRun::sendLangEl( Kid *input )
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
		region = parser->getNextRegion();
		cs = tables->entryByRegion[region];

		if ( parser->isParserStopFinished() ) {
			#ifdef COLM_LOG_PARSE
			cerr << "stopping the parse" << endl;
			#endif
			cs = tables->errorState;
			parser->stopParsing = true;
		}
	}

	/* Reset tokstart. */
	tokstart = 0;

	#ifdef COLM_LOG_PARSE
	cerr << "new token region: " << 
			parser->tables->gbl->regionInfo[region].name << endl;
	#endif
}

void PdaRun::ignore( Tree *tree )
{
	/* Add the ignore string to the head of the ignore list. */
	Kid *ignore = prg->kidPool.allocate();
	ignore->tree = tree;

	/* Pull the reverse code out and store in the token. */
	bool hasrcode = makeReverseCode( allReverseCode, reverseCode );
	if ( hasrcode ) {
		if ( tree->alg == 0 )
			tree->alg = prg->algPool.allocate();
		tree->alg->flags |= AF_HAS_RCODE;
	}

	/* Prepend it to the list of ignore tokens. */
	ignore->next = accumIgnore;
	accumIgnore = ignore;
}

void FsmRun::sendIgnore( long id )
{
	int length = p-tokstart;

	#ifdef COLM_LOG_PARSE
	cerr << "ignoring: " << parser->tables->gbl->lelInfo[id].name << endl;
	#endif

	/* Make the ignore string. */
	Head *ignoreStr = string_alloc_const( prg, tokstart, length );
	
	Tree *tree = prg->treePool.allocate();
	tree->refs = 1;
	tree->id = id;
	tree->tokdata = ignoreStr;

	/* Send it to the parser. */
	parser->ignore( tree );

	/* Prepare for more scanning. */
	tokstart = 0;
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
	long have = 0;
	if ( tokstart != 0 )
		have = p - tokstart;
	else
		tokstart = p;

	/* The generated token length has been stuffed into tokdata. */
	if ( tokstart + length > pe ) {
		/* There is not enough data in the buffer to generate the token.
		 * Shift data over and fill the buffer. */
		if ( have > 0 ) {
			/* There is data that needs to be shifted over. */
			memmove( runBuf->buf, tokstart, have );
			tokend -= (tokstart - runBuf->buf);
			tokstart = runBuf->buf;
		}
		p = pe = runBuf->buf + have;
		peof = 0;

		long space = runBuf->buf + FSM_BUFSIZE - pe;
			
		if ( space == 0 )
			cerr << "OUT OF BUFFER SPACE" << endp;
			
		long len = inputStream->getData( p, space );
		pe = p + len;
	}

	if ( tokstart + length > pe )
		cerr << "NOT ENOUGH DATA TO FETCH TOKEN" << endp;

	Head *tokdata = string_alloc_const( prg, tokstart, length );
	p = tokstart + length;
	tokstart = 0;

	return tokdata;
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
	allReverseCode.transfer( *rev );

	parseToken( 0 );

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
			if ( parser->tables->gbl->regionInfo[region].defaultToken >= 0 ) {
				tokstart = tokend = p;
				sendToken( parser->tables->gbl->regionInfo[region].defaultToken );
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
						parser->tables->gbl->regionInfo[region].name << endl;
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
							parser->tables->gbl->regionInfo[region].name << endl;
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
