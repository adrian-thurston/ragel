/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@complang.org>
 *            2004 Erich Ocean <eric.ocean@ampede.com>
 *            2005 Alan West <alan@alanz.com>
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

#ifndef _C_BINLOOP_H
#define _C_BINLOOP_H

#include <iostream>
#include "binary.h"
#include "vector.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;

namespace C {

class BinaryLooped
	: public Binary
{
public:
	BinaryLooped( const CodeGenArgs &args );

	virtual void writeData();
	virtual void writeExec();

	virtual void calcIndexSize();
	virtual void setTableState( TableArray::State );
	virtual void tableDataPass();

	TableArray condOffsets;
	TableArray condLens;
	TableArray condKeysV1;
	TableArray condSpacesV1;
	TableArray keys;
	TableArray indexOffsets;
	TableArray indicies;
	TableArray transTargsWi;
	TableArray transActionsWi;
	TableArray transCondSpacesWi;
	TableArray transOffsetsWi;
	TableArray transLengthsWi;
	TableArray transTargs;
	TableArray transActions;
	TableArray transCondSpaces;
	TableArray transOffsets;
	TableArray transLengths;
	TableArray condKeys;
	TableArray condTargs;
	TableArray condActions;
	TableArray toStateActions;
	TableArray fromStateActions;
	TableArray eofActions;
	TableArray eofTrans;

};

}

#endif
