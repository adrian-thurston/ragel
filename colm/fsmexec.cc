/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
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

#include "config.h"
#include "defs.h"
#include "redfsm.h"
#include "parsedata.h"
#include "parsetree.h"
#include "pdarun.h"
#include "global.h"

void execAction( FsmRun *fsmRun, GenAction *genAction )
{
	for ( InlineList::Iter item = *genAction->inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case InlineItem::Text:
			assert(false);
			break;
		case InlineItem::LmSetActId:
			fsmRun->act = item->longestMatchPart->longestMatchId;
			break;
		case InlineItem::LmSetTokEnd:
			fsmRun->tokend = fsmRun->toklen + ( fsmRun->p - fsmRun->start ) + 1;
			break;
		case InlineItem::LmInitTokStart:
			assert(false);
			break;
		case InlineItem::LmInitAct:
			fsmRun->act = 0;
			break;
		case InlineItem::LmSetTokStart:
			fsmRun->tokstart = fsmRun->p;
			break;
		case InlineItem::LmSwitch:
			/* If the switch handles error then we also forced the error state. It
			 * will exist. */
			fsmRun->toklen = fsmRun->tokend;
			if ( item->tokenRegion->lmSwitchHandlesError && fsmRun->act == 0 ) {
				fsmRun->cs = fsmRun->tables->errorState;
			}
			else {
				for ( TokenInstanceListReg::Iter lmi = item->tokenRegion->tokenInstanceList; 
						lmi.lte(); lmi++ )
				{
					if ( lmi->inLmSelect && fsmRun->act == lmi->longestMatchId )
						fsmRun->matchedToken = lmi->tokenDef->tdLangEl->id;
				}
			}
			fsmRun->returnResult = true;
			fsmRun->skipToklen = true;
			break;
		case InlineItem::LmOnLast:
			fsmRun->p += 1;
			fsmRun->matchedToken = item->longestMatchPart->tokenDef->tdLangEl->id;
			fsmRun->returnResult = true;
			break;
		case InlineItem::LmOnNext:
			fsmRun->matchedToken = item->longestMatchPart->tokenDef->tdLangEl->id;
			fsmRun->returnResult = true;
			break;
		case InlineItem::LmOnLagBehind:
			fsmRun->toklen = fsmRun->tokend;
			fsmRun->matchedToken = item->longestMatchPart->tokenDef->tdLangEl->id;
			fsmRun->returnResult = true;
			fsmRun->skipToklen = true;
			break;
		}
	}

	if ( genAction->markType == MarkMark )
		fsmRun->mark[genAction->markId-1] = fsmRun->p;
}

extern "C" void internalFsmExecute( FsmRun *fsmRun, StreamImpl *inputStream )
{
	int _klen;
	unsigned int _trans;
	const long *_acts;
	unsigned int _nacts;
	const char *_keys;
		
	fsmRun->start = fsmRun->p;

	/* Init the token match to nothing (the sentinal). */
	fsmRun->matchedToken = 0;

/*_resume:*/
	if ( fsmRun->cs == fsmRun->tables->errorState )
		goto out;

	if ( fsmRun->p == fsmRun->pe )
		goto out;

_loop_head:
	_acts = fsmRun->tables->actions + fsmRun->tables->fromStateActions[fsmRun->cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
		execAction( fsmRun, fsmRun->tables->actionSwitch[*_acts++] );

	_keys = fsmRun->tables->transKeys + fsmRun->tables->keyOffsets[fsmRun->cs];
	_trans = fsmRun->tables->indexOffsets[fsmRun->cs];

	_klen = fsmRun->tables->singleLengths[fsmRun->cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*fsmRun->p) < *_mid )
				_upper = _mid - 1;
			else if ( (*fsmRun->p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = fsmRun->tables->rangeLengths[fsmRun->cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*fsmRun->p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*fsmRun->p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += ((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	fsmRun->cs = fsmRun->tables->transTargsWI[_trans];

	if ( fsmRun->tables->transActionsWI[_trans] == 0 )
		goto _again;

	fsmRun->returnResult = false;
	fsmRun->skipToklen = false;
	_acts = fsmRun->tables->actions + fsmRun->tables->transActionsWI[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
		execAction( fsmRun, fsmRun->tables->actionSwitch[*_acts++] );
	if ( fsmRun->returnResult ) {
		if ( fsmRun->skipToklen )
			goto skip_toklen;
		goto final;
	}

_again:
	_acts = fsmRun->tables->actions + fsmRun->tables->toStateActions[fsmRun->cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
		execAction( fsmRun, fsmRun->tables->actionSwitch[*_acts++] );

	if ( fsmRun->cs == fsmRun->tables->errorState )
		goto out;

	if ( ++fsmRun->p != fsmRun->pe )
		goto _loop_head;
out:
	if ( fsmRun->eof ) {
		fsmRun->returnResult = false;
		fsmRun->skipToklen = false;
		_acts = fsmRun->tables->actions + fsmRun->tables->eofActions[fsmRun->cs];
		_nacts = (unsigned int) *_acts++;

		if ( fsmRun->tables->eofTargs[fsmRun->cs] >= 0 )
			fsmRun->cs = fsmRun->tables->eofTargs[fsmRun->cs];

		while ( _nacts-- > 0 )
			execAction( fsmRun, fsmRun->tables->actionSwitch[*_acts++] );
		if ( fsmRun->returnResult ) {
			if ( fsmRun->skipToklen )
				goto skip_toklen;
			goto final;
		}
	}

final:

	if ( fsmRun->p != 0 )
		fsmRun->toklen += fsmRun->p - fsmRun->start;
skip_toklen:
	{}
}
