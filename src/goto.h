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

#ifndef _C_GOTO_H
#define _C_GOTO_H

#include <iostream>
#include "codegen.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;
struct GenStateCond;

struct IpLabel
{
	IpLabel()
	:
		type(None),
		stid(0),
		isReferenced(false)
	{}

	enum Type
	{
		None = 1,
		TestEof,
		Ctr,
		St,
		Out,
		Pop
	};

	std::string reference()
	{
		isReferenced = true;
		return define();
	}

	std::string define()
	{
		std::stringstream ss;
		switch ( type ) {
			case None: break;
			case TestEof:
				ss << "_test_eof" << stid;
				break;
			case Ctr:
				ss << "_ctr" << stid;
				break;
			case St:
				ss << "_st" << stid;
				break;
			case Out:
				ss << "_out" << stid;
				break;
			case Pop:
				ss << "_pop" << stid;
				break;
		}

		return ss.str();
	}

	Type type;
	int stid;
	bool isReferenced;
};


/*
 * Goto driven fsm.
 */
class Goto
	: public CodeGen
{
public:
	enum Type {
		Loop = 1,
		Exp,
		Ip
	};

	Goto( const CodeGenArgs &args, Type type ) 
	:
		CodeGen( args ),
		type(type),
		acts( "_acts" ),
		nacts( "_nacts" ),
		ck( "_ck" ),
		nbreak( "_nbreak" ),
		ps( "_ps" ),
		_out("_out"),
		_pop("_pop"),
		_again("_again"),
		_resume("_resume"),
		_test_eof("_test_eof"),
		actions(           "actions",             *this ),
		toStateActions(    "to_state_actions",    *this ),
		fromStateActions(  "from_state_actions",  *this ),
		eofActions(        "eof_actions",         *this ),
		ctrLabel(0)
	{}

	void tableDataPass();
	virtual void genAnalysis();
	virtual void writeData();
	virtual void writeExec();

	std::ostream &TRANSITION( RedCondPair *pair );

	void FROM_STATE_ACTION_EMIT( RedStateAp *state );

	std::ostream &STATE_CASES();
	std::ostream &TRANSITIONS();

	Type type;

	Variable acts;
	Variable nacts;
	Variable ck;
	Variable nbreak;
	Variable ps;

	GotoLabel _out;
	GotoLabel _pop;
	GotoLabel _again;
	GotoLabel _resume;
	GotoLabel _test_eof;

	TableArray actions;
	TableArray toStateActions;
	TableArray fromStateActions;
	TableArray eofActions;

	IpLabel *ctrLabel;

	void taActions();
	void taToStateActions();
	void taFromStateActions();
	void taEofActions();
	void taNfaTargs();
	void taNfaOffsets();
	void taNfaPushActions();
	void taNfaPopTrans();

	void EOF_CHECK( ostream &ret );

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

	virtual std::ostream &EXEC_FUNCS() = 0;
	virtual std::ostream &TO_STATE_ACTION_SWITCH() = 0;
	virtual std::ostream &FROM_STATE_ACTION_SWITCH() = 0;
	virtual std::ostream &EOF_ACTION_SWITCH() = 0;

	std::ostream &ACTIONS_ARRAY();

	void setTableState( TableArray::State );

	virtual std::ostream &COND_GOTO( RedCondPair *trans );

	string CKEY( CondKey key );
	void COND_B_SEARCH( RedTransAp *trans, CondKey lower, CondKey upper, int low, int high);

	virtual std::ostream &TRANS_GOTO( RedTransAp *trans );

	void SINGLE_SWITCH( RedStateAp *state );
	void RANGE_B_SEARCH( RedStateAp *state, Key lower, Key upper, int low, int high );

	/* Called from STATE_GOTOS just before writing the gotos */
	virtual void GOTO_HEADER( RedStateAp *state );
	virtual void STATE_GOTO_ERROR();

	virtual void NFA_PUSH_ACTION( RedNfaTarg *targ ) = 0;
	virtual void NFA_POP_TEST( RedNfaTarg *targ ) {}
	virtual void NFA_FROM_STATE_ACTION_EXEC() = 0;

	void NFA_POP() {}

	virtual void FROM_STATE_ACTIONS() = 0;
	virtual void TO_STATE_ACTIONS() = 0;
	virtual void REG_ACTIONS() = 0;
	virtual void EOF_ACTIONS() = 0;

	IpLabel *allocateLabels( IpLabel *labels, IpLabel::Type type, int n );
};

#endif
