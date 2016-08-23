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
#include "binloop.h"
#include "binexp.h"
#include "flatloop.h"
#include "flatexp.h"
#include "gotoloop.h"
#include "gotoexp.h"
#include "ipgoto.h"
#include "asm.h"

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args )
{
	FsmGbl *id = args.id;
	CodeGenData *codeGen = 0;
	if ( hostLang->lang == HostLang::Asm ) {
		codeGen = new AsmCodeGen( args );
	}
	else if ( id->backend == Direct ) {
		switch ( args.codeStyle ) {
		case GenBinaryLoop:
			codeGen = new BinaryLoopGoto( args );
			break;
		case GenBinaryExp:
			codeGen = new BinaryExpGoto( args );
			break;

		case GenFlatLoop:
			codeGen = new FlatLoopGoto( args );
			break;
		case GenFlatExp:
			codeGen = new FlatExpGoto( args );
			break;

		case GenSwitchLoop:
			codeGen = new SwitchLoopGoto( args );
			break;
		case GenSwitchExp:
			codeGen = new SwitchExpGoto( args );
			break;
		case GenIpGoto:
			codeGen = new IpGoto( args );
			break;
		}
	}
	else {
		switch ( args.codeStyle ) {
		case GenBinaryLoop:
			codeGen = new BinaryLoopGoto( args );
			break;
		case GenBinaryExp:
			codeGen = new BinaryExpGoto( args );
			break;

		case GenFlatLoop:
			codeGen = new FlatLoopGoto( args );
			break;
		case GenFlatExp:
			codeGen = new FlatExpGoto( args );
			break;

		case GenSwitchLoop:
			codeGen = new SwitchLoopGoto(args);
			break;
		case GenSwitchExp:
			codeGen = new SwitchExpGoto(args);
			break;

		case GenIpGoto:
			codeGen = new IpGoto(args);
			break;
		}
	}

	return codeGen;
}
