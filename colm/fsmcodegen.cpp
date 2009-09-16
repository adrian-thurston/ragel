/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "parsedata.h"
#include "fsmcodegen.h"
#include "redfsm.h"
#include "bstmap.h"
#include "fsmrun.h"
#include <sstream>
#include <string>
#include <assert.h>


using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;


/* Init code gen with in parameters. */
FsmCodeGen::FsmCodeGen( const char *sourceFileName, const char *fsmName, ostream &out, 
		RedFsm *redFsm, FsmTables *fsmTables )
:
	sourceFileName(sourceFileName),
	fsmName(fsmName), 
	out(out),
	redFsm(redFsm), 
	fsmTables(fsmTables),
	codeGenErrCount(0),
	dataPrefix(true),
	writeFirstFinal(true),
	writeErr(true)
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
	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		/* Write out the length, which will never be the last character. */
		out << act->key.length() << ", ";
		/* Put in a line break every 8 */
		if ( totalActions++ % 8 == 7 )
			out << "\n\t";

		for ( GenActionTable::Iter item = act->key; item.lte(); item++ ) {
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
	/* Expression for retrieving the key, use simple dereference. */
	ret << ACCESS() << "cs";
	return ret.str();
}

string FsmCodeGen::GET_WIDE_KEY()
{
	if ( redFsm->anyConditions() ) 
		return "_widec";
	else
		return GET_KEY();
}

string FsmCodeGen::GET_WIDE_KEY( RedState *state )
{
	if ( state->stateCondList.length() > 0 )
		return "_widec";
	else
		return GET_KEY();
}

string FsmCodeGen::GET_KEY()
{
	ostringstream ret;
	/* Expression for retrieving the key, use simple dereference. */
	ret << "(*" << P() << ")";
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

void FsmCodeGen::SET_ACT( ostream &ret, InlineItem *item )
{
	ret << ACT() << " = " << item->longestMatchPart->longestMatchId << ";";
}

void FsmCodeGen::SET_TOKEND( ostream &ret, InlineItem *item )
{
	/* The tokend action sets tokend. */
	ret << TOKEND() << " = " << P() << "+1;";
}
void FsmCodeGen::INIT_TOKSTART( ostream &ret, InlineItem *item )
{
	ret << TOKSTART() << " = 0;";
}

void FsmCodeGen::INIT_ACT( ostream &ret, InlineItem *item )
{
	ret << ACT() << " = 0;";
}

void FsmCodeGen::SET_TOKSTART( ostream &ret, InlineItem *item )
{
	ret << TOKSTART() << " = " << P() << ";";
}

void FsmCodeGen::EMIT_TOKEN( ostream &ret, KlangEl *token )
{
//	if ( token->transBlock != 0 )
//		ret << "	execGen( " << token->id << " );\n";
//	else if ( token->ignore )
//		ret << "	sendIgnore( " << token->id << " );\n";
//	else 
//		ret << "	sendToken( " << token->id << " );\n";

	ret << "	" << MATCHED_TOKEN() << " = " << token->id << ";\n";
}

void FsmCodeGen::LM_SWITCH( ostream &ret, InlineItem *item, 
		int targState, int inFinish )
{
	ret << 
		"	" << P() << " = " << TOKEND() << ";\n"
		"	switch( " << ACT() << " ) {\n";

	/* If the switch handles error then we also forced the error state. It
	 * will exist. */
	if ( item->tokenRegion->lmSwitchHandlesError ) {
		ret << "	case 0: " << P() << " = " << TOKSTART() << 
				"; goto st" << redFsm->errState->id << ";\n";
	}

	for ( TokenDefList::Iter lmi = item->tokenRegion->tokenDefList; lmi.lte(); lmi++ ) {
		if ( lmi->inLmSelect ) {
			assert( lmi->token != 0 );
			ret << "	case " << lmi->longestMatchId << ":\n";
			EMIT_TOKEN( ret, lmi->token );
			ret << "	break;\n";
		}
	}

	ret << 
		"	}\n"
		"\t"
		" return;\n";
}

void FsmCodeGen::LM_ON_LAST( ostream &ret, InlineItem *item )
{
	assert( item->longestMatchPart->token != 0 );

	ret << "	" << P() << " += 1;\n";
	EMIT_TOKEN( ret, item->longestMatchPart->token );
	ret << "	return;\n";
}

void FsmCodeGen::LM_ON_NEXT( ostream &ret, InlineItem *item )
{
	assert( item->longestMatchPart->token != 0 );

	EMIT_TOKEN( ret, item->longestMatchPart->token );
	ret << "	return;\n";
}

void FsmCodeGen::LM_ON_LAG_BEHIND( ostream &ret, InlineItem *item )
{
	assert( item->longestMatchPart->token != 0 );

	ret << "	" << P() << " = " << TOKEND() << ";\n";
	EMIT_TOKEN( ret, item->longestMatchPart->token );
	ret << "	return;\n";
}


/* Write out an inline tree structure. Walks the list and possibly calls out
 * to virtual functions than handle language specific items in the tree. */
void FsmCodeGen::INLINE_LIST( ostream &ret, InlineList *inlineList, 
		int targState, bool inFinish )
{
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case InlineItem::Text:
			assert( false );
			break;
		case InlineItem::LmSetActId:
			SET_ACT( ret, item );
			break;
		case InlineItem::LmSetTokEnd:
			SET_TOKEND( ret, item );
			break;
		case InlineItem::LmInitTokStart:
			assert( false );
			break;
		case InlineItem::LmInitAct:
			INIT_ACT( ret, item );
			break;
		case InlineItem::LmSetTokStart:
			SET_TOKSTART( ret, item );
			break;
		case InlineItem::LmSwitch:
			LM_SWITCH( ret, item, targState, inFinish );
			break;
		case InlineItem::LmOnLast:
			LM_ON_LAST( ret, item );
			break;
		case InlineItem::LmOnNext:
			LM_ON_NEXT( ret, item );
			break;
		case InlineItem::LmOnLagBehind:
			LM_ON_LAG_BEHIND( ret, item );
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

void FsmCodeGen::ACTION( ostream &ret, GenAction *action, int targState, bool inFinish )
{
	/* Write the block and close it off. */
	ret << "\t{";
	INLINE_LIST( ret, action->inlineList, targState, inFinish );

	if ( action->markId >= 0 )
		ret << "mark[" << action->markId << "] = " << P() << ";\n";

	ret << "}\n";

}

void FsmCodeGen::CONDITION( ostream &ret, GenAction *condition )
{
	ret << "\n";
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


string FsmCodeGen::PTR_CONST()
{
	return "const ";
}

std::ostream &FsmCodeGen::OPEN_ARRAY( string type, string name )
{
	out << "static const " << type << " " << name << "[] = {\n";
	return out;
}

std::ostream &FsmCodeGen::CLOSE_ARRAY()
{
	return out << "};\n";
}

std::ostream &FsmCodeGen::STATIC_VAR( string type, string name )
{
	out << "static const " << type << " " << name;
	return out;
}

string FsmCodeGen::UINT( )
{
	return "unsigned int";
}

string FsmCodeGen::ARR_OFF( string ptr, string offset )
{
	return ptr + " + " + offset;
}

string FsmCodeGen::CAST( string type )
{
	return "(" + type + ")";
}

std::ostream &FsmCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = redFsm->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false );
			out << "\tbreak;\n";
		}
	}

	return out;
}

std::ostream &FsmCodeGen::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = redFsm->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false );
			out << "\tbreak;\n";
		}
	}

	return out;
}

std::ostream &FsmCodeGen::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = redFsm->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false );
			out << "\tbreak;\n";
		}
	}

	return out;
}

void FsmCodeGen::emitSingleSwitch( RedState *state )
{
	/* Load up the singles. */
	int numSingles = state->outSingle.length();
	RedTransEl *data = state->outSingle.data;

	if ( numSingles == 1 ) {
		/* If there is a single single key then write it out as an if. */
		out << "\tif ( " << GET_WIDE_KEY(state) << " == " << 
				KEY(data[0].lowKey) << " )\n\t\t"; 

		/* Virtual function for writing the target of the transition. */
		TRANS_GOTO(data[0].value, 0) << "\n";
	}
	else if ( numSingles > 1 ) {
		/* Write out single keys in a switch if there is more than one. */
		out << "\tswitch( " << GET_WIDE_KEY(state) << " ) {\n";

		/* Write out the single indicies. */
		for ( int j = 0; j < numSingles; j++ ) {
			out << "\t\tcase " << KEY(data[j].lowKey) << ": ";
			TRANS_GOTO(data[j].value, 0) << "\n";
		}
		
		/* Close off the transition switch. */
		out << "\t}\n";
	}
}

void FsmCodeGen::emitRangeBSearch( RedState *state, int level, int low, int high )
{
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
		/* Can go lower and higher than mid. */
		out << TABS(level) << "if ( " << GET_WIDE_KEY(state) << " < " << 
				KEY(data[mid].lowKey) << " ) {\n";
		emitRangeBSearch( state, level+1, low, mid-1 );
		out << TABS(level) << "} else if ( " << GET_WIDE_KEY(state) << " > " << 
				KEY(data[mid].highKey) << " ) {\n";
		emitRangeBSearch( state, level+1, mid+1, high );
		out << TABS(level) << "} else\n";
		TRANS_GOTO(data[mid].value, level+1) << "\n";
	}
	else if ( anyLower && !anyHigher ) {
		/* Can go lower than mid but not higher. */
		out << TABS(level) << "if ( " << GET_WIDE_KEY(state) << " < " << 
				KEY(data[mid].lowKey) << " ) {\n";
		emitRangeBSearch( state, level+1, low, mid-1 );

		/* if the higher is the highest in the alphabet then there is no
		 * sense testing it. */
		if ( limitHigh ) {
			out << TABS(level) << "} else\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
		}
		else {
			out << TABS(level) << "} else if ( " << GET_WIDE_KEY(state) << " <= " << 
					KEY(data[mid].highKey) << " )\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
		}
	}
	else if ( !anyLower && anyHigher ) {
		/* Can go higher than mid but not lower. */
		out << TABS(level) << "if ( " << GET_WIDE_KEY(state) << " > " << 
				KEY(data[mid].highKey) << " ) {\n";
		emitRangeBSearch( state, level+1, mid+1, high );

		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( limitLow ) {
			out << TABS(level) << "} else\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
		}
		else {
			out << TABS(level) << "} else if ( " << GET_WIDE_KEY(state) << " >= " << 
					KEY(data[mid].lowKey) << " )\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << KEY(data[mid].lowKey) << " <= " << 
					GET_WIDE_KEY(state) << " && " << GET_WIDE_KEY(state) << " <= " << 
					KEY(data[mid].highKey) << " )\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
		}
		else if ( limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << GET_WIDE_KEY(state) << " <= " << 
					KEY(data[mid].highKey) << " )\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
		}
		else if ( !limitLow && limitHigh ) {
			out << TABS(level) << "if ( " << KEY(data[mid].lowKey) << " <= " << 
					GET_WIDE_KEY(state) << " )\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			TRANS_GOTO(data[mid].value, level+1) << "\n";
		}
	}
}

void FsmCodeGen::COND_TRANSLATE( GenStateCond *stateCond, int level )
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

void FsmCodeGen::emitCondBSearch( RedState *state, int level, int low, int high )
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

std::ostream &FsmCodeGen::STATE_GOTOS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st == redFsm->errState )
			STATE_GOTO_ERROR();
		else {
			/* Writing code above state gotos. */
			GOTO_HEADER( st );

			if ( st->stateCondVect.length() > 0 ) {
				out << "	_widec = " << GET_KEY() << ";\n";
				emitCondBSearch( st, 1, 0, st->stateCondVect.length() - 1 );
			}

			/* Try singles. */
			if ( st->outSingle.length() > 0 )
				emitSingleSwitch( st );

			/* Default case is to binary search for the ranges, if that fails then */
			if ( st->outRange.length() > 0 )
				emitRangeBSearch( st, 1, 0, st->outRange.length() - 1 );

			/* Write the default transition. */
			TRANS_GOTO( st->defTrans, 1 ) << "\n";
		}
	}
	return out;
}

unsigned int FsmCodeGen::TO_STATE_ACTION( RedState *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	return act;
}

unsigned int FsmCodeGen::FROM_STATE_ACTION( RedState *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	return act;
}

std::ostream &FsmCodeGen::TO_STATE_ACTIONS()
{
	/* Take one off for the psuedo start state. */
	int numStates = redFsm->stateList.length();
	unsigned int *vals = new unsigned int[numStates];
	memset( vals, 0, sizeof(unsigned int)*numStates );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		vals[st->id] = TO_STATE_ACTION(st);

	out << "\t";
	for ( int st = 0; st < redFsm->nextStateId; st++ ) {
		/* Write any eof action. */
		out << vals[st];
		if ( st < numStates-1 ) {
			out << ", ";
			if ( (st+1) % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	delete[] vals;
	return out;
}

std::ostream &FsmCodeGen::FROM_STATE_ACTIONS()
{
	/* Take one off for the psuedo start state. */
	int numStates = redFsm->stateList.length();
	unsigned int *vals = new unsigned int[numStates];
	memset( vals, 0, sizeof(unsigned int)*numStates );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		vals[st->id] = FROM_STATE_ACTION(st);

	out << "\t";
	for ( int st = 0; st < redFsm->nextStateId; st++ ) {
		/* Write any eof action. */
		out << vals[st];
		if ( st < numStates-1 ) {
			out << ", ";
			if ( (st+1) % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	delete[] vals;
	return out;
}

bool FsmCodeGen::IN_TRANS_ACTIONS( RedState *state )
{
	/* Emit any transitions that have actions and that go to this state. */
	for ( int it = 0; it < state->numInTrans; it++ ) {
		RedTrans *trans = state->inTrans[it];
		if ( trans->action != 0 && trans->labelNeeded ) {
			/* Write the label for the transition so it can be jumped to. */
			out << "tr" << trans->id << ":\n";

			/* If the action contains a next, then we must preload the current
			 * state since the action may or may not set it. */
			if ( trans->action->anyNextStmt() )
				out << "	" << CS() << " = " << trans->targ->id << ";\n";

			/* Write each action in the list. */
			for ( GenActionTable::Iter item = trans->action->key; item.lte(); item++ )
				ACTION( out, item->value, trans->targ->id, false );

			out << "\tgoto st" << trans->targ->id << ";\n";
		}
	}

	return 0;
}

/* Called from FsmCodeGen::STATE_GOTOS just before writing the gotos for each
 * state. */
void FsmCodeGen::GOTO_HEADER( RedState *state )
{
	IN_TRANS_ACTIONS( state );

	if ( state->labelNeeded ) 
		out << "st" << state->id << ":\n";

	if ( state->toStateAction != 0 ) {
		/* Remember that we wrote an action. Write every action in the list. */
		for ( GenActionTable::Iter item = state->toStateAction->key; item.lte(); item++ )
			ACTION( out, item->value, state->id, false );
	}

	/* Give the state a switch case. */
	out << "case " << state->id << ":\n";

	/* Advance and test buffer pos. */
	out <<
		"	if ( ++" << P() << " == " << PE() << " )\n"
		"		goto out" << state->id << ";\n";

	if ( state->fromStateAction != 0 ) {
		/* Remember that we wrote an action. Write every action in the list. */
		for ( GenActionTable::Iter item = state->fromStateAction->key; item.lte(); item++ )
			ACTION( out, item->value, state->id, false );
	}

	/* Record the prev state if necessary. */
	if ( state->anyRegCurStateRef() )
		out << "	_ps = " << state->id << ";\n";
}

void FsmCodeGen::STATE_GOTO_ERROR()
{
	/* In the error state we need to emit some stuff that usually goes into
	 * the header. */
	RedState *state = redFsm->errState;
	IN_TRANS_ACTIONS( state );

	if ( state->labelNeeded ) 
		out << "st" << state->id << ":\n";

	/* We do not need a case label here because the the error state is checked
	 * at the head of the loop. */

	/* Break out here. */
	out << "	goto out" << state->id << ";\n";
}


/* Emit the goto to take for a given transition. */
std::ostream &FsmCodeGen::TRANS_GOTO( RedTrans *trans, int level )
{
	if ( trans->action != 0 ) {
		/* Go to the transition which will go to the state. */
		out << TABS(level) << "goto tr" << trans->id << ";";
	}
	else {
		/* Go directly to the target state. */
		out << TABS(level) << "goto st" << trans->targ->id << ";";
	}
	return out;
}

std::ostream &FsmCodeGen::EXIT_STATES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		out << "	case " << st->id << ": out" << st->id << ": ";
		if ( st->eofTrans != 0 ) {
			out << "if ( " << PE() << " == " << PEOF() << " ) {";
			TRANS_GOTO( st->eofTrans, 0 );
			out << "\n";
			out << "}";
		}

		/* Exit. */
		out << CS() << " = " << st->id << "; goto out; \n";
	}
	return out;
}

/* Set up labelNeeded flag for each state. */
void FsmCodeGen::setLabelsNeeded()
{
	/* Do not use all labels by default, init all labelNeeded vars to false. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		st->labelNeeded = false;

	if ( redFsm->errState != 0 && redFsm->anyLmSwitchError() )
		redFsm->errState->labelNeeded = true;

	/* Walk all transitions and set only those that have targs. */
	for ( RedTransSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		/* If there is no action with a next statement, then the label will be
		 * needed. */
		if ( trans->action == 0 || !trans->action->anyNextStmt() )
			trans->targ->labelNeeded = true;
	}

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		st->outNeeded = st->labelNeeded;
}

void FsmCodeGen::writeData()
{
	out << "static const int " << START() << " = " << START_STATE_ID() << ";\n";
	out << "static const int " << FIRST_FINAL() << " = " << FIRST_FINAL_STATE() << ";\n";
	out << "static const int " << ERROR() << " = " << ERROR_STATE() << ";\n";
	out << "\n";

	out << "long " << ENTRY_BY_REGION() << "[] = {\n\t";
	for ( int i = 0; i < fsmTables->numRegions; i++ ) {
		out << fsmTables->entryByRegion[i];

		if ( i < fsmTables->numRegions-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out <<
		"FsmTables fsmTables_start =\n"
		"{\n"
		"	0, "       /* actions */
		" 0, "         /* keyOffsets */
		" 0, "         /* transKeys */
		" 0, "         /* singleLengths */
		" 0, "         /* rangeLengths */
		" 0, "         /* indexOffsets */
		" 0, "         /* transTargsWI */
		" 0, "         /* transActionsWI */
		" 0, "         /* toStateActions */
		" 0, "         /* fromStateActions */
		" 0, "         /* eofActions */
		" 0,\n"        /* eofTargs */
		"	" << ENTRY_BY_REGION() << ",\n"

		"\n"
		"	0, "       /* numStates */
		" 0, "         /* numActions */
		" 0, "         /* numTransKeys */
		" 0, "         /* numSingleLengths */
		" 0, "         /* numRangeLengths */
		" 0, "         /* numIndexOffsets */
		" 0, "         /* numTransTargsWI */
		" 0,\n"        /* numTransActionsWI */
		"	" << redFsm->regionToEntry.length() << ",\n"
		"\n"
		"	" << START() << ",\n"
		"	" << FIRST_FINAL() << ",\n"
		"	" << ERROR() << ",\n"
		"\n"
		"	0,\n"      /* actionSwitch */
		"	0\n"       /* numActionSwitch */
		"};\n"
		"\n";
}

void FsmCodeGen::writeInit()
{
	out << 
		"	" << CS() << " = " << START() << ";\n";
	
	/* If there are any calls, then the stack top needs initialization. */
	if ( redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "\t" << TOP() << " = 0;\n";

	out << 
		"	" << TOKSTART() << " = 0;\n"
		"	" << TOKEND() << " = 0;\n"
		"	" << ACT() << " = 0;\n";

	out << "\n";
}

void FsmCodeGen::writeExec()
{
	setLabelsNeeded();

	out <<
		"void fsm_execute( InputStream *inputStream, FsmRun *fsmRun )\n"
		"{\n"
		"/*_resume:*/\n";

	if ( redFsm->errState != 0 ) {
		out <<
			"	if ( " << CS() << " == " << redFsm->errState->id << " )\n"
			"		goto out;\n";
	}

	out <<
		"	if ( " << P() << " == " << PE() << " )\n"
		"		goto out_switch;\n"
		"	--" << P() << ";\n"
		"\n"
		"	switch ( " << CS() << " )\n	{\n";
		STATE_GOTOS() << 
		"	}\n";

	out << 
		"out_switch:\n"
		"	switch ( " << CS() << " )\n	{\n";
	EXIT_STATES() <<
		"	}\n";

	out <<
		"	out: {}\n"
		"}\n"
		"\n";
}

void FsmCodeGen::writeCode()
{
	redFsm->depthFirstOrdering();

	out << 
		"#include <stdio.h>\n"
		"#include <stdlib.h>\n"
		"#include <string.h>\n"
		"#include <assert.h>\n"
		"#include <colm/config.h>\n"
		"\n"
		"\n";

	writeData();
	writeExec();

	/* Referenced in the runtime lib, but used only in the compiler. */
	out <<
		"void send_named_lang_el( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser )\n"
		"{\n"
		"	assert(false);\n"
		"}\n";

	out << 
		"int main( int argc, char **argv )\n"
		"{\n"
		"	/* Always on because because logging is controlled with ifdefs in\n"
		"	 * the runtime lib. */\n"
		"	colm_log_bytecode = 1;\n"
		"	colm_log_parse = 1;\n"
		"	colm_log_match = 1;\n"
		"	colm_log_compile = 1;\n"
		"	colm_log_conds = 1;\n"
		"	Program program( argc, argv, true, &main_runtimeData );\n"
		"	program.run();\n"
		"	return 0;\n"
		"}\n"
		"\n";

	out.flush();
}

ostream &FsmCodeGen::source_warning( const InputLoc &loc )
{
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": warning: ";
	return cerr;
}

ostream &FsmCodeGen::source_error( const InputLoc &loc )
{
	codeGenErrCount += 1;
	assert( sourceFileName != 0 );
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": ";
	return cerr;
}


