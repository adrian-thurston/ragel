/*
 * Copyright 2001-2014 Adrian Thurston <thurston@colm.net>
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

#ifndef _GOTO_H
#define _GOTO_H

#include <iostream>
#include "codegen.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;
struct GenStateCond;

/*
 * Goto driven fsm.
 */
class Goto
	: public CodeGen
{
public:
	Goto( const CodeGenArgs &args );

	std::ostream &TRANSITION( RedCondPair *pair );

	std::ostream &STATE_GOTOS();
	std::ostream &TRANSITIONS();
	std::ostream &FINISH_CASES();

	TableArray actions;
	TableArray toStateActions;
	TableArray fromStateActions;
	TableArray eofActions;
	TableArray nfaTargs;
	TableArray nfaOffsets;
	TableArray nfaPushActions;
	TableArray nfaPopTrans;

	void taActions();
	void taToStateActions();
	void taFromStateActions();
	void taEofActions();
	void taNfaTargs();
	void taNfaOffsets();
	void taNfaPushActions();
	void taNfaPopTrans();

	void GOTO( ostream &ret, int gotoDest, bool inFinish );
	void CALL( ostream &ret, int callDest, int targState, bool inFinish );
	void NCALL( ostream &ret, int callDest, int targState, bool inFinish );
	void NEXT( ostream &ret, int nextDest, bool inFinish );
	void GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void CURS( ostream &ret, bool inFinish );
	void TARGS( ostream &ret, bool inFinish, int targState );
	void RET( ostream &ret, bool inFinish );
	void NRET( ostream &ret, bool inFinish );
	void BREAK( ostream &ret, int targState, bool csForced );
	void NBREAK( ostream &ret, int targState, bool csForced );

	virtual unsigned int TO_STATE_ACTION( RedStateAp *state );
	virtual unsigned int FROM_STATE_ACTION( RedStateAp *state );
	virtual unsigned int EOF_ACTION( RedStateAp *state );

	std::ostream &ACTIONS_ARRAY();

	std::ostream &TO_STATE_ACTIONS();
	std::ostream &FROM_STATE_ACTIONS();
	std::ostream &EOF_ACTIONS();

	void setTableState( TableArray::State );

	virtual std::ostream &COND_GOTO( RedCondPair *trans, int level );

	string CKEY( CondKey key );
	void COND_B_SEARCH( RedTransAp *trans, int level, CondKey lower, CondKey upper, int low, int high);

	virtual std::ostream &TRANS_GOTO( RedTransAp *trans, int level );

	void SINGLE_SWITCH( RedStateAp *state );
	void RANGE_B_SEARCH( RedStateAp *state, int level, Key lower, Key upper, int low, int high );

	/* Called from STATE_GOTOS just before writing the gotos */
	virtual void GOTO_HEADER( RedStateAp *state );
	virtual void STATE_GOTO_ERROR();

	virtual void NFA_PUSH_ACTION( RedNfaTarg *targ ) = 0;
	virtual void NFA_POP_TEST( RedNfaTarg *targ ) {}
	virtual void NFA_FROM_STATE_ACTION_EXEC() = 0;

	void NFA_PUSH();
	void NFA_POP();
};

#endif
