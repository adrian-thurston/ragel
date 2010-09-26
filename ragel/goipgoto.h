/*
 *  Copyright 2010 Justine Tunney <jtunney@gmail.com>
 *            2001-2006 Adrian Thurston <thurston@complang.org>
 *            2004 Erich Ocean <eric.ocean@ampede.com>
 *            2005 Alan West <alan@alanz.com>
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

#ifndef _GOIPGOTO_H
#define _GOIPGOTO_H

#include "cdipgoto.h"

/*
 * class GoIpGotoCodeGen
 *
 * Keep overriding methods until it works.
 */
struct GoIpGotoCodeGen
	: public IpGotoCodeGen, public CCodeGen
{
public:
	GoIpGotoCodeGen( ostream &out ) :
		FsmCodeGen(out), IpGotoCodeGen(out), CCodeGen(out) {}

	void writeExec();
	void writeData();
	void writeInit();
	void STATE_IDS();
	void CALL( ostream &ret, int callDest, int targState, bool inFinish );
	void CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void RET( ostream &ret, bool inFinish );
	void GOTO_HEADER( RedStateAp *state );
	void LM_SWITCH( ostream &ret, GenInlineItem *item, 
			int targState, int inFinish, bool csForced );
	string GET_KEY();
	void emitSingleSwitch( RedStateAp *state );
	void emitRangeBSearch( RedStateAp *state, int level, int low, int high );
	void COND_TRANSLATE( GenStateCond *stateCond, int level );
	ostream &STATE_GOTOS();
	ostream &STATIC_VAR( string type, string name );
	ostream &TRANS_GOTO( RedTransAp *trans, int level );
	void genLineDirective( ostream &out );
	bool IN_TRANS_ACTIONS( RedStateAp *state );
	void ACTION( ostream &ret, GenAction *action, int targState, 
		     bool inFinish, bool csForced );
	void CONDITION( ostream &ret, GenAction *condition );
	ostream &FINISH_CASES();

protected:
	bool isFirstCase;
};

#endif
