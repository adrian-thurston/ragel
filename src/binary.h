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

#ifndef _C_BINARY_H
#define _C_BINARY_H

#include <iostream>
#include "codegen.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;

class Binary
	: public CodeGen
{
public:
	Binary( const CodeGenArgs &args );

protected:
	TableArray keyOffsets;
	TableArray singleLens;
	TableArray rangeLens;
	TableArray indexOffsets;
	TableArray indicies;
	TableArray transCondSpacesWi;
	TableArray transOffsetsWi;
	TableArray transLengthsWi;
	TableArray transCondSpaces;
	TableArray transOffsets;
	TableArray transLengths;
	TableArray condTargs;
	TableArray condActions;
	TableArray toStateActions;
	TableArray fromStateActions;
	TableArray eofActions;
	TableArray eofTransDirect;
	TableArray eofTransIndexed;
	TableArray actions;
	TableArray keys;
	TableArray condKeys;
	TableArray nfaTargs;
	TableArray nfaOffsets;
	TableArray nfaPushActions;
	TableArray nfaPopTrans;

	std::ostream &COND_KEYS_v1();
	std::ostream &COND_SPACES_v1();
	std::ostream &INDICIES();
	std::ostream &INDEX_OFFSETS();
	std::ostream &SINGLE_LENS();
	std::ostream &RANGE_LENS();
	std::ostream &TRANS_TARGS_WI();
	std::ostream &ACTIONS_ARRAY();

	void taKeyOffsets();
	void taSingleLens();
	void taRangeLens();
	void taIndexOffsets();
	void taIndicies();
	void taTransCondSpacesWi();
	void taTransOffsetsWi();
	void taTransLengthsWi();
	void taTransCondSpaces();
	void taTransOffsets();
	void taTransLengths();
	void taCondTargs();
	void taCondActions();
	void taToStateActions();
	void taFromStateActions();
	void taEofActions();
	void taEofTransDirect();
	void taEofTransIndexed();
	void taKeys();
	void taActions();
	void taCondKeys();
	void taNfaTargs();
	void taNfaOffsets();
	void taNfaPushActions();
	void taNfaPopTrans();

	void setKeyType();

	void LOCATE_TRANS();
	void LOCATE_COND();

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

	void setTableState( TableArray::State );

	void NFA_PUSH();
	void NFA_POP();
};

#endif
