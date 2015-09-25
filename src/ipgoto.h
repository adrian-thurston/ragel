/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
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

#ifndef IPGOTO_H
#define IPGOTO_H

#include <iostream>
#include "goto.h"

/* Forwards. */
struct CodeGenData;

/*
 * class FGotoCodeGen
 */
class IpGoto
	: public Goto
{
public:
	IpGoto( const CodeGenArgs &args ) 
			: Goto(args) {}

	std::ostream &EXIT_STATES();
	std::ostream &TRANS_GOTO( RedTransAp *trans, int level );
	std::ostream &COND_GOTO( RedCondPair *trans, int level );
	std::ostream &FINISH_CASES();
	std::ostream &AGAIN_CASES();
	std::ostream &STATE_GOTOS();
	std::ostream &STATE_GOTO_CASES();

	void GOTO( ostream &ret, int gotoDest, bool inFinish );
	void CALL( ostream &ret, int callDest, int targState, bool inFinish );
	void NCALL( ostream &ret, int callDest, int targState, bool inFinish );
	void NEXT( ostream &ret, int nextDest, bool inFinish );
	void GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void RET( ostream &ret, bool inFinish );
	void NRET( ostream &ret, bool inFinish );
	void CURS( ostream &ret, bool inFinish );
	void TARGS( ostream &ret, bool inFinish, int targState );
	void BREAK( ostream &ret, int targState, bool csForced );
	void NBREAK( ostream &ret, int targState, bool csForced );

	virtual void genAnalysis();
	virtual void writeData();
	virtual void writeExec();

protected:
	bool useAgainLabel();

	/* Called from Goto::STATE_GOTOS just before writing the gotos for
	 * each state. */
	bool IN_TRANS_ACTIONS( RedStateAp *state );
	void GOTO_HEADER( RedStateAp *state );
	void STATE_GOTO_ERROR();

	/* Set up labelNeeded flag for each state. */
	void setLabelsNeeded( RedCondPair *pair );
	void setLabelsNeeded( GenInlineList *inlineList );
	void setLabelsNeeded();

	void NFA_PUSH_ACTION( RedNfaTarg *targ );
	void NFA_POP_TEST( RedNfaTarg *targ );

	void NFA_PUSH( RedStateAp *state );

	void tableDataPass();
};

namespace C
{
	class IpGoto
	:
		public ::IpGoto
	{
	public:
		IpGoto( const CodeGenArgs &args )
			: ::IpGoto( args )
		{}
	};
}

#endif
