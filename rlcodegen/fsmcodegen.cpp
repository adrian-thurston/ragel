/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@cs.queensu.ca>
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

#include "rlcodegen.h"
#include "fsmcodegen.h"
#include "redfsm.h"
#include "gendata.h"
#include "gvdotgen.h"
#include <sstream>
#include <string>
#include <assert.h>

#include "tabcodegen.h"
#include "ftabcodegen.h"
#include "flatcodegen.h"
#include "fflatcodegen.h"
#include "gotocodegen.h"
#include "fgotocodegen.h"
#include "ipgotocodegen.h"
#include "splitcodegen.h"
#include "javacodegen.h"

using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;

CodeGenData *makeCodeGen( char *fileName, char *fsmName, ostream &out, bool wantComplete )
{
	FsmCodeGen *codeGen = 0;
	switch ( hostLangType ) {
	case CCode:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new CTabCodeGen(out);
			break;
		case GenFTables:
			codeGen = new CFTabCodeGen(out);
			break;
		case GenFlat:
			codeGen = new CFlatCodeGen(out);
			break;
		case GenFFlat:
			codeGen = new CFFlatCodeGen(out);
			break;
		case GenGoto:
			codeGen = new CGotoCodeGen(out);
			break;
		case GenFGoto:
			codeGen = new CFGotoCodeGen(out);
			break;
		case GenIpGoto:
			codeGen = new CIpGotoCodeGen(out);
			break;
		case GenSplit:
			codeGen = new CSplitCodeGen(out);
			break;
		}
		break;

	case DCode:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new DTabCodeGen(out);
			break;
		case GenFTables:
			codeGen = new DFTabCodeGen(out);
			break;
		case GenFlat:
			codeGen = new DFlatCodeGen(out);
			break;
		case GenFFlat:
			codeGen = new DFFlatCodeGen(out);
			break;
		case GenGoto:
			codeGen = new DGotoCodeGen(out);
			break;
		case GenFGoto:
			codeGen = new DFGotoCodeGen(out);
			break;
		case GenIpGoto:
			codeGen = new DIpGotoCodeGen(out);
			break;
		case GenSplit:
			codeGen = new DSplitCodeGen(out);
			break;
		}
		break;

	case JavaCode:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new JavaTabCodeGen(out);
			break;
		default:
			assert(false);
			break;
		}
		break;
	}

	codeGen->fileName = fileName;
	codeGen->fsmName = fsmName;
	codeGen->wantComplete = wantComplete;
	codeGen->codeGen = codeGen;

	return codeGen;
}


/* Init code gen with in parameters. */
FsmCodeGen::FsmCodeGen( ostream &out )
:
	CodeGenData(out)
{
}

unsigned int FsmCodeGen::arrayTypeSize( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	HostType *arrayType = keyOps->typeSubsumes( maxValLL );
	assert( arrayType != 0 );
	return arrayType->size;
}

string FsmCodeGen::ARRAY_TYPE( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	HostType *arrayType = keyOps->typeSubsumes( maxValLL );
	assert( arrayType != 0 );

	string ret = arrayType->data1;
	if ( arrayType->data2 != 0 ) {
		ret += " ";
		ret += arrayType->data2;
	}
	return ret;
}


/* Write out the fsm name. */
string FsmCodeGen::FSM_NAME()
{
	return fsmName;
}

/* Emit the offset of the start state as a decimal integer. */
string FsmCodeGen::START_STATE_ID()
{
	ostringstream ret;
	ret << redFsm->startState->id;
	return ret.str();
};

/* Write out the array of actions. */
std::ostream &FsmCodeGen::ACTIONS_ARRAY()
{
	out << "\t0, ";
	int totalActions = 1;
	for ( ActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		/* Write out the length, which will never be the last character. */
		out << act->key.length() << ", ";
		/* Put in a line break every 8 */
		if ( totalActions++ % 8 == 7 )
			out << "\n\t";

		for ( ActionTable::Iter item = act->key; item.lte(); item++ ) {
			out << item->value->actionId;
			if ( ! (act.last() && item.last()) )
				out << ", ";

			/* Put in a line break every 8 */
			if ( totalActions++ % 8 == 7 )
				out << "\n\t";
		}
	}
	out << "\n";
	return out;
}


string FsmCodeGen::CS()
{
	ostringstream ret;
	if ( curStateExpr != 0 ) { 
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, curStateExpr, 0, false );
		ret << ")";
	}
	else {
		/* Expression for retrieving the key, use simple dereference. */
		ret << ACCESS() << "cs";
	}
	return ret.str();
}

string FsmCodeGen::ACCESS()
{
	ostringstream ret;
	if ( accessExpr != 0 )
		INLINE_LIST( ret, accessExpr, 0, false );
	return ret.str();
}

string FsmCodeGen::GET_WIDE_KEY()
{
	if ( redFsm->anyConditions() ) 
		return "_widec";
	else
		return GET_KEY();
}

string FsmCodeGen::GET_WIDE_KEY( RedStateAp *state )
{
	if ( state->stateCondList.length() > 0 )
		return "_widec";
	else
		return GET_KEY();
}

string FsmCodeGen::GET_KEY()
{
	ostringstream ret;
	if ( getKeyExpr != 0 ) { 
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, getKeyExpr, 0, false );
		ret << ")";
	}
	else {
		/* Expression for retrieving the key, use simple dereference. */
		ret << "(*" << P() << ")";
	}
	return ret.str();
}

/* Write out level number of tabs. Makes the nested binary search nice
 * looking. */
string FsmCodeGen::TABS( int level )
{
	string result;
	while ( level-- > 0 )
		result += "\t";
	return result;
}

/* Write out a key from the fsm code gen. Depends on wether or not the key is
 * signed. */
string FsmCodeGen::KEY( Key key )
{
	ostringstream ret;
	if ( keyOps->isSigned || !hostLang->explicitUnsigned )
		ret << key.getVal();
	else
		ret << (unsigned long) key.getVal() << 'u';
	return ret.str();
}

void FsmCodeGen::EXEC( ostream &ret, InlineItem *item, int targState, int inFinish )
{
	/* The parser gives fexec two children. The double brackets are for D
	 * code. If the inline list is a single word it will get interpreted as a
	 * C-style cast by the D compiler. */
	ret << "{" << P() << " = ((";
	INLINE_LIST( ret, item->children, targState, inFinish );
	ret << "))-1;}";
}

void FsmCodeGen::EXECTE( ostream &ret, InlineItem *item, int targState, int inFinish )
{
	/* Tokend version of exec. */

	/* The parser gives fexec two children. The double brackets are for D
	 * code. If the inline list is a single word it will get interpreted as a
	 * C-style cast by the D compiler. */
	ret << "{" << TOKEND() << " = ((";
	INLINE_LIST( ret, item->children, targState, inFinish );
	ret << "));}";
}


void FsmCodeGen::LM_SWITCH( ostream &ret, InlineItem *item, 
		int targState, int inFinish )
{
	ret << 
		"	switch( act ) {\n";

	/* If the switch handles error then we also forced the error state. It
	 * will exist. */
	if ( item->handlesError ) {
		ret << "	case 0: " << TOKEND() << " = " << TOKSTART() << "; ";
		GOTO( ret, redFsm->errState->id, inFinish );
		ret << "\n";
	}

	for ( InlineList::Iter lma = *item->children; lma.lte(); lma++ ) {
		/* Write the case label, the action and the case break. */
		ret << "	case " << lma->lmId << ":\n";

		/* Write the block and close it off. */
		ret << "	{";
		INLINE_LIST( ret, lma->children, targState, inFinish );
		ret << "}\n";

		ret << "	break;\n";
	}
	/* Default required for D code. */
	ret << 
		"	default: break;\n"
		"	}\n"
		"\t";
}

void FsmCodeGen::SET_ACT( ostream &ret, InlineItem *item )
{
	ret << ACT() << " = " << item->lmId << ";";
}

void FsmCodeGen::SET_TOKEND( ostream &ret, InlineItem *item )
{
	/* The tokend action sets tokend. */
	ret << TOKEND() << " = " << P();
	if ( item->offset != 0 ) 
		out << "+" << item->offset;
	out << ";";
}

void FsmCodeGen::GET_TOKEND( ostream &ret, InlineItem *item )
{
	ret << TOKEND();
}

void FsmCodeGen::INIT_TOKSTART( ostream &ret, InlineItem *item )
{
	ret << TOKSTART() << " = " << NULL_ITEM() << ";";
}

void FsmCodeGen::INIT_ACT( ostream &ret, InlineItem *item )
{
	ret << ACT() << " = 0;";
}

void FsmCodeGen::SET_TOKSTART( ostream &ret, InlineItem *item )
{
	ret << TOKSTART() << " = " << P() << ";";
}

void FsmCodeGen::SUB_ACTION( ostream &ret, InlineItem *item, 
		int targState, bool inFinish )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		ret << "{";
		INLINE_LIST( ret, item->children, targState, inFinish );
		ret << "}";
	}
}


/* Write out an inline tree structure. Walks the list and possibly calls out
 * to virtual functions than handle language specific items in the tree. */
void FsmCodeGen::INLINE_LIST( ostream &ret, InlineList *inlineList, 
		int targState, bool inFinish )
{
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case InlineItem::Text:
			ret << item->data;
			break;
		case InlineItem::Goto:
			GOTO( ret, item->targState->id, inFinish );
			break;
		case InlineItem::Call:
			CALL( ret, item->targState->id, targState, inFinish );
			break;
		case InlineItem::Next:
			NEXT( ret, item->targState->id, inFinish );
			break;
		case InlineItem::Ret:
			RET( ret, inFinish );
			break;
		case InlineItem::PChar:
			ret << P();
			break;
		case InlineItem::Char:
			ret << GET_KEY();
			break;
		case InlineItem::Hold:
			ret << P() << "--;";
			break;
		case InlineItem::Exec:
			EXEC( ret, item, targState, inFinish );
			break;
		case InlineItem::HoldTE:
			ret << TOKEND() << "--;";
			break;
		case InlineItem::ExecTE:
			EXECTE( ret, item, targState, inFinish );
			break;
		case InlineItem::Curs:
			CURS( ret, inFinish );
			break;
		case InlineItem::Targs:
			TARGS( ret, inFinish, targState );
			break;
		case InlineItem::Entry:
			ret << item->targState->id;
			break;
		case InlineItem::GotoExpr:
			GOTO_EXPR( ret, item, inFinish );
			break;
		case InlineItem::CallExpr:
			CALL_EXPR( ret, item, targState, inFinish );
			break;
		case InlineItem::NextExpr:
			NEXT_EXPR( ret, item, inFinish );
			break;
		case InlineItem::LmSwitch:
			LM_SWITCH( ret, item, targState, inFinish );
			break;
		case InlineItem::LmSetActId:
			SET_ACT( ret, item );
			break;
		case InlineItem::LmSetTokEnd:
			SET_TOKEND( ret, item );
			break;
		case InlineItem::LmGetTokEnd:
			GET_TOKEND( ret, item );
			break;
		case InlineItem::LmInitTokStart:
			INIT_TOKSTART( ret, item );
			break;
		case InlineItem::LmInitAct:
			INIT_ACT( ret, item );
			break;
		case InlineItem::LmSetTokStart:
			SET_TOKSTART( ret, item );
			break;
		case InlineItem::SubAction:
			SUB_ACTION( ret, item, targState, inFinish );
			break;
		case InlineItem::Break:
			BREAK( ret, targState );
			break;
		}
	}
}
/* Write out paths in line directives. Escapes any special characters. */
string FsmCodeGen::LDIR_PATH( char *path )
{
	ostringstream ret;
	for ( char *pc = path; *pc != 0; pc++ ) {
		if ( *pc == '\\' )
			ret << "\\\\";
		else
			ret << *pc;
	}
	return ret.str();
}

void FsmCodeGen::ACTION( ostream &ret, Action *action, int targState, bool inFinish )
{
	/* Write the preprocessor line info for going into the source file. */
	lineDirective( ret, fileName, action->loc.line );

	/* Write the block and close it off. */
	ret << "\t{";
	INLINE_LIST( ret, action->inlineList, targState, inFinish );
	ret << "}\n";
}

void FsmCodeGen::CONDITION( ostream &ret, Action *condition )
{
	ret << "\n";
	lineDirective( ret, fileName, condition->loc.line );
	INLINE_LIST( ret, condition->inlineList, 0, false );
}

string FsmCodeGen::ERROR_STATE()
{
	ostringstream ret;
	if ( redFsm->errState != 0 )
		ret << redFsm->errState->id;
	else
		ret << "-1";
	return ret.str();
}

string FsmCodeGen::FIRST_FINAL_STATE()
{
	ostringstream ret;
	if ( redFsm->firstFinState != 0 )
		ret << redFsm->firstFinState->id;
	else
		ret << redFsm->nextStateId;
	return ret.str();
}

void FsmCodeGen::writeOutInit()
{
	out << "	{\n";
	out << "\t" << CS() << " = " << START() << ";\n";
	
	/* If there are any calls, then the stack top needs initialization. */
	if ( redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "\t" << TOP() << " = 0;\n";

	if ( hasLongestMatch ) {
		out << 
			"	" << TOKSTART() << " = " << NULL_ITEM() << ";\n"
			"	" << TOKEND() << " = " << NULL_ITEM() << ";\n"
			"	" << ACT() << " = 0;\n";
	}
	out << "	}\n";
}

string FsmCodeGen::DATA_PREFIX()
{
	if ( dataPrefix )
		return FSM_NAME() + "_";
	return "";
}

/* Emit the alphabet data type. */
string FsmCodeGen::ALPH_TYPE()
{
	string ret = keyOps->alphType->data1;
	if ( keyOps->alphType->data2 != 0 ) {
		ret += " ";
		ret += + keyOps->alphType->data2;
	}
	return ret;
}

/* Emit the alphabet data type. */
string FsmCodeGen::WIDE_ALPH_TYPE()
{
	string ret;
	if ( redFsm->maxKey <= keyOps->maxKey )
		ret = ALPH_TYPE();
	else {
		long long maxKeyVal = redFsm->maxKey.getLongLong();
		HostType *wideType = keyOps->typeSubsumes( keyOps->isSigned, maxKeyVal );
		assert( wideType != 0 );

		ret = wideType->data1;
		if ( wideType->data2 != 0 ) {
			ret += " ";
			ret += wideType->data2;
		}
	}
	return ret;
}


/*
 * Language specific, but style independent code generators functions.
 */

string CCodeGen::PTR_CONST()
{
	return "const ";
}

std::ostream &CCodeGen::OPEN_ARRAY( string type, string name )
{
	out << "static const " << type << " " << name << "[] = {\n";
	return out;
}

std::ostream &CCodeGen::CLOSE_ARRAY()
{
	return out << "};\n";
}

std::ostream &CCodeGen::STATIC_VAR( string type, string name )
{
	out << "static const " << type << " " << name;
	return out;
}

string CCodeGen::UINT( )
{
	return "unsigned int";
}

string CCodeGen::ARR_OFF( string ptr, string offset )
{
	return ptr + " + " + offset;
}

string CCodeGen::CAST( string type )
{
	return "(" + type + ")";
}

string CCodeGen::NULL_ITEM()
{
	return "0";
}

string CCodeGen::POINTER()
{
	return " *";
}

std::ostream &CCodeGen::SWITCH_DEFAULT()
{
	return out;
}

string CCodeGen::CTRL_FLOW()
{
	return "";
}

/*
 * D Specific
 */

string DCodeGen::NULL_ITEM()
{
	return "null";
}

string DCodeGen::POINTER()
{
	// multiple items seperated by commas can also be pointer types.
	return "* ";
}

string DCodeGen::PTR_CONST()
{
	return "";
}

std::ostream &DCodeGen::OPEN_ARRAY( string type, string name )
{
	out << "static const " << type << "[] " << name << " = [\n";
	return out;
}

std::ostream &DCodeGen::CLOSE_ARRAY()
{
	return out << "];\n";
}

std::ostream &DCodeGen::STATIC_VAR( string type, string name )
{
	out << "static const " << type << " " << name;
	return out;
}

string DCodeGen::ARR_OFF( string ptr, string offset )
{
	return "&" + ptr + "[" + offset + "]";
}

string DCodeGen::CAST( string type )
{
	return "cast(" + type + ")";
}

string DCodeGen::UINT( )
{
	return "uint";
}

std::ostream &DCodeGen::SWITCH_DEFAULT()
{
	out << "		default: break;\n";
	return out;
}

string DCodeGen::CTRL_FLOW()
{
	return "if (true) ";
}


/* 
 * Java Specific
 */

string JavaCodeGen::PTR_CONST()
{
	/* Not used in Java code. */
	assert( false );
	return "final";
}

std::ostream &JavaCodeGen::OPEN_ARRAY( string type, string name )
{
	out << "static final " << type << "[] " << name << " = {\n";
	return out;
}

std::ostream &JavaCodeGen::CLOSE_ARRAY()
{
	return out << "};\n";
}

std::ostream &JavaCodeGen::STATIC_VAR( string type, string name )
{
	out << "static final " << type << " " << name;
	return out;
}

string JavaCodeGen::UINT( )
{
	/* Not used. */
	assert( false );
	return "long";
}

string JavaCodeGen::ARR_OFF( string ptr, string offset )
{
	return ptr + " + " + offset;
}

string JavaCodeGen::CAST( string type )
{
	return "(" + type + ")";
}

string JavaCodeGen::NULL_ITEM()
{
	/* In java we use integers instead of pointers. */
	return "-1";
}

string JavaCodeGen::POINTER()
{
	/* Not used. */
	assert( false );
	return " *";
}

std::ostream &JavaCodeGen::SWITCH_DEFAULT()
{
	return out;
}

string JavaCodeGen::GET_KEY()
{
	ostringstream ret;
	if ( getKeyExpr != 0 ) { 
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, getKeyExpr, 0, false );
		ret << ")";
	}
	else {
		/* Expression for retrieving the key, use simple dereference. */
		ret << "data[" << P() << "]";
	}
	return ret.str();
}

string JavaCodeGen::CTRL_FLOW()
{
	return "if (true) ";
}

/* Generate the code for an fsm. Assumes parseData is set up properly. Called
 * by parser code. */
void FsmCodeGen::prepareMachine()
{
	if ( hasBeenPrepared )
		return;
	hasBeenPrepared = true;
	
	/* Do this before distributing transitions out to singles and defaults
	 * makes life easier. */
	redFsm->maxKey = findMaxKey();

	redFsm->assignActionLocs();

	/* Order the states. */
	redFsm->depthFirstOrdering();

	if ( codeStyle == GenGoto || codeStyle == GenFGoto || 
			codeStyle == GenIpGoto || codeStyle == GenSplit )
	{
		/* For goto driven machines we can keep the original depth
		 * first ordering because it's ok if the state ids are not
		 * sequential. Split the the ids by final state status. */
		redFsm->sortStateIdsByFinal();
	}
	else {
		/* For table driven machines the location of the state is used to
		 * identify it so the states must be sorted by their final ids.
		 * Though having a deterministic ordering is important,
		 * specifically preserving the depth first ordering is not because
		 * states are stored in tables. */
		redFsm->sortStatesByFinal();
		redFsm->sequentialStateIds();
	}

	/* Find the first final state. This is the final state with the lowest
	 * id.  */
	redFsm->findFirstFinState();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Maybe do flat expand, otherwise choose single. */
	if ( codeStyle == GenFlat || codeStyle == GenFFlat )
		redFsm->makeFlat();
	else
		redFsm->chooseSingle();

	/* If any errors have occured in the input file then don't write anything. */
	if ( gblErrorCount > 0 )
		return;
	
	if ( codeStyle == GenSplit )
		redFsm->partitionFsm( numSplitPartitions );

	if ( codeStyle == GenIpGoto || codeStyle == GenSplit )
		redFsm->setInTrans();

	/* Anlayze Machine will find the final action reference counts, among
	 * other things. We will use these in reporting the usage
	 * of fsm directives in action code. */
	analyzeMachine();

	/* Determine if we should use indicies. */
	codeGen->calcIndexSize();
}

void FsmCodeGen::generateGraphviz()
{
	/* Do ordering and choose state ids. */
	redFsm->depthFirstOrdering();
	redFsm->sequentialStateIds();

	/* For dot file generation we want to pick default transitions. */
	redFsm->chooseDefaultSpan();

	/* Make the generator. */
	GraphvizDotGen dotGen( fsmName, this, redFsm, out );

	/* Write out with it. */
	dotGen.writeDotFile();
}

void FsmCodeGen::generateCode()
{
	if ( writeOps & WO_NOEND )
		hasEnd = false;

	if ( writeOps & WO_NOERROR )
		writeErr = false;

	if ( writeOps & WO_NOPREFIX )
		dataPrefix = false;
	
	if ( writeOps & WO_NOFF )
		writeFirstFinal = false;

	if ( writeData || writeInit || writeExec || writeEOF ) {
		prepareMachine();

		/* Force a newline. */
		out << "\n";
		genLineDirective( out );
	}
	

	if ( writeExec ) {
		/* Must set labels immediately before writing because we may depend
		 * on the noend write option. */
		codeGen->setLabelsNeeded();
	}

	if ( writeData )
		codeGen->writeOutData();
	
	if ( writeInit )
		codeGen->writeOutInit();

	if ( writeExec )
		codeGen->writeOutExec();

	if ( writeEOF )
		codeGen->writeOutEOF();
}

void FsmCodeGen::generate()
{
	if ( redFsm != 0 ) {
		if ( outputFormat == OutCode )
			generateCode();
		else if ( outputFormat == OutGraphvizDot && !graphvizDone ) {
			graphvizDone = true;
			generateGraphviz();
		}
	}
}

