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

#include "binloopvar.h"
#include "binloopgoto.h"
#include "binexpvar.h"
#include "binexpgoto.h"
#include "flatloopgoto.h"
#include "flatexpgoto.h"
#include "switchloopgoto.h"
#include "switchexpgoto.h"
#include "ipgoto.h"
#include "asm.h"

using std::cerr;
using std::endl;

bool langSupportsGoto( const HostLang *hostLang )
{
	if ( hostLang->lang == HostLang::Ruby || hostLang->lang == HostLang::OCaml )
		return false;
	
	return true;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;

	if ( hostLang->lang == HostLang::Asm ) {
		codeGen = new AsmCodeGen( args );
	}
	else if ( args.pd->id->backend == Direct ) {
		switch ( args.codeStyle ) {
		case GenBinaryLoop:
			codeGen = new C::BinaryLoopGoto(args);
			break;
		case GenBinaryExp:
			codeGen = new C::BinaryExpGoto(args);
			break;

		case GenFlatLoop:
			codeGen = new C::FlatLoopGoto(args);
			break;
		case GenFlatExp:
			codeGen = new C::FlatExpGoto(args);
			break;

		case GenSwitchLoop:
			codeGen = new C::SwitchLoopGoto(args);
			break;
		case GenSwitchExp:
			codeGen = new C::SwitchExpGoto(args);
			break;

		case GenIpGoto:
			codeGen = new C::IpGoto(args);
			break;
		}
	}
	else {
		switch ( args.codeStyle ) {

		case GenBinaryLoop:
			if ( langSupportsGoto( hostLang ) )
				codeGen = new BinaryLoopGoto( args );
			else
				codeGen = new BinLoopVar( args);
			break;
		case GenBinaryExp:
			if ( langSupportsGoto( hostLang ) )
				codeGen = new BinaryExpGoto( args );
			else
				codeGen = new BinaryExpVar( args );
			break;

		case GenFlatLoop:
			if ( langSupportsGoto( hostLang ) )
				codeGen = new FlatLoopGoto(args);
			else
				std::cerr << "unsupported lang/style combination" << endp;
			break;
		case GenFlatExp:
			if ( langSupportsGoto( hostLang ) )
				codeGen = new FlatExpGoto(args);
			else
				std::cerr << "unsupported lang/style combination" << endp;
			break;

		case GenSwitchLoop:
			if ( langSupportsGoto( hostLang ) )
				codeGen = new SwitchLoopGoto(args);
			else
				std::cerr << "unsupported lang/style combination" << endp;
			break;
		case GenSwitchExp:
			if ( langSupportsGoto( hostLang ) )
				codeGen = new SwitchExpGoto(args);
			else
				std::cerr << "unsupported lang/style combination" << endp;
			break;

		case GenIpGoto:
			if ( langSupportsGoto( hostLang ) )
				codeGen = new IpGoto(args);
			else
				std::cerr << "unsupported lang/style combination" << endp;
			break;
		}
	}

	return codeGen;
}
