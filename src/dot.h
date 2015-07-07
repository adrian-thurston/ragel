/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
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

#ifndef _GVDOTGEN_H
#define _GVDOTGEN_H

#include <iostream>
#include "gendata.h"

class GraphvizDotGenOrig : public CodeGenData
{
public:
	GraphvizDotGenOrig( const CodeGenArgs &args ) 
			: CodeGenData(args) { }

	/* Print an fsm to out stream. */
	void writeTransList( RedStateAp *state );
	void writeDotFile( );

	virtual void writeStatement( InputLoc &, int, std::string * );

private:
	/* Writing labels and actions. */
	std::ostream &ONCHAR( Key lowKey, Key highKey );
	std::ostream &TRANS_ACTION( RedStateAp *fromState, RedTransAp *trans );
	std::ostream &ACTION( RedAction *action );
	std::ostream &KEY( Key key );
};

class GraphvizDotGen : public GenBase
{
public:
	GraphvizDotGen( const CodeGenArgs &args ) 
	:
		GenBase( args.fsmName, args.machineId, args.pd, args.fsm ),
		out(args.out)
	{ }

	bool makeNameInst( std::string &res, NameInst *nameInst );
	void action( ActionTable *actionTable );
	void transAction( StateAp *fromState, TransData *trans );
	void key( Key key );
	void onChar( Key lowKey, Key highKey, CondSpace *condSpace, long condVals );
	void transList( StateAp *state );
	void write();
	void fromStateAction( StateAp *fromState );
	
	ostream &out;
};

#endif
