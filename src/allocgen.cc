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
#include "rlparse.h"
#include "version.h"

/*
 * Code generators.
 */
#include "xml/xml.h"

#include "c/binloop.h"
#include "c/binexp.h"
#include "c/flat.h"
#include "c/fflat.h"
#include "c/goto.h"
#include "c/fgoto.h"
#include "c/ipgoto.h"
#include "c/split.h"

#include "d/table.h"
#include "d/ftable.h"
#include "d/flat.h"
#include "d/fflat.h"
#include "d/goto.h"
#include "d/fgoto.h"
#include "d/ipgoto.h"
#include "d/split.h"

#include "cs/table.h"
#include "cs/ftable.h"
#include "cs/flat.h"
#include "cs/fflat.h"
#include "cs/goto.h"
#include "cs/fgoto.h"
#include "cs/ipgoto.h"
#include "cs/split.h"

#include "dot/dot.h"

#include "java/java.h"

#include "go/table.h"
#include "go/ftable.h"
#include "go/flat.h"
#include "go/fflat.h"
#include "go/goto.h"
#include "go/fgoto.h"
#include "go/ipgoto.h"

#include "ml/table.h"
#include "ml/ftable.h"
#include "ml/flat.h"
#include "ml/fflat.h"
#include "ml/goto.h"
#include "ml/fgoto.h"

#include "ruby/table.h"
#include "ruby/ftable.h"
#include "ruby/flat.h"
#include "ruby/fflat.h"
#include "rbx/goto.h"

#include "crack/crack.h"

using std::cerr;
using std::endl;

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *cdMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;
	switch ( hostLang->lang ) {
	case HostLang::C:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new C::BinaryLooped(args);
			break;
		case GenFTables:
			codeGen = new C::BinaryExpanded(args);
			break;
		case GenFlat:
			codeGen = new C::FlatLooped(args);
			break;
		case GenFFlat:
			codeGen = new C::FlatExpanded(args);
			break;
		case GenGoto:
			codeGen = new C::GotoLooped(args);
			break;
		case GenFGoto:
			codeGen = new C::GotoExpanded(args);
			break;
		case GenIpGoto:
			codeGen = new C::IpGoto(args);
			break;
		case GenSplit:
			codeGen = new C::SplitGoto(args);
			break;
		}
		break;

	case HostLang::D:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new D::DTabCodeGen(args);
			break;
		case GenFTables:
			codeGen = new D::DFTabCodeGen(args);
			break;
		case GenFlat:
			codeGen = new D::DFlatCodeGen(args);
			break;
		case GenFFlat:
			codeGen = new D::DFFlatCodeGen(args);
			break;
		case GenGoto:
			codeGen = new D::DGotoCodeGen(args);
			break;
		case GenFGoto:
			codeGen = new D::DFGotoCodeGen(args);
			break;
		case GenIpGoto:
			codeGen = new D::DIpGotoCodeGen(args);
			break;
		case GenSplit:
			codeGen = new D::DSplitCodeGen(args);
			break;
		}
		break;

	case HostLang::D2:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new D::D2TabCodeGen(args);
			break;
		case GenFTables:
			codeGen = new D::D2FTabCodeGen(args);
			break;
		case GenFlat:
			codeGen = new D::D2FlatCodeGen(args);
			break;
		case GenFFlat:
			codeGen = new D::D2FFlatCodeGen(args);
			break;
		case GenGoto:
			codeGen = new D::D2GotoCodeGen(args);
			break;
		case GenFGoto:
			codeGen = new D::D2FGotoCodeGen(args);
			break;
		case GenIpGoto:
			codeGen = new D::D2IpGotoCodeGen(args);
			break;
		case GenSplit:
			codeGen = new D::D2SplitCodeGen(args);
			break;
		}
		break;

	default: break;
	}

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *javaMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = new Java::JavaTabCodeGen(args);

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *goMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;

	switch ( codeStyle ) {
	case GenTables:
		codeGen = new Go::GoTabCodeGen(args);
		break;
	case GenFTables:
		codeGen = new Go::GoFTabCodeGen(args);
		break;
	case GenFlat:
		codeGen = new Go::GoFlatCodeGen(args);
		break;
	case GenFFlat:
		codeGen = new Go::GoFFlatCodeGen(args);
		break;
	case GenGoto:
		codeGen = new Go::GoGotoCodeGen(args);
		break;
	case GenFGoto:
		codeGen = new Go::GoFGotoCodeGen(args);
		break;
	case GenIpGoto:
		codeGen = new Go::GoIpGotoCodeGen(args);
		break;
	default:
		cerr << "Invalid output style, only -T0, -T1, -F0, -F1, -G0, -G1 and -G2 are supported.\n";
		exit(1);
	}

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *crackMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen;

	switch ( codeStyle ) {
	case GenFlat:
		codeGen = new Crack::CrackFlatCodeGen(args);
		break;
	default:
		cerr << "Invalid output style, only -F0 is supported. Please "
			"rerun ragel including this flag.\n";
		exit(1);
	}

	return codeGen;
}


/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *rubyMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;
	switch ( codeStyle ) {
		case GenTables: 
			codeGen = new Ruby::RubyTabCodeGen(args);
			break;
		case GenFTables:
			codeGen = new Ruby::RubyFTabCodeGen(args);
			break;
		case GenFlat:
			codeGen = new Ruby::RubyFlatCodeGen(args);
			break;
		case GenFFlat:
			codeGen = new Ruby::RubyFFlatCodeGen(args);
			break;
		case GenGoto:
			if ( rubyImpl == Rubinius ) {
				codeGen = new Rbx::RbxGotoCodeGen(args);
			} else {
				cerr << "Goto style is still _very_ experimental " 
					"and only supported using Rubinius.\n"
					"You may want to enable the --rbx flag "
					" to give it a try.\n";
				exit(1);
			}
			break;
		default:
			cerr << "Invalid code style\n";
			exit(1);
			break;
	}

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *csharpMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;

	switch ( codeStyle ) {
	case GenTables:
		codeGen = new CSharp::CSharpTabCodeGen(args);
		break;
	case GenFTables:
		codeGen = new CSharp::CSharpFTabCodeGen(args);
		break;
	case GenFlat:
		codeGen = new CSharp::CSharpFlatCodeGen(args);
		break;
	case GenFFlat:
		codeGen = new CSharp::CSharpFFlatCodeGen(args);
		break;
	case GenGoto:
		codeGen = new CSharp::CSharpGotoCodeGen(args);
		break;
	case GenFGoto:
		codeGen = new CSharp::CSharpFGotoCodeGen(args);
		break;
	case GenIpGoto:
		codeGen = new CSharp::CSharpIpGotoCodeGen(args);
		break;
	case GenSplit:
		codeGen = new CSharp::CSharpSplitCodeGen(args);
		break;
	}

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *ocamlMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;

	switch ( codeStyle ) {
	case GenTables:
		codeGen = new OCaml::OCamlTabCodeGen(args);
		break;
	case GenFTables:
		codeGen = new OCaml::OCamlFTabCodeGen(args);
		break;
	case GenFlat:
		codeGen = new OCaml::OCamlFlatCodeGen(args);
		break;
	case GenFFlat:
		codeGen = new OCaml::OCamlFFlatCodeGen(args);
		break;
	case GenGoto:
		codeGen = new OCaml::OCamlGotoCodeGen(args);
		break;
	case GenFGoto:
		codeGen = new OCaml::OCamlFGotoCodeGen(args);
		break;
	default:
		cerr << "I only support the -T0 -T1 -F0 -F1 -G0 and -G1 output styles for OCaml.\n";
		exit(1);
	}

	return codeGen;
}

CodeGenData *makeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *cgd = 0;
	if ( hostLang == &hostLangC )
		cgd = cdMakeCodeGen( args );
	else if ( hostLang == &hostLangD )
		cgd = cdMakeCodeGen( args );
	else if ( hostLang == &hostLangD2 )
		cgd = cdMakeCodeGen( args );
	else if ( hostLang == &hostLangGo )
		cgd = goMakeCodeGen( args );
	else if ( hostLang == &hostLangJava )
		cgd = javaMakeCodeGen( args );
	else if ( hostLang == &hostLangRuby )
		cgd = rubyMakeCodeGen( args );
	else if ( hostLang == &hostLangCSharp )
		cgd = csharpMakeCodeGen( args );
	else if ( hostLang == &hostLangOCaml )
		cgd = ocamlMakeCodeGen( args );
	else if ( hostLang == &hostLangCrack )
		cgd = crackMakeCodeGen( args );
	return cgd;
}
