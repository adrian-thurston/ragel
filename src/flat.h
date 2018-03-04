/*
 * Copyright 2004-2018 Adrian Thurston <thurston@colm.net>
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

#ifndef _C_FLAT_H
#define _C_FLAT_H

#include <iostream>
#include "codegen.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;

class Flat
	: public CodeGen
{
public:
	Flat( const CodeGenArgs &args );

	virtual ~Flat() { }

protected:
	TableArray actions;
	TableArray keys;
	TableArray charClass;
	TableArray flatIndexOffset;
	TableArray indicies;
	TableArray indexDefaults;
	TableArray transCondSpaces;
	TableArray transOffsets;
	TableArray condTargs;
	TableArray condActions;
	TableArray toStateActions;
	TableArray fromStateActions;
	TableArray eofActions;
	TableArray eofTrans;
	TableArray nfaTargs;
	TableArray nfaOffsets;
	TableArray nfaPushActions;
	TableArray nfaPopTrans;

	void taKeys();
	void taKeySpans();
	void taCharClass();
	void taActions();
	void taFlatIndexOffset();
	void taIndicies();
	void taIndexDefaults();
	void taTransCondSpaces();
	void taTransOffsets();
	void taCondTargs();
	void taCondActions();
	void taToStateActions();
	void taFromStateActions();
	void taEofActions();
	void taEofTrans();
	void taNfaTargs();
	void taNfaOffsets();
	void taNfaPushActions();
	void taNfaPopTrans();

	void setKeyType();

	std::ostream &INDICIES();
	std::ostream &TO_STATE_ACTIONS();
	std::ostream &FROM_STATE_ACTIONS();
	std::ostream &EOF_ACTIONS();
	std::ostream &EOF_TRANS();
	std::ostream &TRANS_COND_SPACES();
	std::ostream &TRANS_OFFSETS();
	std::ostream &TRANS_LENGTHS();
	std::ostream &COND_KEYS();
	std::ostream &COND_TARGS();
	std::ostream &COND_ACTIONS();

	void LOCATE_TRANS();

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

	virtual void TO_STATE_ACTION( RedStateAp *state ) = 0;
	virtual void FROM_STATE_ACTION( RedStateAp *state ) = 0;
	virtual void EOF_ACTION( RedStateAp *state ) = 0;
	virtual void COND_ACTION( RedCondPair *cond ) = 0;
	virtual void NFA_PUSH_ACTION( RedNfaTarg *targ ) = 0;
	virtual void NFA_POP_TEST( RedNfaTarg *targ ) = 0;
	virtual void NFA_FROM_STATE_ACTION_EXEC() = 0;

	virtual void setTableState( TableArray::State );

	void NFA_PUSH();
	void NFA_POP();
};

#endif
