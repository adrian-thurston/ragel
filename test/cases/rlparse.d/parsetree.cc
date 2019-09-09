/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@complang.org>
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
#include <sstream>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "inputdata.h"
#include "ragel.h"
#include "parsetree.h"
#include "parsedata.h"

using namespace std;
ostream &operator<<( ostream &out, const NameRef &nameRef );
ostream &operator<<( ostream &out, const NameInst &nameInst );

/* Read string literal (and regex) options and return the true end. */
const char *checkLitOptions( InputData *id, const InputLoc &loc, const char *data, int length, bool &caseInsensitive )
{
	const char *end = data + length - 1;
	while ( *end != '\'' && *end != '\"' && *end != '/' ) {
		if ( *end == 'i' )
			caseInsensitive = true;
		else {
			id->error( loc ) << "literal string '" << *end << 
					"' option not supported" << endl;
		}
		end -= 1;
	}
	return end;
}

/* Convert the literal string which comes in from the scanner into an array of
 * characters with escapes and options interpreted. Also null terminates the
 * string. Though this null termination should not be relied on for
 * interpreting literals in the parser because the string may contain \0 */
char *prepareLitString( InputData *id, const InputLoc &loc, const char *data, long length, 
		long &resLen, bool &caseInsensitive )
{
	char *resData = new char[length+1];
	caseInsensitive = false;

	const char *src = data + 1;
	const char *end = checkLitOptions( id, loc, data, length, caseInsensitive );

	char *dest = resData;
	long dlen = 0;
	while ( src != end ) {
		if ( *src == '\\' ) {
			switch ( src[1] ) {
			case '0': dest[dlen++] = '\0'; break;
			case 'a': dest[dlen++] = '\a'; break;
			case 'b': dest[dlen++] = '\b'; break;
			case 't': dest[dlen++] = '\t'; break;
			case 'n': dest[dlen++] = '\n'; break;
			case 'v': dest[dlen++] = '\v'; break;
			case 'f': dest[dlen++] = '\f'; break;
			case 'r': dest[dlen++] = '\r'; break;
			case '\n':  break;
			default: dest[dlen++] = src[1]; break;
			}
			src += 2;
		}
		else {
			dest[dlen++] = *src++;
		}
	}

	resLen = dlen;
	resData[resLen] = 0;
	return resData;
}

Key *prepareHexString( ParseData *pd, const InputLoc &loc, const char *data, long length, long &resLen )
{
	Key *dest = new Key[( length - 2 ) >> 1];
	const char *src = data;
	const char *end = data + length;
	long dlen = 0;
	char s[3];

	/* Scan forward over 0x. */
	src += 2;

	s[2] = 0;
	while ( src < end ) {
		s[0] = src[0];
		s[1] = src[1];
	
		dest[dlen++] = makeFsmKeyHex( s, loc, pd );

		/* Scan forward over the hex chars, then any whitespace or . characters. */
		src += 2;
		while ( *src == ' ' || *src == '\t' || *src == '\n' || *src == '.' )
			src += 1;

		/* Scan forward over 0x. */
		src += 2;
	}

	resLen = dlen;
	return dest;
}

void VarDef::makeNameTree( const InputLoc &loc, ParseData *pd )
{
	/* The variable definition enters a new scope. */
	NameInst *prevNameInst = pd->curNameInst;
	pd->curNameInst = pd->addNameInst( loc, name, false );

	if ( machineDef->type == MachineDef::LongestMatchType )
		pd->curNameInst->isLongestMatch = true;

	/* Recurse. */
	machineDef->makeNameTree( pd );

	/* The name scope ends, pop the name instantiation. */
	pd->curNameInst = prevNameInst;
}

void VarDef::resolveNameRefs( ParseData *pd )
{
	/* Entering into a new scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Recurse. */
	machineDef->resolveNameRefs( pd );
	
	/* The name scope ends, pop the name instantiation. */
	pd->popNameScope( nameFrame );
}

VarDef::~VarDef()
{
	delete machineDef;
}

InputLoc LongestMatchPart::getLoc()
{ 
	return action != 0 ? action->loc : semiLoc;
}

/*
 * If there are any LMs then all of the following entry points must reset
 * tokstart:
 *
 *  1. fentry(StateRef)
 *  2. ftoto(StateRef), fcall(StateRef), fnext(StateRef)
 *  3. targt of any transition that has an fcall (the return loc).
 *  4. start state of all longest match routines.
 */

Action *LongestMatch::newLmAction( ParseData *pd, const InputLoc &loc, 
		const char *name, InlineList *inlineList )
{
	Action *action = new Action( loc, name, inlineList, pd->fsmCtx->nextCondId++ );
	action->embedRoots.append( pd->curNameInst );
	pd->fsmCtx->actionList.append( action );
	action->isLmAction = true;
	return action;
}

void LongestMatch::makeActions( ParseData *pd )
{
	/* Make actions that set the action id. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		inlineList->head->children = new InlineList;
		inlineList->head->children->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmSetActId ) );
		char *actName = new char[50];
		sprintf( actName, "store%i", lmi->longestMatchId );
		lmi->setActId = newLmAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart on the last
	 * character. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		inlineList->head->children = new InlineList;
		inlineList->head->children->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnLast ) );
		char *actName = new char[50];
		sprintf( actName, "last%i", lmi->longestMatchId );
		lmi->actOnLast = newLmAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart on the next
	 * character.  These actions will set tokend themselves (it is the current
	 * char). */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		inlineList->head->children = new InlineList;
		inlineList->head->children->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnNext ) );
		char *actName = new char[50];
		sprintf( actName, "next%i", lmi->longestMatchId );
		lmi->actOnNext = newLmAction( pd, lmi->getLoc(), actName, inlineList );
	}

	/* Make actions that execute the user action and restart at tokend. These
	 * actions execute some time after matching the last char. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* For each part create actions for setting the match type.  We need
		 * to do this so that the actions will go into the actionIndex. */
		InlineList *inlineList = new InlineList;
		inlineList->append( new InlineItem( InputLoc(), InlineItem::Stmt ) );
		inlineList->head->children = new InlineList;
		inlineList->head->children->append( new InlineItem( lmi->getLoc(), this, lmi, 
				InlineItem::LmOnLagBehind ) );
		char *actName = new char[50];
		sprintf( actName, "lag%i", lmi->longestMatchId );
		lmi->actLagBehind = newLmAction( pd, lmi->getLoc(), actName, inlineList );
	}

	InputLoc loc;
	loc.line = 1;
	loc.col = 1;
	loc.fileName = "NONE";

	/* Create the error action. */
	InlineList *il6 = new InlineList;
	il6->append( new InlineItem( loc, this, 0, InlineItem::LmSwitch ) );
	lmActSelect = newLmAction( pd, loc, "switch", il6 );
}

void LongestMatch::findName( ParseData *pd )
{
	NameInst *nameInst = pd->curNameInst;
	while ( nameInst->name.empty() ) {
		nameInst = nameInst->parent;
		/* Since every machine must must have a name, we should always find a
		 * name for the longest match. */
		assert( nameInst != 0 );
	}
	name = nameInst->name;
}

void LongestMatch::makeNameTree( ParseData *pd )
{
	/* Create an anonymous scope for the longest match. Will be used for
	 * restarting machine after matching a token. */
	NameInst *prevNameInst = pd->curNameInst;
	pd->curNameInst = pd->addNameInst( loc, std::string(), false );

	/* Recurse into all parts of the longest match operator. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ )
		lmi->join->makeNameTree( pd );

	/* Traverse the name tree upwards to find a name for this lm. */
	findName( pd );

	/* Also make the longest match's actions at this point. */
	makeActions( pd );

	/* The name scope ends, pop the name instantiation. */
	pd->curNameInst = prevNameInst;
}

void LongestMatch::resolveNameRefs( ParseData *pd )
{
	/* The longest match gets its own name scope. */
	NameFrame nameFrame = pd->enterNameScope( true, 1 );

	/* Take an action reference for each longest match item and recurse. */
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		/* Record the reference if the item has an action. */
		if ( lmi->action != 0 )
			lmi->action->embedRoots.append( pd->localNameScope );

		/* Recurse down the join. */
		lmi->join->resolveNameRefs( pd );
	}

	/* The name scope ends, pop the name instantiation. */
	pd->popNameScope( nameFrame );
}


NfaUnion::~NfaUnion()
{
	for ( TermVect::Iter term = terms; term.lte(); term++ )
		delete *term;
	if ( roundsList != 0 )
		delete roundsList;
}

void NfaUnion::makeNameTree( ParseData *pd )
{
	for ( TermVect::Iter term = terms; term.lte(); term++ )
		(*term)->makeNameTree( pd );
}

void NfaUnion::resolveNameRefs( ParseData *pd )
{
	for ( TermVect::Iter term = terms; term.lte(); term++ )
		(*term)->resolveNameRefs( pd );
}

void MachineDef::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case JoinType:
		join->makeNameTree( pd );
		break;
	case LongestMatchType:
		longestMatch->makeNameTree( pd );
		break;
	case LengthDefType:
		break;
	case NfaUnionType:
		nfaUnion->makeNameTree( pd );
		break;
	}
}

void MachineDef::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case JoinType:
		join->resolveNameRefs( pd );
		break;
	case LongestMatchType:
		longestMatch->resolveNameRefs( pd );
		break;
	case LengthDefType:
		break;
	case NfaUnionType:
		nfaUnion->resolveNameRefs( pd );
		break;
	}
}

MachineDef::~MachineDef()
{
	if ( join != 0 )
		delete join;
	if ( longestMatch != 0 )
		delete longestMatch;
	if ( lengthDef != 0 )
		delete lengthDef;
	if ( nfaUnion != 0 )
		delete nfaUnion;
}

/* Construct with a location and the first expression. */
Join::Join( const InputLoc &loc, Expression *expr )
:
	loc(loc)
{
	exprList.append( expr );
}

/* Construct with a location and the first expression. */
Join::Join( Expression *expr )
{
	exprList.append( expr );
}

void Join::makeNameTree( ParseData *pd )
{
	if ( exprList.length() > 1 ) {
		/* Create the new anonymous scope. */
		NameInst *prevNameInst = pd->curNameInst;
		pd->curNameInst = pd->addNameInst( loc, std::string(), false );

		/* Join scopes need an implicit "final" target. */
		pd->curNameInst->final = new NameInst( InputLoc(), pd->curNameInst, "final", 
				pd->nextNameId++, false );

		/* Recurse into all expressions in the list. */
		for ( ExprList::Iter expr = exprList; expr.lte(); expr++ )
			expr->makeNameTree( pd );

		/* The name scope ends, pop the name instantiation. */
		pd->curNameInst = prevNameInst;
	}
	else {
		/* Recurse into the single expression. */
		exprList.head->makeNameTree( pd );
	}
}


void Join::resolveNameRefs( ParseData *pd )
{
	/* Branch on whether or not there is to be a join. */
	if ( exprList.length() > 1 ) {
		/* The variable definition enters a new scope. */
		NameFrame nameFrame = pd->enterNameScope( true, 1 );

		/* The join scope must contain a start label. */
		NameSet resolved = pd->resolvePart( pd->localNameScope, "start", true );
		if ( resolved.length() > 0 ) {
			/* Take the first. */
			pd->curNameInst->start = resolved[0];
			if ( resolved.length() > 1 ) {
				/* Complain about the multiple references. */
				pd->id->error(loc) << "join operation has multiple start labels" << endl;
				pd->errorStateLabels( resolved );
			}
		}

		/* Make sure there is a start label. */
		if ( pd->curNameInst->start != 0 ) {
			/* There is an implicit reference to start name. */
			pd->curNameInst->start->numRefs += 1;
		}
		else {
			/* No start label. */
			pd->id->error(loc) << "join operation has no start label" << endl;
		}

		/* Recurse into all expressions in the list. */
		for ( ExprList::Iter expr = exprList; expr.lte(); expr++ )
			expr->resolveNameRefs( pd );

		/* The name scope ends, pop the name instantiation. */
		pd->popNameScope( nameFrame );
	}
	else {
		/* Recurse into the single expression. */
		exprList.head->resolveNameRefs( pd );
	}
}

/* Clean up after an expression node. */
Expression::~Expression()
{
	if ( expression )
		delete expression;
	if ( term )
		delete term;
}


void Expression::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case OrType:
	case IntersectType:
	case SubtractType:
	case StrongSubtractType:
		expression->makeNameTree( pd );
		term->makeNameTree( pd );
		break;
	case TermType:
		term->makeNameTree( pd );
		break;
	case BuiltinType:
		break;
	}
}

void Expression::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case OrType:
	case IntersectType:
	case SubtractType:
	case StrongSubtractType:
		expression->resolveNameRefs( pd );
		term->resolveNameRefs( pd );
		break;
	case TermType:
		term->resolveNameRefs( pd );
		break;
	case BuiltinType:
		break;
	}
}

/* Clean up after a term node. */
Term::~Term()
{
	if ( term )
		delete term;
	if ( factorWithAug )
		delete factorWithAug;
}


void Term::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case ConcatType:
	case RightStartType:
	case RightFinishType:
	case LeftType:
		term->makeNameTree( pd );
		factorWithAug->makeNameTree( pd );
		break;
	case FactorWithAugType:
		factorWithAug->makeNameTree( pd );
		break;
	}
}

void Term::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case ConcatType:
	case RightStartType:
	case RightFinishType:
	case LeftType:
		term->resolveNameRefs( pd );
		factorWithAug->resolveNameRefs( pd );
		break;
	case FactorWithAugType:
		factorWithAug->resolveNameRefs( pd );
		break;
	}
}

/* Clean up after a factor with augmentation node. */
FactorWithAug::~FactorWithAug()
{
	delete factorWithRep;

	/* Walk the vector of parser actions, deleting function names. */

	/* Clean up priority descriptors. */
	if ( priorDescs != 0 )
		delete[] priorDescs;
}

void FactorWithAug::makeNameTree( ParseData *pd )
{
	/* Add the labels to the tree of instantiated names. Each label
	 * makes a new scope. */
	NameInst *prevNameInst = pd->curNameInst;
	for ( size_t i = 0; i < labels.size(); i++ ) {
		pd->curNameInst = pd->addNameInst( labels[i].loc, labels[i].data, true );

		if ( labels[i].cut )
			pd->curNameInst->numRefs += 1;
	}

	/* Recurse, then pop the names. */
	factorWithRep->makeNameTree( pd );
	pd->curNameInst = prevNameInst;
}


void FactorWithAug::resolveNameRefs( ParseData *pd )
{
	/* Enter into the name scope created by any labels. */
	NameFrame nameFrame = pd->enterNameScope( false, labels.size() );

	/* Note action references. */
	for ( int i = 0; i < actions.length(); i++ ) 
		actions[i].action->embedRoots.append( pd->localNameScope );

	/* Recurse first. IMPORTANT: we must do the exact same traversal as when
	 * the tree is constructed. */
	factorWithRep->resolveNameRefs( pd );

	/* Resolve epsilon transitions. */
	for ( int ep = 0; ep < epsilonLinks.length(); ep++ ) {
		/* Get the link. */
		EpsilonLink &link = epsilonLinks[ep];
		NameInst *resolvedName = 0;

		if ( link.target->length() == 1 && link.target->data[0] == "final" ) {
			/* Epsilon drawn to an implicit final state. An implicit final is
			 * only available in join operations. */
			resolvedName = pd->localNameScope->final;
		}
		else {
			/* Do an search for the name. */
			NameSet resolved;
			pd->resolveFrom( resolved, pd->localNameScope, link.target, 0 );
			if ( resolved.length() > 0 ) {
				/* Take the first one. */
				resolvedName = resolved[0];
				if ( resolved.length() > 1 ) {
					/* Complain about the multiple references. */
					pd->id->error(link.loc) << "state reference " << link.target << 
							" resolves to multiple entry points" << endl;
					pd->errorStateLabels( resolved );
				}
			}
		}

		/* This is tricky, we stuff resolved epsilon transitions into one long
		 * vector in the parse data structure. Since the name resolution and
		 * graph generation both do identical walks of the parse tree we
		 * should always find the link resolutions in the right place.  */
		pd->epsilonResolvedLinks.append( resolvedName );

		if ( resolvedName != 0 ) {
			/* Found the name, bump of the reference count on it. */
			resolvedName->numRefs += 1;
		}
		else {
			/* Complain, no recovery action, the epsilon op will ignore any
			 * epsilon transitions whose names did not resolve. */
			pd->id->error(link.loc) << "could not resolve label " << link.target << endl;
		}
	}

	if ( labels.size() > 0 )
		pd->popNameScope( nameFrame );
}


/* Clean up after a factor with repetition node. */
FactorWithRep::~FactorWithRep()
{
	switch ( type ) {
		case StarType: case StarStarType: case OptionalType: case PlusType:
		case ExactType: case MaxType: case MinType: case RangeType:
			delete factorWithRep;
		case FactorWithNegType:
			delete factorWithNeg;
			break;
	}
}


void FactorWithRep::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case StarType:
	case StarStarType:
	case OptionalType:
	case PlusType:
	case ExactType:
	case MaxType:
	case MinType:
	case RangeType:
		factorWithRep->makeNameTree( pd );
		break;
	case FactorWithNegType:
		factorWithNeg->makeNameTree( pd );
		break;
	}
}

void FactorWithRep::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case StarType:
	case StarStarType:
	case OptionalType:
	case PlusType:
	case ExactType:
	case MaxType:
	case MinType:
	case RangeType:
		factorWithRep->resolveNameRefs( pd );
		break;
	case FactorWithNegType:
		factorWithNeg->resolveNameRefs( pd );
		break;
	}
}

/* Clean up after a factor with negation node. */
FactorWithNeg::~FactorWithNeg()
{
	switch ( type ) {
		case NegateType:
		case CharNegateType:
			delete factorWithNeg;
			break;
		case FactorType:
			delete factor;
			break;
	}
}

void FactorWithNeg::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case NegateType:
	case CharNegateType:
		factorWithNeg->makeNameTree( pd );
		break;
	case FactorType:
		factor->makeNameTree( pd );
		break;
	}
}

void FactorWithNeg::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case NegateType:
	case CharNegateType:
		factorWithNeg->resolveNameRefs( pd );
		break;
	case FactorType:
		factor->resolveNameRefs( pd );
		break;
	}
}

/* Clean up after a factor node. */
Factor::~Factor()
{
	switch ( type ) {
		case LiteralType:
			delete literal;
			break;
		case RangeType:
			delete range;
			break;
		case OrExprType:
			delete reItem;
			break;
		case RegExprType:
			delete regExpr;
			break;
		case ReferenceType:
			break;
		case ParenType:
			delete join;
			break;
		case LongestMatchType:
			delete longestMatch;
			break;
		case NfaRep: case CondStar: case CondPlus:
			delete expression;
			break;
	}
}


void Factor::makeNameTree( ParseData *pd )
{
	switch ( type ) {
	case LiteralType:
	case RangeType:
	case OrExprType:
	case RegExprType:
		break;
	case ReferenceType:
		varDef->makeNameTree( loc, pd );
		break;
	case ParenType:
		join->makeNameTree( pd );
		break;
	case LongestMatchType:
		longestMatch->makeNameTree( pd );
		break;
	case NfaRep:
	case CondStar:
	case CondPlus:
		expression->makeNameTree( pd );
		break;
	}
}

void Factor::resolveNameRefs( ParseData *pd )
{
	switch ( type ) {
	case LiteralType:
	case RangeType:
	case OrExprType:
	case RegExprType:
		break;
	case ReferenceType:
		varDef->resolveNameRefs( pd );
		break;
	case ParenType:
		join->resolveNameRefs( pd );
		break;
	case LongestMatchType:
		longestMatch->resolveNameRefs( pd );
		break;
	case NfaRep:
	case CondStar:
	case CondPlus:
		expression->resolveNameRefs( pd );
		break;
	}
}

/* Clean up a range object. Must delete the two literals. */
Range::~Range()
{
	delete lowerLit;
	delete upperLit;
}

/* Clean up after a regular expression object. */
RegExpr::~RegExpr()
{
	switch ( type ) {
		case RecurseItem:
			delete regExpr;
			delete item;
			break;
		case Empty:
			break;
	}
}

/* Clean up after an item in a regular expression. */
ReItem::~ReItem()
{
	switch ( type ) {
		case Data:
		case Dot:
			break;
		case OrBlock:
		case NegOrBlock:
			delete orBlock;
			break;
	}
}

/* Clean up after an or block of a regular expression. */
ReOrBlock::~ReOrBlock()
{
	switch ( type ) {
		case RecurseItem:
			delete orBlock;
			delete item;
			break;
		case Empty:
			break;
	}
}


