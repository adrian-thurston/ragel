/*
 *  Copyright 2005-2011 Adrian Thurston <thurston@complang.org>
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

#include "bingotoloop.h"
#include "bingotoexp.h"
#include "flatgotoloop.h"
#include "flatgotoexp.h"
#include "switchgotoloop.h"
#include "switchgotoexp.h"
#include "ipgoto.h"

#include "binvarloop.h"

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

	switch ( args.codeStyle ) {

	case GenBinaryLoop:
		if ( langSupportsGoto( hostLang ) )
			codeGen = new BinaryGotoLoop(args);
		else
			codeGen = new BinaryVarLoop(args);
		break;
	case GenBinaryExp:
		if ( langSupportsGoto( hostLang ) )
			codeGen = new BinaryGotoExp(args);
		else
			std::cerr << "unsupported lang/style combination" << endp;
		break;

	case GenFlatLoop:
		if ( langSupportsGoto( hostLang ) )
			codeGen = new FlatGotoLoop(args);
		else
			std::cerr << "unsupported lang/style combination" << endp;
		break;
	case GenFlatExp:
		if ( langSupportsGoto( hostLang ) )
			codeGen = new FlatGotoExp(args);
		else
			std::cerr << "unsupported lang/style combination" << endp;
		break;

	case GenSwitchLoop:
		if ( langSupportsGoto( hostLang ) )
			codeGen = new SwitchGotoLoop(args);
		else
			std::cerr << "unsupported lang/style combination" << endp;
		break;
	case GenSwitchExp:
		if ( langSupportsGoto( hostLang ) )
			codeGen = new SwitchGotoExp(args);
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

	return codeGen;
}
