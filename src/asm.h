/*
 *  Copyright 2014-2015 Adrian Thurston <thurston@complang.org>
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

#ifndef _ASM_H
#define _ASM_H

#include <iostream>
#include <string>
#include <iomanip>
#include <stdio.h>

#include "common.h"
#include "gendata.h"
#include "ragel.h"

using std::string;
using std::ostream;

/* Integer array line length. */
#define IALL_INTEGRAL 8
#define IALL_STRING 128

/* Forwards. */
struct RedFsmAp;
struct RedStateAp;
struct CodeGenData;
struct GenAction;
struct NameInst;
struct GenInlineItem;
struct GenInlineList;
struct RedAction;
struct LongestMatch;
struct LongestMatchPart;
struct AsmCodeGen;
struct RedTransAp;
struct RedStateAp;
struct GenStateCond;

string itoa( int i );

/*
 * class AsmCodeGen
 */
class AsmCodeGen : public CodeGenData
{
public:
	AsmCodeGen( const CodeGenArgs &args );
	virtual ~AsmCodeGen() {}

	virtual void writeInit();
	virtual void writeStart();
	virtual void writeFirstFinal();
	virtual void writeError();

	virtual void statsSummary() {}
	virtual void genAnalysis();

protected:
	string FSM_NAME();
	string START_STATE_ID();
	string TABS( int level );
	string KEY( Key key );
	string COND_KEY( CondKey key );
	string LDIR_PATH( char *path );
	virtual void ACTION( ostream &ret, GenAction *action, int targState, 
			bool inFinish, bool csForced );
	void CONDITION( ostream &ret, GenAction *condition );
	void NFA_CONDITION( ostream &ret, GenAction *condition, bool last );
	string ALPH_TYPE();
	string ARRAY_TYPE( unsigned long maxVal );

	bool isAlphTypeSigned();

	string GET_KEY();

	string P();
	string PE();
	string vEOF();
	string NBREAK();

	string ACCESS();
	string vCS();
	string STACK();
	string TOP();
	string TOKSTART();
	string TOKEND();
	string ACT();

	string NFA_STACK();
	string NFA_TOP();
	string NFA_SZ();

	string DATA_PREFIX();
	string PM() { return "_" + DATA_PREFIX() + "partition_map"; }
	string C() { return "_" + DATA_PREFIX() + "cond_spaces"; }
	string CK() { return "_" + DATA_PREFIX() + "cond_keys"; }
	string K() { return "_" + DATA_PREFIX() + "trans_keys"; }
	string I() { return "_" + DATA_PREFIX() + "indicies"; }
	string CO() { return "_" + DATA_PREFIX() + "cond_offsets"; }
	string KO() { return "_" + DATA_PREFIX() + "key_offsets"; }
	string IO() { return "_" + DATA_PREFIX() + "index_offsets"; }
	string CL() { return "_" + DATA_PREFIX() + "cond_lengths"; }
	string SL() { return "_" + DATA_PREFIX() + "single_lengths"; }
	string RL() { return "_" + DATA_PREFIX() + "range_lengths"; }
	string A() { return "_" + DATA_PREFIX() + "actions"; }
	string TA() { return "_" + DATA_PREFIX() + "trans_actions"; }
	string TT() { return "_" + DATA_PREFIX() + "trans_targs"; }
	string TSA() { return "_" + DATA_PREFIX() + "to_state_actions"; }
	string FSA() { return "_" + DATA_PREFIX() + "from_state_actions"; }
	string EA() { return "_" + DATA_PREFIX() + "eof_actions"; }
	string ET() { return "_" + DATA_PREFIX() + "eof_trans"; }
	string SP() { return "_" + DATA_PREFIX() + "key_spans"; }
	string CSP() { return "_" + DATA_PREFIX() + "cond_key_spans"; }
	string START() { return DATA_PREFIX() + "start"; }
	string ERROR() { return DATA_PREFIX() + "error"; }
	string FIRST_FINAL() { return DATA_PREFIX() + "first_final"; }
	string CTXDATA() { return DATA_PREFIX() + "ctxdata"; }

	string LABEL( const char *type, long i );
	string LABEL( const char *name );

	void INLINE_LIST( ostream &ret, GenInlineList *inlineList, 
			int targState, bool inFinish, bool csForced );
	void EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish );
	void LM_SWITCH( ostream &ret, GenInlineItem *item, int targState, 
			int inFinish, bool csForced );
	void SET_ACT( ostream &ret, GenInlineItem *item );
	void INIT_TOKSTART( ostream &ret, GenInlineItem *item );
	void INIT_ACT( ostream &ret, GenInlineItem *item );
	void SET_TOKSTART( ostream &ret, GenInlineItem *item );
	void SET_TOKEND( ostream &ret, GenInlineItem *item );
	void GET_TOKEND( ostream &ret, GenInlineItem *item );
	void STATIC_CONST_INT( const string &name, const string &val );
	void STATE_IDS();

	string ERROR_STATE();
	string FIRST_FINAL_STATE();

	ostream &source_warning(const InputLoc &loc);
	ostream &source_error(const InputLoc &loc);

	unsigned int arrayTypeSize( unsigned long maxVal );

	bool outLabelUsed;
	bool testEofUsed;
	bool againLabelUsed;
	bool useIndicies;
	long nextLmSwitchLabel;
	bool stackCS;

	void genLineDirective( ostream &out );

	/* Return types in HostType form. */
	HostType *arrayType( unsigned long maxVal );

	void NBREAK( ostream &ret, int targState, bool csForced );
	void NCALL( ostream &ret, int callDest, int targState, bool inFinish );
	void NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void NRET( ostream &ret, bool inFinish );

	void HOST_STMT( ostream &ret, GenInlineItem *item, 
			int targState, bool inFinish, bool csForced );
	void HOST_EXPR( ostream &ret, GenInlineItem *item, 
			int targState, bool inFinish, bool csForced );
	void HOST_TEXT( ostream &ret, GenInlineItem *item, 
			int targState, bool inFinish, bool csForced );
	void GEN_STMT( ostream &ret, GenInlineItem *item, 
			int targState, bool inFinish, bool csForced );
	void GEN_EXPR( ostream &ret, GenInlineItem *item, 
			int targState, bool inFinish, bool csForced );

public:
	/* Determine if we should use indicies. */
	virtual void calcIndexSize() {}

	virtual string NULL_ITEM();
	virtual string POINTER();
	virtual ostream &SWITCH_DEFAULT();
	virtual ostream &OPEN_ARRAY( string type, string name );
	virtual ostream &CLOSE_ARRAY();
	virtual ostream &STATIC_VAR( string type, string name );
	virtual string ARR_OFF( string ptr, string offset );
	virtual string CAST( string type );
	virtual string UINT();
	virtual string PTR_CONST();
	virtual string PTR_CONST_END();
	virtual string CTRL_FLOW();

	virtual void writeExports();

	unsigned int TO_STATE_ACTION( RedStateAp *state );
	unsigned int FROM_STATE_ACTION( RedStateAp *state );
	unsigned int EOF_ACTION( RedStateAp *state );

	void COND_TRANSLATE( GenStateCond *stateCond, int level );
	void STATE_CONDS( RedStateAp *state, bool genDefault ); 

	std::ostream &EXIT_STATES();
	std::string TRANS_GOTO_TARG( RedTransAp *trans );
	std::string TRANS_GOTO_TARG( RedCondPair *pair );
	std::ostream &TRANS_GOTO( RedTransAp *trans );
	std::ostream &AGAIN_CASES();
	std::ostream &FINISH_CASES();
	std::ostream &ENTRY_CASES();

	void GOTO( ostream &ret, int gotoDest, bool inFinish );
	void CALL( ostream &ret, int callDest, int targState, bool inFinish );
	void NEXT( ostream &ret, int nextDest, bool inFinish );
	void GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void RET( ostream &ret, bool inFinish );
	void CURS( ostream &ret, bool inFinish );
	void TARGS( ostream &ret, bool inFinish, int targState );
	void BREAK( ostream &ret, int targState, bool csForced );
	void LM_EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish );

	virtual void writeData();
	virtual void writeExec();

	bool useAgainLabel();

	void NFA_PUSH( RedStateAp *state );

	/* Called from GotoCodeGen::STATE_GOTOS just before writing the gotos for
	 * each state. */
	bool IN_TRANS_ACTIONS( RedStateAp *state );
	void GOTO_HEADER( RedStateAp *state );
	void STATE_GOTO_ERROR();
	std::ostream &STATE_GOTOS();

	void emitSingleIfElseIf( RedStateAp *state );
	void emitSingleJumpTable( RedStateAp *state, std::string def );
	void emitRangeBSearch( RedStateAp *state, int level, int low, int high );
	void emitCharClassIfElseIf( RedStateAp *state );
	void emitCharClassJumpTable( RedStateAp *state, std::string def );

	/* Set up labelNeeded flag for each state. */
	void setLabelsNeeded( RedCondPair *pair );
	void setLabelsNeeded( GenInlineList *inlineList );
	void setLabelsNeeded();

	void setNfaIds();
};

#endif
