/*
 *  Copyright 2001-2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
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
