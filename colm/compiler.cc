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
#include "lmparse.h"
#include "parsedata.h"
#include "parsetree.h"
#include "mergesort.h"
#include "redbuild.h"
#include "pdacodegen.h"
#include "fsmcodegen.h"
#include "fsmrun.h"
#include "pdarun.h"
#include "colm.h"
#include "pool.h"

using namespace std;
using std::ostringstream;

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

	if ( unusedBits && keyOps->alphType->isSigned && ul >> (size * 8 - 1) )
		ul |= (0xffffffff >> (size*8 ) ) << (size*8);

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

	if ( keyOps->alphType->isSigned )
		return Key( (long)ll );
	else
		return Key( (unsigned long)ll );
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
void makeFsmKeyArray( Key *result, char *data, int len, Compiler *pd )
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
		bool caseInsensitive, Compiler *pd )
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
 * Compiler
 */

/* Initialize the structure that will collect info during the parse of a
 * machine. */
Compiler::Compiler( const String &fileName, const String &sectionName, 
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
	mainReturnUT(0),
	parserName(sectionName),
	out(out),
	access(0),
	tokenStruct(0),
	rootLangEl(0),
	eofLangEl(0),
	errorLangEl(0),
	defaultCharLangEl(0),
	rootRegion(0),
	defaultRegion(0),
	firstNonTermId(0),
	prodIdIndex(0),
	nextPatConsId(0),
	nextGenericId(1),
	nextFuncId(0),
	loopCleanup(0),
	nextObjectId(1),     /* 0 is  reserved for no object. */
	nextFrameId(0),
	nextParserId(0),
	revertOn(true),
	predValue(0),
	nextMatchEndNum(0),
	argvTypeRef(0),
	context(0),
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
}

/* Make a name id in the current name instantiation scope if it is not
 * already there. */
NameInst *Compiler::addNameInst( const InputLoc &loc, char *data, bool isLabel )
{
	/* Create the name instantitaion object and insert it. */
	NameInst *newNameInst = new NameInst( loc, curNameInst, data, nextNameId++, isLabel );
	curNameInst->childVect.append( newNameInst );
	if ( data != 0 )
		curNameInst->children.insertMulti( data, newNameInst );
	return newNameInst;
}

void Compiler::initNameWalk( NameInst *rootName )
{
	curNameInst = rootName;
	curNameChild = 0;
}

/* Goes into the next child scope. The number of the child is already set up.
 * We need this for the syncronous name tree and parse tree walk to work
 * properly. It is reset on entry into a scope and advanced on poping of a
 * scope. A call to enterNameScope should be accompanied by a corresponding
 * popNameScope. */
NameFrame Compiler::enterNameScope( bool isLocal, int numScopes )
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
void Compiler::popNameScope( const NameFrame &frame )
{
	/* Pop the name scope. */
	curNameInst = frame.prevNameInst;
	curNameChild = frame.prevNameChild+1;
	localNameScope = frame.prevLocalScope;
}

void Compiler::resetNameScope( const NameFrame &frame )
{
	/* Pop the name scope. */
	curNameInst = frame.prevNameInst;
	curNameChild = frame.prevNameChild;
	localNameScope = frame.prevLocalScope;
}


void Compiler::unsetObsoleteEntries( FsmGraph *graph )
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

NameSet Compiler::resolvePart( NameInst *refFrom, const char *data, bool recLabelsOnly )
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

void Compiler::resolveFrom( NameSet &result, NameInst *refFrom, 
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


void Compiler::referenceRegions( NameInst *rootName )
{
	for ( NameVect::Iter inst = rootName->childVect; inst.lte(); inst++ ) {
		/* Inc the reference in the name. This will cause the entry point to
		 * survive to the end of the graph generating walk. */
		(*inst)->numRefs += 1;
	}
}

/* Walk a name tree starting at from and fill the name index. */
void Compiler::fillNameIndex( NameInst **nameIndex, NameInst *from )
{
	/* Fill the value for from in the name index. */
	nameIndex[from->id] = from;

	/* Recurse on the implicit final state and then all children. */
	if ( from->final != 0 )
		fillNameIndex( nameIndex, from->final );
	for ( NameVect::Iter name = from->childVect; name.lte(); name++ )
		fillNameIndex( nameIndex, *name );
}

NameInst **Compiler::makeNameIndex( NameInst *rootName )
{
	/* The number of nodes in the tree can now be given by nextNameId. Put a
	 * null pointer on the end of the list to terminate it. */
	NameInst **nameIndex = new NameInst*[nextNameId+1];
	memset( nameIndex, 0, sizeof(NameInst*)*(nextNameId+1) );
	fillNameIndex( nameIndex, rootName );
	return nameIndex;
}

void Compiler::createBuiltin( const char *name, BuiltinMachine builtin )
{
	LexExpression *expression = LexExpression::cons( builtin );
	LexJoin *join = new LexJoin( expression );
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

	thisCondData.nextCondKey = thisKeyOps.maxKey;
	thisCondData.nextCondKey.increment();
}

void Compiler::printNameInst( NameInst *nameInst, int level )
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
	if ( regionList.length() > 0 ) {
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

void Compiler::printNameTree( NameInst *rootName )
{
	/* Print the name instance map. */
	cerr << "name tree:" << endl;
	for ( NameVect::Iter name = rootName->childVect; name.lte(); name++ )
		printNameInst( *name, 0 );
}

void Compiler::printNameIndex( NameInst **nameIndex )
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
NameInst *Compiler::makeNameTree()
{
	/* Create the root name. */
	nextNameId = 0;
	NameInst *rootName = new NameInst( InputLoc(), 0, 0, nextNameId++, false );

	/* First make the name tree. */
	initNameWalk( rootName );
	for ( RegionGraphList::Iter glel = instanceList; glel.lte(); glel++ ) {
		/* Recurse on the instance. */
		glel->value->makeNameTree( glel->loc, this );
	}

	return rootName;
}

FsmGraph *Compiler::makeAllRegions()
{
	/* Build the name tree and supporting data structures. */
	NameInst *rootName = makeNameTree( );
	NameInst **nameIndex = makeNameIndex( rootName );

	/* Resovle the implicit name references to the nfa instantiations. */
	referenceRegions( rootName );

	int numGraphs = 0;
	FsmGraph **graphs = new FsmGraph*[instanceList.length()];

	/* Make all the instantiations, we know that main exists in this list. */
	initNameWalk( rootName );
	for ( RegionGraphList::Iter glel = instanceList; glel.lte();  glel++ ) {
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

void Compiler::analyzeAction( Action *action, InlineList *inlineList )
{
	/* FIXME: Actions used as conditions should be very constrained. */
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		//if ( item->type == InlineItem::Call || item->type == InlineItem::CallExpr )
		//	action->anyCall = true;

		/* Need to recurse into longest match items. */
		if ( item->type == InlineItem::LmSwitch ) {
			TokenRegion *lm = item->tokenRegion;
			for ( TokenDefListReg::Iter lmi = lm->tokenDefList; lmi.lte(); lmi++ ) {
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

		for ( StateCondList::Iter sc = st->stateCondList; sc.lte(); sc++ ) {
			for ( CondSet::Iter sci = sc->condSpace->condSet; sci.lte(); sci++ )
				(*sci)->numCondRefs += 1;
		}
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

void Compiler::createDefaultScanner()
{
	InputLoc loc = { 0, 0, 0 };

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

	/* Insert the machine definition into the graph dictionary. */
	RegionGraphDictEl *newEl = rootNamespace->graphDict.insert( name );
	assert( newEl != 0 );
	newEl->value = new RegionDef( name, defaultRegion );
	newEl->isInstance = true;
	instanceList.append( newEl );

	LexJoin *join = new LexJoin( LexExpression::cons( BT_Any ) );
		
	TokenDef *tokenDef = new TokenDef( name, String(), false, false, 
			join, 0, loc, nextTokenId++, 
			rootNamespace, defaultRegion, 0, 0, 0 );

	defaultRegion->tokenDefList.append( tokenDef );

	/* Now create the one and only token -> "<chr>" / any /  */
	name = "___DEFAULT_SCANNER_CHR";
	defaultCharLangEl = addLangEl( this, defaultNamespace, name, LangEl::Term );

	tokenDef->tdLangEl = defaultCharLangEl;
	defaultCharLangEl->tokenDef = tokenDef;
}

LangEl *Compiler::makeRepeatProd( Namespace *nspace, const String &repeatName, 
		NamespaceQual *nspaceQual, const String &name )
{
	LangEl *prodName = addLangEl( this, nspace, repeatName, LangEl::NonTerm );
	prodName->isRepeat = true;

	ProdElList *prodElList1 = new ProdElList;

	/* Build the first production of the repeat. */
	TypeRef *typeRef1 = TypeRef::cons( InputLoc(), nspaceQual, name );
	ProdEl *factor1 = new ProdEl( ProdEl::ReferenceType, InputLoc(), 0, false, typeRef1, 0 );

	UniqueType *prodNameUT = findUniqueType( TYPE_TREE, prodName );
	TypeRef *typeRef2 = TypeRef::cons( InputLoc(), prodNameUT );
	ProdEl *factor2 = new ProdEl( ProdEl::ReferenceType, InputLoc(), 0, false, typeRef2, 0 );

	prodElList1->append( factor1 );
	prodElList1->append( factor2 );

	Production *newDef1 = Production::cons( InputLoc(),
			prodName, prodElList1, false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef1 );
	prodList.append( newDef1 );

	/* Build the second production of the repeat. */
	ProdElList *prodElList2 = new ProdElList;

	Production *newDef2 = Production::cons( InputLoc(),
			prodName, prodElList2, false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef2 );
	prodList.append( newDef2 );

	return prodName;
}

LangEl *Compiler::makeListProd( Namespace *nspace, const String &listName, NamespaceQual *nspaceQual, const String &name )
{
	LangEl *prodName = addLangEl( this, nspace, listName, LangEl::NonTerm );
	prodName->isList = true;

	/* Build the first production of the list. */
	TypeRef *typeRef1 = TypeRef::cons( InputLoc(), nspaceQual, name );
	ProdEl *factor1 = new ProdEl( ProdEl::ReferenceType, InputLoc(), 0, false, typeRef1, 0 );

	UniqueType *prodNameUT = findUniqueType( TYPE_TREE, prodName );
	TypeRef *typeRef2 = TypeRef::cons( InputLoc(), prodNameUT );
	ProdEl *factor2 = new ProdEl( ProdEl::ReferenceType, InputLoc(), 0, false, typeRef2, 0 );

	ProdElList *prodElList1 = new ProdElList;
	prodElList1->append( factor1 );
	prodElList1->append( factor2 );

	Production *newDef1 = Production::cons( InputLoc(),
			prodName, prodElList1, false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef1 );
	prodList.append( newDef1 );

	/* Build the second production of the list. */
	TypeRef *typeRef3 = TypeRef::cons( InputLoc(), nspaceQual, name );
	ProdEl *factor3 = new ProdEl( ProdEl::ReferenceType, InputLoc(), 0, false, typeRef3, 0 );

	ProdElList *prodElList2 = new ProdElList;
	prodElList2->append( factor3 );

	Production *newDef2 = Production::cons( InputLoc(),
			prodName, prodElList2, false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef2 );
	prodList.append( newDef2 );

	return prodName;
}

LangEl *Compiler::makeOptProd( Namespace *nspace, const String &optName, NamespaceQual *nspaceQual, const String &name )
{
	LangEl *prodName = addLangEl( this, nspace, optName, LangEl::NonTerm );
	prodName->isOpt = true;

	ProdElList *prodElList1 = new ProdElList;

	/* Build the first production of the repeat. */
	TypeRef *typeRef1 = TypeRef::cons( InputLoc(), nspaceQual, name );
	ProdEl *factor1 = new ProdEl( ProdEl::ReferenceType, InputLoc(), 0, false, typeRef1, 0 );
	prodElList1->append( factor1 );

	Production *newDef1 = Production::cons( InputLoc(),
			prodName, prodElList1, false, 0,
			prodList.length(), prodName->defList.length() );

	prodName->defList.append( newDef1 );
	prodList.append( newDef1 );

	/* Build the second production of the repeat. */
	ProdElList *prodElList2 = new ProdElList;

	Production *newDef2 = Production::cons( InputLoc(),
			prodName, prodElList2, false, 0,
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

void Compiler::initEmptyScanners()
{
	for ( RegionList::Iter reg = regionList; reg.lte(); reg++ ) {
		if ( reg->tokenDefList.length() == 0 ) {
			reg->wasEmpty = true;

			static int def = 1;
			InputLoc loc = { 0, 0, 0 };
			String name( reg->name.length() + 16, "__%s_DEF_PAT_%d", reg->name.data, def++ );

			LexJoin *join = new LexJoin( LexExpression::cons( BT_Any ) );
				
			TokenDef *tokenDef = new TokenDef( name, String(), false, false, join, 
					0, loc, nextTokenId++, rootNamespace, reg, 0, 0, 0 );
			reg->tokenDefList.append( tokenDef );

			/* These do not go in the namespace so so they cannot get declared
			 * in the declare pass. */
			LangEl *lel = addLangEl( this, rootNamespace, name, LangEl::Term );

			tokenDef->tdLangEl = lel;
			lel->tokenDef = tokenDef;
		}
	}
}


void Compiler::parsePatterns()
{
	Program *prg = colmNewProgram( runtimeData );

	/* Turn off context-dependent parsing. */
	prg->ctxDepParsing = 0;

	Tree **sp = prg->stackRoot;

	for ( ConsList::Iter cons = replList; cons.lte(); cons++ ) {
		//cerr << "parsing replacement at " << 
		//		cons->loc.line << ' ' << cons->loc.col << endl;

		InputStream *in = new InputStream;
		FsmRun *fsmRun = new FsmRun;
		cons->pdaRun = new PdaRun;

		initInputStream( in );
		initPdaRun( cons->pdaRun, prg, pdaTables, fsmRun, cons->langEl->parserId, 0, false, 0 );
		initFsmRun( fsmRun, prg );

		Stream *res = streamAllocate( prg );
		res->id = LEL_ID_STREAM;
		res->in = newSourceStreamCons( cons );
		appendStream( in, (Tree*)res );
		setEof( in );

		newToken( prg, cons->pdaRun, fsmRun );
		long pcr = parseLoop( prg, sp, cons->pdaRun, fsmRun, in, PcrStart );
		assert( pcr == PcrDone );
		if ( cons->pdaRun->parseError ) {
			cout << "PARSE ERROR " << cons->loc.line << ":" <<  cons->loc.col;

			if ( cons->pdaRun->parseErrorText != 0 ) {
				cout << ", offset into constructor " << 
						cons->pdaRun->parseErrorText->tokdata->data;
			}

			cout << endl;
		}
	}

	for ( PatList::Iter pat = patternList; pat.lte(); pat++ ) {
		//cerr << "parsing pattern at " << 
		//	pat->loc.line << ' ' << pat->loc.col << endl;

		InputStream *in = new InputStream;
		FsmRun *fsmRun = new FsmRun;
		pat->pdaRun = new PdaRun;

		initInputStream( in );
		initPdaRun( pat->pdaRun, prg, pdaTables, fsmRun, pat->langEl->parserId, 0, false, 0 );
		initFsmRun( fsmRun, prg );

		Stream *res = streamAllocate( prg );
		res->id = LEL_ID_STREAM;
		res->in = newSourceStreamPat( pat );
		appendStream( in, (Tree*)res );
		setEof( in );

		newToken( prg, pat->pdaRun, fsmRun );
		long pcr = parseLoop( prg, sp, pat->pdaRun, fsmRun, in, PcrStart );
		assert( pcr == PcrDone );
		if ( pat->pdaRun->parseError ) {
			cout << "PARSE ERROR " << pat->loc.line << 
					":" <<  pat->loc.col;

			if ( pat->pdaRun->parseErrorText != 0 ) {
				cout << ", offset into pattern " << 
						pat->pdaRun->parseErrorText->tokdata->data;
			}

			cout << endl;
		}
	}

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


void Compiler::generateOutput()
{
	FsmCodeGen *fsmGen = new FsmCodeGen("<INPUT>", sectionName,
			*outStream, redFsm, fsmTables );

	PdaCodeGen *pdaGen = new PdaCodeGen( outputFileName, "parser", this, *outStream );

	fsmGen->writeIncludes();
	pdaGen->defineRuntime();
	fsmGen->writeCode();

	/* Make parsers that we need. */
	pdaGen->writeParserData( 0, pdaTables );

	/* Write the runtime data. */
	pdaGen->writeRuntimeData( runtimeData, pdaTables );

	if ( !gblLibrary ) 
		fsmGen->writeMain();

	outStream->flush();
}


void Compiler::prepGrammar()
{
	/* This will create language elements. */
	wrapNonTerminals();

	makeLangElIds();
	makeLangElNames();
	makeDefinitionNames();
	noUndefindLangEls();

	/* Put the language elements in an index by language element id. */
	langElIndex = new LangEl*[nextSymbolId+1];
	memset( langElIndex, 0, sizeof(LangEl*)*(nextSymbolId+1) );
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ )
		langElIndex[lel->id] = lel;

	makeProdFsms();

	/* Allocate the Runtime data now. Every PdaTable that we make 
	 * will reference it, but it will be filled in after all the tables are
	 * built. */
	runtimeData = new RuntimeData;
}

void Compiler::compile()
{
	beginProcessing();
	initKeyOps();


	/* Type declaration. */
	typeDeclaration();

	/* Type resolving. */
	typeResolve();

	makeTerminalWrappers();
	makeEofElements();

	/*
	 * Parsers
	 */

	/* Init the longest match data */
	initLongestMatchData();
	FsmGraph *fsmGraph = makeScanner();

	//printNameTree( fsmGraph->rootName );
	//printNameIndex( fsmGraph->nameIndex );

	prepGrammar();

	/* Compile bytecode. */
	compileByteCode();

	/* Make the reduced fsm. */
	RedFsmBuild reduce( sectionName, this, fsmGraph );
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
	
	/* Parse patterns and replacements. */
	parsePatterns();
}

