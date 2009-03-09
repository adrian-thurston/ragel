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

#include "config.h"
#include "fsmrun.h"
#include "redfsm.h"
#include "parsedata.h"
#include "parsetree.h"
#include "pdarun.h"
#include "colm.h"

void FsmRun::execAction( GenAction *genAction )
{
	for ( InlineList::Iter item = *genAction->inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case InlineItem::Text:
			assert(false);
			break;
		case InlineItem::LmSetActId:
			act = item->longestMatchPart->longestMatchId;
			break;
		case InlineItem::LmSetTokEnd:
			tokend = p + 1;
			break;
		case InlineItem::LmInitTokStart:
			assert(false);
			break;
		case InlineItem::LmInitAct:
			act = 0;
			break;
		case InlineItem::LmSetTokStart:
			tokstart = p;
			break;
		case InlineItem::LmSwitch:
			/* If the switch handles error then we also forced the error state. It
			 * will exist. */
			p = tokend;
			if ( item->tokenRegion->lmSwitchHandlesError && act == 0 ) {
				p = tokstart;
				cs = tables->errorState;
			}
			else {
				for ( TokenDefList::Iter lmi = item->tokenRegion->tokenDefList; 
						lmi.lte(); lmi++ )
				{
					if ( lmi->inLmSelect && act == lmi->longestMatchId )
						matchedToken = lmi->token->id;
				}
			}
			returnResult = true;
			break;
		case InlineItem::LmOnLast:
			p += 1;
			matchedToken = item->longestMatchPart->token->id;
			returnResult = true;
			break;
		case InlineItem::LmOnNext:
			matchedToken = item->longestMatchPart->token->id;
			returnResult = true;
			break;
		case InlineItem::LmOnLagBehind:
			p = tokend;
			matchedToken = item->longestMatchPart->token->id;
			returnResult = true;
			break;
		}
	}

	if ( genAction->markType == MarkMark )
		mark[genAction->markId] = p;
}

void FsmRun::execute()
{
	int _klen;
	unsigned int _trans;
	const long *_acts;
	unsigned int _nacts;
	const char *_keys;

	/* Init the token match to nothing (the sentinal). */
	matchedToken = 0;

/*_resume:*/
	if ( cs == tables->errorState )
		goto out;

	if ( p == pe )
		goto out;

_loop_head:
	_acts = tables->actions + tables->fromStateActions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
		execAction( tables->actionSwitch[*_acts++] );

	_keys = tables->transKeys + tables->keyOffsets[cs];
	_trans = tables->indexOffsets[cs];

	_klen = tables->singleLengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = tables->rangeLengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += ((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	cs = tables->transTargsWI[_trans];

	if ( tables->transActionsWI[_trans] == 0 )
		goto _again;

	returnResult = false;
	_acts = tables->actions + tables->transActionsWI[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
		execAction( tables->actionSwitch[*_acts++] );
	if ( returnResult )
		return;

_again:
	_acts = tables->actions + tables->toStateActions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
		execAction( tables->actionSwitch[*_acts++] );

	if ( cs == tables->errorState )
		goto out;

	if ( ++p != pe )
		goto _loop_head;
out:
	if ( p == peof ) {
		returnResult = false;
		_acts = tables->actions + tables->eofActions[cs];
		_nacts = (unsigned int) *_acts++;

		if ( tables->eofTargs[cs] >= 0 )
			cs = tables->eofTargs[cs];

		while ( _nacts-- > 0 )
			execAction( tables->actionSwitch[*_acts++] );
		if ( returnResult )
			return;
	}
}


