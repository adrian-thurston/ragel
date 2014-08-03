/*
 *  Copyright 2005-2011 Adrian Thurston <thurston@complang.org>
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


#include "ragel.h"
#include "parsedata.h"
#include "fsmgraph.h"
#include "gendata.h"
#include "inputdata.h"
#include "version.h"

/*
 * Code generators.
 */
#include "xml.h"

#include "binloop.h"
#include "binexp.h"
#include "flatloop.h"
#include "flatexp.h"
#include "gotoloop.h"
#include "gotoexp.h"
#include "ipgoto.h"
#include "binbasic.h"

using std::cerr;
using std::endl;

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;

	switch ( args.codeStyle ) {
	case GenTables:
		codeGen = new BinaryLooped(args);
		break;
	case GenFTables:
		codeGen = new BinaryExpanded(args);
		break;
	case GenFlat:
		codeGen = new FlatLooped(args);
		break;
	case GenFFlat:
		codeGen = new FlatExpanded(args);
		break;
	case GenGoto:
		codeGen = new GotoLooped(args);
		break;
	case GenFGoto:
		codeGen = new GotoExpanded(args);
		break;
	case GenIpGoto:
		codeGen = new IpGoto(args);
		break;
	case GenBasic:
		codeGen = new BinaryBasic(args);
		break;
	}

	return codeGen;
}
