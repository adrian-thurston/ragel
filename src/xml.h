/*
 * Copyright 2005-2007 Adrian Thurston <thurston@colm.net>
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

#ifndef _XMLCODEGEN_H
#define _XMLCODEGEN_H

#if 0

#include <iostream>
#include "avltree.h"
#include "fsmgraph.h"
#include "parsedata.h"
#include "redfsm.h"
#include "gendata.h"

class XMLCodeGen : protected RedBase
{
public:
	XMLCodeGen( std::string fsmName, int machineId, FsmGbl *id, PdBase *pd, FsmAp *fsm, std::ostream &out );

	void writeXML( );

private:
	void writeStateActions( StateAp *state );
	void writeStateList();

	void writeKey( Key key );
	void writeText( InlineItem *item );
	void writeGoto( InlineItem *item );
	void writeGotoExpr( InlineItem *item );
	void writeCall( InlineItem *item );
	void writeCallExpr( InlineItem *item );
	void writeNext( InlineItem *item );
	void writeNextExpr( InlineItem *item );
	void writeEntry( InlineItem *item );
	void writeLmOnLast( InlineItem *item );
	void writeLmOnNext( InlineItem *item );
	void writeLmOnLagBehind( InlineItem *item );

	void writeExports();
	bool writeNameInst( NameInst *nameInst );
	void writeEntryPoints();
	void writeConditions();
	void writeInlineList( InlineList *inlineList );
	void writeActionList();
	void writeActionTableList();
	void reduceTrans( TransAp *trans );
	void writeTransList( StateAp *state );
	void writeEofTrans( StateAp *state );
	void writeTrans( Key lowKey, Key highKey, TransAp *defTrans );
	void writeAction( Action *action );
	void writeLmSwitch( InlineItem *item );
	void writeMachine();
	void writeActionExec( InlineItem *item );

	std::ostream &out;
};

#endif

#endif

