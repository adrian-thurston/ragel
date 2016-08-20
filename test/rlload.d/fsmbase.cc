/*
 *  Copyright 2001-2016 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "fsmgraph.h"
#include "parsedata.h"

#include <string.h>
#include <assert.h>
#include <iostream>

FsmCtx::FsmCtx( FsmGbl *fsmGbl )
:
	minimizeLevel(fsmGbl->minimizeLevel),
	minimizeOpt(fsmGbl->minimizeOpt),

	/* No limit. */
	stateLimit(STATE_UNLIMITED),

	printStatistics(fsmGbl->printStatistics),

	checkPriorInteraction(fsmGbl->checkPriorInteraction),

	unionOp(false),

	condsCheckDepth(0),

	curActionOrd(0),
	curPriorOrd(0),

	nextPriorKey(0),
	nextCondId(0),

	fsmGbl(fsmGbl),
	generatingSectionSubset(false),
	lmRequiresErrorState(false),
	nameIndex(0),

	getKeyExpr(0),
	accessExpr(0),
	prePushExpr(0),
	postPopExpr(0),
	nfaPrePushExpr(0),
	nfaPostPopExpr(0),
	pExpr(0),
	peExpr(0),
	eofExpr(0),
	csExpr(0),
	topExpr(0),
	stackExpr(0),
	actExpr(0),
	tokstartExpr(0),
	tokendExpr(0),
	dataExpr(0)
{
	keyOps = new KeyOps;
	condData = new CondData;
}

FsmCtx::~FsmCtx()
{
	delete keyOps;
	delete condData;
	priorDescList.empty();

	actionList.empty();

	if ( getKeyExpr != 0 )
		delete getKeyExpr;
	if ( accessExpr != 0 )
		delete accessExpr;
	if ( prePushExpr != 0 )
		delete prePushExpr;
	if ( postPopExpr != 0 )
		delete postPopExpr;
	if ( nfaPrePushExpr != 0 )
		delete nfaPrePushExpr;
	if ( nfaPostPopExpr != 0 )
		delete nfaPostPopExpr;
	if ( pExpr != 0 )
		delete pExpr;
	if ( peExpr != 0 )
		delete peExpr;
	if ( eofExpr != 0 )
		delete eofExpr;
	if ( csExpr != 0 )
		delete csExpr;
	if ( topExpr != 0 )
		delete topExpr;
	if ( stackExpr != 0 )
		delete stackExpr;
	if ( actExpr != 0 )
		delete actExpr;
	if ( tokstartExpr != 0 )
		delete tokstartExpr;
	if ( tokendExpr != 0 )
		delete tokendExpr;
	if ( dataExpr != 0 )
		delete dataExpr;
}

