/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@complang.org>
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
struct AsmFsmCodeGen;
struct RedTransAp;
struct RedStateAp;
struct GenStateCond;

string itoa( int i );

struct AsmTableArray
{
	AsmTableArray( AsmFsmCodeGen &codeGen, HostType *hostType, std::string name );

	void OPEN();
	void CLOSE();

	void fmt()
	{
		if ( ! first ) {
			if ( str ) {
				if ( ln % iall == 0 ) {
					out << "\"\n\t\"";
					ln = 0;
				}
			}
			else {
				out << ", ";

				if ( ln % iall == 0 ) {
					out << "\n\t";
					ln = 0;
				}
			}
		}
		ln += 1;
		first = false;
		count += 1;
	}

	void SVAL( long long value )
	{
		char c;
		short h;
		int i;
		long l;
		unsigned char *p = 0;
		int n = 0;
		switch ( hostType->size ) {
			case sizeof( char ):
				c = value;
				p = (unsigned char *)&c;
				n = sizeof(char);
				break;
			case sizeof( short ):
				h = value;
				p = (unsigned char *)&h;
				n = sizeof(short);
				break;
			case sizeof( int ):
				i = value;
				p = (unsigned char *)&i;
				n = sizeof(int);
				break;
			case sizeof( long ):
				l = value;
				p = (unsigned char *)&l;
				n = sizeof(long);
				break;
		}

		std::ios_base::fmtflags prevFlags = out.flags( std::ios::hex );
		int prevFill = out.fill( '0' );    

		while ( n-- > 0 ) {
			out << '\\';
			out << 'x';
			out << std::setw(2) << (unsigned int) *p++;
		}

		out.flags( prevFlags );
		out.fill( prevFill );
	}

	void VAL( long long ll ) { fmt(); if (str) SVAL(ll); else out << ll; }
	void VAL( long l )       { fmt(); if (str) SVAL(l);  else out << l; }
	void VAL( int i )        { fmt(); if (str) SVAL(i);  else out << i; }
	void VAL( short s )      { fmt(); if (str) SVAL(s);  else out << s; }
	void VAL( char c )       { fmt(); if (str) SVAL(c);  else out << c; }

	void VAL( unsigned long long ull ) { fmt(); if (str) SVAL(ull); else out << ull; }
	void VAL( unsigned long ul )       { fmt(); if (str) SVAL(ul);  else out << ul; }
	void VAL( unsigned int ui )        { fmt(); if (str) SVAL(ui);  else out << ui; }
	void VAL( unsigned short us )      { fmt(); if (str) SVAL(us);  else out << us; }
	void VAL( unsigned char uc )       { fmt(); if (str) SVAL(uc);  else out << uc; }

	void KEY( Key key )
	{
		fmt();
		if ( str )
			SVAL( key.getVal() );
		else {
			if ( keyOps->isSigned || !hostLang->explicitUnsigned )
				out << key.getVal();
			else
				out << (unsigned long) key.getVal() << 'u';
		}
	}

	AsmFsmCodeGen &codeGen;
	HostType *hostType;
	std::string name;
	ostream &out;
	int iall;
	bool first;
	long ln;
	bool str;
	long long count;
};

/*
 * class AsmFsmCodeGen
 */
class AsmFsmCodeGen : public CodeGenData
{
public:
	AsmFsmCodeGen( ostream &out );
	virtual ~AsmFsmCodeGen() {}

	virtual void finishRagelDef();
	virtual void writeInit();
	virtual void writeStart();
	virtual void writeFirstFinal();
	virtual void writeError();

	virtual void statsSummary();

protected:
	friend AsmTableArray;

	string FSM_NAME();
	string START_STATE_ID();
	ostream &ACTIONS_ARRAY();
	string GET_WIDE_KEY();
	string GET_WIDE_KEY( RedStateAp *state );
	string TABS( int level );
	string KEY( Key key );
	string WIDE_KEY( RedStateAp *state, Key key );
	string LDIR_PATH( char *path );
	virtual void ACTION( ostream &ret, GenAction *action, int targState, 
			bool inFinish, bool csForced );
	void CONDITION( ostream &ret, GenAction *condition );
	string ALPH_TYPE();
	string WIDE_ALPH_TYPE();
	string ARRAY_TYPE( unsigned long maxVal );

	bool isAlphTypeSigned();
	bool isWideAlphTypeSigned();

	virtual string ARR_OFF( string ptr, string offset ) = 0;
	virtual string CAST( string type ) = 0;
	virtual string UINT() = 0;
	virtual string NULL_ITEM() = 0;
	virtual string POINTER() = 0;
	virtual string GET_KEY();
	virtual ostream &SWITCH_DEFAULT() = 0;

	string P();
	string PE();
	string vEOF();

	string ACCESS();
	string vCS();
	string STACK();
	string TOP();
	string TOKSTART();
	string TOKEND();
	string ACT();

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

	void INLINE_LIST( ostream &ret, GenInlineList *inlineList, 
			int targState, bool inFinish, bool csForced );
	virtual void GOTO( ostream &ret, int gotoDest, bool inFinish ) = 0;
	virtual void CALL( ostream &ret, int callDest, int targState, bool inFinish ) = 0;
	virtual void NEXT( ostream &ret, int nextDest, bool inFinish ) = 0;
	virtual void GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish ) = 0;
	virtual void NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish ) = 0;
	virtual void CALL_EXPR( ostream &ret, GenInlineItem *ilItem, 
			int targState, bool inFinish ) = 0;
	virtual void RET( ostream &ret, bool inFinish ) = 0;
	virtual void BREAK( ostream &ret, int targState, bool csForced ) = 0;
	virtual void CURS( ostream &ret, bool inFinish ) = 0;
	virtual void TARGS( ostream &ret, bool inFinish, int targState ) = 0;
	void EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish );
	void LM_SWITCH( ostream &ret, GenInlineItem *item, int targState, 
			int inFinish, bool csForced );
	void SET_ACT( ostream &ret, GenInlineItem *item );
	void INIT_TOKSTART( ostream &ret, GenInlineItem *item );
	void INIT_ACT( ostream &ret, GenInlineItem *item );
	void SET_TOKSTART( ostream &ret, GenInlineItem *item );
	void SET_TOKEND( ostream &ret, GenInlineItem *item );
	void GET_TOKEND( ostream &ret, GenInlineItem *item );
	virtual void SUB_ACTION( ostream &ret, GenInlineItem *item, 
			int targState, bool inFinish, bool csForced );
	void STATIC_CONST_INT( const string &name, const string &val );
	void STATE_IDS();

	string ERROR_STATE();
	string FIRST_FINAL_STATE();

	virtual string PTR_CONST() = 0;
	virtual string PTR_CONST_END() = 0;
	virtual ostream &OPEN_ARRAY( string type, string name ) = 0;
	virtual ostream &CLOSE_ARRAY() = 0;
	virtual ostream &STATIC_VAR( string type, string name ) = 0;

	virtual string CTRL_FLOW() = 0;

	ostream &source_warning(const InputLoc &loc);
	ostream &source_error(const InputLoc &loc);

	unsigned int arrayTypeSize( unsigned long maxVal );

	bool outLabelUsed;
	bool testEofUsed;
	bool againLabelUsed;
	bool useIndicies;
	long long tableData;

	void genLineDirective( ostream &out );

	/* Return types in HostType form. */
	HostType *wideAlphType();
	HostType *arrayType( unsigned long maxVal );

public:
	/* Determine if we should use indicies. */
	virtual void calcIndexSize() {}
};

class AsmCodeGen : public AsmFsmCodeGen
{
public:
	AsmCodeGen( ostream &out ) : AsmFsmCodeGen(out) {}

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
};


class AsmGotoCodeGen : public AsmCodeGen
{
public:
	AsmGotoCodeGen( ostream &out ) : AsmCodeGen(out) {}
	std::ostream &TO_STATE_ACTION_SWITCH();
	std::ostream &FROM_STATE_ACTION_SWITCH();
	std::ostream &EOF_ACTION_SWITCH();
	std::ostream &ACTION_SWITCH();
	std::ostream &STATE_GOTOS();
	std::ostream &TRANSITIONS();
	std::ostream &EXEC_FUNCS();
	std::ostream &FINISH_CASES();

	void GOTO( ostream &ret, int gotoDest, bool inFinish );
	void CALL( ostream &ret, int callDest, int targState, bool inFinish );
	void NEXT( ostream &ret, int nextDest, bool inFinish );
	void GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void CURS( ostream &ret, bool inFinish );
	void TARGS( ostream &ret, bool inFinish, int targState );
	void RET( ostream &ret, bool inFinish );
	void BREAK( ostream &ret, int targState, bool csForced );

	unsigned int TO_STATE_ACTION( RedStateAp *state );
	unsigned int FROM_STATE_ACTION( RedStateAp *state );
	unsigned int EOF_ACTION( RedStateAp *state );

	std::ostream &TO_STATE_ACTIONS();
	std::ostream &FROM_STATE_ACTIONS();
	std::ostream &EOF_ACTIONS();

	void COND_TRANSLATE( GenStateCond *stateCond, int level );
	void emitCondBSearch( RedStateAp *state, int level, int low, int high );
	void STATE_CONDS( RedStateAp *state, bool genDefault ); 

	virtual std::ostream &TRANS_GOTO( RedTransAp *trans, int level );

	void emitSingleSwitch( RedStateAp *state );
	void emitRangeBSearch( RedStateAp *state, int level, int low, int high );

	/* Called from STATE_GOTOS just before writing the gotos */
	virtual void GOTO_HEADER( RedStateAp *state );
	virtual void STATE_GOTO_ERROR();
};


class AsmIpGotoCodeGen : public AsmGotoCodeGen
{
public:
	AsmIpGotoCodeGen( ostream &out ) : AsmGotoCodeGen(out) {}

	std::ostream &EXIT_STATES();
	std::ostream &TRANS_GOTO( RedTransAp *trans, int level );
	std::ostream &FINISH_CASES();
	std::ostream &AGAIN_CASES();

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

	virtual void writeData();
	virtual void writeExec();

protected:
	bool useAgainLabel();

	/* Called from GotoCodeGen::STATE_GOTOS just before writing the gotos for
	 * each state. */
	bool IN_TRANS_ACTIONS( RedStateAp *state );
	void GOTO_HEADER( RedStateAp *state );
	void STATE_GOTO_ERROR();

	/* Set up labelNeeded flag for each state. */
	void setLabelsNeeded( GenInlineList *inlineList );
	void setLabelsNeeded();
};

#endif
