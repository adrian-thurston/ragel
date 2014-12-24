/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
 */

#ifndef _C_CODEGEN_H
#define _C_CODEGEN_H

#include <iostream>
#include <string>
#include <stdio.h>
#include "common.h"
#include "gendata.h"
#include "vector.h"

using std::string;
using std::ostream;

/* Integer array line length. */
#define IALL 8

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

string itoa( int i );

struct TableArray;
typedef Vector<TableArray*> ArrayVector;
struct CodeGen;

extern bool directBackend;

struct TableArray
{
	enum State {
		InitialState = 1,
		AnalyzePass,
		GeneratePass
	};
		
	TableArray( const char *name, CodeGen &codeGen );

	void start();
	void startAnalyze();
	void startGenerate();

	void setType( std::string type, int width, bool isChar )
	{
		this->type = type;
		this->width = width;
		this->isChar = isChar;
	}

	std::string ref() const;

	void value( long long v );

	void valueAnalyze( long long v );
	void valueGenerate( long long v );

	void finish();
	void finishAnalyze();
	void finishGenerate();

	void setState( TableArray::State state )
		{ this->state = state; }

	long long size();

	State state;
	const char *name;
	std::string type;
	int width;
	bool isSigned;
	bool isChar;
	long long values;
	long long min;
	long long max;
	CodeGen &codeGen;
	std::ostream &out;
	int ln;
};

struct IlOpts
{
	IlOpts( int targState, bool inFinish, bool csForced )
		: targState(targState), inFinish(inFinish), csForced(csForced) {}

	int targState;
	bool inFinish;
	bool csForced;
};


/*
 * class CodeGen
 */
class CodeGen : public CodeGenData
{
public:
	CodeGen( const CodeGenArgs &args );

	virtual ~CodeGen() {}

	virtual void writeInit();
	virtual void writeStart();
	virtual void writeFirstFinal();
	virtual void writeError();

protected:
	friend class TableArray;
	typedef Vector<TableArray*> ArrayVector;
	ArrayVector arrayVector;

	string FSM_NAME();
	string START_STATE_ID();
	void taActions();
	string TABS( int level );
	string KEY( Key key );
	string LDIR_PATH( char *path );

	void ACTION( ostream &ret, GenAction *action, IlOpts opts );
	void CONDITION( ostream &ret, GenAction *condition );
	string ALPH_TYPE();
	string ARRAY_TYPE( unsigned long maxVal );

	bool isAlphTypeSigned();
	bool isWideAlphTypeSigned();

	virtual string GET_KEY();

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
	string START() { return DATA_PREFIX() + "start"; }
	string ERROR() { return DATA_PREFIX() + "error"; }
	string FIRST_FINAL() { return DATA_PREFIX() + "first_final"; }

	string ARR_TYPE( const TableArray &ta )
		{ return ta.type; }

	string ARR_REF( const TableArray &ta )
		{ return ta.ref(); }

	void INLINE_LIST( ostream &ret, GenInlineList *inlineList, 
			int targState, bool inFinish, bool csForced );
	virtual void GOTO( ostream &ret, int gotoDest, bool inFinish ) = 0;
	virtual void CALL( ostream &ret, int callDest, int targState, bool inFinish ) = 0;
	virtual void NCALL( ostream &ret, int callDest, int targState, bool inFinish ) = 0;
	virtual void NEXT( ostream &ret, int nextDest, bool inFinish ) = 0;
	virtual void GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish ) = 0;
	virtual void NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish ) = 0;
	virtual void CALL_EXPR( ostream &ret, GenInlineItem *ilItem, 
			int targState, bool inFinish ) = 0;
	virtual void NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, 
			int targState, bool inFinish ) = 0;
	virtual void RET( ostream &ret, bool inFinish ) = 0;
	virtual void NRET( ostream &ret, bool inFinish ) = 0;
	virtual void BREAK( ostream &ret, int targState, bool csForced ) = 0;
	virtual void NBREAK( ostream &ret, int targState, bool csForced ) = 0;
	virtual void CURS( ostream &ret, bool inFinish ) = 0;
	virtual void TARGS( ostream &ret, bool inFinish, int targState ) = 0;
	void EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish );
	void LM_SWITCH( ostream &ret, GenInlineItem *item, int targState, 
			int inFinish, bool csForced );
	void LM_EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish );
	void SET_ACT( ostream &ret, GenInlineItem *item );
	void INIT_TOKSTART( ostream &ret, GenInlineItem *item );
	void INIT_ACT( ostream &ret, GenInlineItem *item );
	void SET_TOKSTART( ostream &ret, GenInlineItem *item );
	void SET_TOKEND( ostream &ret, GenInlineItem *item );
	void GET_TOKEND( ostream &ret, GenInlineItem *item );

	void HOST_STMT( ostream &ret, GenInlineItem *item, int targState, bool inFinish, bool csForced );
	void HOST_EXPR( ostream &ret, GenInlineItem *item, int targState, bool inFinish, bool csForced );
	void HOST_TEXT( ostream &ret, GenInlineItem *item, int targState, bool inFinish, bool csForced );
	void GEN_STMT( ostream &ret, GenInlineItem *item, int targState, bool inFinish, bool csForced );
	void GEN_EXPR( ostream &ret, GenInlineItem *item, int targState, bool inFinish, bool csForced );

	void STATE_IDS();

	string ERROR_STATE();
	string FIRST_FINAL_STATE();

	string STR( int v );

	ostream &source_warning(const InputLoc &loc);
	ostream &source_error(const InputLoc &loc);

	unsigned int arrayTypeSize( unsigned long maxVal );

	bool outLabelUsed;
	bool testEofUsed;
	bool againLabelUsed;
	bool useIndicies;
	bool matchCondLabelUsed;

	void VALUE( string type, string name, string value );

	string ACCESS_OPER()
		{ return directBackend ? "" : " -> "; }

	string OPEN_HOST_EXPR()
		{ return directBackend ? "(" : "host( \"-\", 1 ) ={"; }

	string CLOSE_HOST_EXPR()
		{ return directBackend ? ")" : "}="; }

	string OPEN_HOST_BLOCK()
		{ return directBackend ? "{" : "host( \"-\", 1 ) ${"; }

	string OPEN_HOST_BLOCK( string fileName, int line )
	{ 
		return directBackend ? "{" : "host( \"" + fileName + "\", " + STR(line) + " ) ${";
	}

	string CLOSE_HOST_BLOCK()
		{ return directBackend ? "}" : "}$"; }

	string OPEN_HOST_PLAIN()
		{ return directBackend ? "" : "host( \"-\", 1 ) @{"; }

	string CLOSE_HOST_PLAIN()
		{ return directBackend ? "" : "}@"; }

	string OPEN_GEN_EXPR()
		{ return directBackend ? "(" : "={"; }

	string CLOSE_GEN_EXPR()
		{ return directBackend ? ")" : "}="; }

	string OPEN_GEN_BLOCK()
		{ return directBackend ? "{" : "${"; }

	string CLOSE_GEN_BLOCK()
		{ return directBackend ? "}" : "}$"; }

	string OPEN_GEN_PLAIN()
		{ return directBackend ? "" : "@{"; }

	string CLOSE_GEN_PLAIN()
		{ return directBackend ? "" : "}@"; }
	
	string UINT()
		{ return directBackend ? "unsigned int" : "uint"; }

	string INDEX( string type, string name )
	{
		if ( directBackend )
			return "const " + type + " *" + name;
		else
			return "index " + type + " " + name;
	}

	string ENTRY()
	{
		if ( directBackend )
			return "";
		else
			return "entry";
	}

	string LABEL( string name )
	{
		if ( directBackend )
			return name + ": ";
		else
			return "label " + name;
	}

	string OFFSET( string arr, string off )
	{
		if ( directBackend )
			return "( " + arr + " + (" + off + "))";
		else
			return "offset( " + arr + ", " + off + " )";
	}

	string TRUE()
	{
		if ( directBackend )
			return "1";
		else
			return "TRUE";
	}

	string DEREF( string arr, string off )
	{
		if ( directBackend )
			return "(*( " + off + "))";
		else
			return "deref( " + arr + ", " + off + " )";
	}
	
	string CASE( string val )
	{
		if ( directBackend )
			return "case " + val + ": ";
		else
			return "case " + val;
	}

	string CEND( )
	{
		if ( directBackend )
			return " break; ";
		else
			return " ";
	}

public:
	virtual void writeExports();
};

#endif
