/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
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

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <sstream>

#include "global.h"
#include "avltree.h"
#include "compiler.h"
#include "parser.h"
#include "parsetree.h"
#include "mergesort.h"
#include "redbuild.h"
#include "pdacodegen.h"
#include "fsmcodegen.h"
#include "pdarun.h"
#include "colm.h"
#include "pool.h"
#include "struct.h"

using std::ostringstream;
using std::cout;
using std::cerr;
using std::endl;

char machineMain[] = "main";
exit_object endp;
void operator<<( ostream &out, exit_object & )
{
	out << endl;
	exit(1);
}

/* Perform minimization after an operation according 
 * to the command line args. */
void afterOpMinimize( FsmGraph *fsm, bool lastInSeq )
{
	/* Switch on the prefered minimization algorithm. */
	if ( lastInSeq ) {
		/* First clean up the graph. FsmGraph operations may leave these
		 * lying around. There should be no dead end states. The subtract
		 * intersection operators are the only places where they may be
		 * created and those operators clean them up. */
		fsm->removeUnreachableStates();
		fsm->minimizePartition2();
	}
}

/* Count the transitions in the fsm by walking the state list. */
int countTransitions( FsmGraph *fsm )
{
	int numTrans = 0;
	FsmState *state = fsm->stateList.head;
	while ( state != 0 ) {
		numTrans += state->outList.length();
		state = state->next;
	}
	return numTrans;
}

Key makeFsmKeyHex( char *str, const InputLoc &loc, Compiler *pd )
{
	/* Reset errno so we can check for overflow or underflow. In the event of
	 * an error, sets the return val to the upper or lower bound being tested
	 * against. */
	errno = 0;
	unsigned int size = keyOps->alphType->size;
	bool unusedBits = size < sizeof(unsigned long);

	unsigned long ul = strtoul( str, 0, 16 );

	if ( errno == ERANGE || (unusedBits && ul >> (size * 8)) ) {
		error(loc) << "literal " << str << " overflows the alphabet type" << endl;
		ul = 1 << (size * 8);
	}

	if ( unusedBits && ul >> (size * 8 - 1) )
		ul |= (ULONG_MAX >> (size*8 ) ) << (size*8);

	return Key( (long)ul );
}

Key makeFsmKeyDec( char *str, const InputLoc &loc, Compiler *pd )
{
	/* Convert the number to a decimal. First reset errno so we can check
	 * for overflow or underflow. */
	errno = 0;
	long long minVal = keyOps->alphType->minVal;
	long long maxVal = keyOps->alphType->maxVal;

	long long ll = strtoll( str, 0, 10 );

	/* Check for underflow. */
	if ( (errno == ERANGE && ll < 0) || ll < minVal) {
		error(loc) << "literal " << str << " underflows the alphabet type" << endl;
		ll = minVal;
	}
	/* Check for overflow. */
	else if ( (errno == ERANGE && ll > 0) || ll > maxVal ) {
		error(loc) << "literal " << str << " overflows the alphabet type" << endl;
		ll = maxVal;
	}

	return Key( (long)ll );
}

/* Make an fsm key in int format (what the fsm graph uses) from an alphabet
 * number returned by the parser. Validates that the number doesn't overflow
 * the alphabet type. */
Key makeFsmKeyNum( char *str, const InputLoc &loc, Compiler *pd )
{
	/* Switch on hex/decimal format. */
	if ( str[0] == '0' && str[1] == 'x' )
		return makeFsmKeyHex( str, loc, pd );
	else
		return makeFsmKeyDec( str, loc, pd );
}

/* Make an fsm int format (what the fsm graph uses) from a single character.
 * Performs proper conversion depending on signed/unsigned property of the
 * alphabet. */
Key makeFsmKeyChar( char c, Compiler *pd )
{
	/* Copy from a char type. */
	return Key( c );
}

/* Make an fsm key array in int format (what the fsm graph uses) from a string
 * of characters. Performs proper conversion depending on signed/unsigned
 * property of the alphabet. */
void makeFsmKeyArray( Key *result, char *data, int len, Compiler *pd )
{
	/* Copy from a char star type. */
	char *src = data;
	for ( int i = 0; i < len; i++ )
		result[i] = Key(src[i]);
}

/* Like makeFsmKeyArray except the result has only unique keys. They ordering
 * will be changed. */
void makeFsmUniqueKeyArray( KeySet &result, char *data, int len, 
		bool caseInsensitive, Compiler *pd )
{
	/* Copy from a char star type. */
	char *src = data;
	for ( int si = 0; si < len; si++ ) {
		Key key( src[si] );
		result.insert( key );
		if ( caseInsensitive ) {
			if ( key.isLower() )
				result.insert( key.toUpper() );
			else if ( key.isUpper() )
				result.insert( key.toLower() );
		}
	}
}

FsmGraph *dotFsm( Compiler *pd )
{
	FsmGraph *retFsm = new FsmGraph();
	retFsm->rangeFsm( keyOps->minKey, keyOps->maxKey );
	return retFsm;
}

FsmGraph *dotStarFsm( Compiler *pd )
{
	FsmGraph *retFsm = new FsmGraph();
	retFsm->rangeStarFsm( keyOps->minKey, keyOps->maxKey );
	return retFsm;
}

/* Make a builtin type. Depends on the signed nature of the alphabet type. */
FsmGraph *makeBuiltin( BuiltinMachine builtin, Compiler *pd )
{
	/* FsmGraph created to return. */
	FsmGraph *retFsm = 0;

	switch ( builtin ) {
	case BT_Any: {
		/* All characters. */
		retFsm = dotFsm( pd );
		break;
	}
	case BT_Ascii: {
		/* Ascii characters 0 to 127. */
		retFsm = new FsmGraph();
		retFsm->rangeFsm( 0, 127 );
		break;
	}
	case BT_Extend: {
		/* Ascii extended characters. This is the full byte range. Dependent
		 * on signed, vs no signed. If the alphabet is one byte then just use
		 * dot fsm. */
		retFsm = new FsmGraph();
		retFsm->rangeFsm( -128, 127 );
		break;
	}
	case BT_Alpha: {
		/* Alpha [A-Za-z]. */
		FsmGraph *upper = new FsmGraph(), *lower = new FsmGraph();
		upper->rangeFsm( 'A', 'Z' );
		lower->rangeFsm( 'a', 'z' );
		upper->unionOp( lower );
		upper->minimizePartition2();
		retFsm = upper;
		break;
	}
	case BT_Digit: {
		/* Digits [0-9]. */
		retFsm = new FsmGraph();
		retFsm->rangeFsm( '0', '9' );
		break;
	}
	case BT_Alnum: {
		/* Alpha numerics [0-9A-Za-z]. */
		FsmGraph *digit = new FsmGraph(), *lower = new FsmGraph();
		FsmGraph *upper = new FsmGraph();
		digit->rangeFsm( '0', '9' );
		upper->rangeFsm( 'A', 'Z' );
		lower->rangeFsm( 'a', 'z' );
		digit->unionOp( upper );
		digit->unionOp( lower );
		digit->minimizePartition2();
		retFsm = digit;
		break;
	}
	case BT_Lower: {
		/* Lower case characters. */
		retFsm = new FsmGraph();
		retFsm->rangeFsm( 'a', 'z' );
		break;
	}
	case BT_Upper: {
		/* Upper case characters. */
		retFsm = new FsmGraph();
		retFsm->rangeFsm( 'A', 'Z' );
		break;
	}
	case BT_Cntrl: {
		/* Control characters. */
		FsmGraph *cntrl = new FsmGraph();
		FsmGraph *highChar = new FsmGraph();
		cntrl->rangeFsm( 0, 31 );
		highChar->concatFsm( 127 );
		cntrl->unionOp( highChar );
		cntrl->minimizePartition2();
		retFsm = cntrl;
		break;
	}
	case BT_Graph: {
		/* Graphical ascii characters [!-~]. */
		retFsm = new FsmGraph();
		retFsm->rangeFsm( '!', '~' );
		break;
	}
	case BT_Print: {
		/* Printable characters. Same as graph except includes space. */
		retFsm = new FsmGraph();
		retFsm->rangeFsm( ' ', '~' );
		break;
	}
	case BT_Punct: {
		/* Punctuation. */
		FsmGraph *range1 = new FsmGraph();
		FsmGraph *range2 = new FsmGraph();
		FsmGraph *range3 = new FsmGraph(); 
		FsmGraph *range4 = new FsmGraph();
		range1->rangeFsm( '!', '/' );
		range2->rangeFsm( ':', '@' );
		range3->rangeFsm( '[', '`' );
		range4->rangeFsm( '{', '~' );
		range1->unionOp( range2 );
		range1->unionOp( range3 );
		range1->unionOp( range4 );
		range1->minimizePartition2();
		retFsm = range1;
		break;
	}
	case BT_Space: {
		/* Whitespace: [\t\v\f\n\r ]. */
		FsmGraph *cntrl = new FsmGraph();
		FsmGraph *space = new FsmGraph();
		cntrl->rangeFsm( '\t', '\r' );
		space->concatFsm( ' ' );
		cntrl->unionOp( space );
		cntrl->minimizePartition2();
		retFsm = cntrl;
		break;
	}
	case BT_Xdigit: {
		/* Hex digits [0-9A-Fa-f]. */
		FsmGraph *digit = new FsmGraph();
		FsmGraph *upper = new FsmGraph();
		FsmGraph *lower = new FsmGraph();
		digit->rangeFsm( '0', '9' );
		upper->rangeFsm( 'A', 'F' );
		lower->rangeFsm( 'a', 'f' );
		digit->unionOp( upper );
		digit->unionOp( lower );
		digit->minimizePartition2();
		retFsm = digit;
		break;
	}
	case BT_Lambda: {
		retFsm = new FsmGraph();
		retFsm->lambdaFsm();
		break;
	}
	case BT_Empty: {
		retFsm = new FsmGraph();
		retFsm->emptyFsm();
		break;
	}}

	return retFsm;
}

/*
 * Compiler
 */

/* Initialize the structure that will collect info during the parse of a
 * machine. */
Compiler::Compiler( )
:	
	nextPriorKey(0),
	nextNameId(0),
	alphTypeSet(false),
	getKeyExpr(0),
	accessExpr(0),
	curStateExpr(0),
	lowerNum(0),
	upperNum(0),
	errorCount(0),
	curActionOrd(0),
	curPriorOrd(0),
	nextEpsilonResolvedLink(0),
	nextTokenId(1),
	rootCodeBlock(0),
	mainReturnUT(0),
	//access(0),
	//tokenStruct(0),

	ptrLangEl(0),
	strLangEl(0),
	anyLangEl(0),
	rootLangEl(0),
	noTokenLangEl(0),
	eofLangEl(0),
	errorLangEl(0),
	ignoreLangEl(0),

	firstNonTermId(0),
	prodIdIndex(0),

	global(0),
	globalSel(0),
	globalObjectDef(0),
	arg0(0),
	argv(0),

	stream(0),
	streamSel(0),

	uniqueTypeNil(0),
	uniqueTypePtr(0),
	uniqueTypeBool(0),
	uniqueTypeInt(0),
	uniqueTypeStr(0),
	uniqueTypeIgnore(0),
	uniqueTypeAny(0),
	uniqueTypeStream(0),
	nextPatConsId(0),
	nextGenericId(1),
	nextFuncId(0),
	nextHostId(0),
	nextObjectId(1),     /* 0 is  reserved for no object. */
	nextFrameId(0),
	nextParserId(0),
	revertOn(true),
	predValue(0),
	nextMatchEndNum(0),
	argvTypeRef(0),
	inContiguous(false),
	contiguousOffset(0),
	contiguousStretch(0)
{
}

/* Clean up the data collected during a parse. */
Compiler::~Compiler()
{
	/* Delete all the nodes in the action list. Will cause all the
	 * string data that represents the actions to be deallocated. */
	actionList.empty();

	for ( CharVectVect::Iter fns = streamFileNames; fns.lte(); fns++ ) {
		const char **ptr = *fns;
		while ( *ptr != 0 ) {
			::free( (void*)*ptr );
			ptr += 1;
		}
		free( (void*) *fns );
	}
}

ostream &operator<<( ostream &out, const Token &token )
{
	out << token.data;
	return out;
}

/* Write out a name reference. */
ostream &operator<<( ostream &out, const NameRef &nameRef )
{
	int pos = 0;
	if ( nameRef[pos] == 0 ) {
		out << "::";
		pos += 1;
	}
	out << nameRef[pos++];
	for ( ; pos < nameRef.length(); pos++ )
		out << "::" << nameRef[pos];
	return out;
}

NameInst **Compiler::makeNameIndex()
{
	/* The number of nodes in the tree can now be given by nextNameId. Put a
	 * null pointer on the end of the list to terminate it. */
	NameInst **nameIndex = new NameInst*[nextNameId+1];
	memset( nameIndex, 0, sizeof(NameInst*)*(nextNameId+1) );

	for ( NameInstList::Iter ni = nameInstList; ni.lte(); ni++ )
		nameIndex[ni->id] = ni;

	return nameIndex;
}

void Compiler::createBuiltin( const char *name, BuiltinMachine builtin )
{
	LexExpression *expression = LexExpression::cons( builtin );
	LexJoin *join = LexJoin::cons( expression );
	LexDefinition *varDef = new LexDefinition( name, join );
	GraphDictEl *graphDictEl = new GraphDictEl( name, varDef );
	rootNamespace->rlMap.insert( graphDictEl );
}

/* Initialize the graph dict with builtin types. */
void Compiler::initGraphDict( )
{
	createBuiltin( "any", BT_Any );
	createBuiltin( "ascii", BT_Ascii );
	createBuiltin( "extend", BT_Extend );
	createBuiltin( "alpha", BT_Alpha );
	createBuiltin( "digit", BT_Digit );
	createBuiltin( "alnum", BT_Alnum );
	createBuiltin( "lower", BT_Lower );
	createBuiltin( "upper", BT_Upper );
	createBuiltin( "cntrl", BT_Cntrl );
	createBuiltin( "graph", BT_Graph );
	createBuiltin( "print", BT_Print );
	createBuiltin( "punct", BT_Punct );
	createBuiltin( "space", BT_Space );
	createBuiltin( "xdigit", BT_Xdigit );
	createBuiltin( "null", BT_Lambda );
	createBuiltin( "zlen", BT_Lambda );
	createBuiltin( "empty", BT_Empty );
}

/* Initialize the key operators object that will be referenced by all fsms
 * created. */
void Compiler::initKeyOps( )
{
	/* Signedness and bounds. */
	HostType *alphType = alphTypeSet ? userAlphType : hostLang->defaultAlphType;
	thisKeyOps.setAlphType( alphType );

	if ( lowerNum != 0 ) {
		/* If ranges are given then interpret the alphabet type. */
		thisKeyOps.minKey = makeFsmKeyNum( lowerNum, rangeLowLoc, this );
		thisKeyOps.maxKey = makeFsmKeyNum( upperNum, rangeHighLoc, this );
	}
}

/* Remove duplicates of unique actions from an action table. */
void Compiler::removeDups( ActionTable &table )
{
	/* Scan through the table looking for unique actions to 
	 * remove duplicates of. */
	for ( int i = 0; i < table.length(); i++ ) {
		/* Remove any duplicates ahead of i. */
		for ( int r = i+1; r < table.length(); ) {
			if ( table[r].value == table[i].value )
				table.vremove(r);
			else
				r += 1;
		}
	}
}

/* Remove duplicates from action lists. This operates only on transition and
 * eof action lists and so should be called once all actions have been
 * transfered to their final resting place. */
void Compiler::removeActionDups( FsmGraph *graph )
{
	/* Loop all states. */
	for ( StateList::Iter state = graph->stateList; state.lte(); state++ ) {
		/* Loop all transitions. */
		for ( TransList::Iter trans = state->outList; trans.lte(); trans++ )
			removeDups( trans->actionTable );
		removeDups( state->toStateActionTable );
		removeDups( state->fromStateActionTable );
		removeDups( state->eofActionTable );
	}
}

Action *Compiler::newAction( const String &name, InlineList *inlineList )
{
	InputLoc loc;
	loc.line = 1;
	loc.col = 1;
	loc.fileName = 0;

	Action *action = Action::cons( loc, name, inlineList );
	actionList.append( action );
	return action;
}

void Compiler::initLongestMatchData()
{
	if ( regionSetList.length() > 0 ) {
		/* The initActId action gives act a default value. */
		InlineList *il4 = InlineList::cons();
		il4->append( InlineItem::cons( InputLoc(), InlineItem::LmInitAct ) );
		initActId = newAction( "initact", il4 );
		initActId->isLmAction = true;

		/* The setTokStart action sets tokstart. */
		InlineList *il5 = InlineList::cons();
		il5->append( InlineItem::cons( InputLoc(), InlineItem::LmSetTokStart ) );
		setTokStart = newAction( "tokstart", il5 );
		setTokStart->isLmAction = true;

		/* The setTokEnd action sets tokend. */
		InlineList *il3 = InlineList::cons();
		il3->append( InlineItem::cons( InputLoc(), InlineItem::LmSetTokEnd ) );
		setTokEnd = newAction( "tokend", il3 );
		setTokEnd->isLmAction = true;

		/* The action will also need an ordering: ahead of all user action
		 * embeddings. */
		initActIdOrd = curActionOrd++;
		setTokStartOrd = curActionOrd++;
		setTokEndOrd = curActionOrd++;
	}
}

void Compiler::finishGraphBuild( FsmGraph *graph )
{
	/* Resolve any labels that point to multiple states. Any labels that are
	 * still around are referenced only by gotos and calls and they need to be
	 * made into deterministic entry points. */
	graph->deterministicEntry();

	/*
	 * All state construction is now complete.
	 */

	/* Transfer global error actions. */
	for ( StateList::Iter state = graph->stateList; state.lte(); state++ )
		graph->transferErrorActions( state, 0 );
	
	removeActionDups( graph );

	/* Remove unreachable states. There should be no dead end states. The
	 * subtract and intersection operators are the only places where they may
	 * be created and those operators clean them up. */
	graph->removeUnreachableStates();

	/* No more fsm operations are to be done. Action ordering numbers are
	 * no longer of use and will just hinder minimization. Clear them. */
	graph->nullActionKeys();

	/* Transition priorities are no longer of use. We can clear them
	 * because they will just hinder minimization as well. Clear them. */
	graph->clearAllPriorities();

	/* Minimize here even if we minimized at every op. Now that function
	 * keys have been cleared we may get a more minimal fsm. */
	graph->minimizePartition2();
	graph->compressTransitions();
}

/* Build the name tree and supporting data structures. */
NameInst *Compiler::makeNameTree()
{
	/* Create the root name. */
	nextNameId = 1;

	/* First make the name tree. */
	for ( RegionImplList::Iter rel = regionImplList; rel.lte(); rel++ ) {
		/* Recurse on the instance. */
		rel->makeNameTree( rel->loc, this );
	}

	return 0;
}

FsmGraph *Compiler::makeAllRegions()
{
	/* Build the name tree and supporting data structures. */
	makeNameTree();
	NameInst **nameIndex = makeNameIndex();

	int numGraphs = 0;
	FsmGraph **graphs = new FsmGraph*[regionImplList.length()];

	/* Make all the instantiations, we know that main exists in this list. */
	for ( RegionImplList::Iter rel = regionImplList; rel.lte(); rel++ ) {
		/* Build the graph from a walk of the parse tree. */
		FsmGraph *newGraph = rel->walk( this );

		/* Wrap up the construction. */
		finishGraphBuild( newGraph );

		/* Save off the new graph. */
		graphs[numGraphs++] = newGraph;
	}

	/* NOTE: If putting in minimization here we need to include eofTarget
	 * into the minimization algorithm. It is currently set by the longest
	 * match operator and not considered anywhere else. */

	FsmGraph *all;
	if ( numGraphs == 0 ) {
		all = new FsmGraph;
		all->lambdaFsm();
	}
	else {
		/* Add all the other graphs into the first. */
		all = graphs[0];
		all->globOp( graphs+1, numGraphs-1 );
		delete[] graphs;
	}

	/* Go through all the token regions and check for lmRequiresErrorState. */
	for ( RegionImplList::Iter reg = regionImplList; reg.lte(); reg++ ) {
		if ( reg->lmSwitchHandlesError )
			all->lmRequiresErrorState = true;
	}

	all->nameIndex = nameIndex;

	return all;
}

void Compiler::analyzeAction( Action *action, InlineList *inlineList )
{
	/* FIXME: Actions used as conditions should be very constrained. */
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		//if ( item->type == InlineItem::Call || item->type == InlineItem::CallExpr )
		//	action->anyCall = true;

		/* Need to recurse into longest match items. */
		if ( item->type == InlineItem::LmSwitch ) {
			RegionImpl *lm = item->tokenRegion;
			for ( TokenInstanceListReg::Iter lmi = lm->tokenInstanceList; lmi.lte(); lmi++ ) {
				if ( lmi->action != 0 )
					analyzeAction( action, lmi->action->inlineList );
			}
		}

		if ( item->type == InlineItem::LmOnLast || 
				item->type == InlineItem::LmOnNext ||
				item->type == InlineItem::LmOnLagBehind )
		{
			TokenInstance *lmi = item->longestMatchPart;
			if ( lmi->action != 0 )
				analyzeAction( action, lmi->action->inlineList );
		}

		if ( item->children != 0 )
			analyzeAction( action, item->children );
	}
}

void Compiler::analyzeGraph( FsmGraph *graph )
{
	for ( ActionList::Iter act = actionList; act.lte(); act++ )
		analyzeAction( act, act->inlineList );

	for ( StateList::Iter st = graph->stateList; st.lte(); st++ ) {
		/* The transition list. */
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			for ( ActionTable::Iter at = trans->actionTable; at.lte(); at++ )
				at->value->numTransRefs += 1;
		}

		for ( ActionTable::Iter at = st->toStateActionTable; at.lte(); at++ )
			at->value->numToStateRefs += 1;

		for ( ActionTable::Iter at = st->fromStateActionTable; at.lte(); at++ )
			at->value->numFromStateRefs += 1;

		for ( ActionTable::Iter at = st->eofActionTable; at.lte(); at++ )
			at->value->numEofRefs += 1;
	}
}

FsmGraph *Compiler::makeScanner()
{
	/* Make the graph, do minimization. */
	FsmGraph *fsmGraph = makeAllRegions();

	/* If any errors have occured in the input file then don't write anything. */
	if ( gblErrorCount > 0 )
		return 0;

	analyzeGraph( fsmGraph );

	/* Decide if an error state is necessary.
	 *  1. There is an error transition
	 *  2. There is a gap in the transitions
	 *  3. The longest match operator requires it. */
	if ( fsmGraph->lmRequiresErrorState || fsmGraph->hasErrorTrans() )
		fsmGraph->errState = fsmGraph->addState();

	/* State numbers need to be assigned such that all final states have a
	 * larger state id number than all non-final states. This enables the
	 * first_final mechanism to function correctly. We also want states to be
	 * ordered in a predictable fashion. So we first apply a depth-first
	 * search, then do a stable sort by final state status, then assign
	 * numbers. */

	fsmGraph->depthFirstOrdering();
	fsmGraph->sortStatesByFinal();
	fsmGraph->setStateNumbers( 0 );

	return fsmGraph;
}

LangEl *Compiler::makeRepeatProd( const InputLoc &loc, Namespace *nspace,
		const String &repeatName, UniqueType *ut )
{
	LangEl *prodName = addLangEl( this, nspace, repeatName, LangEl::NonTerm );
	prodName->isRepeat = true;

	ProdElList *prodElList1 = new ProdElList;

	/* Build the first production of the repeat. */
	TypeRef *typeRef1 = TypeRef::cons( loc, ut );
	ProdEl *factor1 = new ProdEl( ProdEl::ReferenceType,
			InputLoc(), 0, false, typeRef1, 0 );

	UniqueType *prodNameUT = findUniqueType( TYPE_TREE, prodName );
	TypeRef *typeRef2 = TypeRef::cons( loc, prodNameUT );
	ProdEl *factor2 = new ProdEl( ProdEl::ReferenceType,
			InputLoc(), 0, false, typeRef2, 0 );

	prodElList1->append( factor1 );
	prodElList1->append( factor2 );

	Production *newDef1 = Production::cons( InputLoc(),
			prodName, prodElList1, String(), false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef1 );
	prodList.append( newDef1 );

	/* Build the second production of the repeat. */
	ProdElList *prodElList2 = new ProdElList;

	Production *newDef2 = Production::cons( InputLoc(),
			prodName, prodElList2, String(), false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef2 );
	prodList.append( newDef2 );

	return prodName;
}

LangEl *Compiler::makeListProd( const InputLoc &loc, Namespace *nspace,
		const String &listName, UniqueType *ut )
{
	LangEl *prodName = addLangEl( this, nspace, listName, LangEl::NonTerm );
	prodName->isList = true;

	/* Build the first production of the list. */
	TypeRef *typeRef1 = TypeRef::cons( loc, ut );
	ProdEl *factor1 = new ProdEl( ProdEl::ReferenceType, InputLoc(), 0, false, typeRef1, 0 );

	UniqueType *prodNameUT = findUniqueType( TYPE_TREE, prodName );
	TypeRef *typeRef2 = TypeRef::cons( loc, prodNameUT );
	ProdEl *factor2 = new ProdEl( ProdEl::ReferenceType, loc, 0, false, typeRef2, 0 );

	ProdElList *prodElList1 = new ProdElList;
	prodElList1->append( factor1 );
	prodElList1->append( factor2 );

	Production *newDef1 = Production::cons( loc,
			prodName, prodElList1, String(), false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef1 );
	prodList.append( newDef1 );

	/* Build the second production of the list. */
	TypeRef *typeRef3 = TypeRef::cons( loc, ut );
	ProdEl *factor3 = new ProdEl( ProdEl::ReferenceType, loc, 0, false, typeRef3, 0 );

	ProdElList *prodElList2 = new ProdElList;
	prodElList2->append( factor3 );

	Production *newDef2 = Production::cons( loc,
			prodName, prodElList2, String(), false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef2 );
	prodList.append( newDef2 );

	return prodName;
}

LangEl *Compiler::makeOptProd( const InputLoc &loc, Namespace *nspace,
		const String &optName, UniqueType *ut )
{
	LangEl *prodName = addLangEl( this, nspace, optName, LangEl::NonTerm );
	prodName->isOpt = true;

	ProdElList *prodElList1 = new ProdElList;

	/* Build the first production of the repeat. */
	TypeRef *typeRef1 = TypeRef::cons( loc, ut );
	ProdEl *factor1 = new ProdEl( ProdEl::ReferenceType, loc, 0, false, typeRef1, 0 );
	prodElList1->append( factor1 );

	Production *newDef1 = Production::cons( loc,
			prodName, prodElList1, String(), false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef1 );
	prodList.append( newDef1 );

	/* Build the second production of the repeat. */
	ProdElList *prodElList2 = new ProdElList;

	Production *newDef2 = Production::cons( loc,
			prodName, prodElList2, String(), false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef2 );
	prodList.append( newDef2 );

	return prodName;
}

Namespace *Namespace::findNamespace( const String &name )
{
	for ( NamespaceVect::Iter c = childNamespaces; c.lte(); c++ ) {
		if ( strcmp( name, (*c)->name ) == 0 )
			return *c;
	}
	return 0;
}

Reduction *Namespace::findReduction( const String &name )
{
	for ( ReductionVect::Iter r = reductions; r.lte(); r++ ) {
		if ( strcmp( name, (*r)->name ) == 0 )
			return *r;
	}
	return 0;
}

/* Search from a previously resolved qualification. (name 1+ in a qual list). */
Namespace *NamespaceQual::searchFrom( Namespace *from, StringVect::Iter &qualPart )
{
	/* While there are still parts in the qualification.  */
	while ( qualPart.lte() ) {
		Namespace *child = from->findNamespace( *qualPart );
		if ( child == 0 )
			return 0;

		from = child;
		qualPart.increment();
	}

	return from;
}

Namespace *NamespaceQual::getQual( Compiler *pd )
{
	/* Do the search only once. */
	if ( cachedNspaceQual != 0 )
		return cachedNspaceQual;
	
	if ( qualNames.length() == 0 ) {
		/* No qualification, use the region the qualification was 
		 * declared in. */
		cachedNspaceQual = declInNspace;
	}
	else if ( strcmp( qualNames[0], "root" ) == 0 ) {
		/* First item is "root." Start the downward search from there. */
		StringVect::Iter qualPart = qualNames;
		qualPart.increment();
		cachedNspaceQual = searchFrom( pd->rootNamespace, qualPart );
		return cachedNspaceQual;
	}
	else {
		/* Have a qualification. Move upwards through the declared
		 * regions looking for the first part. */
		StringVect::Iter qualPart = qualNames;
		Namespace *parentNamespace = declInNspace;
		while ( parentNamespace != 0 ) {
			/* Search for the first part underneath the current parent. */
			Namespace *child = parentNamespace->findNamespace( *qualPart );

			if ( child != 0 ) {
				/* Found the first part. Start going below the result.  */
				qualPart.increment();
				cachedNspaceQual = searchFrom( child, qualPart );
				return cachedNspaceQual;
			}

			/* Not found, move up to the parent. */
			parentNamespace = parentNamespace->parentNamespace;
		}

		/* Failed to find the place to start from. */
		cachedNspaceQual = 0;
	}

	return cachedNspaceQual;
}

void Compiler::initEmptyScanner( RegionSet *regionSet, TokenRegion *reg )
{
	if ( reg != 0 && reg->impl->tokenInstanceList.length() == 0 ) {
		reg->impl->wasEmpty = true;

		static int def = 1;
		String name( 64, "__%p_DEF_PAT_%d", reg, def++ );

		LexJoin *join = LexJoin::cons( LexExpression::cons( BT_Any ) );

		TokenDef *tokenDef = TokenDef::cons( name, String(), false, false,
				join, 0, internal, nextTokenId++, rootNamespace, 
				regionSet, 0, 0 );
			
		TokenInstance *tokenInstance = TokenInstance::cons( tokenDef,
				join, internal, nextTokenId++,
				rootNamespace, reg );

		reg->impl->tokenInstanceList.append( tokenInstance );

		/* These do not go in the namespace so so they cannot get declared
		 * in the declare pass. */
		LangEl *lel = addLangEl( this, rootNamespace, name, LangEl::Term );

		tokenInstance->tokenDef->tdLangEl = lel;
		lel->tokenDef = tokenDef;
	}
}

void Compiler::initEmptyScanners()
{
	for ( RegionSetList::Iter regionSet = regionSetList; regionSet.lte(); regionSet++ ) {
		initEmptyScanner( regionSet, regionSet->tokenIgnore );
		initEmptyScanner( regionSet, regionSet->tokenOnly );
		initEmptyScanner( regionSet, regionSet->ignoreOnly );
		initEmptyScanner( regionSet, regionSet->collectIgnore );
	}
}

pda_run *Compiler::parsePattern( program_t *prg, tree_t **sp, const InputLoc &loc, 
		int parserId, struct stream_impl *sourceStream )
{
	struct stream_impl *in = colm_impl_new_generic( strdup("<internal>") );

	struct pda_run *pdaRun = new pda_run;
	colm_pda_init( prg, pdaRun, pdaTables, parserId, 0, false, 0, false );

	stream_t *stream = colm_stream_new_struct( prg );
	stream->impl = sourceStream;

	in->funcs->append_stream( in, (tree_t*)stream );
	in->funcs->set_eof( in );

	long pcr = colm_parse_loop( prg, sp, pdaRun, in, PCR_START );
	assert( pcr == PCR_DONE );
	if ( pdaRun->parse_error ) {
		cerr << ( loc.fileName != 0 ? loc.fileName : "<input>" ) <<
				":" << loc.line << ":" << loc.col;

		if ( pdaRun->parse_error_text != 0 ) {
			cerr << ": relative error: " << 
					pdaRun->parse_error_text->tokdata->data;
		}
		else {
			cerr << ": parse error";
		}

		cerr << endl;
		gblErrorCount += 1;
	}

	return pdaRun;
}

void Compiler::parsePatterns()
{
	program_t *prg = colm_new_program( runtimeData );

	colm_set_debug( prg, gblActiveRealm );

	/* Turn off context-dependent parsing. */
	prg->ctx_dep_parsing = 0;

	tree_t **sp = prg->stack_root;

	for ( ConsList::Iter cons = replList; cons.lte(); cons++ ) {
		if ( cons->langEl != 0 ) {
			struct stream_impl *in = colm_impl_new_cons( strdup("<internal>"), cons );
			cons->pdaRun = parsePattern( prg, sp, cons->loc, cons->langEl->parserId, in );
		}
	}

	for ( PatList::Iter pat = patternList; pat.lte(); pat++ ) {
		struct stream_impl *in = colm_impl_new_pat( strdup("<internal>"), pat );
		pat->pdaRun = parsePattern( prg, sp, pat->loc, pat->langEl->parserId, in );
	}

	/* Bail on above errors. */
	if ( gblErrorCount > 0 )
		exit(1);

	fillInPatterns( prg );
}

void Compiler::collectParserEls( BstSet<LangEl*> &parserEls )
{
	for ( PatList::Iter pat = patternList; pat.lte(); pat++ ) {
		/* We assume the reduction action compilation phase was run before
		 * pattern parsing and it decorated the pattern with the target type. */
		assert( pat->langEl != 0 );
		if ( pat->langEl->type != LangEl::NonTerm )
			error(pat->loc) << "pattern type is not a non-terminal" << endp;

		if ( pat->langEl->parserId < 0 ) {
			/* Make a parser for the language element. */
			parserEls.insert( pat->langEl );
			pat->langEl->parserId = nextParserId++;
		}
	}

	for ( ConsList::Iter repl = replList; repl.lte(); repl++ ) {
		/* We need the the language element from the compilation process. */
		assert( repl->langEl != 0 );

		if ( repl->langEl->parserId < 0 ) {
			/* Make a parser for the language element. */
			parserEls.insert( repl->langEl );
			repl->langEl->parserId = nextParserId++;
		}
	}

	/* Make parsers that we need. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->parserId >= 0 )
			parserEls.insert( lel );
	}
}

void Compiler::writeHostCall()
{
	/*
	 * Host Call
	 */
	for ( FunctionList::Iter hc = inHostList; hc.lte(); hc++ ) {
		*outStream <<
			"value_t " << hc->hostCall << "( program_t *prg, tree_t **sp";
		for ( ParameterList::Iter p = *hc->paramList; p.lte(); p++ ) {
			*outStream <<
				", value_t";
		}
		*outStream << " );\n";
	}

	*outStream <<
		"tree_t **" << objectName << "_host_call( program_t *prg, long code, tree_t **sp )\n"
		"{\n"
		"	value_t rtn = 0;\n"
		"	switch ( code ) {\n";
	
	for ( FunctionList::Iter hc = inHostList; hc.lte(); hc++ ) {
		*outStream <<
			"		case " << hc->funcId << ": {\n";

		int pos = hc->paramList->length() - 1;
		for ( ParameterList::Iter p = *hc->paramList; p.lte(); p++, pos-- ) {
			*outStream <<
				"			value_t p" << pos << " = vm_pop_value();\n";
		}
		
		*outStream <<
			"			rtn = " << hc->hostCall << "( prg, sp";

		pos = 0;
		for ( ParameterList::Iter p = *hc->paramList; p.lte(); p++, pos++ ) {
			*outStream <<
				", p" << pos;
		}
		*outStream << " );\n"
			"			break;\n"
			"		}\n";
	}
		
	*outStream <<
		"	}\n"
		"	vm_push_value( rtn );\n"
		"	return sp;\n"
		"}\n";

}

void Compiler::generateOutput( long activeRealm, bool includeCommit )
{
	FsmCodeGen *fsmGen = new FsmCodeGen( *outStream, redFsm, fsmTables );

	PdaCodeGen *pdaGen = new PdaCodeGen( *outStream );

	fsmGen->writeIncludes();
	pdaGen->defineRuntime();
	fsmGen->writeCode();

	/* Make parsers that we need. */
	pdaGen->writeParserData( 0, pdaTables );

	/* Write the runtime data. */
	pdaGen->writeRuntimeData( runtimeData, pdaTables );

	writeHostCall();

	if ( includeCommit )
		writeCommitStub();

	if ( !gblLibrary ) 
		fsmGen->writeMain( activeRealm );

	outStream->flush();
}


void Compiler::prepGrammar()
{
	/* This will create language elements. */
	wrapNonTerminals();

	makeLangElIds();
	makeStructElIds();
	makeLangElNames();
	makeDefinitionNames();
	noUndefindLangEls();

	/* Put the language elements in an index by language element id. */
	langElIndex = new LangEl*[nextLelId+1];
	memset( langElIndex, 0, sizeof(LangEl*)*(nextLelId+1) );
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ )
		langElIndex[lel->id] = lel;

	makeProdFsms();

	/* Allocate the Runtime data now. Every PdaTable that we make 
	 * will reference it, but it will be filled in after all the tables are
	 * built. */
	runtimeData = new colm_sections;
}

void Compiler::compile()
{
	beginProcessing();
	initKeyOps();

	declarePass();

	resolvePass();

	makeTerminalWrappers();
	makeEofElements();

	/*
	 * Parsers
	 */

	/* Init the longest match data */
	initLongestMatchData();
	FsmGraph *fsmGraph = makeScanner();

	prepGrammar();

	placeAllLanguageObjects();
	placeAllStructObjects();
	placeAllFrameObjects();
	placeAllFunctions();

	/* Compile bytecode. */
	compileByteCode();

	/* Make the reduced fsm. */
	RedFsmBuild reduce( this, fsmGraph );
	redFsm = reduce.reduceMachine();

	BstSet<LangEl*> parserEls;
	collectParserEls( parserEls );

	makeParser( parserEls );

	/* Make the scanner tables. */
	fsmTables = redFsm->makeFsmTables();

	/* Now that all parsers are built, make the global runtimeData. */
	makeRuntimeData();

	/* 
	 * All compilation is now complete.
	 */
	
	/* Parse constructors and patterns. */
	parsePatterns();
}

