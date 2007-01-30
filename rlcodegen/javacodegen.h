/*
 *  Copyright 2006 Adrian Thurston <thurston@cs.queensu.ca>
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

#ifndef _JAVACODEGEN_H
#define _JAVACODEGEN_H

#include "tabcodegen.h"

/*
 * JavaTabCodeGen
 */
struct JavaTabCodeGen
	: public TabCodeGen, public JavaCodeGen
{
	JavaTabCodeGen( ostream &out ) : 
		FsmCodeGen(out), TabCodeGen(out), JavaCodeGen(out) {}

	void BREAK( ostream &ret, int targState );
	void GOTO( ostream &ret, int gotoDest, bool inFinish );
	void GOTO_EXPR( ostream &ret, InlineItem *ilItem, bool inFinish );
	void CALL( ostream &ret, int callDest, int targState, bool inFinish );
	void CALL_EXPR( ostream &ret, InlineItem *ilItem, int targState, bool inFinish );
	void RET( ostream &ret, bool inFinish );

	void COND_TRANSLATE();
	void LOCATE_TRANS();
	virtual void writeOutExec();
	virtual void writeOutEOF();
};


#endif
