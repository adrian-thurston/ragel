/*
 *  Copyright 2001-2008 Adrian Thurston <thurston@complang.org>
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

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

#include "ragel.h"
#include "parsedata.h"
#include "parsetree.h"
#include "mergesort.h"
#include "version.h"
#include "inputdata.h"
#include "xml.h"
#include <colm/tree.h>

using namespace std;

const char mainMachine[] = "main";

void Token::_set( const char *str, int len )
{
	length = len;
	data = new char[len+1];
	memcpy( data, str, len );
	data[len] = 0;
}

void Token::set( const char *str, int len, colm_location *cl )
{
	_set( str, len );

	if ( cl != 0 ) {
		loc.fileName = cl->name;
		loc.line = cl->line;
		loc.col = cl->column;
	}
}

void Token::set( colm_data *cd, colm_location *cl )
{
	set( cd->data, cd->length, cl );
}

void Token::set( const char *str, int len, const InputLoc &l )
{
	_set( str, len );

	loc.fileName = l.fileName;
	loc.line = l.line;
	loc.col = l.col;
}

void Token::set( const char *str, int len, const ParserLoc &l )
{
	_set( str, len );
	loc = l;
}

void RedToken::set( colm_data *cd, colm_location *cl )
{
	data = cd->data;
	length = cd->length;
	loc.fileName = cl->name;
	loc.line = cl->line;
	loc.col = cl->column;
}

/* Count the transitions in the fsm by walking the state list. */
int countTransitions( FsmAp *fsm )
{
	int numTrans = 0;
	StateAp *state = fsm->stateList.head;
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
	unsigned int size = pd->alphType->size;
	bool unusedBits = size < sizeof(unsigned long);

	unsigned long ul = strtoul( str, 0, 16 );

	if ( errno == ERANGE || ( unusedBits && ul >> (size * 8) ) ) {
		pd->id->error(loc) << "literal " << str << " overflows the alphabet type" << endl;
		ul = 1 << (size * 8);
	}

	if ( unusedBits && pd->alphType->isSigned && ul >> (size * 8 - 1) )
		ul |= ( -1L >> (size*8) ) << (size*8);

	return Key( (long)ul );
}

Key makeFsmKeyDec( char *str, const InputLoc &loc, ParseData *pd )
{
	if ( pd->alphType->isSigned ) {
		/* Convert the number to a decimal. First reset errno so we can check
		 * for overflow or underflow. */
		errno = 0;
		long long minVal = pd->alphType->sMinVal;
		long long maxVal = pd->alphType->sMaxVal;

		long long ll = strtoll( str, 0, 10 );

		/* Check for underflow. */
		if ( ( errno == ERANGE && ll < 0 ) || ll < minVal ) {
			pd->id->error(loc) << "literal " << str << " underflows the alphabet type" << endl;
			ll = minVal;
		}
		/* Check for overflow. */
		else if ( ( errno == ERANGE && ll > 0 ) || ll > maxVal ) {
			pd->id->error(loc) << "literal " << str << " overflows the alphabet type" << endl;
			ll = maxVal;
		}

		return Key( (long)ll );
	}
	else {
		/* Convert the number to a decimal. First reset errno so we can check
		 * for overflow or underflow. */
		errno = 0;
		unsigned long long minVal = pd->alphType->uMinVal;
		unsigned long long maxVal = pd->alphType->uMaxVal;

		unsigned long long ull = strtoull( str, 0, 10 );

		/* Check for underflow. */
		if ( ( errno == ERANGE && ull < 0 ) || ull < minVal ) {
			pd->id->error(loc) << "literal " << str << " underflows the alphabet type" << endl;
			ull = minVal;
		}
		/* Check for overflow. */
		else if ( ( errno == ERANGE && ull > 0 ) || ull > maxVal ) {
			pd->id->error(loc) << "literal " << str << " overflows the alphabet type" << endl;
			ull = maxVal;
		}

		return Key( (unsigned long)ull );
	}
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
	if ( pd->fsmCtx->keyOps->isSigned ) {
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
	if ( pd->fsmCtx->keyOps->isSigned ) {
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
void makeFsmUniqueKeyArray( KeySet &result, const char *data, int len, 
		bool caseInsensitive, ParseData *pd )
{
	/* Use a transitions list for getting unique keys. */
	if ( pd->fsmCtx->keyOps->isSigned ) {
		/* Copy from a char star type. */
		const char *src = data;
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
		const unsigned char *src = (unsigned char*) data;
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

/* Make a builtin type. Depends on the signed nature of the alphabet type. */
FsmAp *makeBuiltin( BuiltinMachine builtin, ParseData *pd )
{
	/* FsmAp created to return. */
	FsmAp *retFsm = 0;
	bool isSigned = pd->fsmCtx->keyOps->isSigned;

	switch ( builtin ) {
	case BT_Any: {
		/* All characters. */
		retFsm = FsmAp::dotFsm( pd->fsmCtx );
		break;
	}
	case BT_Ascii: {
		/* Ascii characters 0 to 127. */
		retFsm = FsmAp::rangeFsm( pd->fsmCtx, 0, 127 );
		break;
	}
	case BT_Extend: {
		/* Ascii extended characters. This is the full byte range. Dependent
		 * on signed, vs no signed. If the alphabet is one byte then just use
		 * dot fsm. */
		if ( isSigned )
			retFsm = FsmAp::rangeFsm( pd->fsmCtx, -128, 127 );
		else
			retFsm = FsmAp::rangeFsm( pd->fsmCtx, 0, 255 );
		break;
	}
	case BT_Alpha: {
		/* Alpha [A-Za-z]. */
		FsmAp *upper = FsmAp::rangeFsm( pd->fsmCtx, 'A', 'Z' );
		FsmAp *lower = FsmAp::rangeFsm( pd->fsmCtx, 'a', 'z' );
		FsmRes res = FsmAp::unionOp( upper, lower );
		upper = res.fsm;
		upper->minimizePartition2();
		retFsm = upper;
		break;
	}
	case BT_Digit: {
		/* Digits [0-9]. */
		retFsm = FsmAp::rangeFsm( pd->fsmCtx, '0', '9' );
		break;
	}
	case BT_Alnum: {
		/* Alpha numerics [0-9A-Za-z]. */
		FsmAp *digit = FsmAp::rangeFsm( pd->fsmCtx, '0', '9' );
		FsmAp *upper = FsmAp::rangeFsm( pd->fsmCtx, 'A', 'Z' );
		FsmAp *lower = FsmAp::rangeFsm( pd->fsmCtx, 'a', 'z' );
		FsmRes res1 = FsmAp::unionOp( digit, upper );
		digit = res1.fsm;
		FsmRes res2 = FsmAp::unionOp( digit, lower );
		digit = res2.fsm;
		digit->minimizePartition2();
		retFsm = digit;
		break;
	}
	case BT_Lower: {
		/* Lower case characters. */
		retFsm = FsmAp::rangeFsm( pd->fsmCtx, 'a', 'z' );
		break;
	}
	case BT_Upper: {
		/* Upper case characters. */
		retFsm = FsmAp::rangeFsm( pd->fsmCtx, 'A', 'Z' );
		break;
	}
	case BT_Cntrl: {
		/* Control characters. */
		FsmAp *cntrl = FsmAp::rangeFsm( pd->fsmCtx, 0, 31 );
		FsmAp *highChar = FsmAp::concatFsm( pd->fsmCtx, 127 );
		FsmRes res = FsmAp::unionOp( cntrl, highChar );
		cntrl = res.fsm;
		cntrl->minimizePartition2();
		retFsm = cntrl;
		break;
	}
	case BT_Graph: {
		/* Graphical ascii characters [!-~]. */
		retFsm = FsmAp::rangeFsm( pd->fsmCtx, '!', '~' );
		break;
	}
	case BT_Print: {
		/* Printable characters. Same as graph except includes space. */
		retFsm = FsmAp::rangeFsm( pd->fsmCtx, ' ', '~' );
		break;
	}
	case BT_Punct: {
		/* Punctuation. */
		FsmAp *range1 = FsmAp::rangeFsm( pd->fsmCtx, '!', '/' );
		FsmAp *range2 = FsmAp::rangeFsm( pd->fsmCtx, ':', '@' );
		FsmAp *range3 = FsmAp::rangeFsm( pd->fsmCtx, '[', '`' );
		FsmAp *range4 = FsmAp::rangeFsm( pd->fsmCtx, '{', '~' );

		FsmRes res1 = FsmAp::unionOp( range1, range2 );
		range1 = res1.fsm;
		FsmRes res2 = FsmAp::unionOp( range1, range3 );
		range1 = res2.fsm;
		FsmRes res3 = FsmAp::unionOp( range1, range4 );
		range1 = res3.fsm;
		range1->minimizePartition2();
		retFsm = range1;
		break;
	}
	case BT_Space: {
		/* Whitespace: [\t\v\f\n\r ]. */
		FsmAp *cntrl = FsmAp::rangeFsm( pd->fsmCtx, '\t', '\r' );
		FsmAp *space = FsmAp::concatFsm( pd->fsmCtx, ' ' );
		FsmRes res = FsmAp::unionOp( cntrl, space );
		cntrl = res.fsm;
		cntrl->minimizePartition2();
		retFsm = cntrl;
		break;
	}
	case BT_Xdigit: {
		/* Hex digits [0-9A-Fa-f]. */
		FsmAp *digit = FsmAp::rangeFsm( pd->fsmCtx, '0', '9' );
		FsmAp *upper = FsmAp::rangeFsm( pd->fsmCtx, 'A', 'F' );
		FsmAp *lower = FsmAp::rangeFsm( pd->fsmCtx, 'a', 'f' );

		FsmRes res1 = FsmAp::unionOp( digit, upper );
		digit = res1.fsm;
		FsmRes res2 = FsmAp::unionOp( digit, lower );
		digit = res2.fsm;

		digit->minimizePartition2();
		retFsm = digit;
		break;
	}
	case BT_Lambda: {
		retFsm = FsmAp::lambdaFsm( pd->fsmCtx );
		break;
	}
	case BT_Empty: {
		retFsm = FsmAp::emptyFsm( pd->fsmCtx );
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

NameInst::~NameInst()
{
	/* Recurse on the implicit final state and then all children. */
	if ( final != 0 )
		delete final;
	for ( NameVect::Iter name = childVect; name.lte(); name++ )
		delete *name;
}

/*
 * ParseData
 */

/* Initialize the structure that will collect info during the parse of a
 * machine. */
ParseData::ParseData( InputData *id, string sectionName, 
		int machineId, const InputLoc &sectionLoc, const HostLang *hostLang,
		MinimizeLevel minimizeLevel, MinimizeOpt minimizeOpt )
:	
	sectionName(sectionName),
	sectionGraph(0),
	/* 0 is reserved for global error actions. */
	nextLocalErrKey(1),
	nextNameId(0),
	alphTypeSet(false),
	lowerNum(0),
	upperNum(0),
	id(id),
	machineId(machineId),
	sectionLoc(sectionLoc),
	rootName(0),
	exportsRootName(0),
	nextEpsilonResolvedLink(0),
	nextLongestMatchId(1),
	cgd(0)
{
	fsmCtx = new FsmCtx( id );

	/* Initialize the dictionary of graphs. This is our symbol table. The
	 * initialization needs to be done on construction which happens at the
	 * beginning of a machine spec so any assignment operators can reference
	 * the builtins. */
	initGraphDict();
}

/* Clean up the data collected during a parse. */
ParseData::~ParseData()
{
	graphDict.empty();
	fsmCtx->actionList.empty();

	if ( fsmCtx->nameIndex != 0 )
		delete[] fsmCtx->nameIndex;

	if ( rootName != 0 )
		delete rootName;
	if ( exportsRootName != 0 )
		delete exportsRootName;

	delete fsmCtx;
}

/* Make a name id in the current name instantiation scope if it is not
 * already there. */
NameInst *ParseData::addNameInst( const InputLoc &loc, std::string data, bool isLabel )
{
	/* Create the name instantitaion object and insert it. */
	NameInst *newNameInst = new NameInst( loc, curNameInst, data, nextNameId++, isLabel );
	curNameInst->childVect.append( newNameInst );
	if ( !data.empty() )
		curNameInst->children.insertMulti( data, newNameInst );
	return newNameInst;
}

void ParseData::initNameWalk()
{
	curNameInst = rootName;
	curNameChild = 0;
}

void ParseData::initExportsNameWalk()
{
	curNameInst = exportsRootName;
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


void ParseData::unsetObsoleteEntries( FsmAp *graph )
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
			assert( graph->entryPoints.find( name->id ) == 0 );
		}
	}
}

NameSet ParseData::resolvePart( NameInst *refFrom,
		const std::string &data, bool recLabelsOnly )
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
		NameRef *nameRef, int namePos )
{
	/* Look for the name in the owning scope of the factor with aug. */
	NameSet partResult = resolvePart( refFrom, nameRef->data[namePos], false );
	
	/* If there are more parts to the name then continue on. */
	if ( ++namePos < nameRef->length() ) {
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

/* Write out a name reference. */
ostream &operator<<( ostream &out, const NameRef &nameRef )
{
	int pos = 0;
	if ( nameRef[pos] == "" ) {
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
		out << "::" << ( !parents[p]->name.empty() ? parents[p]->name : "<ANON>" );

	/* Write the name and cleanup. */
	out << "::" << ( !nameInst.name.empty() ? nameInst.name : "<ANON>" );
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

void ParseData::errorStateLabels( const NameSet &resolved )
{
	MergeSort<NameInst*, CmpNameInstLoc> mergeSort;
	mergeSort.sort( resolved.data, resolved.length() );
	for ( NameSet::Iter res = resolved; res.lte(); res++ )
		id->error((*res)->loc) << "  -> " << **res << endl;
}


NameInst *ParseData::resolveStateRef( NameRef *nameRef, InputLoc &loc, Action *action )
{
	NameInst *nameInst = 0;

	/* Do the local search if the name is not strictly a root level name
	 * search. */
	if ( nameRef->data[0] != "" ) {
		/* If the action is referenced, resolve all of them. */
		if ( action != 0 && action->embedRoots.length() > 0 ) {
			/* Look for the name in all referencing scopes. */
			NameSet resolved;
			for ( NameInstVect::Iter actRef = action->embedRoots; actRef.lte(); actRef++ )
				resolveFrom( resolved, *actRef, nameRef, 0 );

			if ( resolved.length() > 0 ) {
				/* Take the first one. */
				nameInst = resolved[0];
				if ( resolved.length() > 1 ) {
					/* Complain about the multiple references. */
					id->error(loc) << "state reference " << *nameRef << 
							" resolves to multiple entry points" << endl;
					errorStateLabels( resolved );
				}
			}
		}
	}

	/* If not found in the local scope, look in global. */
	if ( nameInst == 0 ) {
		NameSet resolved;
		int fromPos = nameRef->data[0] != "" ? 0 : 1;
		resolveFrom( resolved, rootName, nameRef, fromPos );

		if ( resolved.length() > 0 ) {
			/* Take the first. */
			nameInst = resolved[0];
			if ( resolved.length() > 1 ) {
				/* Complain about the multiple references. */
				id->error(loc) << "state reference " << *nameRef << 
						" resolves to multiple entry points" << endl;
				errorStateLabels( resolved );
			}
		}
	}

	if ( nameInst == 0 ) {
		/* If not found then complain. */
		id->error(loc) << "could not resolve state reference " << *nameRef << endl;
	}
	return nameInst;
}

void ParseData::resolveNameRefs( InlineList *inlineList, Action *action )
{
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
			case InlineItem::Entry: case InlineItem::Goto:
			case InlineItem::Call: case InlineItem::Ncall:
			case InlineItem::Next: {
				/* Resolve, pass action for local search. */
				NameInst *target = resolveStateRef( item->nameRef, item->loc, action );

				/* Name lookup error reporting is handled by resolveStateRef. */
				if ( target != 0 ) {
					/* Check if the target goes into a longest match. */
					NameInst *search = target->parent;
					while ( search != 0 ) {
						if ( search->isLongestMatch ) {
							id->error(item->loc) << "cannot enter inside a longest "
									"match construction as an entry point" << endl;
							break;
						}
						search = search->parent;
					}

					/* Record the reference in the name. This will cause the
					 * entry point to survive to the end of the graph
					 * generating walk. */
					target->numRefs += 1;
				}

				item->nameTarg = target;
				break;
			}
			default:
				break;
		}

		/* Some of the item types may have children. */
		if ( item->children != 0 )
			resolveNameRefs( item->children, action );
	}
}

/* Resolve references to labels in actions. */
void ParseData::resolveActionNameRefs()
{
	for ( ActionList::Iter act = fsmCtx->actionList; act.lte(); act++ ) {
		/* Only care about the actions that are referenced. */
		if ( act->embedRoots.length() > 0 )
			resolveNameRefs( act->inlineList, act );
	}
}

/* Walk a name tree starting at from and fill the name index. */
void ParseData::fillNameIndex( NameInst *from )
{
	/* Fill the value for from in the name index. */
	fsmCtx->nameIndex[from->id] = from;

	/* Recurse on the implicit final state and then all children. */
	if ( from->final != 0 )
		fillNameIndex( from->final );
	for ( NameVect::Iter name = from->childVect; name.lte(); name++ )
		fillNameIndex( *name );
}

void ParseData::makeRootNames()
{
	/* Create the root name. */
	rootName = new NameInst( InputLoc(), 0, string(), nextNameId++, false );
	exportsRootName = new NameInst( InputLoc(), 0, string(), nextNameId++, false );
}

/* Build the name tree and supporting data structures. */
void ParseData::makeNameTree( GraphDictEl *dictEl )
{
	/* Set up curNameInst for the walk. */
	initNameWalk();

	if ( dictEl != 0 ) {
		/* A start location has been specified. */
		dictEl->value->makeNameTree( dictEl->loc, this );
	}
	else {
		/* First make the name tree. */
		for ( GraphList::Iter glel = instanceList; glel.lte(); glel++ ) {
			/* Recurse on the instance. */
			glel->value->makeNameTree( glel->loc, this );
		}
	}
	
	/* The number of nodes in the tree can now be given by nextNameId */
	fsmCtx->nameIndex = new NameInst*[nextNameId];
	memset( fsmCtx->nameIndex, 0, sizeof(NameInst*)*nextNameId );
	fillNameIndex( rootName );
	fillNameIndex( exportsRootName );
}


void ParseData::createBuiltin( const char *name, BuiltinMachine builtin )
{
	Expression *expression = new Expression( builtin );
	Join *join = new Join( expression );
	MachineDef *machineDef = new MachineDef( join );
	VarDef *varDef = new VarDef( name, machineDef );
	GraphDictEl *graphDictEl = new GraphDictEl( name, varDef );
	graphDict.insert( graphDictEl );
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

/* Set the alphabet type. If the types are not valid returns false. */
bool ParseData::setAlphType( const InputLoc &loc, const HostLang *hostLang, const char *s1, const char *s2 )
{
	alphTypeLoc = loc;
	userAlphType = findAlphType( hostLang, s1, s2 );
	alphTypeSet = true;
	return userAlphType != 0;
}

/* Set the alphabet type. If the types are not valid returns false. */
bool ParseData::setAlphType( const InputLoc &loc, const HostLang *hostLang, const char *s1 )
{
	alphTypeLoc = loc;
	userAlphType = findAlphType( hostLang, s1 );
	alphTypeSet = true;
	return userAlphType != 0;
}

bool ParseData::setVariable( const char *var, InlineList *inlineList )
{
	bool set = true;

	if ( strcmp( var, "p" ) == 0 )
		fsmCtx->pExpr = inlineList;
	else if ( strcmp( var, "pe" ) == 0 )
		fsmCtx->peExpr = inlineList;
	else if ( strcmp( var, "eof" ) == 0 )
		fsmCtx->eofExpr = inlineList;
	else if ( strcmp( var, "cs" ) == 0 )
		fsmCtx->csExpr = inlineList;
	else if ( strcmp( var, "data" ) == 0 )
		fsmCtx->dataExpr = inlineList;
	else if ( strcmp( var, "top" ) == 0 )
		fsmCtx->topExpr = inlineList;
	else if ( strcmp( var, "stack" ) == 0 )
		fsmCtx->stackExpr = inlineList;
	else if ( strcmp( var, "act" ) == 0 )
		fsmCtx->actExpr = inlineList;
	else if ( strcmp( var, "ts" ) == 0 )
		fsmCtx->tokstartExpr = inlineList;
	else if ( strcmp( var, "te" ) == 0 )
		fsmCtx->tokendExpr = inlineList;
	else
		set = false;

	return set;
}

/* Initialize the key operators object that will be referenced by all fsms
 * created. */
void ParseData::initKeyOps( const HostLang *hostLang )
{
	/* Signedness and bounds. */
	alphType = alphTypeSet ? userAlphType : hostLang->defaultAlphType;
	fsmCtx->keyOps->setAlphType( hostLang, alphType );

	if ( lowerNum != 0 ) {
		/* If ranges are given then interpret the alphabet type. */
		fsmCtx->keyOps->minKey = makeFsmKeyNum( lowerNum, rangeLowLoc, this );
		fsmCtx->keyOps->maxKey = makeFsmKeyNum( upperNum, rangeHighLoc, this );
	}
}

void ParseData::printNameInst( std::ostream &out, NameInst *nameInst, int level )
{
	for ( int i = 0; i < level; i++ )
		out << "  ";
	out << (!nameInst->name.empty() ? nameInst->name : "<ANON>") << 
			"  id: " << nameInst->id << 
			"  refs: " << nameInst->numRefs <<
			"  uses: " << nameInst->numUses << endl;
	for ( NameVect::Iter name = nameInst->childVect; name.lte(); name++ )
		printNameInst( out, *name, level+1 );
}

Action *ParseData::newLmCommonAction( const char *name, InlineList *inlineList )
{
	InputLoc loc;
	loc.line = 1;
	loc.col = 1;
	loc.fileName = "NONE";

	Action *action = new Action( loc, name, inlineList, fsmCtx->nextCondId++ );
	action->embedRoots.append( rootName );
	fsmCtx->actionList.append( action );
	return action;
}

void ParseData::initLongestMatchData()
{
	if ( lmList.length() > 0 ) {
		/* The initTokStart action resets the token start. */
		InlineList *il1 = new InlineList;
		il1->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		il1->head->children = new InlineList;
		il1->head->children->append( new InlineItem( InputLoc(),
				InlineItem::LmInitTokStart ) );
		initTokStart = newLmCommonAction( "initts", il1 );
		initTokStart->isLmAction = true;

		/* The initActId action gives act a default value. */
		InlineList *il4 = new InlineList;
		il4->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		il4->head->children = new InlineList;
		il4->head->children->append( new InlineItem( InputLoc(), InlineItem::LmInitAct ) );
		initActId = newLmCommonAction( "initact", il4 );
		initActId->isLmAction = true;

		/* The setTokStart action sets tokstart. */
		InlineList *il5 = new InlineList;
		il5->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		il5->head->children = new InlineList;
		il5->head->children->append( new InlineItem( InputLoc(), InlineItem::LmSetTokStart ) );
		setTokStart = newLmCommonAction( "ts", il5 );
		setTokStart->isLmAction = true;

		/* The setTokEnd action sets tokend. */
		InlineList *il3 = new InlineList;
		il3->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		il3->head->children = new InlineList;
		il3->head->children->append( new InlineItem( InputLoc(), InlineItem::LmSetTokEnd ) );
		setTokEnd = newLmCommonAction( "te", il3 );
		setTokEnd->isLmAction = true;

		/* The action will also need an ordering: ahead of all user action
		 * embeddings. */
		initTokStartOrd = fsmCtx->curActionOrd++;
		initActIdOrd = fsmCtx->curActionOrd++;
		setTokStartOrd = fsmCtx->curActionOrd++;
		setTokEndOrd = fsmCtx->curActionOrd++;
	}
}

/* After building the graph, do some extra processing to ensure the runtime
 * data of the longest mactch operators is consistent. */
void ParseData::setLongestMatchData( FsmAp *graph )
{
	if ( lmList.length() > 0 ) {
		/* Make sure all entry points (targets of fgoto, fcall, fnext, fentry)
		 * init the tokstart. */
		for ( EntryMap::Iter en = graph->entryPoints; en.lte(); en++ ) {
			/* This is run after duplicates are removed, we must guard against
			 * inserting a duplicate. */
			ActionTable &actionTable = en->value->toStateActionTable;
			if ( ! actionTable.hasAction( initTokStart ) )
				actionTable.setAction( initTokStartOrd, initTokStart );
		}

		/* Find the set of states that are the target of transitions with
		 * actions that have calls. These states will be targeted by fret
		 * statements. */
		StateSet states;
		for ( StateList::Iter state = graph->stateList; state.lte(); state++ ) {
			for ( TransList::Iter trans = state->outList; trans.lte(); trans++ ) {
				if ( trans->plain() ) {
					for ( ActionTable::Iter ati = trans->tdap()->actionTable; ati.lte(); ati++ ) {
						if ( ati->value->anyCall && trans->tdap()->toState != 0 )
							states.insert( trans->tdap()->toState );
					}
				}
				else {
					for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) { 
						for ( ActionTable::Iter ati = cond->actionTable; ati.lte(); ati++ ) {
							if ( ati->value->anyCall && cond->toState != 0 )
								states.insert( cond->toState );
						}
					}
				}
			}
		}


		/* Init tokstart upon entering the above collected states. */
		for ( StateSet::Iter ps = states; ps.lte(); ps++ ) {
			/* This is run after duplicates are removed, we must guard against
			 * inserting a duplicate. */
			ActionTable &actionTable = (*ps)->toStateActionTable;
			if ( ! actionTable.hasAction( initTokStart ) )
				actionTable.setAction( initTokStartOrd, initTokStart );
		}
	}
}

/* Make the graph from a graph dict node. Does minimization and state sorting. */
FsmRes ParseData::makeInstance( GraphDictEl *gdNode )
{
	if ( id->printStatistics )
		id->stats() << "compiling\t" << sectionName << endl;

	/* Build the graph from a walk of the parse tree. */
	FsmRes graph = gdNode->value->walk( this );
	if ( !graph.success() )
		return graph;

	fsmCtx->finalizeInstance( graph.fsm );

	return graph;
}

void ParseData::printNameTree( ostream &out )
{
	/* Print the name instance map. */
	for ( NameVect::Iter name = rootName->childVect; name.lte(); name++ )
		printNameInst( out, *name, 0 );
	
	out << "name index:" << endl;
	/* Show that the name index is correct. */
	for ( int ni = 0; ni < nextNameId; ni++ ) {
		out << ni << ": ";
		std::string name = fsmCtx->nameIndex[ni]->name;
		out << ( !name.empty() ? name : "<ANON>" ) << endl;
	}
}

FsmRes ParseData::makeSpecific( GraphDictEl *gdNode )
{
	/* Build the name tree and supporting data structures. */
	makeNameTree( gdNode );

	/* Resove name references from gdNode. */
	initNameWalk();
	gdNode->value->resolveNameRefs( this );

	/* Do not resolve action references. Since we are not building the entire
	 * graph there's a good chance that many name references will fail. This
	 * is okay since generating part of the graph is usually only done when
	 * inspecting the compiled machine. */

	/* Same story for extern entry point references. */

	/* Flag this case so that the XML code generator is aware that we haven't
	 * looked up name references in actions. It can then avoid segfaulting. */
	fsmCtx->generatingSectionSubset = true;

	/* Just building the specified graph. */
	initNameWalk();
	FsmRes mainGraph = makeInstance( gdNode );

	return mainGraph;
}

FsmRes ParseData::makeAll()
{
	/* Build the name tree and supporting data structures. */
	makeNameTree( 0 );

	/* Resove name references in the tree. */
	initNameWalk();
	for ( GraphList::Iter glel = instanceList; glel.lte(); glel++ )
		glel->value->resolveNameRefs( this );

	/* Resolve action code name references. */
	resolveActionNameRefs();

	/* Force name references to the top level instantiations. */
	for ( NameVect::Iter inst = rootName->childVect; inst.lte(); inst++ )
		(*inst)->numRefs += 1;

	FsmAp *mainGraph = 0;
	FsmAp **graphs = new FsmAp*[instanceList.length()];
	int numOthers = 0;

	/* Make all the instantiations, we know that main exists in this list. */
	initNameWalk();
	for ( GraphList::Iter glel = instanceList; glel.lte();  glel++ ) {
		FsmRes res = makeInstance( glel );
		if ( !res.success() ) {
			for ( int i = 0; i < numOthers; i++ )
				delete graphs[i];
			delete[] graphs;
			return res;
		}

		/* Main graph is always instantiated. */
		if ( glel->key == MAIN_MACHINE )
			mainGraph = res.fsm;
		else
			graphs[numOthers++] = res.fsm;
	}

	if ( mainGraph == 0 )
		mainGraph = graphs[--numOthers];

	if ( numOthers > 0 ) {
		/* Add all the other graphs into main. */
		mainGraph->globOp( graphs, numOthers );
	}

	delete[] graphs;
	return FsmRes( FsmRes::Fsm(), mainGraph );
}


void ParseData::makeExportsNameTree()
{
	/* Make a name tree for the exports. */
	initExportsNameWalk();

	/* First make the name tree. */
	for ( GraphDict::Iter gdel = graphDict; gdel.lte(); gdel++ ) {
		if ( gdel->value->isExport ) {
			/* Recurse on the instance. */
			gdel->value->makeNameTree( gdel->loc, this );
		}
	}
}

void ParseData::makeExports()
{
	makeExportsNameTree();

	/* Resove name references in the tree. */
	initExportsNameWalk();
	for ( GraphDict::Iter gdel = graphDict; gdel.lte(); gdel++ ) {
		if ( gdel->value->isExport )
			gdel->value->resolveNameRefs( this );
	}

	/* Make all the instantiations, we know that main exists in this list. */
	initExportsNameWalk();
	for ( GraphDict::Iter gdel = graphDict; gdel.lte();  gdel++ ) {
		/* Check if this var def is an export. */
		if ( gdel->value->isExport ) {
			/* Build the graph from a walk of the parse tree. */
			FsmRes graph = gdel->value->walk( this );

			/* Build the graph from a walk of the parse tree. */
			if ( !graph.fsm->checkSingleCharMachine() ) {
				id->error(gdel->loc) << "bad export machine, must define "
						"a single character" << endl;
			}
			else {
				/* Safe to extract the key and declare the export. */
				Key exportKey = graph.fsm->startState->outList.head->lowKey;
				fsmCtx->exportList.append( new Export( gdel->value->name, exportKey ) );
			}
		}
	}
}

FsmRes ParseData::prepareMachineGen( GraphDictEl *graphDictEl, const HostLang *hostLang )
{
	initKeyOps( hostLang );
	makeRootNames();
	initLongestMatchData();

	/* Make the graph, do minimization. */
	if ( graphDictEl == 0 ) {
		FsmRes res = makeAll();
		if ( !res.success() )
			return res;
		sectionGraph = res.fsm;
	}
	else {
		FsmRes res = makeSpecific( graphDictEl );
		if ( !res.success() )
			return res;
		sectionGraph = res.fsm;
	}
	
	/* If any errors have occured in the input file then don't write anything. */
	if ( id->errorCount > 0 )
		return FsmRes( FsmRes::Aborted() );

	fsmCtx->analyzeGraph( sectionGraph );

	/* Depends on the graph analysis. */
	setLongestMatchData( sectionGraph );

	fsmCtx->prepareReduction( sectionGraph );

	return FsmRes( FsmRes::Fsm(), sectionGraph );
}

void ParseData::generateReduced( const char *inputFileName, CodeStyle codeStyle,
		std::ostream &out, const HostLang *hostLang )
{
	Reducer *red = new Reducer( this->id, fsmCtx, sectionGraph, sectionName, machineId );
	red->make( hostLang, alphType );

	CodeGenArgs args( this->id, red, alphType, machineId, inputFileName, sectionName, out, codeStyle );

	/* Write out with it. */
	cgd = makeCodeGen( hostLang, args );

	/* Code generation anlysis step. */
	cgd->genAnalysis();
}

#if 0
void ParseData::generateXML( ostream &out )
{
	/* Make the generator. */
	XMLCodeGen codeGen( sectionName, machineId, id, this, sectionGraph, out );

	/* Write out with it. */
	codeGen.writeXML();
}
#endif

void ParseData::clear()
{
	cgd->clear();

	delete sectionGraph;
	sectionGraph = 0;

	graphDict.empty();

	/* Delete all the nodes in the action list. Will cause all the
	 * string data that represents the actions to be deallocated. */
	fsmCtx->actionList.empty();
}
