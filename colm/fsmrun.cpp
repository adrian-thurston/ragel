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
#include "pdarun.h"
#include "debug.h"
#include "bytecode.h"

using std::cerr;
using std::endl;
using std::ostream;

exit_object endp;

void operator<<( ostream &out, exit_object & )
{
	out << endl;
	exit(1);
}

void initInputStream( InputStream *inputStream )
{
	/* FIXME: correct values here. */
	inputStream->line = 0;
	inputStream->column = 0;
	inputStream->byte = 0;
}

void newToken( PdaRun *pdaRun, FsmRun *fsmRun )
{
	/* Init the scanner vars. */
	fsmRun->act = 0;
	fsmRun->tokstart = 0;
	fsmRun->tokend = 0;
	fsmRun->matchedToken = 0;
	fsmRun->tokstart = 0;

	/* Set the state using the state of the parser. */
	fsmRun->region = pdaRunGetNextRegion( pdaRun, 0 );
	fsmRun->cs = fsmRun->tables->entryByRegion[fsmRun->region];

	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "scanning using token region: " << 
				pdaRun->tables->rtd->regionInfo[fsmRun->region].name << endl;
	}
	#endif

	/* Clear the mark array. */
	memset( fsmRun->mark, 0, sizeof(fsmRun->mark) );
}

void breakRunBuf( FsmRun *fsmRun )
{
	/* We break the runbuf on named language elements and trees. This makes
	 * backtracking simpler because it allows us to always push back whole
	 * runBufs only. If we did not do this we could get half a runBuf, a named
	 * langEl, then the second half full. During backtracking we would need to
	 * push the halves back separately. */
	if ( fsmRun->p > fsmRun->runBuf->data ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse )
			cerr << "have a langEl, making a new runBuf" << endl;
		#endif

		/* Compute the length of the current before before we move
		 * past it. */
		fsmRun->runBuf->length = fsmRun->p - fsmRun->runBuf->data;;

		/* Make the new one. */
		RunBuf *newBuf = newRunBuf();
		fsmRun->p = fsmRun->pe = newBuf->data;
		newBuf->next = fsmRun->runBuf;
		fsmRun->runBuf = newBuf;
	}
}

#define SCAN_IGNORE            -6
#define SCAN_TREE              -5
#define SCAN_TRY_AGAIN_LATER   -4
#define SCAN_ERROR             -3
#define SCAN_LANG_EL           -2
#define SCAN_EOF               -1

long scanToken( PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	while ( true ) {
		if ( inputStream->funcs->needFlush( inputStream ) )
			fsmRun->peof = fsmRun->pe;

		fsmExecute( fsmRun, inputStream );

		/* First check if scanning stopped because we have a token. */
		if ( fsmRun->matchedToken > 0 ) {
			/* If the token has a marker indicating the end (due to trailing
			 * context) then adjust data now. */
			LangElInfo *lelInfo = pdaRun->tables->rtd->lelInfo;
			if ( lelInfo[fsmRun->matchedToken].markId >= 0 )
				fsmRun->p = fsmRun->mark[lelInfo[fsmRun->matchedToken].markId];

			return fsmRun->matchedToken;
		}

		/* Check for error. */
		if ( fsmRun->cs == fsmRun->tables->errorState ) {
			/* If a token was started, but not finished (tokstart != 0) then
			 * restore data to the beginning of that token. */
			if ( fsmRun->tokstart != 0 )
				fsmRun->p = fsmRun->tokstart;

			/* Check for a default token in the region. If one is there
			 * then send it and continue with the processing loop. */
			if ( pdaRun->tables->rtd->regionInfo[fsmRun->region].defaultToken >= 0 ) {
				fsmRun->tokstart = fsmRun->tokend = fsmRun->p;
				return pdaRun->tables->rtd->regionInfo[fsmRun->region].defaultToken;
			}

			return SCAN_ERROR;
		}

		/* Got here because the state machine didn't match a token or
		 * encounter an error. Must be because we got to the end of the buffer
		 * data. */
		assert( fsmRun->p == fsmRun->pe );

		/* Check for a named language element or constructed trees. Note that
		 * we can do this only when data == de otherwise we get ahead of what's
		 * already in the buffer. */
		if ( inputStream->funcs->isLangEl( inputStream ) ) {
			breakRunBuf( fsmRun );
			return SCAN_LANG_EL;
		}
		else if ( inputStream->funcs->isTree( inputStream ) ) {
			breakRunBuf( fsmRun );
			return SCAN_TREE;
		}
		else if ( inputStream->funcs->isIgnore( inputStream ) ) {
			breakRunBuf( fsmRun );
			return SCAN_IGNORE;
		}

		/* Maybe need eof. */
 		if ( inputStream->funcs->isEof( inputStream ) ) {
			if ( fsmRun->tokstart != 0 ) {
				/* If a token has been started, but not finshed 
				 * this is an error. */
				fsmRun->cs = fsmRun->tables->errorState;
				return SCAN_ERROR;
			}
			else {
				return SCAN_EOF;
			}
		}

		/* Maybe need to pause parsing until more data is inserted into the
		 * input inputStream. */
		if ( inputStream->funcs->tryAgainLater( inputStream ) )
			return SCAN_TRY_AGAIN_LATER;

		/* There may be space left in the current buffer. If not then we need
		 * to make some. */
		long space = fsmRun->runBuf->data + FSM_BUFSIZE - fsmRun->pe;
		if ( space == 0 ) {
			/* Create a new run buf. */
			RunBuf *newBuf = newRunBuf();

			/* If partway through a token then preserve the prefix. */
			long have = 0;

			if ( fsmRun->tokstart == 0 ) {
				/* No prefix. We filled the previous buffer. */
				fsmRun->runBuf->length = FSM_BUFSIZE;
			}
			else {
				if ( fsmRun->tokstart == fsmRun->runBuf->data ) {
					/* A token is started and it is already at the beginning
					 * of the current buffer. This means buffer is full and it
					 * must be grown. Probably need to do this sooner. */
					cerr << "OUT OF BUFFER SPACE" << endp;
				}

				/* There is data that needs to be shifted over. */
				have = fsmRun->pe - fsmRun->tokstart;
				memcpy( newBuf->data, fsmRun->tokstart, have );

				/* Compute the length of the previous buffer. */
				fsmRun->runBuf->length = FSM_BUFSIZE - have;

				/* Compute tokstart and tokend. */
				long dist = fsmRun->tokstart - newBuf->data;

				fsmRun->tokend -= dist;
				fsmRun->tokstart = newBuf->data;

				/* Shift any markers. */
				for ( int i = 0; i < MARK_SLOTS; i++ ) {
					if ( fsmRun->mark[i] != 0 )
						fsmRun->mark[i] -= dist;
				}
			}

			fsmRun->p = fsmRun->pe = newBuf->data + have;
			fsmRun->peof = 0;

			newBuf->next = fsmRun->runBuf;
			fsmRun->runBuf = newBuf;
		}

		/* We don't have any data. What is next in the input inputStream? */
		space = fsmRun->runBuf->data + FSM_BUFSIZE - fsmRun->pe;
		assert( space > 0 );
			
		connect( fsmRun, inputStream );

		/* Get more data. */
		int len = inputStream->funcs->getData( inputStream, fsmRun->p, space );
		fsmRun->pe = fsmRun->p + len;
	}

	/* Should not be reached. */
	return SCAN_ERROR;
}

void scannerError( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun )
{
	if ( pdaRunGetNextRegion( pdaRun, 1 ) != 0 ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "scanner failed, trying next region" << endl;
		}
		#endif

		/* May have accumulated ignore tokens from a previous region.
		 * need to rescan them since we won't be sending tokens from
		 * this region. */
		sendBackQueuedIgnore( sp, inputStream, fsmRun, pdaRun );
		pdaRun->nextRegionInd += 1;
	}
	else if ( pdaRun->numRetry > 0 ) {
		/* Invoke the parser's error handling. */
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse ) {
			cerr << "invoking parse error from the scanner" << endl;
		}
		#endif

		sendBackQueuedIgnore( sp, inputStream, fsmRun, pdaRun );
		parseToken( sp, pdaRun, fsmRun, inputStream, 0 );

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

void sendTree( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
//	RunBuf *runBuf = inputStream->queue;
//	inputStream->queue = inputStream->queue->next;
//
//	/* FIXME: using runbufs here for this is a poor use of memory. */
//	input->tree = runBuf->tree;
//	delete runBuf;
	Kid *input = kidAllocate( fsmRun->prg );
	input->tree = inputStream->funcs->getTree( inputStream );

	incrementConsumed( pdaRun );

	sendHandleError( sp, pdaRun, fsmRun, inputStream, input );
}

void sendTreeIgnore( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	RunBuf *runBuf = inputStreamPopHead( inputStream );

	/* FIXME: using runbufs here for this is a poor use of memory. */
	Tree *tree = runBuf->tree;
	delete runBuf;

	incrementConsumed( pdaRun );

	ignore( pdaRun, tree );
}

void parseLoop( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	pdaRun->stop = false;

	while ( true ) {
		/* Pull the current scanner from the parser. This can change during
		 * parsing due to inputStream pushes, usually for the purpose of includes.
		 * */
		int tokenId = scanToken( pdaRun, fsmRun, inputStream );

		if ( tokenId == SCAN_TRY_AGAIN_LATER )
			break;

		/* Check for EOF. */
		if ( tokenId == SCAN_EOF ) {
			inputStream->eofSent = true;
			sendEof( sp, inputStream, fsmRun, pdaRun );

			newToken( pdaRun, fsmRun );

			if ( inputStream->eofSent )
				break;
			continue;
		}

		if ( tokenId == SCAN_ERROR ) {
			/* Error. */
			scannerError( sp, inputStream, fsmRun, pdaRun );
		}
		else if ( tokenId == SCAN_LANG_EL ) {
			/* A named language element (parsing colm program). */
			sendNamedLangEl( sp, pdaRun, fsmRun, inputStream );
		}
		else if ( tokenId == SCAN_TREE ) {
			/* A tree already built. */
			sendTree( sp, pdaRun, fsmRun, inputStream );
		}
		else if ( tokenId == SCAN_IGNORE ) {
			/* A tree to ignore. */
			sendTreeIgnore( sp, pdaRun, fsmRun, inputStream );
		}
		else {
			/* Plain token. */
			bool ctxDepParsing = fsmRun->prg->ctxDepParsing;
			LangElInfo *lelInfo = pdaRun->tables->rtd->lelInfo;
			if ( ctxDepParsing && lelInfo[tokenId].frameId >= 0 )
				execGen( sp, inputStream, fsmRun, pdaRun, tokenId );
			else if ( lelInfo[tokenId].ignore )
				sendIgnore( inputStream, fsmRun, pdaRun, tokenId );
			else
				sendToken( sp, inputStream, fsmRun, pdaRun, tokenId );
		}

		newToken( pdaRun, fsmRun );

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

		if ( pdaRun->stop ) {
			cerr << "parsing has been stopped by consumedCount" << endl;
			break;
		}
	}
}
