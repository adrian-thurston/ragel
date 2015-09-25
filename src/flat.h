/*
 *  Copyright 2004-2014 Adrian Thurston <thurston@complang.org>
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

	virtual void setTableState( TableArray::State );

	void NFA_PUSH();
	void NFA_POP();
};

#endif
