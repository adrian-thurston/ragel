/*
 *  Copyright 2001-2016 Adrian Thurston <thurston@complang.org>
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

#ifndef _ACTION_H
#define _ACTION_H

#include "fsmgraph.h"

struct NameInst;
struct NameRef;
struct LongestMatch;
struct InlineList;

/*
 * Inline code tree
 */
struct InlineItem
{
	enum Type 
	{
		Text, Goto, Call, Ncall, Next, GotoExpr, CallExpr, NcallExpr, NextExpr, Ret, Nret,
		PChar, Char, Hold, Curs, Targs, Entry, Exec, Break, Nbreak,
		LmSwitch, LmSetActId, LmSetTokEnd, LmOnLast, LmOnNext, LmOnLagBehind,
		LmInitAct, LmInitTokStart, LmSetTokStart, Stmt, Subst,
		NfaWrapAction, NfaWrapConds
	};

	InlineItem( const InputLoc &loc, std::string data, Type type ) : 
		loc(loc), data(data), nameRef(0), children(0), type(type) { }

	InlineItem( const InputLoc &loc, NameRef *nameRef, Type type ) : 
		loc(loc), nameRef(nameRef), children(0), type(type) { }

	InlineItem( const InputLoc &loc, LongestMatch *longestMatch, 
		LongestMatchPart *longestMatchPart, Type type ) : loc(loc),
		nameRef(0), children(0), longestMatch(longestMatch),
		longestMatchPart(longestMatchPart), type(type) { } 

	InlineItem( const InputLoc &loc, NameInst *nameTarg, Type type ) : 
		loc(loc), nameRef(0), nameTarg(nameTarg), children(0),
		type(type) { }

	InlineItem( const InputLoc &loc, Type type ) : 
		loc(loc), nameRef(0), children(0), type(type) { }

	InlineItem( const InputLoc &loc, Action *wrappedAction, Type type )
	:
		loc(loc), nameRef(0), children(0), longestMatch(0),
		longestMatchPart(0), wrappedAction(wrappedAction), type(type)
	{} 

	InlineItem( const InputLoc &loc, CondSpace *condSpace,
			const CondKeySet &condKeySet, Type type )
	:
		loc(loc), nameRef(0), children(0), longestMatch(0),
		longestMatchPart(0), wrappedAction(0), condSpace(condSpace),
		condKeySet(condKeySet), type(type)
	{} 

	~InlineItem();
	
	InputLoc loc;
	std::string data;
	NameRef *nameRef;
	NameInst *nameTarg;
	InlineList *children;
	LongestMatch *longestMatch;
	LongestMatchPart *longestMatchPart;
	long substPos;
	Action *wrappedAction;
	CondSpace *condSpace;
	CondKeySet condKeySet;
	Type type;

	InlineItem *prev, *next;
};

/* Normally this would be atypedef, but that would entail including DList from
 * ptreetypes, which should be just typedef forwards. */
struct InlineList : public DList<InlineItem> { };

struct InlineBlock
{
	InlineBlock( const InputLoc &loc, InlineList *inlineList )
		: loc(loc), inlineList(inlineList) {}

	~InlineBlock()
	{
		inlineList->empty();
		delete inlineList;
	}

	InputLoc loc;
	InlineList *inlineList;
};

#endif
