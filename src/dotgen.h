/*
 *  Copyright 2001-2012 Adrian Thurston <thurston@complang.org>
 */

#ifndef _GVDOTGEN_H
#define _GVDOTGEN_H

#include <iostream>

#if 0

class GraphvizDotGen : public CodeGenData
{
public:
	GraphvizDotGen( ostream &out ) : CodeGenData(out) { }

	/* Print an fsm to out stream. */
	void writeTransList( RedState *state );
	void writeDotFile( );

	virtual void finishRagelDef();

private:
	/* Writing labels and actions. */
	std::ostream &ONCHAR( Key lowKey, Key highKey );
	std::ostream &TRANS_ACTION( RedState *fromState, RedTrans *trans );
	std::ostream &ACTION( RedAction *action );
	std::ostream &KEY( Key key );
};

#endif


#endif /* _GVDOTGEN_H */
