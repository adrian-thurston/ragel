/*
 *  Copyright 2007-2009 Adrian Thurston <thurston@complang.org>
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

void undo_stream_push( InputStream *inputStream, FsmRun *fsmRun, long length )
{
	long remainder = inputStream->de - inputStream->data;
	memmove( inputStream->runBuf->buf, inputStream->runBuf->buf + length, remainder );
	inputStream->de -= length;
}

void stream_push( InputStream *inputStream, FsmRun *fsmRun, const char *data, long length )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "readying fake push" << endl;
	}
	#endif

	if ( inputStream->data == inputStream->runBuf->buf ) {
		cerr << "case 1" << endl;
		assert(false);
	}
	else if ( inputStream->data == (inputStream->runBuf->buf + inputStream->runBuf->length) ) {
		cerr << "case 2" << endl;
		assert(false);
	}
	else {
		/* Send back the second half of the current run buffer. */
		RunBuf *dup = new RunBuf;
		memcpy( dup, inputStream->runBuf, sizeof(RunBuf) );

		/* Need to fix the offset. */
		dup->length = inputStream->de - inputStream->runBuf->buf;
		dup->offset = inputStream->data - inputStream->runBuf->buf;

		/* Send it back. */
		inputStream->pushBackBuf( dup );

		/* Since the second half is gone the current buffer now ends at data. */
		inputStream->de = inputStream->data;
		inputStream->runBuf->length = inputStream->data - inputStream->runBuf->buf;

		/* Create a new buffer for the data. This is the easy implementation.
		 * Something better is needed here. It puts a max on the amount of
		 * data that can be pushed back to the stream. */
		assert( length < FSM_BUFSIZE );
		RunBuf *newBuf = new RunBuf;
		newBuf->next = inputStream->runBuf;
		newBuf->offset = 0;
		newBuf->length = length;
		memcpy( newBuf->buf, data, length );

		inputStream->data = newBuf->buf;
		inputStream->de = newBuf->buf + newBuf->length;
		inputStream->runBuf = newBuf;
	}
}

/* Keep the position up to date after consuming text. */
void update_position( InputStream *inputStream, const char *data, long length )
{
	if ( !inputStream->handlesLine ) {
		for ( int i = 0; i < length; i++ ) {
			if ( data[i] == '\n' )
				inputStream->line += 1;
		}
	}

	inputStream->position += length;
}

/* Keep the position up to date after sending back text. */
void undo_position( InputStream *inputStream, const char *data, long length )
{
	if ( !inputStream->handlesLine ) {
		for ( int i = 0; i < length; i++ ) {
			if ( data[i] == '\n' )
				inputStream->line -= 1;
		}
	}

	inputStream->position -= length;
}

void send_back_runbuf_head( InputStream *inputStream, FsmRun *fsmRun )
{
	if ( inputStream->data == inputStream->runBuf->buf ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "pushing back runbuf" << endl;
		}
		#endif

		/* Move to the next run buffer. */
		RunBuf *back = inputStream->runBuf;
		inputStream->runBuf = inputStream->runBuf->next;
		
		/* Flush out the input buffer. */
		back->length = inputStream->de - inputStream->data;
		back->offset = 0;
		inputStream->pushBackBuf( back );

		/* Set data and de. */
		if ( inputStream->runBuf == 0 ) {
			inputStream->runBuf = new RunBuf;
			inputStream->runBuf->next = 0;
			inputStream->data = inputStream->de = inputStream->runBuf->buf;
		}

		inputStream->data = inputStream->de = inputStream->runBuf->buf + inputStream->runBuf->length;
	}
}

/* Should only be sending back whole tokens/ignores, therefore the send back
 * should never cross a buffer boundary. Either we slide back data, or we move to
 * a previous buffer and slide back data. */
void send_back_text( InputStream *inputStream, FsmRun *fsmRun, const char *data, long length )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "push back of " << length << " characters" << endl;
	}
	#endif

	if ( length == 0 )
		return;

	send_back_runbuf_head( inputStream, fsmRun );

	/* If there is data in the current buffer then the whole send back
	 * should be in this buffer. */
	assert( (inputStream->data - inputStream->runBuf->buf) >= length );

	/* slide data back. */
	inputStream->data -= length;

	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		if ( memcmp( data, inputStream->data, length ) != 0 )
			cerr << "mismatch of pushed back text" << endl;
	}
	#endif

	assert( memcmp( data, inputStream->data, length ) == 0 );
		
	undo_position( inputStream, data, length );
}

void queue_back( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser, Kid *input )
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
				send_back( sp, inputStream, fsmRun, parser, last );
				last = next;
			}

			parser->queue = 0;
		}

		/* Now that the queue is flushed, can send back the original item. */
		send_back( sp, inputStream, fsmRun, parser, input );
	}
}

void send_back_ignore( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, Kid *ignore )
{
	/* Ignore tokens are queued in reverse order. */
	while ( tree_is_ignore( fsmRun->prg, ignore ) ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			LangElInfo *lelInfo = pdaRun->tables->rtd->lelInfo;
			cerr << "sending back: " << lelInfo[ignore->tree->id].name;
			if ( ignore->tree->flags & AF_ARTIFICIAL )
				cerr << " (artificial)";
			cerr << endl;
		}
		#endif

		Head *head = ignore->tree->tokdata;
		bool artificial = ignore->tree->flags & AF_ARTIFICIAL;

		if ( head != 0 && !artificial )
			send_back_text( inputStream, fsmRun, string_data( head ), head->length );

		/* Check for reverse code. */
		if ( ignore->tree->flags & AF_HAS_RCODE ) {
			Execution execution( fsmRun->prg, pdaRun->reverseCode, 
					pdaRun, 0, 0, 0, 0, fsmRun->mark );

			/* Do the reverse exeuction. */
			execution.rexecute( sp, pdaRun->allReverseCode );
			ignore->tree->flags &= ~AF_HAS_RCODE;
		}

		ignore = ignore->next;
	}
}

void send_back( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser, Kid *input )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		LangElInfo *lelInfo = parser->tables->rtd->lelInfo;
		cerr << "sending back: " << lelInfo[input->tree->id].name << "  text: ";
		cerr.write( string_data( input->tree->tokdata ), 
				string_length( input->tree->tokdata ) );
		if ( input->tree->flags & AF_ARTIFICIAL )
			cerr << " (artificial)";
		cerr << endl;
	}
	#endif

	if ( input->tree->flags & AF_NAMED ) {
		/* Send back anything in the buffer that has not been parsed. */
		send_back_runbuf_head( inputStream, fsmRun );

		/* Send the named lang el back first, then send back any leading
		 * whitespace. */
		inputStream->pushBackNamed();
	}

	if ( !(input->tree->flags & AF_ARTIFICIAL) ) {
		/* Push back the token data. */
		send_back_text( inputStream, fsmRun, string_data( input->tree->tokdata ), 
				string_length( input->tree->tokdata ) );
	}

	/* Check for reverse code. */
	if ( input->tree->flags & AF_HAS_RCODE ) {
		Execution execution( fsmRun->prg, parser->reverseCode, 
				parser, 0, 0, 0, 0, fsmRun->mark );

		/* Do the reverse exeuction. */
		execution.rexecute( sp, parser->allReverseCode );
		input->tree->flags &= ~AF_HAS_RCODE;
	}

	/* Always push back the ignore text. */
	send_back_ignore( sp, inputStream, fsmRun, parser, tree_ignore( fsmRun->prg, input->tree ) );

	/* If eof was just sent back remember that it needs to be sent again. */
	if ( input->tree->id == parser->tables->rtd->eofLelIds[parser->parserId] )
		inputStream->eofSent = false;

	/* If the item is bound then store remove it from the bindings array. */
	Tree *lastBound = parser->bindings.top();
	if ( lastBound == input->tree ) {
		parser->bindings.pop();
		tree_downref( fsmRun->prg, sp, input->tree );
	}

	/* Downref the tree that was sent back and free the kid. */
	tree_downref( fsmRun->prg, sp, input->tree );
	fsmRun->prg->kidPool.free( input );
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

void send_queued_tokens( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser )
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
			
			ignore( parser, send->tree );
			fsmRun->prg->kidPool.free( send );
		}
		else {
			#ifdef COLM_LOG_PARSE
			if ( colm_log_parse ) {
				cerr << "sending queue item: " << 
						parser->tables->rtd->lelInfo[send->tree->id].name << endl;
			}
			#endif

			send_handle_error( sp, inputStream, fsmRun, parser, send );
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

void send_named_lang_el( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser )
{
	/* All three set by getLangEl. */
	long bindId;
	char *data;
	long length;

	KlangEl *klangEl = inputStream->getLangEl( bindId, data, length );
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
	send_handle_error( sp, inputStream, fsmRun, parser, input );
}

void execute_generation_action( Tree **sp, Program *prg, FsmRun *fsmRun, PdaRun *pdaRun, Code *code, long id, Head *tokdata )
{
	/* Execute the translation. */
	Execution execution( prg, pdaRun->reverseCode, pdaRun, code, 0, id, tokdata, fsmRun->mark );
	execution.execute( sp );

	/* If there is revese code but nothing generated we need a noToken. */
	add_notoken( prg, pdaRun );

	/* If there is reverse code then add_notoken will guarantee that the
	 * queue is not empty. Pull the reverse code out and store in the
	 * token. */
	Tree *tree = pdaRun->queue->tree;
	bool hasrcode = make_reverse_code( pdaRun->allReverseCode, pdaRun->reverseCode );
	if ( hasrcode )
		tree->flags |= AF_HAS_RCODE;

	/* Mark generated tokens as belonging to a group. */
	set_AF_GROUP_MEM( pdaRun );
}

/* 
 * Not supported:
 *  -invoke failure (the backtracker)
 */

void generation_action( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, 
		PdaRun *pdaRun, int id, Head *tokdata, bool namedLangEl, int bindId )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "generation action: " << 
				pdaRun->tables->rtd->lelInfo[id].name << endl;
	}
	#endif

	/* Find the code. */
	Code *code = pdaRun->tables->rtd->frameInfo[
			pdaRun->tables->rtd->lelInfo[id].frameId].codeWV;

	/* Execute the action and process the queue. */
	execute_generation_action( sp, fsmRun->prg, fsmRun, pdaRun, code, id, tokdata );

	/* Finished with the match text. */
	string_free( fsmRun->prg, tokdata );

	/* Send the queued tokens. */
	send_queued_tokens( sp, inputStream, fsmRun, pdaRun );
}

Kid *extract_ignore( PdaRun *pdaRun )
{
	Kid *ignore = pdaRun->accumIgnore;
	pdaRun->accumIgnore = 0;
	return ignore;
}

/* Send back the accumulated ignore tokens. */
void send_back_queued_ignore( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun )
{
	Kid *ignore = extract_ignore( pdaRun );
	send_back_ignore( sp, inputStream, fsmRun, pdaRun, ignore );
	while ( ignore != 0 ) {
		Kid *next = ignore->next;
		tree_downref( pdaRun->prg, sp, ignore->tree );
		pdaRun->prg->kidPool.free( ignore );
		ignore = next;
	}
}

void send( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, Kid *input )
{
	/* Need to preserve the layout under a tree:
	 *    attributes, ignore tokens, grammar children. */

	/* Pull the ignore tokens out and store in the token. */
	Kid *ignore = extract_ignore( pdaRun );
	if ( ignore != 0 ) {
		if ( input->tree->child == 0 ) {
			/* No children, set the ignore as the first child. */
			input->tree->child = ignore;
		}
		else {
			/* There are children. Find where the attribute list ends and the
			 * grammatical children begin. */
			LangElInfo *lelInfo = pdaRun->prg->rtd->lelInfo;
			long objectLength = lelInfo[input->tree->id].objectLength;
			Kid *attrEnd = 0, *childBegin = input->tree->child;
			for ( long a = 0; a < objectLength; a++ ) {
				attrEnd = childBegin;
				childBegin = childBegin->next;
			}

			if ( attrEnd == 0 ) {
				/* No attributes. concat ignore + the existing list. */
				input->tree->child = kid_list_concat( ignore, input->tree->child );
			}
			else {
				/* There are attributes. concat child, ignore, childBegin. */
				attrEnd->next = 0;
				input->tree->child = kid_list_concat( input->tree->child, kid_list_concat( ignore, childBegin ) );
			}
		}
	}

	parse_token( sp, inputStream, fsmRun, pdaRun, input );
}

void send_handle_error( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, Kid *input )
{
	long id = input->tree->id;

	/* Send the token to the parser. */
	send( sp, inputStream, fsmRun, pdaRun, input );
		
	/* Check the result. */
	if ( pdaRun->errCount > 0 ) {
		/* Error occured in the top-level parser. */
		parse_error( inputStream, fsmRun, pdaRun, id, input->tree ) << "parse error" << endp;
	}
	else {
		if ( pdaRun->isParserStopFinished() ) {
			#ifdef COLM_LOG_PARSE
			if ( colm_log_parse ) {
				cerr << "stopping the parse" << endl;
			}
			#endif
			pdaRun->stopParsing = true;
		}
	}
}

void ignore( PdaRun *pdaRun, Tree *tree )
{
	/* Add the ignore string to the head of the ignore list. */
	Kid *ignore = pdaRun->prg->kidPool.allocate();
	ignore->tree = tree;

	/* Prepend it to the list of ignore tokens. */
	ignore->next = pdaRun->accumIgnore;
	pdaRun->accumIgnore = ignore;
}

void exec_gen( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, long id )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "token gen action: " << pdaRun->tables->rtd->lelInfo[id].name << endl;
	}
	#endif

	/* Make the token data. */
	Head *tokdata = extract_match( inputStream, fsmRun );

	/* Note that we don't update the position now. It is done when the token
	 * data is pulled from the stream. */

	inputStream->data = inputStream->token;
	inputStream->token = 0;

	generation_action( sp, inputStream, fsmRun, pdaRun, id, tokdata, false, 0 );
}

void send_ignore( InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, long id )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "ignoring: " << pdaRun->tables->rtd->lelInfo[id].name << endl;
	}
	#endif

	/* Make the ignore string. */
	Head *ignoreStr = extract_match( inputStream, fsmRun );
	update_position( inputStream, inputStream->token, ignoreStr->length );
	
	Tree *tree = fsmRun->prg->treePool.allocate();
	tree->refs = 1;
	tree->id = id;
	tree->tokdata = ignoreStr;

	/* Send it to the pdaRun. */
	ignore( pdaRun, tree );
}

Head *extract_match( InputStream *inputStream, FsmRun *fsmRun )
{
	long length = inputStream->data - inputStream->token;
	return string_alloc_const( fsmRun->prg, inputStream->token, length );
}

void send_token( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser, long id )
{
	/* Make the token data. */
	Head *tokdata = extract_match( inputStream, fsmRun );

	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "token: " << parser->tables->rtd->lelInfo[id].name << "  text: ";
		cerr.write( string_data( tokdata ), string_length( tokdata ) );
		cerr << endl;
	}
	#endif

	update_position( inputStream, inputStream->token, tokdata->length );

	Kid *input = make_token( fsmRun, parser, id, tokdata, false, 0 );
	send_handle_error( sp, inputStream, fsmRun, parser, input );
}

/* Load up a token, starting from tokstart if it is set. If not set then
 * start it at data. */
Head *extract_prefix( InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser, long length )
{
	/* We should not be in the midst of getting a token. */
	assert( inputStream->token == 0 );

	/* The generated token length has been stuffed into tokdata. */
	if ( inputStream->data + length > inputStream->de ) {
		inputStream->data = inputStream->de = inputStream->runBuf->buf;
		inputStream->deof = 0;

		long space = inputStream->runBuf->buf + FSM_BUFSIZE - inputStream->de;
			
		if ( space == 0 )
			cerr << "OUT OF BUFFER SPACE" << endp;
			
		long len = inputStream->getData( inputStream->data, space );
		inputStream->de = inputStream->data + len;
	}

	if ( inputStream->data + length > inputStream->de )
		cerr << "NOT ENOUGH DATA TO FETCH TOKEN" << endp;

	Head *tokdata = string_alloc_const( fsmRun->prg, inputStream->data, length );
	update_position( inputStream, inputStream->data, length );
	inputStream->data += length;

	return tokdata;
}

void send_eof( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun )
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
	input->tree->id = pdaRun->tables->rtd->eofLelIds[pdaRun->parserId];

	bool ctxDepParsing = fsmRun->prg->ctxDepParsing;
	long frameId = pdaRun->tables->rtd->regionInfo[fsmRun->region].eofFrameId;
	if ( ctxDepParsing && frameId >= 0 ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "HAVE PRE_EOF BLOCK" << endl;
		}
		#endif

		/* Get the code for the pre-eof block. */
		Code *code = pdaRun->tables->rtd->frameInfo[frameId].codeWV;

		/* Execute the action and process the queue. */
		execute_generation_action( sp, fsmRun->prg, fsmRun, pdaRun, code, input->tree->id, 0 );

		/* Send the generated tokens. */
		send_queued_tokens( sp, inputStream, fsmRun, pdaRun );
	}

	send( sp, inputStream, fsmRun, pdaRun, input );

	if ( pdaRun->errCount > 0 ) {
		parse_error( inputStream, fsmRun, pdaRun, input->tree->id, input->tree ) << 
				"parse error" << endp;
	}
}

void init_input_stream( InputStream *inputStream )
{
	/* Run buffers need to stick around because 
	 * token strings point into them. */
	inputStream->runBuf = new RunBuf;
	inputStream->runBuf->next = 0;

	inputStream->data = inputStream->de = inputStream->runBuf->buf;
	inputStream->deof = 0;
	inputStream->eofSent = false;
	inputStream->position = 0;
}

long undo_parse( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, Tree *tree, CodeVect *rev )
{
	/* PDA must be init first to set next region. */
	pdaRun->init();
	Kid *top = pdaRun->prg->kidPool.allocate();
	top->next = pdaRun->stackTop;
	top->tree = tree;
	pdaRun->stackTop = top;
	pdaRun->numRetry += 1;
	pdaRun->allReverseCode = rev;

//	PdaRun *prevParser = fsmRun->parser;
//	fsmRun->parser = this;

	parse_token( sp, inputStream, fsmRun, pdaRun, 0 );

//	fsmRun->parser = prevParser;

	assert( pdaRun->stackTop->next == 0 );

	tree_downref( pdaRun->prg, sp, pdaRun->stackTop->tree );
	pdaRun->prg->kidPool.free( pdaRun->stackTop );
	return 0;
}

#define SCAN_ERROR    -3
#define SCAN_LANG_EL  -2
#define SCAN_EOF      -1

void scanner_error( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun )
{
	if ( pdaRun->getNextRegion( 1 ) != 0 ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "scanner failed, trying next region" << endl;
		}
		#endif

		/* May have accumulated ignore tokens from a previous region.
		 * need to rescan them since we won't be sending tokens from
		 * this region. */
		send_back_queued_ignore( sp, inputStream, fsmRun, pdaRun );
		pdaRun->nextRegionInd += 1;
	}
	else if ( pdaRun->numRetry > 0 ) {
		/* Invoke the parser's error handling. */
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "invoking parse error from the scanner" << endl;
		}
		#endif

		send_back_queued_ignore( sp, inputStream, fsmRun, pdaRun );
		parse_token( sp, inputStream, fsmRun, pdaRun, 0 );

		if ( pdaRun->errCount > 0 ) {
			/* Error occured in the top-level parser. */
			cerr << "PARSE ERROR" << endp;
		}
	}
	else {
		/* There are no alternative scanning regions to try, nor are there any
		 * alternatives stored in the current parse tree. No choice but to
		 * kill the parse. */
		cerr << "error:" << inputStream->line << ": scanner error" << endp;
	}
}

void parse( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun )
{
	pdaRun->init();

	while ( true ) {
		/* Pull the current scanner from the parser. This can change during
		 * parsing due to stream pushes, usually for the purpose of includes.
		 * */
		int tokenId = scan_token( inputStream, fsmRun, pdaRun );

		/* Check for EOF. */
		if ( tokenId == SCAN_EOF ) {
			inputStream->eofSent = true;
			send_eof( sp, inputStream, fsmRun, pdaRun );
			if ( inputStream->eofSent )
				break;
			continue;
		}

		/* Check for a named language element. */
		if ( tokenId == SCAN_LANG_EL ) {
			send_named_lang_el( sp, inputStream, fsmRun, pdaRun );
			continue;
		}

		/* Check for error. */
		if ( tokenId == SCAN_ERROR ) {
			scanner_error( sp, inputStream, fsmRun, pdaRun );
			continue;
		}

		bool ctxDepParsing = fsmRun->prg->ctxDepParsing;
		LangElInfo *lelInfo = pdaRun->tables->rtd->lelInfo;
		if ( ctxDepParsing && lelInfo[tokenId].frameId >= 0 )
			exec_gen( sp, inputStream, fsmRun, pdaRun, tokenId );
		else if ( lelInfo[tokenId].ignore )
			send_ignore( inputStream, fsmRun, pdaRun, tokenId );
		else
			send_token( sp, inputStream, fsmRun, pdaRun, tokenId );

		/* Fall through here either when the input buffer has been exhausted
		 * or the scanner is in an error state. Otherwise we must continue. */
		if ( pdaRun->stopParsing ) {
			#ifdef COLM_LOG_PARSE
			if ( colm_log_parse ) {
				cerr << "scanner has been stopped" << endl;
			}
			#endif
			break;
		}
	}
}

long scan_token( InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun )
{
	/* Init the scanner vars. */
	fsmRun->act = 0;
	fsmRun->tokstart = 0;
	fsmRun->tokend = 0;
	fsmRun->matchedToken = 0;
	inputStream->token = 0;

	/* Set the state using the state of the parser. */
	fsmRun->region = pdaRun->getNextRegion();
	fsmRun->cs = fsmRun->tables->entryByRegion[fsmRun->region];

	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "scanning using token region: " << 
				pdaRun->tables->rtd->regionInfo[fsmRun->region].name << endl;
	}
	#endif

	/* Clear the mark array. */
	memset( fsmRun->mark, 0, sizeof(fsmRun->mark) );

	while ( true ) {
		fsmRun->p = inputStream->data;
		fsmRun->pe = inputStream->de;
		fsmRun->peof = inputStream->deof;
		fsmRun->tokstart = inputStream->token;

		fsm_execute( inputStream, fsmRun );

		inputStream->data = fsmRun->p;
		inputStream->de = fsmRun->pe;
		inputStream->deof = fsmRun->peof;
		inputStream->token = fsmRun->tokstart;

		/* First check if scanning stopped because we have a token. */
		if ( fsmRun->matchedToken > 0 ) {
			/* If the token has a marker indicating the end (due to trailing
			 * context) then adjust data now. */
			LangElInfo *lelInfo = pdaRun->tables->rtd->lelInfo;
			if ( lelInfo[fsmRun->matchedToken].markId >= 0 )
				inputStream->data = fsmRun->mark[lelInfo[fsmRun->matchedToken].markId];

			return fsmRun->matchedToken;
		}

		/* Check for error. */
		if ( fsmRun->cs == fsmRun->tables->errorState ) {
			/* If a token was started, but not finished (tokstart != 0) then
			 * restore data to the beginning of that token. */
			if ( inputStream->token != 0 )
				inputStream->data = inputStream->token;

			/* Check for a default token in the region. If one is there
			 * then send it and continue with the processing loop. */
			if ( pdaRun->tables->rtd->regionInfo[fsmRun->region].defaultToken >= 0 ) {
				inputStream->token = fsmRun->tokend = inputStream->data;
				return pdaRun->tables->rtd->regionInfo[fsmRun->region].defaultToken;
			}

			return SCAN_ERROR;
		}

		/* Got here because the state machine didn't match a token or
		 * encounter an error. Must be because we got to the end of the buffer
		 * data. */
		assert( inputStream->data == inputStream->de );

		/* Check for a named language element. Note that we can do this only
		 * when data == de otherwise we get ahead of what's already in the
		 * buffer. */
		if ( inputStream->isLangEl() ) {
			/* Always break the runBuf on named language elements. This makes
			 * backtracking simpler because it allows us to always push back
			 * whole runBufs only. If we did not do this we could get half a
			 * runBuf, a named langEl, then the second half full. During
			 * backtracking we would need to push the halves back separately.
			 * */
			if ( inputStream->data > inputStream->runBuf->buf ) {
				#ifdef COLM_LOG_PARSE
				if ( colm_log_parse )
					cerr << "have a langEl, making a new runBuf" << endl;
				#endif

				/* Compute the length of the current before before we move
				 * past it. */
				inputStream->runBuf->length = inputStream->data - inputStream->runBuf->buf;;

				/* Make the new one. */
				RunBuf *newBuf = new RunBuf;
				inputStream->data = inputStream->de = newBuf->buf;
				newBuf->next = inputStream->runBuf;
				inputStream->runBuf = newBuf;
			}

			return SCAN_LANG_EL;
		}

		/* Maybe need eof. */
 		if ( inputStream->isEOF() ) {
			if ( inputStream->token != 0 ) {
				/* If a token has been started, but not finshed 
				 * this is an error. */
				fsmRun->cs = fsmRun->tables->errorState;
				return SCAN_ERROR;
			}
			else {
				return SCAN_EOF;
			}
		}

		/* There may be space left in the current buffer. If not then we need
		 * to make some. */
		long space = inputStream->runBuf->buf + FSM_BUFSIZE - inputStream->de;
		if ( space == 0 ) {
			/* Create a new run buf. */
			RunBuf *newBuf = new RunBuf;

			/* If partway through a token then preserve the prefix. */
			long have = 0;

			if ( inputStream->token == 0 ) {
				/* No prefix. We filled the previous buffer. */
				inputStream->runBuf->length = FSM_BUFSIZE;
			}
			else {
				if ( inputStream->token == inputStream->runBuf->buf ) {
					/* A token is started and it is already at the beginning
					 * of the current buffer. This means buffer is full and it
					 * must be grown. Probably need to do this sooner. */
					cerr << "OUT OF BUFFER SPACE" << endp;
				}

				/* There is data that needs to be shifted over. */
				have = inputStream->de - inputStream->token;
				memcpy( newBuf->buf, inputStream->token, have );

				/* Compute the length of the previous buffer. */
				inputStream->runBuf->length = FSM_BUFSIZE - have;

				/* Compute tokstart and tokend. */
				long dist = inputStream->token - newBuf->buf;

				fsmRun->tokend -= dist;
				inputStream->token = newBuf->buf;

				/* Shift any markers. */
				for ( int i = 0; i < MARK_SLOTS; i++ ) {
					if ( fsmRun->mark[i] != 0 )
						fsmRun->mark[i] -= dist;
				}
			}

			inputStream->data = inputStream->de = newBuf->buf + have;
			inputStream->deof = 0;

			newBuf->next = inputStream->runBuf;
			inputStream->runBuf = newBuf;
		}

		/* We don't have any data. What is next in the input stream? */
		space = inputStream->runBuf->buf + FSM_BUFSIZE - inputStream->de;
		assert( space > 0 );
			
		/* Get more data. */
		int len = inputStream->getData( inputStream->data, space );
		inputStream->de = inputStream->data + len;
		if ( inputStream->needFlush() )
			inputStream->deof = inputStream->de;
	}

	/* Should not be reached. */
	return SCAN_ERROR;
}
