/*
 *  Copyright 2004-2014 Adrian Thurston <thurston@complang.org>
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
	TableArray keySpans;
	TableArray flatIndexOffset;
	TableArray indicies;
	TableArray transCondSpaces;
	TableArray transOffsets;
	TableArray transLengths;
	TableArray condKeys;
	TableArray condTargs;
	TableArray condActions;
	TableArray toStateActions;
	TableArray fromStateActions;
	TableArray eofActions;
	TableArray eofTrans;

	void taKeys();
	void taKeySpans();
	void taActions();
	void taFlatIndexOffset();
	void taIndicies();
	void taTransCondSpaces();
	void taTransOffsets();
	void taTransLengths();
	void taCondKeys();
	void taCondTargs();
	void taCondActions();
	void taToStateActions();
	void taFromStateActions();
	void taEofActions();
	void taEofTrans();

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
	virtual void COND_ACTION( RedCondAp *cond ) = 0;

	virtual void setTableState( TableArray::State );
};

#endif
