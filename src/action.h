/*
 * Copyright 2001-2016 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
