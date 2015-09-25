/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
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

#ifndef BINEXPVAR_H
#define BINEXPVAR_H

#include <iostream>
#include "binvar.h"

/* Forwards. */
struct CodeGenData;

class BinaryExpVar
:
	public BinaryVar
{
public:
	BinaryExpVar( const CodeGenArgs &args );

	void calcIndexSize();
	void tableDataPass();

	virtual void genAnalysis();
	virtual void writeData();
	virtual void writeExec();

protected:

	std::ostream &TO_STATE_ACTION_SWITCH();
	std::ostream &FROM_STATE_ACTION_SWITCH();
	std::ostream &EOF_ACTION_SWITCH();
	std::ostream &ACTION_SWITCH();

	virtual void TO_STATE_ACTION( RedStateAp *state );
	virtual void FROM_STATE_ACTION( RedStateAp *state );
	virtual void EOF_ACTION( RedStateAp *state );
	virtual void COND_ACTION( RedCondPair *cond );

	void NFA_PUSH_ACTION( RedNfaTarg *targ );
	void NFA_POP_TEST( RedNfaTarg *targ );
};

namespace C
{
	class BinaryExpVar
	:
		public ::BinaryExpVar
	{
	public:
		BinaryExpVar( const CodeGenArgs &args )
			: ::BinaryExpVar( args )
		{}
	};
}

#endif
