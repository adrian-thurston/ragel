/*
 * Copyright 2005-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
#include "binvarloop.h"
#include "bingotoloop.h"
#include "binvarexp.h"
#include "bingotoexp.h"
#include "flatgotoloop.h"
#include "flatvarloop.h"
#include "flatgotoexp.h"
#include "flatvarexp.h"
#include "gotoloop.h"
#include "gotoexp.h"
#include "ipgoto.h"

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args )
{
	FsmGbl *id = args.id;
	CodeGenData *codeGen = 0;

	switch ( args.codeStyle ) {
	case GenBinaryLoop:
		if ( id->backendFeature == GotoFeature )
			codeGen = new BinaryLoopGoto( args );
		else
			codeGen = new BinaryLoopVar( args );
		break;

	case GenBinaryExp:
		if ( id->backendFeature == GotoFeature )
			codeGen = new BinaryExpGoto( args );
		else
			codeGen = new BinaryExpVar( args );
		break;

	case GenFlatLoop:
		if ( id->backendFeature == GotoFeature )
			codeGen = new FlatLoopGoto( args );
		else
			codeGen = new FlatLoopVar( args );
		break;

	case GenFlatExp:
		if ( id->backendFeature == GotoFeature )
			codeGen = new FlatExpGoto( args );
		else
			codeGen = new FlatExpVar( args );
		break;

	case GenSwitchLoop:
		if ( id->backendFeature == GotoFeature )
			codeGen = new SwitchLoopGoto(args);
		else
			id->error() << "unsupported lang/style combination" << endp;
		break;
	case GenSwitchExp:
		if ( id->backendFeature == GotoFeature )
			codeGen = new SwitchExpGoto(args);
		else
			id->error() << "unsupported lang/style combination" << endp;
		break;

	case GenIpGoto:
		if ( id->backendFeature == GotoFeature )
			codeGen = new IpGoto(args);
		else
			id->error() << "unsupported lang/style combination" << endp;
		break;
	}

	return codeGen;
}
