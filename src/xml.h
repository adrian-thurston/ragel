/*
 *  Copyright 2005-2007 Adrian Thurston <thurston@complang.org>
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

#ifndef _XMLCODEGEN_H
#define _XMLCODEGEN_H

#include <iostream>
#include "avltree.h"
#include "fsmgraph.h"
#include "parsedata.h"
#include "redfsm.h"
#include "gendata.h"

class XMLCodeGen : protected GenBase
{
public:
	XMLCodeGen( std::string fsmName, int machineId, ParseData *pd, FsmAp *fsm, std::ostream &out );

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

