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

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <sstream>

#include "colm.h"
#include "lmparse.h"
#include "parsedata.h"
#include "parsetree.h"
#include "mergesort.h"
#include "redbuild.h"
#include "pdacodegen.h"
#include "fsmcodegen.h"
#include "fsmrun.h"

using namespace std;
using std::ostringstream;

char machineMain[] = "main";

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

Key makeFsmKeyHex( char *str, const InputLoc &loc, ParseData *pd )
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

	if ( unusedBits && keyOps->alphType->isSigned && ul >> (size * 8 - 1) )
		ul |= (0xffffffff >> (size*8 ) ) << (size*8);

	return Key( (long)ul );
}

Key makeFsmKeyDec( char *str, const InputLoc &loc, ParseData *pd )
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

	if ( keyOps->alphType->isSigned )
		return Key( (long)ll );
	else
		return Key( (unsigned long)ll );
}

/* Make an fsm key in int format (what the fsm graph uses) from an alphabet
 * number returned by the parser. Validates that the number doesn't overflow
 * the alphabet type. */
Key makeFsmKeyNum( char *str, const InputLoc &loc, ParseData *pd )
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
Key makeFsmKeyChar( char c, ParseData *pd )
{
	if ( keyOps->isSigned ) {
		/* Copy from a char type. */
		return Key( c );
	}
	else {
		/* Copy from an unsigned byte type. */
		return Key( (unsigned char)c );
	}
}

/* Make an fsm key array in int format (what the fsm graph uses) from a string
 * of characters. Performs proper conversion depending on signed/unsigned
 * property of the alphabet. */
void makeFsmKeyArray( Key *result, char *data, int len, ParseData *pd )
{
	if ( keyOps->isSigned ) {
		/* Copy from a char star type. */
		char *src = data;
		for ( int i = 0; i < len; i++ )
			result[i] = Key(src[i]);
	}
	else {
		/* Copy from an unsigned byte ptr type. */
		unsigned char *src = (unsigned char*) data;
		for ( int i = 0; i < len; i++ )
			result[i] = Key(src[i]);
	}
}

/* Like makeFsmKeyArray except the result has only unique keys. They ordering
 * will be changed. */
void makeFsmUniqueKeyArray( KeySet &result, char *data, int len, 
		bool caseInsensitive, ParseData *pd )
{
	/* Use a transitions list for getting unique keys. */
	if ( keyOps->isSigned ) {
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
	else {
		/* Copy from an unsigned byte ptr type. */
		unsigned char *src = (unsigned char*) data;
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
}

FsmGraph *dotFsm( ParseData *pd )
{
	FsmGraph *retFsm = new FsmGraph();
	retFsm->rangeFsm( keyOps->minKey, keyOps->maxKey );
	return retFsm;
}

FsmGraph *dotStarFsm( ParseData *pd )
{
	FsmGraph *retFsm = new FsmGraph();
	retFsm->rangeStarFsm( keyOps->minKey, keyOps->maxKey );
	return retFsm;
}

/* Make a builtin type. Depends on the signed nature of the alphabet type. */
FsmGraph *makeBuiltin( BuiltinMachine builtin, ParseData *pd )
{
	/* FsmGraph created to return. */
	FsmGraph *retFsm = 0;
	bool isSigned = keyOps->isSigned;

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
		if ( isSigned ) {
			retFsm = new FsmGraph();
			retFsm->rangeFsm( -128, 127 );
		}
		else {
			retFsm = new FsmGraph();
			retFsm->rangeFsm( 0, 255 );
		}
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

/* Check if this name inst or any name inst below is referenced. */
bool NameInst::anyRefsRec()
{
	if ( numRefs > 0 )
		return true;

	/* Recurse on children until true. */
	for ( NameVect::Iter ch = childVect; ch.lte(); ch++ ) {
		if ( (*ch)->anyRefsRec() )
			return true;
	}

	return false;
}

/*
 * ParseData
 */

/* Initialize the structure that will collect info during the parse of a
 * machine. */
ParseData::ParseData( const String &fileName, const String &sectionName, 
			const InputLoc &sectionLoc, ostream &out )
:	
	nextPriorKey(0),
	nextLocalErrKey(1),          /* 0 is reserved for global error actions. */
	nextNameId(0),
	alphTypeSet(false),
	getKeyExpr(0),
	accessExpr(0),
	curStateExpr(0),
	lowerNum(0),
	upperNum(0),
	fileName(fileName),
	sectionName(sectionName),
	sectionLoc(sectionLoc),
	errorCount(0),
	curActionOrd(0),
	curPriorOrd(0),
	nextEpsilonResolvedLink(0),
	nextTokenId(1),
	rootCodeBlock(0),
	parserName(sectionName),
	out(out),
	access(0),
	tokenStruct(0),
	rootKlangEl(0),
	eofKlangEl(0),
	errorKlangEl(0),
	defaultCharKlangEl(0),
	rootRegion(0),
	defaultRegion(0),
	firstNonTermId(0),
	prodIdIndex(0),
	nextPatReplId(0),
	nextGenericId(1),
	nextFuncId(0),
	loopCleanup(0),
	nextObjectId(1),     /* 0 is  reserved for no object. */
	nextFrameId(0),
	nextParserId(0),
	nextLabelId(0),
	revertOn(true),
	predValue(0),
	nextMatchEndNum(0)
{
}

/* Clean up the data collected during a parse. */
ParseData::~ParseData()
{
	/* Delete all the nodes in the action list. Will cause all the
	 * string data that represents the actions to be deallocated. */
	actionList.empty();
}

/* Make a name id in the current name instantiation scope if it is not
 * already there. */
NameInst *ParseData::addNameInst( const InputLoc &loc, char *data, bool isLabel )
{
	/* Create the name instantitaion object and insert it. */
	NameInst *newNameInst = new NameInst( loc, curNameInst, data, nextNameId++, isLabel );
	curNameInst->childVect.append( newNameInst );
	if ( data != 0 )
		curNameInst->children.insertMulti( data, newNameInst );
	return newNameInst;
}

void ParseData::initNameWalk( NameInst *rootName )
{
	curNameInst = rootName;
	curNameChild = 0;
}

/* Goes into the next child scope. The number of the child is already set up.
 * We need this for the syncronous name tree and parse tree walk to work
 * properly. It is reset on entry into a scope and advanced on poping of a
 * scope. A call to enterNameScope should be accompanied by a corresponding
 * popNameScope. */
NameFrame ParseData::enterNameScope( bool isLocal, int numScopes )
{
	/* Save off the current data. */
	NameFrame retFrame;
	retFrame.prevNameInst = curNameInst;
	retFrame.prevNameChild = curNameChild;
	retFrame.prevLocalScope = localNameScope;

	/* Enter into the new name scope. */
	for ( int i = 0; i < numScopes; i++ ) {
		curNameInst = curNameInst->childVect[curNameChild];
		curNameChild = 0;
	}

	if ( isLocal )
		localNameScope = curNameInst;

	return retFrame;
}

/* Return from a child scope to a parent. The parent info must be specified as
 * an argument and is obtained from the corresponding call to enterNameScope.
 * */
void ParseData::popNameScope( const NameFrame &frame )
{
	/* Pop the name scope. */
	curNameInst = frame.prevNameInst;
	curNameChild = frame.prevNameChild+1;
	localNameScope = frame.prevLocalScope;
}

void ParseData::resetNameScope( const NameFrame &frame )
{
	/* Pop the name scope. */
	curNameInst = frame.prevNameInst;
	curNameChild = frame.prevNameChild;
	localNameScope = frame.prevLocalScope;
}


void ParseData::unsetObsoleteEntries( FsmGraph *graph )
{
	/* Loop the reference names and increment the usage. Names that are no
	 * longer needed will be unset in graph. */
	for ( NameVect::Iter ref = curNameInst->referencedNames; ref.lte(); ref++ ) {
		/* Get the name. */
		NameInst *name = *ref;
		name->numUses += 1;

		/* If the name is no longer needed unset its corresponding entry. */
		if ( name->numUses == name->numRefs ) {
			assert( graph->entryPoints.find( name->id ) != 0 );
			graph->unsetEntry( name->id );
		}
	}
}

NameSet ParseData::resolvePart( NameInst *refFrom, const char *data, bool recLabelsOnly )
{
	/* Queue needed for breadth-first search, load it with the start node. */
	NameInstList nameQueue;
	nameQueue.append( refFrom );

	NameSet result;
	while ( nameQueue.length() > 0 ) {
		/* Pull the next from location off the queue. */
		NameInst *from = nameQueue.detachFirst();

		/* Look for the name. */
		NameMapEl *low, *high;
		if ( from->children.findMulti( data, low, high ) ) {
			/* Record all instances of the name. */
			for ( ; low <= high; low++ )
				result.insert( low->value );
		}

		/* Name not there, do breadth-first operation of appending all
		 * childrent to the processing queue. */
		for ( NameVect::Iter name = from->childVect; name.lte(); name++ ) {
			if ( !recLabelsOnly || (*name)->isLabel )
				nameQueue.append( *name );
		}
	}

	/* Queue exhausted and name never found. */
	return result;
}

void ParseData::resolveFrom( NameSet &result, NameInst *refFrom, 
		const NameRef &nameRef, int namePos )
{
	/* Look for the name in the owning scope of the factor with aug. */
	NameSet partResult = resolvePart( refFrom, nameRef[namePos], false );
	
	/* If there are more parts to the name then continue on. */
	if ( ++namePos < nameRef.length() ) {
		/* There are more components to the name, search using all the part
		 * results as the base. */
		for ( NameSet::Iter name = partResult; name.lte(); name++ )
			resolveFrom( result, *name, nameRef, namePos );
	}
	else {
		/* This is the last component, append the part results to the final
		 * results. */
		result.insert( partResult );
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

ostream &operator<<( ostream &out, const NameInst &nameInst )
{
	/* Count the number fully qualified name parts. */
	int numParents = 0;
	NameInst *curParent = nameInst.parent;
	while ( curParent != 0 ) {
		numParents += 1;
		curParent = curParent->parent;
	}

	/* Make an array and fill it in. */
	curParent = nameInst.parent;
	NameInst **parents = new NameInst*[numParents];
	for ( int p = numParents-1; p >= 0; p-- ) {
		parents[p] = curParent;
		curParent = curParent->parent;
	}
		
	/* Write the parents out, skip the root. */
	for ( int p = 1; p < numParents; p++ )
		out << "::" << ( parents[p]->name != 0 ? parents[p]->name : "<ANON>" );

	/* Write the name and cleanup. */
	out << "::" << ( nameInst.name != 0 ? nameInst.name : "<ANON>" );
	delete[] parents;
	return out;
}

struct CmpNameInstLoc
{
	static int compare( const NameInst *ni1, const NameInst *ni2 )
	{
		if ( ni1->loc.line < ni2->loc.line )
			return -1;
		else if ( ni1->loc.line > ni2->loc.line )
			return 1;
		else if ( ni1->loc.col < ni2->loc.col )
			return -1;
		else if ( ni1->loc.col > ni2->loc.col )
			return 1;
		return 0;
	}
};

void errorStateLabels( const NameSet &resolved )
{
	MergeSort<NameInst*, CmpNameInstLoc> mergeSort;
	mergeSort.sort( resolved.data, resolved.length() );
	for ( NameSet::Iter res = resolved; res.lte(); res++ )
		error((*res)->loc) << "  -> " << **res << endl;
}


void ParseData::referenceRegions( NameInst *rootName )
{
	for ( NameVect::Iter inst = rootName->childVect; inst.lte(); inst++ ) {
		/* Inc the reference in the name. This will cause the entry point to
		 * survive to the end of the graph generating walk. */
		(*inst)->numRefs += 1;
	}
}

/* Walk a name tree starting at from and fill the name index. */
void ParseData::fillNameIndex( NameInst **nameIndex, NameInst *from )
{
	/* Fill the value for from in the name index. */
	nameIndex[from->id] = from;

	/* Recurse on the implicit final state and then all children. */
	if ( from->final != 0 )
		fillNameIndex( nameIndex, from->final );
	for ( NameVect::Iter name = from->childVect; name.lte(); name++ )
		fillNameIndex( nameIndex, *name );
}

NameInst **ParseData::makeNameIndex( NameInst *rootName )
{
	/* The number of nodes in the tree can now be given by nextNameId. Put a
	 * null pointer on the end of the list to terminate it. */
	NameInst **nameIndex = new NameInst*[nextNameId+1];
	memset( nameIndex, 0, sizeof(NameInst*)*(nextNameId+1) );
	fillNameIndex( nameIndex, rootName );
	return nameIndex;
}

void ParseData::createBuiltin( const char *name, BuiltinMachine builtin )
{
	Expression *expression = new Expression( builtin );
	Join *join = new Join( expression );
	JoinOrLm *joinOrLm = new JoinOrLm( join );
	VarDef *varDef = new VarDef( name, joinOrLm );
	GraphDictEl *graphDictEl = new GraphDictEl( name, varDef );
	rootNamespace->graphDict.insert( graphDictEl );
}

/* Initialize the graph dict with builtin types. */
void ParseData::initGraphDict( )
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
void ParseData::initKeyOps( )
{
	/* Signedness and bounds. */
	HostType *alphType = alphTypeSet ? userAlphType : hostLang->defaultAlphType;
	thisKeyOps.setAlphType( alphType );

	if ( lowerNum != 0 ) {
		/* If ranges are given then interpret the alphabet type. */
		thisKeyOps.minKey = makeFsmKeyNum( lowerNum, rangeLowLoc, this );
		thisKeyOps.maxKey = makeFsmKeyNum( upperNum, rangeHighLoc, this );
	}

	thisCondData.nextCondKey = thisKeyOps.maxKey;
	thisCondData.nextCondKey.increment();
}

void ParseData::printNameInst( NameInst *nameInst, int level )
{
	for ( int i = 0; i < level; i++ )
		cerr << "  ";
	cerr << (nameInst->name != 0 ? nameInst->name : "<ANON>") << 
			"  id: " << nameInst->id << 
			"  refs: " << nameInst->numRefs << endl;
	for ( NameVect::Iter name = nameInst->childVect; name.lte(); name++ )
		printNameInst( *name, level+1 );
}

/* Remove duplicates of unique actions from an action table. */
void ParseData::removeDups( ActionTable &table )
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
void ParseData::removeActionDups( FsmGraph *graph )
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

Action *ParseData::newAction( const String &name, InlineList *inlineList )
{
	InputLoc loc;
	loc.line = 1;
	loc.col = 1;

	Action *action = new Action( loc, name, inlineList );
	actionList.append( action );
	return action;
}

void ParseData::initLongestMatchData()
{
	if ( regionList.length() > 0 ) {
		/* The initActId action gives act a default value. */
		InlineList *il4 = new InlineList;
		il4->append( new InlineItem( InputLoc(), InlineItem::LmInitAct ) );
		initActId = newAction( "initact", il4 );
		initActId->isLmAction = true;

		/* The setTokStart action sets tokstart. */
		InlineList *il5 = new InlineList;
		il5->append( new InlineItem( InputLoc(), InlineItem::LmSetTokStart ) );
		setTokStart = newAction( "tokstart", il5 );
		setTokStart->isLmAction = true;

		/* The setTokEnd action sets tokend. */
		InlineList *il3 = new InlineList;
		il3->append( new InlineItem( InputLoc(), InlineItem::LmSetTokEnd ) );
		setTokEnd = newAction( "tokend", il3 );
		setTokEnd->isLmAction = true;

		/* The action will also need an ordering: ahead of all user action
		 * embeddings. */
		initActIdOrd = curActionOrd++;
		setTokStartOrd = curActionOrd++;
		setTokEndOrd = curActionOrd++;
	}
}

void ParseData::finishGraphBuild( FsmGraph *graph )
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

void ParseData::printNameTree( NameInst *rootName )
{
	/* Print the name instance map. */
	cerr << "name tree:" << endl;
	for ( NameVect::Iter name = rootName->childVect; name.lte(); name++ )
		printNameInst( *name, 0 );
}

void ParseData::printNameIndex( NameInst **nameIndex )
{
	/* The name index is terminated with a null pointer. */
	cerr << "name index:" << endl;
	for ( int ni = 0; nameIndex[ni]; ni++ ) {
		cerr << ni << ": ";
		char *name = nameIndex[ni]->name;
		cerr << ( name != 0 ? name : "<ANON>" ) << endl;
	}
}

/* Build the name tree and supporting data structures. */
NameInst *ParseData::makeJoinNameTree( Join *join )
{
	/* Create the root name. */
	nextNameId = 0;
	NameInst *rootName = new NameInst( InputLoc(), 0, 0, nextNameId++, false );

	/* Make the name tree. */
	initNameWalk( rootName );
	join->makeNameTree( this );

	return rootName;
}


/* Build the name tree and supporting data structures. */
NameInst *ParseData::makeNameTree()
{
	/* Create the root name. */
	nextNameId = 0;
	NameInst *rootName = new NameInst( InputLoc(), 0, 0, nextNameId++, false );

	/* First make the name tree. */
	initNameWalk( rootName );
	for ( GraphList::Iter glel = instanceList; glel.lte(); glel++ ) {
		/* Recurse on the instance. */
		glel->value->makeNameTree( glel->loc, this );
	}

	return rootName;
}


FsmGraph *ParseData::makeJoin( Join *join )
{
	/* Build the name tree and supporting data structures. */
	NameInst *rootName = makeJoinNameTree( join );
	NameInst **nameIndex = makeNameIndex( rootName );

	/* Resove name references in the tree. */
	initNameWalk( rootName );
	join->resolveNameRefs( this );

	/* Make all the instantiations, we know that main exists in this list. */
	initNameWalk( rootName );

	/* Build the graph from a walk of the parse tree. */
	FsmGraph *newGraph = join->walk( this );

	/* Wrap up the construction. */
	finishGraphBuild( newGraph );

	newGraph->rootName = rootName;
	newGraph->nameIndex = nameIndex;

	return newGraph;
}

FsmGraph *ParseData::makeAllRegions()
{
	/* Build the name tree and supporting data structures. */
	NameInst *rootName = makeNameTree( );
	NameInst **nameIndex = makeNameIndex( rootName );

	/* Resove name references in the tree. */
	initNameWalk( rootName );
	for ( GraphList::Iter glel = instanceList; glel.lte(); glel++ )
		glel->value->resolveNameRefs( this );

	/* Resovle the implicit name references to the nfa instantiations. */
	referenceRegions( rootName );

	int numGraphs = 0;
	FsmGraph **graphs = new FsmGraph*[instanceList.length()];

	/* Make all the instantiations, we know that main exists in this list. */
	initNameWalk( rootName );
	for ( GraphList::Iter glel = instanceList; glel.lte();  glel++ ) {
		/* Build the graph from a walk of the parse tree. */
		FsmGraph *newGraph = glel->value->walk( this );

		/* Wrap up the construction. */
		finishGraphBuild( newGraph );

		/* Save off the new graph. */
		graphs[numGraphs++] = newGraph;
	}

	/* NOTE: If putting in minimization here we need to include eofTarget
	 * into the minimization algorithm. It is currently set by the longest
	 * match operator and not considered anywhere else. */

	/* Add all the other graphs into the first. */
	FsmGraph *all = graphs[0];
	all->globOp( graphs+1, numGraphs-1 );
	delete[] graphs;

	/* Go through all the token regions and check for lmRequiresErrorState. */
	for ( RegionList::Iter reg = regionList; reg.lte(); reg++ ) {
		if ( reg->lmSwitchHandlesError )
			all->lmRequiresErrorState = true;
	}

	all->rootName = rootName;
	all->nameIndex = nameIndex;

	return all;
}

void ParseData::analyzeAction( Action *action, InlineList *inlineList )
{
	/* FIXME: Actions used as conditions should be very constrained. */
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		//if ( item->type == InlineItem::Call || item->type == InlineItem::CallExpr )
		//	action->anyCall = true;

		/* Need to recurse into longest match items. */
		if ( item->type == InlineItem::LmSwitch ) {
			TokenRegion *lm = item->tokenRegion;
			for ( TokenDefList::Iter lmi = lm->tokenDefList; lmi.lte(); lmi++ ) {
				if ( lmi->action != 0 )
					analyzeAction( action, lmi->action->inlineList );
			}
		}

		if ( item->type == InlineItem::LmOnLast || 
				item->type == InlineItem::LmOnNext ||
				item->type == InlineItem::LmOnLagBehind )
		{
			TokenDef *lmi = item->longestMatchPart;
			if ( lmi->action != 0 )
				analyzeAction( action, lmi->action->inlineList );
		}

		if ( item->children != 0 )
			analyzeAction( action, item->children );
	}
}

void ParseData::analyzeGraph( FsmGraph *graph )
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

		for ( StateCondList::Iter sc = st->stateCondList; sc.lte(); sc++ ) {
			for ( CondSet::Iter sci = sc->condSpace->condSet; sci.lte(); sci++ )
				(*sci)->numCondRefs += 1;
		}
	}
}

FsmGraph *ParseData::makeFsmGraph( Join *join )
{
	/* Make the graph, do minimization. */
	FsmGraph *fsmGraph = join != 0 ? makeJoin( join ) : makeAllRegions();

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

void ParseData::createDefaultScanner()
{
	InputLoc loc;

	const char *name = "___DEFAULT_SCANNER";

	/* Create the default namespace. */
	defaultNamespace = new Namespace( InputLoc(), name,
			namespaceList.length(), 0 );
	namespaceList.append( defaultNamespace );

	/* Create a scanner which will be used when no other scanner can be
	 * figured out. It returns single characters. */
	defaultRegion = new TokenRegion( InputLoc(), name, 
			regionList.length(), 0 );
	regionList.append( defaultRegion );
	JoinOrLm *joinOrLm = new JoinOrLm( defaultRegion );

	/* Insert the machine definition into the graph dictionary. */
	GraphDictEl *newEl = rootNamespace->graphDict.insert( name );
	assert( newEl != 0 );
	newEl->value = new VarDef( name, joinOrLm );
	newEl->isInstance = true;
	instanceList.append( newEl );

	/* Now create the one and only token -> "<chr>" / any /  */
	name = "___DEFAULT_SCANNER_CHR";
	defaultCharKlangEl = getKlangEl( this, defaultNamespace, name );
	assert( defaultCharKlangEl->type == KlangEl::Unknown );
	defaultCharKlangEl->type = KlangEl::Term;

	Join *join = new Join( new Expression( BT_Any ) );
		
	TokenDef *tokenDef = new TokenDef( join, defaultCharKlangEl, loc, 
			nextTokenId++, rootNamespace, defaultRegion );
	defaultRegion->tokenDefList.append( tokenDef );
	defaultCharKlangEl->tokenDef = tokenDef;
}

void ParseData::resolveLiteralFactor( PdaFactor *fact )
{
	/* Interpret escape sequences and remove quotes. */
	bool unusedCI;
	String interp;
	prepareLitString( interp, unusedCI, fact->literal->token.data, 
			fact->literal->token.loc );

	//cerr << "resolving literal: " << fact->literal->token << endl;

	/* Look for the production's associated region. */
	Namespace *nspace = fact->nspaceQual->getQual( this );

	if ( nspace == 0 )
		error(fact->loc) << "do not have region for resolving literal" << endp;

	LiteralDictEl *ldel = nspace->literalDict.find( interp );
	if ( ldel == 0 )
		cerr << "could not resolve literal: " << fact->literal->token << endp;

	TokenDef *tokenDef = ldel->value->tokenDef;
	fact->langEl = tokenDef->token;
}

KlangEl *ParseData::makeRepeatProd( Namespace *nspace, const String &repeatName, NamespaceQual *nspaceQual, const String &name )
{
	KlangEl *prodName = getKlangEl( this, nspace, repeatName );
	prodName->type = KlangEl::NonTerm;
	prodName->isRepeat = true;

	ProdElList *prodElList1 = new ProdElList;

	/* Build the first production of the repeat. */
	PdaFactor *factor1 = new PdaFactor( InputLoc(), false, nspaceQual, 
			name, 0, RepeatNone, false, false );
	PdaFactor *factor2 = new PdaFactor( InputLoc(), false, nspaceQual, 
			repeatName, 0, RepeatNone, false, false );

	prodElList1->append( factor1 );
	prodElList1->append( factor2 );

	Definition *newDef1 = new Definition( InputLoc(),
			prodName, prodElList1, false, 0,
			prodList.length(), Definition::Production );

	prodName->defList.append( newDef1 );
	prodList.append( newDef1 );

	/* Build the second production of the repeat. */
	ProdElList *prodElList2 = new ProdElList;

	Definition *newDef2 = new Definition( InputLoc(),
			prodName, prodElList2, false, 0,
			prodList.length(), Definition::Production );

	prodName->defList.append( newDef2 );
	prodList.append( newDef2 );

	return prodName;
}

KlangEl *ParseData::makeListProd( Namespace *nspace, const String &listName, NamespaceQual *nspaceQual, const String &name )
{
	KlangEl *prodName = getKlangEl( this, nspace, listName );
	prodName->type = KlangEl::NonTerm;
	prodName->isList = true;

	/* Build the first production of the list. */
	PdaFactor *factor1 = new PdaFactor( InputLoc(), false, nspaceQual, 
			name, 0, RepeatNone, false, false );
	PdaFactor *factor2 = new PdaFactor( InputLoc(), false, nspaceQual, 
			listName, 0, RepeatNone, false, false );

	ProdElList *prodElList1 = new ProdElList;
	prodElList1->append( factor1 );
	prodElList1->append( factor2 );

	Definition *newDef1 = new Definition( InputLoc(),
			prodName, prodElList1, false, 0,
			prodList.length(), Definition::Production );

	prodName->defList.append( newDef1 );
	prodList.append( newDef1 );

	/* Build the second production of the list. */
	PdaFactor *factor3 = new PdaFactor( InputLoc(), false, nspaceQual, 
			name, 0, RepeatNone, false, false );

	ProdElList *prodElList2 = new ProdElList;
	prodElList2->append( factor3 );

	Definition *newDef2 = new Definition( InputLoc(),
			prodName, prodElList2, false, 0,
			prodList.length(), Definition::Production );

	prodName->defList.append( newDef2 );
	prodList.append( newDef2 );

	return prodName;
}

KlangEl *ParseData::makeOptProd( Namespace *nspace, const String &optName, NamespaceQual *nspaceQual, const String &name )
{
	KlangEl *prodName = getKlangEl( this, nspace, optName );
	prodName->type = KlangEl::NonTerm;
	prodName->isOpt = true;

	ProdElList *prodElList1 = new ProdElList;

	/* Build the first production of the repeat. */
	PdaFactor *factor1 = new PdaFactor( InputLoc(), false, nspaceQual, 
			name, 0, RepeatNone, false, false );
	prodElList1->append( factor1 );

	Definition *newDef1 = new Definition( InputLoc(),
			prodName, prodElList1, false, 0,
			prodList.length(), Definition::Production );

	prodName->defList.append( newDef1 );
	prodList.append( newDef1 );

	/* Build the second production of the repeat. */
	ProdElList *prodElList2 = new ProdElList;

	Definition *newDef2 = new Definition( InputLoc(),
			prodName, prodElList2, false, 0,
			prodList.length(), Definition::Production );

	prodName->defList.append( newDef2 );
	prodList.append( newDef2 );

	return prodName;
}

void ParseData::resolveReferenceFactor( PdaFactor *fact )
{
	/* Look for the production's associated region. */
	Namespace *nspace = fact->nspaceQual->getQual( this );

	if ( nspace == 0 )
		error(fact->loc) << "do not have namespace for resolving reference" << endp;
	
	fact->nspace = nspace;

	/* Look up the language element in the region. */
	KlangEl *langEl = getKlangEl( this, nspace, fact->refName );

	if ( fact->repeatType == RepeatRepeat ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String repeatName( 32, "_repeat_%s", fact->refName.data );

    	SymbolMapEl *inDict = nspace->symbolMap.find( repeatName );
	    if ( inDict != 0 )
			fact->langEl = inDict->value;
		else
			fact->langEl = makeRepeatProd( nspace, repeatName, fact->nspaceQual, fact->refName );
	}
	else if ( fact->repeatType == RepeatList ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String listName( 32, "_list_%s", fact->refName.data );

    	SymbolMapEl *inDict = nspace->symbolMap.find( listName );
	    if ( inDict != 0 )
			fact->langEl = inDict->value;
		else
			fact->langEl = makeListProd( nspace, listName, fact->nspaceQual, fact->refName );
	}
	else if ( fact->repeatType == RepeatOpt ) {
		/* If the factor is an opt, create the opt element and link the factor
		 * to it. */
		String optName( 32, "_opt_%s", fact->refName.data );

    	SymbolMapEl *inDict = nspace->symbolMap.find( optName );
	    if ( inDict != 0 )
			fact->langEl = inDict->value;
		else
			fact->langEl = makeOptProd( nspace, optName, fact->nspaceQual, fact->refName );
	}
	else {
		/* The factor is not a repeat. Link to the language element. */
		fact->langEl = langEl;
	}
}

void ParseData::resolveFactor( PdaFactor *fact )
{
	switch ( fact->type ) {
		case PdaFactor::LiteralType:
			resolveLiteralFactor( fact );
			break;
		case PdaFactor::ReferenceType:
			resolveReferenceFactor( fact );
			break;
	}
}

/* Resolves production els and computes the precedence of each prod. */
void ParseData::resolveProductionEls()
{
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		/* First resolve. */
		for ( ProdElList::Iter fact = *prod->prodElList; fact.lte(); fact++ )
			resolveFactor( fact );

		/* If there is no explicit precdence ... */
		if ( prod->predOf == 0 )  {
			/* Compute the precedence of the productions. */
			for ( ProdElList::Iter fact = prod->prodElList->last(); fact.gtb(); fact-- ) {
				/* Production inherits the precedence of the last terminal with
				 * precedence. */
				if ( fact->langEl->predType != PredNone ) {
					prod->predOf = fact->langEl;
					break;
				}
			}
		}
	}
}

Namespace *Namespace::findNamespace( const String &name )
{
	for ( NamespaceVect::Iter c = childNamespaces; c.lte(); c++ ) {
		if ( strcmp( name, (*c)->name ) == 0 )
			return *c;
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

Namespace *NamespaceQual::getQual( ParseData *pd )
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

void ParseData::resolvePatternEls()
{
	for ( PatternList::Iter pat = patternList; pat.lte(); pat++ ) {
		for ( PatternItemList::Iter item = *pat->list; item.lte(); item++ ) {
			switch ( item->type ) {
			case PatternItem::FactorType:
				/* Use pdaFactor reference resolving. */
				resolveFactor( item->factor );
				break;
			case PatternItem::InputText:
				/* Nothing to do here. */
				break;
			}
		}
	}
}

void ParseData::resolveReplacementEls()
{
	for ( ReplList::Iter repl = replList; repl.lte(); repl++ ) {
		for ( ReplItemList::Iter item = *repl->list; item.lte(); item++ ) {
			switch ( item->type ) {
			case ReplItem::FactorType:
				/* Use pdaFactor reference resolving. */
				resolveFactor( item->factor );
				break;
			case ReplItem::InputText:
			case ReplItem::VarRefType:
				break;
			}
		}
	}
}

void ParseData::resolveAccumEls()
{
	for ( AccumTextList::Iter accum = accumTextList; accum.lte(); accum++ ) {
		for ( ReplItemList::Iter item = *accum->list; item.lte(); item++ ) {
			switch ( item->type ) {
			case ReplItem::FactorType:
				resolveFactor( item->factor );
				break;
			case ReplItem::InputText:
			case ReplItem::VarRefType:
				break;
			}
		}
	}
}

void ParseData::initEmptyScanners()
{
	for ( RegionList::Iter reg = regionList; reg.lte(); reg++ ) {
		if ( reg->tokenDefList.length() == 0 ) {
			InputLoc loc;
			String name( reg->name.length() + 16, "__%s_DEF_PAT", reg->name.data );

			KlangEl *lel = getKlangEl( this, rootNamespace, name.data );
			assert( lel->type == KlangEl::Unknown );
			lel->type = KlangEl::Term;

			Join *join = new Join( new Expression( BT_Any ) );
				
			TokenDef *tokenDef = new TokenDef( join, lel, loc, nextTokenId++,
					rootNamespace, reg );
			reg->tokenDefList.append( tokenDef );
			lel->tokenDef = tokenDef;
		}
	}
}


void ParseData::parsePatterns()
{
	Program program( 0, 0, false, runtimeData );
	FsmRun fsmRun( &program );
	Tree **vm_stack = stack_alloc();
	Tree **root = &vm_stack[VM_STACK_SIZE];

	for ( ReplList::Iter repl = replList; repl.lte(); repl++ ) {
		if ( colm_log_compile ) {
			cerr << "parsing replacement at " << 
					repl->loc.line << ' ' << repl->loc.col << endl;
		}

		InputStreamRepl in( repl );
		init_input_stream( &in );

		repl->pdaRun = new PdaRun( &program,
				pdaTables, repl->langEl->parserId, 0, false );
		parse( root, &in, &fsmRun, repl->pdaRun );

			//#ifdef COLM_LOG_COMPILE
			//if ( colm_log_compile ) {
			//xml_print_list( runtimeData, repl->pdaRun->stackTop, 0 );
			//#endif
			//}
	}

	for ( PatternList::Iter pat = patternList; pat.lte(); pat++ ) {
		if ( colm_log_compile ) {
			cerr << "parsing pattern at " << 
					pat->loc.line << ' ' << pat->loc.col << endl;
		}

		InputStreamPattern in( pat );
		init_input_stream( &in );

		pat->pdaRun = new PdaRun( &program,
				pdaTables, pat->langEl->parserId, 0, false );
		parse( root, &in, &fsmRun, pat->pdaRun );

			//#ifdef COLM_LOG_COMPILE
			//if ( colm_log_compile ) {
			//xml_print_list( runtimeData, pat->pdaRun->stackTop, 0 );
			//#endif
			//}
	}

	fillInPatterns( &program );
}

void ParseData::resolveUses()
{
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->objectDefUses != 0 ) {
			/* Look for the production's associated region. */
			Namespace *nspace = lel->objectDefUsesQual->getQual( this );

			if ( nspace == 0 )
				error() << "do not have namespace for resolving reference" << endp;
	
			/* Look up the language element in the region. */
			KlangEl *langEl = getKlangEl( this, nspace, lel->objectDefUses );
			lel->objectDef = langEl->objectDef;
		}
	}
}

void ParseData::collectParserEls( BstSet<KlangEl*> &parserEls )
{
	for ( PatternList::Iter pat = patternList; pat.lte(); pat++ ) {
		/* We assume the reduction action compilation phase was run before
		 * pattern parsing and it decorated the pattern with the target type. */
		assert( pat->langEl != 0 );
		if ( pat->langEl->type != KlangEl::NonTerm )
			error(pat->loc) << "pattern type is not a non-terminal" << endp;

		if ( pat->langEl->parserId < 0 ) {
			/* Make a parser for the language element. */
			parserEls.insert( pat->langEl );
			pat->langEl->parserId = nextParserId++;
		}
	}

	for ( ReplList::Iter repl = replList; repl.lte(); repl++ ) {
		/* We assume the reduction action compilation phase was run before
		 * replacement parsing decorated the replacement with the target type. */
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

void ParseData::semanticAnalysis()
{
	beginProcessing();
	initKeyOps();

	/* Resolve uses statements. */
	resolveUses();
	
	/* Init the longest match data and create the default scanner which will
	 * return single characters for us when we have no other scanner */
	initLongestMatchData();
	createDefaultScanner();

	/* Resolve pattern and replacement elements. */
	resolvePatternEls();
	resolveReplacementEls();
	resolveAccumEls();

	analyzeParseTree();

	/* This needs to happen before the scanner is built. */
	resolveProductionEls();

	/* Fill any empty scanners with a default token. */
	initEmptyScanners();

	FsmGraph *fsmGraph = makeScanner();

	if ( colm_log_compile ) {
		printNameTree( fsmGraph->rootName );
		printNameIndex( fsmGraph->nameIndex );
	}

	prepGrammar();

	/* Compile bytecode. */
	compileByteCode();

	/* Make the reduced fsm. */
	RedFsmBuild reduce( sectionName, this, fsmGraph );
	redFsm = reduce.reduceMachine();

	BstSet<KlangEl*> parserEls;
	collectParserEls( parserEls );

	makeParser( parserEls );

	/* Make the scanner tables. */
	fsmTables = redFsm->makeFsmTables();

	/* Now that all parsers are built, make the global runtimeData. */
	makeRuntimeData();

	/* 
	 * All compilation is now complete.
	 */
	
	/* Parse patterns and replacements. */
	parsePatterns();
}

void ParseData::generateOutput()
{
	FsmCodeGen *fsmGen = new FsmCodeGen("<INPUT>", sectionName,
			*outStream, redFsm, fsmTables );

	PdaCodeGen *pdaGen = new PdaCodeGen( outputFileName, "parser", this, *outStream );

	pdaGen->writeFirst();
	fsmGen->writeCode();

	/* Make parsers that we need. */
	pdaGen->writeParserData( 0, pdaTables );

	/* Write the runtime data. */
	pdaGen->writeRuntimeData( runtimeData, pdaTables );

	outStream->flush();
}
