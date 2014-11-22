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

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;

	switch ( args.codeStyle ) {
	case GenBinaryGotoLoop:
		codeGen = new BinaryGotoLoop(args);
		break;
	case GenBinaryGotoExp:
		codeGen = new BinaryGotoExp(args);
		break;
	case GenBinaryVarLoop:
		codeGen = new BinaryVarLoop(args);
		break;
	case GenFlatGotoLoop:
		codeGen = new FlatGotoLoop(args);
		break;
	case GenFlatGotoExp:
		codeGen = new FlatGotoExp(args);
		break;
	case GenSwitchGotoLoop:
		codeGen = new SwitchGotoLoop(args);
		break;
	case GenSwitchGotoExp:
		codeGen = new SwitchGotoExp(args);
		break;
	case GenIpGoto:
		codeGen = new IpGoto(args);
		break;
	}

	return codeGen;
}
