/*
 *  Copyright 2008-2016 Adrian Thurston <thurston@complang.org>
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
#include "fsmgraph.h"
#include "parsedata.h"

/* Error reporting format. */
ErrorFormat errorFormat = ErrorFormatGNU;

void FsmCtx::analyzeAction( Action *action, InlineList *inlineList )
{
	/* FIXME: Actions used as conditions should be very constrained. */
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		if ( item->type == InlineItem::Call || item->type == InlineItem::CallExpr ||
				item->type == InlineItem::Ncall || item->type == InlineItem::NcallExpr )
		{
			action->anyCall = true;
		}

		/* Need to recurse into longest match items. */
		if ( item->type == InlineItem::LmSwitch ) {
			LongestMatch *lm = item->longestMatch;
			for ( LmPartList::Iter lmi = *lm->longestMatchList; lmi.lte(); lmi++ ) {
				if ( lmi->action != 0 )
					analyzeAction( action, lmi->action->inlineList );
			}
		}

		if ( item->type == InlineItem::LmOnLast || 
				item->type == InlineItem::LmOnNext ||
				item->type == InlineItem::LmOnLagBehind )
		{
			LongestMatchPart *lmi = item->longestMatchPart;
			if ( lmi->action != 0 )
				analyzeAction( action, lmi->action->inlineList );
		}

		if ( item->children != 0 )
			analyzeAction( action, item->children );
	}
}


/* Check actions for bad uses of fsm directives. We don't go inside longest
 * match items in actions created by ragel, since we just want the user
 * actions. */
void FsmCtx::checkInlineList( Action *act, InlineList *inlineList )
{
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		/* EOF checks. */
		if ( act->numEofRefs > 0 ) {
			switch ( item->type ) {
				/* Currently no checks. */
				default:
					break;
			}
		}

		/* Recurse. */
		if ( item->children != 0 )
			checkInlineList( act, item->children );
	}
}

void FsmCtx::checkAction( Action *action )
{
	/* Check for actions with calls that are embedded within a longest match
	 * machine. */
	if ( !action->isLmAction && action->numRefs() > 0 && action->anyCall ) {
		for ( NameInstVect::Iter ar = action->embedRoots; ar.lte(); ar++ ) {
			NameInst *check = *ar;
			while ( check != 0 ) {
				if ( check->isLongestMatch ) {
					fsmGbl->error(action->loc) << "within a scanner, fcall and fncall are permitted"
						" only in pattern actions" << endl;
					break;
				}
				check = check->parent;
			}
		}
	}

	checkInlineList( action, action->inlineList );
}


/* This create an action that refs the original embed roots, if the optWrap arg
 * is supplied. */
Action *FsmCtx::newNfaWrapAction( const char *name, InlineList *inlineList, Action *optWrap )
{
	InputLoc loc;
	loc.line = 1;
	loc.col = 1;
	loc.fileName = "NONE";

	Action *action = new Action( loc, name, inlineList, nextCondId++ );

	if ( optWrap != 0 )
		action->embedRoots.append( optWrap->embedRoots );

	actionList.append( action );
	return action;
}


void translatedHostData( ostream &out, const std::string &data )
{
	const char *p = data.c_str();
	for ( const char *c = p; *c != 0; ) {
		if ( c[0] == '}' && ( c[1] == '@' || c[1] == '$' || c[1] == '=' ) ) {
			out << "@}@" << c[1];
			c += 2;
		}
		else if ( c[0] == '@' ) {
			out << "@@";
			c += 1;
		}
		// Have some escaping issues that these fix, but they lead to other problems.
		// Can be reproduced by passing "={}" through ragel and adding --colm-backend
		// else if ( c[0] == '=' ) {
		//	out << "@=";
		//	c += 1;
		//}
		// else if ( c[0] == '$' ) {
		//	out << "@$";
		//	c += 1;
		//}
		else {
			out << c[0];
			c += 1;
		}
	}
}


void FsmGbl::abortCompile( int code )
{
	throw AbortCompile( code );
}

/* Print the opening to a warning in the input, then return the error ostream. */
ostream &FsmGbl::warning( const InputLoc &loc )
{
	ostream &err = inLibRagel ? libcerr : std::cerr;
	err << loc << ": warning: ";
	return err;
}

/* Print the opening to a program error, then return the error stream. */
ostream &FsmGbl::error()
{
	errorCount += 1;
	ostream &err = inLibRagel ? libcerr : std::cerr;
	err << PROGNAME ": ";
	return err;
}

ostream &FsmGbl::error( const InputLoc &loc )
{
	errorCount += 1;
	ostream &err = inLibRagel ? libcerr : std::cerr;
	err << loc << ": ";
	return err;
}

std::ostream &FsmGbl::stats()
{
	return inLibRagel ? libcout : std::cout;
}

/* Requested info. */
std::ostream &FsmGbl::info()
{
	return inLibRagel ? libcout : std::cout;
}

ostream &operator<<( ostream &out, const InputLoc &loc )
{
	assert( loc.fileName != 0 );
	switch ( errorFormat ) {
	case ErrorFormatMSVC:
		out << loc.fileName << "(" << loc.line;
		if ( loc.col )
			out << "," << loc.col;
		out << ")";
		break;

	default:
		out << loc.fileName << ":" << loc.line;
		if ( loc.col )
			out << ":" << loc.col;
		break;
	}
	return out;
}

