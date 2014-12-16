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

#include "ragel.h"
#include "asm.h"
#include "redfsm.h"
#include "gendata.h"
#include "bstmap.h"
#include "ragel.h"
#include "redfsm.h"
#include "bstmap.h"
#include "gendata.h"
#include <sstream>

using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;
using std::istream;
using std::ifstream;
using std::ostream;
using std::ios;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

extern int numSplitPartitions;
extern bool noLineDirectives;

void asmLineDirective( ostream &out, const char *fileName, int line )
{
	if ( noLineDirectives )
		out << "/* ";

	/* Write the preprocessor line info for to the input file. */
	out << "#line " << line  << " \"";
	for ( const char *pc = fileName; *pc != 0; pc++ ) {
		if ( *pc == '\\' )
			out << "\\\\";
		else
			out << *pc;
	}
	out << '"';

	if ( noLineDirectives )
		out << " */";

	out << '\n';
}

void AsmCodeGen::statsSummary()
{
	if ( printStatistics )
		cout << "table-data\t" << tableData << endl << endl;
}

void AsmCodeGen::genLineDirective( ostream &out )
{
	std::streambuf *sbuf = out.rdbuf();
	output_filter *filter = static_cast<output_filter*>(sbuf);
	asmLineDirective( out, filter->fileName, filter->line + 1 );
}

/* Init code gen with in parameters. */
AsmCodeGen::AsmCodeGen( ostream &out )
:
	CodeGenData( out ),
	tableData( 0 )
{
}

unsigned int AsmCodeGen::arrayTypeSize( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	HostType *arrayType = keyOps->typeSubsumes( maxValLL );
	assert( arrayType != 0 );
	return arrayType->size;
}

string AsmCodeGen::ARRAY_TYPE( unsigned long maxVal )
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

HostType *AsmCodeGen::arrayType( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	return keyOps->typeSubsumes( maxValLL );
}

/* Write out the fsm name. */
string AsmCodeGen::FSM_NAME()
{
	return fsmName;
}

/* Emit the offset of the start state as a decimal integer. */
string AsmCodeGen::START_STATE_ID()
{
	ostringstream ret;
	ret << redFsm->startState->id;
	return ret.str();
};

string AsmCodeGen::ACCESS()
{
	ostringstream ret;
	if ( accessExpr != 0 )
		INLINE_LIST( ret, accessExpr, 0, false, false );
	return ret.str();
}


string AsmCodeGen::P()
{ 
	ostringstream ret;
	if ( pExpr == 0 )
		ret << "p";
	else {
		ret << "(";
		INLINE_LIST( ret, pExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::PE()
{
	ostringstream ret;
	if ( peExpr == 0 )
		ret << "pe";
	else {
		ret << "(";
		INLINE_LIST( ret, peExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::vEOF()
{
	ostringstream ret;
	if ( eofExpr == 0 )
		ret << "eof";
	else {
		ret << "(";
		INLINE_LIST( ret, eofExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::vCS()
{
	ostringstream ret;
	if ( csExpr == 0 )
		ret << ACCESS() << "cs";
	else {
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, csExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::TOP()
{
	ostringstream ret;
	if ( topExpr == 0 )
		ret << ACCESS() + "top";
	else {
		ret << "(";
		INLINE_LIST( ret, topExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::STACK()
{
	ostringstream ret;
	if ( stackExpr == 0 )
		ret << ACCESS() + "stack";
	else {
		ret << "(";
		INLINE_LIST( ret, stackExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::ACT()
{
	ostringstream ret;
	if ( actExpr == 0 )
		ret << ACCESS() + "act";
	else {
		ret << "(";
		INLINE_LIST( ret, actExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::TOKSTART()
{
	ostringstream ret;
	if ( tokstartExpr == 0 )
		ret << ACCESS() + "ts";
	else {
		ret << "(";
		INLINE_LIST( ret, tokstartExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::TOKEND()
{
	ostringstream ret;
	if ( tokendExpr == 0 )
		ret << ACCESS() + "te";
	else {
		ret << "(";
		INLINE_LIST( ret, tokendExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::GET_WIDE_KEY()
{
	if ( redFsm->anyConditions() ) 
		return "_widec";
	else
		return GET_KEY();
}

string AsmCodeGen::GET_WIDE_KEY( RedStateAp *state )
{
	if ( state->stateCondList.length() > 0 )
		return "_widec";
	else
		return GET_KEY();
}

string AsmCodeGen::GET_KEY()
{
	ostringstream ret;
	if ( getKeyExpr != 0 ) { 
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, getKeyExpr, 0, false, false );
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
string AsmCodeGen::TABS( int level )
{
	string result;
	while ( level-- > 0 )
		result += "\t";
	return result;
}

/* Write out a key from the fsm code gen. Depends on wether or not the key is
 * signed. */
string AsmCodeGen::KEY( Key key )
{
	ostringstream ret;
//	ostringstream ret;
//	if ( keyOps->isSigned || !hostLang->explicitUnsigned )
//		ret << key.getVal();
//	else
//		ret << (unsigned long) key.getVal() << 'u';
	ret << "$" << key.getVal();
	return ret.str();
}

bool AsmCodeGen::isAlphTypeSigned()
{
	return keyOps->isSigned;
}

bool AsmCodeGen::isWideAlphTypeSigned()
{
	string ret;
	if ( redFsm->maxKey <= keyOps->maxKey )
		return isAlphTypeSigned();
	else {
		long long maxKeyVal = redFsm->maxKey.getLongLong();
		HostType *wideType = keyOps->typeSubsumes( keyOps->isSigned, maxKeyVal );
		return wideType->isSigned;
	}
}

string AsmCodeGen::WIDE_KEY( RedStateAp *state, Key key )
{
	if ( state->stateCondList.length() > 0 ) {
		ostringstream ret;
		if ( isWideAlphTypeSigned() )
			ret << key.getVal();
		else
			ret << (unsigned long) key.getVal() << 'u';
		return ret.str();
	}
	else {
		return KEY( key );
	}
}



void AsmCodeGen::EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish )
{
	/* The parser gives fexec two children. The double brackets are for D
	 * code. If the inline list is a single word it will get interpreted as a
	 * C-style cast by the D compiler. */
	ret << "{" << P() << " = ((";
	INLINE_LIST( ret, item->children, targState, inFinish, false );
	ret << "))-1;}";
}

void AsmCodeGen::LM_SWITCH( ostream &ret, GenInlineItem *item, 
		int targState, int inFinish, bool csForced )
{
	ret << 
		"	switch( " << ACT() << " ) {\n";

	bool haveDefault = false;
	for ( GenInlineList::Iter lma = *item->children; lma.lte(); lma++ ) {
		/* Write the case label, the action and the case break. */
		if ( lma->lmId < 0 ) {
			ret << "	default:\n";
			haveDefault = true;
		}
		else
			ret << "	case " << lma->lmId << ":\n";

		/* Write the block and close it off. */
		ret << "	{";
		INLINE_LIST( ret, lma->children, targState, inFinish, csForced );
		ret << "}\n";

		ret << "	break;\n";
	}

	if ( (hostLang->lang == HostLang::D || hostLang->lang == HostLang::D2) && !haveDefault )
		ret << "	default: break;";

	ret << 
		"	}\n"
		"\t";
}

void AsmCodeGen::SET_ACT( ostream &ret, GenInlineItem *item )
{
	ret << ACT() << " = " << item->lmId << ";";
}

void AsmCodeGen::SET_TOKEND( ostream &ret, GenInlineItem *item )
{
	/* The tokend action sets tokend. */
	ret << TOKEND() << " = " << P();
	if ( item->offset != 0 ) 
		out << "+" << item->offset;
	out << ";";
}

void AsmCodeGen::GET_TOKEND( ostream &ret, GenInlineItem *item )
{
	ret << TOKEND();
}

void AsmCodeGen::INIT_TOKSTART( ostream &ret, GenInlineItem *item )
{
	ret << TOKSTART() << " = " << NULL_ITEM() << ";";
}

void AsmCodeGen::INIT_ACT( ostream &ret, GenInlineItem *item )
{
	ret << ACT() << " = 0;";
}

void AsmCodeGen::SET_TOKSTART( ostream &ret, GenInlineItem *item )
{
	ret << TOKSTART() << " = " << P() << ";";
}

void AsmCodeGen::SUB_ACTION( ostream &ret, GenInlineItem *item, 
		int targState, bool inFinish, bool csForced )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		ret << "{";
		INLINE_LIST( ret, item->children, targState, inFinish, csForced );
		ret << "}";
	}
}


/* Write out an inline tree structure. Walks the list and possibly calls out
 * to virtual functions than handle language specific items in the tree. */
void AsmCodeGen::INLINE_LIST( ostream &ret, GenInlineList *inlineList, 
		int targState, bool inFinish, bool csForced )
{
	for ( GenInlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case GenInlineItem::Text:
			ret << item->data;
			break;
		case GenInlineItem::Goto:
			GOTO( ret, item->targState->id, inFinish );
			break;
		case GenInlineItem::Call:
			CALL( ret, item->targState->id, targState, inFinish );
			break;
		case GenInlineItem::Next:
			NEXT( ret, item->targState->id, inFinish );
			break;
		case GenInlineItem::Ret:
			RET( ret, inFinish );
			break;
		case GenInlineItem::PChar:
			ret << P();
			break;
		case GenInlineItem::Char:
			ret << GET_KEY();
			break;
		case GenInlineItem::Hold:
			ret << P() << "--;";
			break;
		case GenInlineItem::Exec:
			EXEC( ret, item, targState, inFinish );
			break;
		case GenInlineItem::Curs:
			CURS( ret, inFinish );
			break;
		case GenInlineItem::Targs:
			TARGS( ret, inFinish, targState );
			break;
		case GenInlineItem::Entry:
			ret << item->targState->id;
			break;
		case GenInlineItem::GotoExpr:
			GOTO_EXPR( ret, item, inFinish );
			break;
		case GenInlineItem::CallExpr:
			CALL_EXPR( ret, item, targState, inFinish );
			break;
		case GenInlineItem::NextExpr:
			NEXT_EXPR( ret, item, inFinish );
			break;
		case GenInlineItem::LmSwitch:
			LM_SWITCH( ret, item, targState, inFinish, csForced );
			break;
		case GenInlineItem::LmSetActId:
			SET_ACT( ret, item );
			break;
		case GenInlineItem::LmSetTokEnd:
			SET_TOKEND( ret, item );
			break;
		case GenInlineItem::LmGetTokEnd:
			GET_TOKEND( ret, item );
			break;
		case GenInlineItem::LmInitTokStart:
			INIT_TOKSTART( ret, item );
			break;
		case GenInlineItem::LmInitAct:
			INIT_ACT( ret, item );
			break;
		case GenInlineItem::LmSetTokStart:
			SET_TOKSTART( ret, item );
			break;
		case GenInlineItem::SubAction:
			SUB_ACTION( ret, item, targState, inFinish, csForced );
			break;
		case GenInlineItem::Break:
			BREAK( ret, targState, csForced );
			break;
		}
	}
}
/* Write out paths in line directives. Escapes any special characters. */
string AsmCodeGen::LDIR_PATH( char *path )
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

void AsmCodeGen::ACTION( ostream &ret, GenAction *action, int targState, 
		bool inFinish, bool csForced )
{
	/* Write the preprocessor line info for going into the source file. */
	asmLineDirective( ret, action->loc.fileName, action->loc.line );

	/* Write the block and close it off. */
	// ret << "\t{";
	INLINE_LIST( ret, action->inlineList, targState, inFinish, csForced );
	// ret << "}\n";
}

void AsmCodeGen::CONDITION( ostream &ret, GenAction *condition )
{
	ret << "\n";
	asmLineDirective( ret, condition->loc.fileName, condition->loc.line );
	INLINE_LIST( ret, condition->inlineList, 0, false, false );
}

string AsmCodeGen::ERROR_STATE()
{
	ostringstream ret;
	if ( redFsm->errState != 0 )
		ret << redFsm->errState->id;
	else
		ret << "-1";
	return ret.str();
}

string AsmCodeGen::FIRST_FINAL_STATE()
{
	ostringstream ret;
	if ( redFsm->firstFinState != 0 )
		ret << redFsm->firstFinState->id;
	else
		ret << redFsm->nextStateId;
	return ret.str();
}

void AsmCodeGen::writeInit()
{
	out << "	{\n";

	if ( !noCS )
		out << "\t" << vCS() << " = " << START() << ";\n";
	
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

string AsmCodeGen::DATA_PREFIX()
{
	if ( !noPrefix )
		return FSM_NAME() + "_";
	return "";
}

/* Emit the alphabet data type. */
string AsmCodeGen::ALPH_TYPE()
{
	string ret = keyOps->alphType->data1;
	if ( keyOps->alphType->data2 != 0 ) {
		ret += " ";
		ret += + keyOps->alphType->data2;
	}
	return ret;
}

/* Emit the alphabet data type. */
string AsmCodeGen::WIDE_ALPH_TYPE()
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

HostType *AsmCodeGen::wideAlphType()
{
	if ( redFsm->maxKey <= keyOps->maxKey )
		return keyOps->alphType;
	else {
		long long maxKeyVal = redFsm->maxKey.getLongLong();
		return keyOps->typeSubsumes( keyOps->isSigned, maxKeyVal );
	}
}

void AsmCodeGen::STATIC_CONST_INT( const string &name, const string &value )
{
	out <<
		"	.align	4\n"
		"	.type	" << name << ", @object\n"
		"	.size	" << name << ", 4\n" <<
		name << ":\n"
		"	.long	" << value << "\n";
}

void AsmCodeGen::STATE_IDS()
{
	if ( redFsm->startState != 0 )
		STATIC_CONST_INT( START(), START_STATE_ID() );

	if ( !noFinal )
		STATIC_CONST_INT( FIRST_FINAL(), FIRST_FINAL_STATE() );

	if ( !noError )
		STATIC_CONST_INT( ERROR(), ERROR_STATE() );

	out << "\n";

	if ( entryPointNames.length() > 0 ) {
		for ( EntryNameVect::Iter en = entryPointNames; en.lte(); en++ ) {
			ostringstream ret;
			ret << redFsm->startState->id;

			STATIC_CONST_INT( string( DATA_PREFIX() + "en_" + *en ),
					ret.str() );
		}
		out << "\n";
	}
}

void AsmCodeGen::writeStart()
{
	out << START_STATE_ID();
}

void AsmCodeGen::writeFirstFinal()
{
	out << FIRST_FINAL_STATE();
}

void AsmCodeGen::writeError()
{
	out << ERROR_STATE();
}

string AsmCodeGen::PTR_CONST()
{
	return "const ";
}

string AsmCodeGen::PTR_CONST_END()
{
	return "";
}

std::ostream &AsmCodeGen::OPEN_ARRAY( string type, string name )
{
	out << "static const " << type << " " << name << "[] = {\n";
	return out;
}

std::ostream &AsmCodeGen::CLOSE_ARRAY()
{
	return out << "};\n";
}

std::ostream &AsmCodeGen::STATIC_VAR( string type, string name )
{
	out << "static const " << type << " " << name;
	return out;
}

string AsmCodeGen::UINT( )
{
	return "unsigned int";
}

string AsmCodeGen::ARR_OFF( string ptr, string offset )
{
	return ptr + " + " + offset;
}

string AsmCodeGen::CAST( string type )
{
	return "(" + type + ")";
}

string AsmCodeGen::NULL_ITEM()
{
	return "0";
}

string AsmCodeGen::POINTER()
{
	return " *";
}

std::ostream &AsmCodeGen::SWITCH_DEFAULT()
{
	return out;
}

string AsmCodeGen::CTRL_FLOW()
{
	return "";
}

void AsmCodeGen::writeExports()
{
	if ( exportList.length() > 0 ) {
		for ( ExportList::Iter ex = exportList; ex.lte(); ex++ ) {
			out << "#define " << DATA_PREFIX() << "ex_" << ex->name << " " << 
					KEY(ex->key) << "\n";
		}
		out << "\n";
	}
}

void AsmCodeGen::finishRagelDef()
{
	/* For directly executable machines there is no required state
	 * ordering. Choose a depth-first ordering to increase the
	 * potential for fall-throughs. */
	redFsm->depthFirstOrdering();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* If any errors have occured in the input file then don't write anything. */
	if ( gblErrorCount > 0 )
		return;
	
	redFsm->setInTrans();

	/* Anlayze Machine will find the final action reference counts, among
	 * other things. We will use these in reporting the usage
	 * of fsm directives in action code. */
	analyzeMachine();

	/* Determine if we should use indicies. */
	calcIndexSize();
}

ostream &AsmCodeGen::source_warning( const InputLoc &loc )
{
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": warning: ";
	return cerr;
}

ostream &AsmCodeGen::source_error( const InputLoc &loc )
{
	gblErrorCount += 1;
	assert( sourceFileName != 0 );
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": ";
	return cerr;
}

std::ostream &AsmCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &AsmCodeGen::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &AsmCodeGen::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, true, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &AsmCodeGen::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

void AsmCodeGen::emitSingleSwitch( RedStateAp *state )
{
	/* Load up the singles. */
	int numSingles = state->outSingle.length();
	RedTransEl *data = state->outSingle.data;

	if ( numSingles == 1 ) {
		/* If there is a single single key then write it out as an if. */
		out << "\tif ( " << GET_WIDE_KEY(state) << " == " << 
				WIDE_KEY(state, data[0].lowKey) << " )\n\t\t"; 

		/* Virtual function for writing the target of the transition. */
		TRANS_GOTO(data[0].value, 0);
	}
	else if ( numSingles > 1 ) {
		/* Write out single keys in a switch if there is more than one. */
		out << "\tswitch( " << GET_WIDE_KEY(state) << " ) {\n";

		/* Write out the single indicies. */
		for ( int j = 0; j < numSingles; j++ ) {
			out << "\t\tcase " << WIDE_KEY(state, data[j].lowKey) << ": ";
			TRANS_GOTO(data[j].value, 0);
		}
		
		/* Emits a default case for D code. */
		SWITCH_DEFAULT();

		/* Close off the transition switch. */
		out << "\t}\n";
	}
}

void AsmCodeGen::emitRangeBSearch( RedStateAp *state, int level, int low, int high )
{
	static int nl = 1;
	/* Get the mid position, staying on the lower end of the range. */
	int mid = (low + high) >> 1;
	RedTransEl *data = state->outRange.data;

	/* Determine if we need to look higher or lower. */
	bool anyLower = mid > low;
	bool anyHigher = mid < high;

	/* Determine if the keys at mid are the limits of the alphabet. */
	bool limitLow = data[mid].lowKey == keyOps->minKey;
	bool limitHigh = data[mid].highKey == keyOps->maxKey;

	if ( anyLower && anyHigher ) {
		int l1 = nl++;
		int l2 = nl++;

		// out << "if ( " << GET_WIDE_KEY(state) << " < " << 
		//		WIDE_KEY(state, data[mid].lowKey) << " ) {\n";

		/* Can go lower and higher than mid. */
		out <<
			"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
			"	jge	.nl" << l1 << "\n";
			
		
		emitRangeBSearch( state, level+1, low, mid-1 );

		out <<
			".nl" << l1 << ":\n"
			"	cmpb	" << KEY ( data[mid].highKey ) << ", %r14b\n"
			"	jle	.nl" << l2 << "\n";

		// out << "} else if ( " << GET_WIDE_KEY(state) << " > " << 
		//		WIDE_KEY(state, data[mid].highKey) << " ) {\n";

		emitRangeBSearch( state, level+1, mid+1, high );

		// out << "} else\n";

		out <<
			".nl" << l2 << ":\n";

		TRANS_GOTO(data[mid].value, level+1);
	}
	else if ( anyLower && !anyHigher ) {
		int l1 = nl++;

		/* Can go lower than mid but not higher. */
		// out << TABS(level) << "if ( " << GET_WIDE_KEY(state) << " < " << 
		//		WIDE_KEY(state, data[mid].lowKey) << " ) {\n";

		out <<
			"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
			"	jge	.nl" << l1 << "\n";

		emitRangeBSearch( state, level+1, low, mid-1 );

		/* if the higher is the highest in the alphabet then there is no
		 * sense testing it. */
		if ( limitHigh ) {

			// out << TABS(level) << "} else\n";

			out <<
				".nl" << l1 << ":\n";
			TRANS_GOTO(data[mid].value, level+1);
		}
		else {

			// out << TABS(level) << "} else if ( " << GET_WIDE_KEY(state) << " <= " << 
			//		WIDE_KEY(state, data[mid].highKey) << " )\n";


			out <<
				".nl" << l1 << ":\n"
				"	cmpb	" << KEY ( data[mid].highKey ) << ", %r14b\n"
				"	jg	.nf" << state->id << "\n";

			TRANS_GOTO(data[mid].value, level+1);
		}
	}
	else if ( !anyLower && anyHigher ) {
		int l1 = nl++;

		/* Can go higher than mid but not lower. */
		// out << TABS(level) << "if ( " << GET_WIDE_KEY(state) << " > " << 
		//		WIDE_KEY(state, data[mid].highKey) << " ) {\n";

		out <<
			"	cmpb	" << KEY( data[mid].highKey ) << ", %r14b\n"
			"	jle	.nl" << l1 << "\n";

		emitRangeBSearch( state, level+1, mid+1, high );

		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( limitLow ) {
			// out << TABS(level) << "} else\n";

			out <<
				".nl" << l1 << ":\n";

			TRANS_GOTO(data[mid].value, level+1);
		}
		else {
			// out << TABS(level) << "} else if ( " << GET_WIDE_KEY(state) << " >= " << 
			//		WIDE_KEY(state, data[mid].lowKey) << " )\n";

			out <<
				".nl" << l1 << ":\n"
				"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
				"	jl	.nf" << state->id << "\n";

			TRANS_GOTO(data[mid].value, level+1);
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {
			//out << TABS(level) << "if ( " << WIDE_KEY(state, data[mid].lowKey) << " <= " << 
			//		GET_WIDE_KEY(state) << " && " << GET_WIDE_KEY(state) << " <= " << 
			//		WIDE_KEY(state, data[mid].highKey) << " )\n";

			out <<
				"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
				"	jl	.nf" << state->id << "\n"
				"	cmpb	" << KEY( data[mid].highKey ) << ", %r14b\n"
				"	jg	.nf" << state->id << "\n";

			TRANS_GOTO(data[mid].value, level+1);
		}
		else if ( limitLow && !limitHigh ) {
			//out << TABS(level) << "if ( " << GET_WIDE_KEY(state) << " <= " << 
			//		WIDE_KEY(state, data[mid].highKey) << " )\n";

			out <<
				"	cmpb	" << KEY( data[mid].highKey ) << ", %r14b\n"
				"	jg	.nf" << state->id << "\n";

			TRANS_GOTO(data[mid].value, level+1);
		}
		else if ( !limitLow && limitHigh ) {
			//out << TABS(level) << "if ( " << WIDE_KEY(state, data[mid].lowKey) << " <= " << 
			//		GET_WIDE_KEY(state) << " )\n";

			out <<
				"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
				"	jl	.nf" << state->id << "\n";
			TRANS_GOTO(data[mid].value, level+1);
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			TRANS_GOTO(data[mid].value, level+1);
		}
	}
}

void AsmCodeGen::COND_TRANSLATE( GenStateCond *stateCond, int level )
{
	GenCondSpace *condSpace = stateCond->condSpace;
	out << TABS(level) << "_widec = " << CAST(WIDE_ALPH_TYPE()) << "(" <<
			KEY(condSpace->baseKey) << " + (" << GET_KEY() << 
			" - " << KEY(keyOps->minKey) << "));\n";

	for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
		out << TABS(level) << "if ( ";
		CONDITION( out, *csi );
		Size condValOffset = ((1 << csi.pos()) * keyOps->alphSize());
		out << " ) _widec += " << condValOffset << ";\n";
	}
}

void AsmCodeGen::emitCondBSearch( RedStateAp *state, int level, int low, int high )
{
	/* Get the mid position, staying on the lower end of the range. */
	int mid = (low + high) >> 1;
	GenStateCond **data = state->stateCondVect.data;

	/* Determine if we need to look higher or lower. */
	bool anyLower = mid > low;
	bool anyHigher = mid < high;

	/* Determine if the keys at mid are the limits of the alphabet. */
	bool limitLow = data[mid]->lowKey == keyOps->minKey;
	bool limitHigh = data[mid]->highKey == keyOps->maxKey;

	if ( anyLower && anyHigher ) {
		/* Can go lower and higher than mid. */
		out << TABS(level) << "if ( " << GET_KEY() << " < " << 
				KEY(data[mid]->lowKey) << " ) {\n";
		emitCondBSearch( state, level+1, low, mid-1 );
		out << TABS(level) << "} else if ( " << GET_KEY() << " > " << 
				KEY(data[mid]->highKey) << " ) {\n";
		emitCondBSearch( state, level+1, mid+1, high );
		out << TABS(level) << "} else {\n";
		COND_TRANSLATE(data[mid], level+1);
		out << TABS(level) << "}\n";
	}
	else if ( anyLower && !anyHigher ) {
		/* Can go lower than mid but not higher. */
		out << TABS(level) << "if ( " << GET_KEY() << " < " << 
				KEY(data[mid]->lowKey) << " ) {\n";
		emitCondBSearch( state, level+1, low, mid-1 );

		/* if the higher is the highest in the alphabet then there is no
		 * sense testing it. */
		if ( limitHigh ) {
			out << TABS(level) << "} else {\n";
			COND_TRANSLATE(data[mid], level+1);
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << GET_KEY() << " <= " << 
					KEY(data[mid]->highKey) << " ) {\n";
			COND_TRANSLATE(data[mid], level+1);
			out << TABS(level) << "}\n";
		}
	}
	else if ( !anyLower && anyHigher ) {
		/* Can go higher than mid but not lower. */
		out << TABS(level) << "if ( " << GET_KEY() << " > " << 
				KEY(data[mid]->highKey) << " ) {\n";
		emitCondBSearch( state, level+1, mid+1, high );

		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( limitLow ) {
			out << TABS(level) << "} else {\n";
			COND_TRANSLATE(data[mid], level+1);
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << GET_KEY() << " >= " << 
					KEY(data[mid]->lowKey) << " ) {\n";
			COND_TRANSLATE(data[mid], level+1);
			out << TABS(level) << "}\n";
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << KEY(data[mid]->lowKey) << " <= " << 
					GET_KEY() << " && " << GET_KEY() << " <= " << 
					KEY(data[mid]->highKey) << " ) {\n";
			COND_TRANSLATE(data[mid], level+1);
			out << TABS(level) << "}\n";
		}
		else if ( limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << GET_KEY() << " <= " << 
					KEY(data[mid]->highKey) << " ) {\n";
			COND_TRANSLATE(data[mid], level+1);
			out << TABS(level) << "}\n";
		}
		else if ( !limitLow && limitHigh ) {
			out << TABS(level) << "if ( " << KEY(data[mid]->lowKey) << " <= " << 
					GET_KEY() << " )\n {";
			COND_TRANSLATE(data[mid], level+1);
			out << TABS(level) << "}\n";
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			COND_TRANSLATE(data[mid], level);
		}
	}
}

std::ostream &AsmCodeGen::STATE_GOTOS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st == redFsm->errState )
			STATE_GOTO_ERROR();
		else {
			/* Writing code above state gotos. */
			GOTO_HEADER( st );

//			if ( st->stateCondVect.length() > 0 ) {
//				out << "	_widec = " << GET_KEY() << ";\n";
//				emitCondBSearch( st, 1, 0, st->stateCondVect.length() - 1 );
//			}

//			/* Try singles. */
//			if ( st->outSingle.length() > 0 )
//				emitSingleSwitch( st );

			/* Default case is to binary search for the ranges, if that fails then */
			if ( st->outRange.length() > 0 ) {
				out << "	movb	(%r12), %r14b\n";
				emitRangeBSearch( st, 1, 0, st->outRange.length() - 1 );
			}

			/* Write the default transition. */
			out << ".nf" << st->id << ":\n";
			TRANS_GOTO( st->defTrans, 1 );
		}
	}
	return out;
}

std::ostream &AsmCodeGen::TRANSITIONS()
{
	/* Emit any transitions that have functions and that go to 
	 * this state. */
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		/* Write the label for the transition so it can be jumped to. */
		out << "	tr" << trans->id << ": ";

		/* Destination state. */
		if ( trans->action != 0 && trans->action->anyCurStateRef() )
			out << "_ps = " << vCS() << ";";
		out << vCS() << " = " << trans->targ->id << "; ";

		if ( trans->action != 0 ) {
			/* Write out the transition func. */
			out << "goto f" << trans->action->actListId << ";\n";
		}
		else {
			/* No code to execute, just loop around. */
			out << "goto _again;\n";
		}
	}
	return out;
}

std::ostream &AsmCodeGen::EXEC_FUNCS()
{
	/* Make labels that set acts and jump to execFuncs. Loop func indicies. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numTransRefs > 0 ) {
			out << "	f" << redAct->actListId << ": " <<
				"_acts = " << ARR_OFF(A(), itoa( redAct->location+1 ) ) << ";"
				" goto execFuncs;\n";
		}
	}

	out <<
		"\n"
		"execFuncs:\n"
		"	_nacts = *_acts++;\n"
		"	while ( _nacts-- > 0 ) {\n"
		"		switch ( *_acts++ ) {\n";
		ACTION_SWITCH();
		SWITCH_DEFAULT() <<
		"		}\n"
		"	}\n"
		"	goto _again;\n";
	return out;
}

unsigned int AsmCodeGen::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	return act;
}

unsigned int AsmCodeGen::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	return act;
}

unsigned int AsmCodeGen::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	return act;
}

bool AsmCodeGen::useAgainLabel()
{
	return redFsm->anyRegActionRets() || 
			redFsm->anyRegActionByValControl() || 
			redFsm->anyRegNextStmt();
}

void AsmCodeGen::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << "{" << CTRL_FLOW() << "goto st" << gotoDest << ";}";
}

void AsmCodeGen::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << targState << 
			"; " << CTRL_FLOW() << "goto st" << callDest << ";}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void AsmCodeGen::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << targState << "; " << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void AsmCodeGen::RET( ostream &ret, bool inFinish )
{
	ret << "{" << vCS() << " = " << STACK() << "[--" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}";
	}

	ret << CTRL_FLOW() << "goto _again;}";
}

void AsmCodeGen::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << "{" << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";
}

void AsmCodeGen::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << vCS() << " = " << nextDest << ";";
}

void AsmCodeGen::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << ");";
}

void AsmCodeGen::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void AsmCodeGen::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << targState;
}

void AsmCodeGen::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << "++; ";
	if ( !csForced ) 
		ret << vCS() << " = " << targState << "; ";
	ret << CTRL_FLOW() << "goto _out;}";
}

bool AsmCodeGen::IN_TRANS_ACTIONS( RedStateAp *state )
{
	bool anyWritten = false;

	/* Emit any transitions that have actions and that go to this state. */
	for ( int it = 0; it < state->numInTrans; it++ ) {
		RedTransAp *trans = state->inTrans[it];
		if ( trans->action != 0 && trans->labelNeeded ) {
			/* Remember that we wrote an action so we know to write the
			 * line directive for going back to the output. */
			anyWritten = true;

			/* Write the label for the transition so it can be jumped to. */
			out << ".tr" << trans->id << ":\n";

//			/* If the action contains a next, then we must preload the current
//			 * state since the action may or may not set it. */
//			if ( trans->action->anyNextStmt() )
//				out << "	" << vCS() << " = " << trans->targ->id << ";\n";

			/* Write each action in the list. */
			for ( GenActionTable::Iter item = trans->action->key; item.lte(); item++ ) {
				ACTION( out, item->value, trans->targ->id, false, 
						trans->action->anyNextStmt() );
			}

			out << "\n";

			/* If the action contains a next then we need to reload, otherwise
			 * jump directly to the target state. */
			if ( trans->action->anyNextStmt() )
				out << "	jmp .L_again\n";
			else
				out << "	jmp .L_st" << trans->targ->id << "\n";
		}
	}

	return anyWritten;
}

void AsmCodeGen::GOTO_HEADER( RedStateAp *state )
{
	/* bool anyWritten = */ IN_TRANS_ACTIONS( state );

	if ( state->labelNeeded ) 
		out << ".L_st" << state->id << ":\n";

#if 0
	if ( state->toStateAction != 0 ) {
		/* Remember that we wrote an action. Write every action in the list. */
		anyWritten = true;
		for ( GenActionTable::Iter item = state->toStateAction->key; item.lte(); item++ ) {
			ACTION( out, item->value, state->id, false, 
					state->toStateAction->anyNextStmt() );
		}
	}
#endif

	/* Advance and test buffer pos. */
	if ( state->labelNeeded ) {
		if ( !noEnd ) {
			// out <<
			//	"	if ( ++" << P() << " == " << PE() << " )\n"
			//	"		goto _test_eof" << state->id << ";\n";
			out <<
				"	addq	$1, %r12\n"
				"	cmpq	%r12, %r13\n"
				"	je	.L_test_eof" << state->id << "\n";
		}
		else {
			// out << 
			//	"	" << P() << " += 1;\n";
			out <<
				"	addq	$1, %r12\n";
		}
	}

	/* This is the entry label for starting a run. */
	out << ".L_en" << state->id << ":\n";

#if 0
	if ( state->fromStateAction != 0 ) {
		/* Remember that we wrote an action. Write every action in the list. */
		anyWritten = true;
		for ( GenActionTable::Iter item = state->fromStateAction->key; item.lte(); item++ ) {
			ACTION( out, item->value, state->id, false,
					state->fromStateAction->anyNextStmt() );
		}
	}

	if ( anyWritten )
		genLineDirective( out );

	/* Record the prev state if necessary. */
	if ( state->anyRegCurStateRef() )
		out << "	_ps = " << state->id << ";\n";
#endif
}

void AsmCodeGen::STATE_GOTO_ERROR()
{
	/* In the error state we need to emit some stuff that usually goes into
	 * the header. */
	RedStateAp *state = redFsm->errState;
	/* bool anyWritten = */ IN_TRANS_ACTIONS( state );

//	/* No case label needed since we don't switch on the error state. */
//	if ( anyWritten )
//		genLineDirective( out );

	if ( state->labelNeeded ) 
		out << ".L_st" << state->id << ":\n";

	/* Break out here. */
	outLabelUsed = true;
	out << "	movl	$" << state->id << ", cs(%rip)\n";
	out << "	jmp .L_out\n";
}


/* Emit the goto to take for a given transition. */
std::ostream &AsmCodeGen::TRANS_GOTO( RedTransAp *trans, int level )
{
	if ( trans->action != 0 ) {
		/* Go to the transition which will go to the state. */
		// out << TABS(level) << "goto tr" << trans->id << ";";

		out << "	jmp	.tr" << trans->id << "\n";
	}
	else {
		/* Go directly to the target state. */
		//out << TABS(level) << "goto st" << trans->targ->id << ";";

		out << "	jmp	.L_st" << trans->targ->id << "\n";
	}
	return out;
}

std::ostream &AsmCodeGen::EXIT_STATES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->outNeeded ) {
			testEofUsed = true;

			// out << "_test_eof" << st->id << ": " << vCS() << " = " << 
			//		st->id << "; goto _test_eof; \n";

			out << 
				".L_test_eof" << st->id << ":\n"
				"	movl	$" << st->id << ", cs(%rip)\n"
				"	jmp .L_test_eof\n";
		}
	}
	return out;
}

std::ostream &AsmCodeGen::AGAIN_CASES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		out << 
			"		case " << st->id << ": goto st" << st->id << ";\n";
	}
	return out;
}

std::ostream &AsmCodeGen::FINISH_CASES()
{
	bool anyWritten = false;

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofAction != 0 ) {
			if ( st->eofAction->eofRefs == 0 )
				st->eofAction->eofRefs = new IntSet;
			st->eofAction->eofRefs->insert( st->id );
		}
	}

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 )
			out << "	case " << st->id << ": goto tr" << st->eofTrans->id << ";\n";
	}

	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		if ( act->eofRefs != 0 ) {
			for ( IntSet::Iter pst = *act->eofRefs; pst.lte(); pst++ )
				out << "	case " << *pst << ": \n";

			/* Remember that we wrote a trans so we know to write the
			 * line directive for going back to the output. */
			anyWritten = true;

			/* Write each action in the eof action list. */
			for ( GenActionTable::Iter item = act->key; item.lte(); item++ )
				ACTION( out, item->value, STATE_ERR_STATE, true, false );
			out << "\tbreak;\n";
		}
	}

	if ( anyWritten )
		genLineDirective( out );
	return out;
}

void AsmCodeGen::setLabelsNeeded( GenInlineList *inlineList )
{
	for ( GenInlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case GenInlineItem::Goto: case GenInlineItem::Call: {
			/* Mark the target as needing a label. */
			item->targState->labelNeeded = true;
			break;
		}
		default: break;
		}

		if ( item->children != 0 )
			setLabelsNeeded( item->children );
	}
}

/* Set up labelNeeded flag for each state. */
void AsmCodeGen::setLabelsNeeded()
{
	/* If we use the _again label, then we the _again switch, which uses all
	 * labels. */
	if ( useAgainLabel() ) {
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
			st->labelNeeded = true;
	}
	else {
		/* Do not use all labels by default, init all labelNeeded vars to false. */
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
			st->labelNeeded = false;

		/* Walk all transitions and set only those that have targs. */
		for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
			/* If there is no action with a next statement, then the label will be
			 * needed. */
			if ( trans->action == 0 || !trans->action->anyNextStmt() )
				trans->targ->labelNeeded = true;

			/* Need labels for states that have goto or calls in action code
			 * invoked on characters (ie, not from out action code). */
			if ( trans->action != 0 ) {
				/* Loop the actions. */
				for ( GenActionTable::Iter act = trans->action->key; act.lte(); act++ ) {
					/* Get the action and walk it's tree. */
					setLabelsNeeded( act->value->inlineList );
				}
			}
		}
	}

	if ( !noEnd ) {
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			if ( st != redFsm->errState )
				st->outNeeded = st->labelNeeded;
		}
	}
}

void AsmCodeGen::writeData()
{
	STATE_IDS();
}

void AsmCodeGen::writeExec()
{
	out << "# WRITE EXEC BEGIN\n";

	/* Must set labels immediately before writing because we may depend on the
	 * noend write option. */
	setLabelsNeeded();
	testEofUsed = false;
	outLabelUsed = false;

	/* Data arrives in %rdi, length in %esi */

	/* p  : %r12 */
	/* pe : %r13 */
	/* pc : %r14b */

	out << 
		"	push	%r12\n"
		"	push	%r13\n"
		"	push	%r14\n"
		"	movq	%rdi, %r12\n"
		"	movslq	%esi, %r13\n"
		"	addq	%rdi, %r13\n"
	;
	

#if 0
	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps = 0;\n";

	if ( redFsm->anyConditions() )
		out << "	" << WIDE_ALPH_TYPE() << " _widec;\n";

	if ( !noEnd ) {
		testEofUsed = true;
		out << 
			"	if ( " << P() << " == " << PE() << " )\n"
			"		goto _test_eof;\n";
	}

	if ( useAgainLabel() ) {
		out << 
			"	goto _resume;\n"
			"\n"
			"_again:\n"
			"	switch ( " << vCS() << " ) {\n";
			AGAIN_CASES() <<
			"	default: break;\n"
			"	}\n"
			"\n";

		if ( !noEnd ) {
			testEofUsed = true;
			out << 
				"	if ( ++" << P() << " == " << PE() << " )\n"
				"		goto _test_eof;\n";
		}
		else {
			out << 
				"	" << P() << " += 1;\n";
		}

		out << "_resume:\n";
	}

#endif

//	out << "	switch ( " << vCS() << " )\n	{\n";

	/* One shot, for now. */
	out <<
		"	jmp	.L_en" << redFsm->startState->id << "\n";

	STATE_GOTOS();
	EXIT_STATES() << "\n";

	if ( testEofUsed ) 
		out << ".L_test_eof:\n";

#if 0
	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out <<
			"	if ( " << P() << " == " << vEOF() << " )\n"
			"	{\n"
			"	switch ( " << vCS() << " ) {\n";
			FINISH_CASES();
			SWITCH_DEFAULT() <<
			"	}\n"
			"	}\n"
			"\n";
	}

#endif

	if ( outLabelUsed ) 
		out << ".L_out:\n";

	out << 
		"	pop	%r14\n"
		"	pop	%r13\n"
		"	pop	%r12\n"
	;

	out << "# WRITE EXEC END\n";
}
