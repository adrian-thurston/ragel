/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
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

	virtual void finishRagelDef();
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
		GenBase( args.fsmName, args.pd, args.fsm ),
		out(args.out)
	{ }

	bool makeNameInst( std::string &res, NameInst *nameInst );
	void action( ActionTable *actionTable );
	void transAction( StateAp *fromState, CondAp *trans );
	void key( Key key );
	void onChar( Key lowKey, Key highKey, CondSpace *condSpace, long condVals );
	void transList( StateAp *state );
	void write();
	
	ostream &out;
};

#endif
