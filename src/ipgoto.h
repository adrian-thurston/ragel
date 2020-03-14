/*
 * Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
	:
		Goto( args, Ip ),
		stLabel(0),
		ctrLabel(0),
		outLabel(0),
		popLabel(0)
	{}

	std::ostream &EXIT_STATES();
	std::ostream &TRANS_GOTO( RedTransAp *trans );
	std::ostream &COND_GOTO( RedCondPair *trans );
	std::ostream &FINISH_CASES();
	std::ostream &AGAIN_CASES();
	std::ostream &STATE_GOTOS();
	std::ostream &STATE_GOTO_CASES();

	/* unused. */
	virtual std::ostream &ACTION_SWITCH() { return out; }
	virtual std::ostream &EXEC_FUNCS() { return out; }
	virtual std::ostream &TO_STATE_ACTION_SWITCH() { return out; }
	virtual std::ostream &FROM_STATE_ACTION_SWITCH() { return out; }
	virtual std::ostream &EOF_ACTION_SWITCH() { return out; }

	/* Unused */
	virtual void FROM_STATE_ACTIONS() {}
	virtual void TO_STATE_ACTIONS() {}
	virtual void REG_ACTIONS() {}
	virtual void EOF_ACTIONS() {}

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
	virtual void NFA_FROM_STATE_ACTION_EXEC();

	void NFA_PUSH_ST( RedStateAp *state );

	void tableDataPass();

	IpLabel *stLabel;
	IpLabel *ctrLabel;
	IpLabel *outLabel;
	IpLabel *popLabel;
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
