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

#ifndef FLATLOOPVAR_H
#define FLATLOOPVAR_H

#include <iostream>
#include "flatvar.h"
#include "vector.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;

class FlatLoopVar
	: public FlatVar
{
public:
	FlatLoopVar( const CodeGenArgs &args );

	void calcIndexSize();
	void tableDataPass();

	virtual void genAnalysis();
	virtual void writeData();
	virtual void writeExec();

	virtual void TO_STATE_ACTION( RedStateAp *state );
	virtual void FROM_STATE_ACTION( RedStateAp *state );
	virtual void EOF_ACTION( RedStateAp *state );
	virtual void COND_ACTION( RedCondPair *cond );

	std::ostream &TO_STATE_ACTION_SWITCH();
	std::ostream &FROM_STATE_ACTION_SWITCH();
	std::ostream &EOF_ACTION_SWITCH();
	std::ostream &ACTION_SWITCH();

	virtual void NFA_PUSH_ACTION( RedNfaTarg *targ );
	virtual void NFA_POP_TEST( RedNfaTarg *targ );
};

namespace C
{
	class FlatLoopVar
	:
		public ::FlatLoopVar
	{
	public:
		FlatLoopVar( const CodeGenArgs &args )
			: ::FlatLoopVar( args )
		{}
	};
}

#endif
