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
#include "bingoto.h"
#include "binbreak.h"
#include "binvar.h"
#include "flatgoto.h"
#include "flatbreak.h"
#include "flatvar.h"
#include "switchgoto.h"
#include "switchbreak.h"
#include "switchvar.h"
#include "gotoloop.h"
#include "gotoexp.h"
#include "ipgoto.h"
#include "asm.h"

CodeGenData *makeCodeGenAsm( const HostLang *hostLang, const CodeGenArgs &args )
{
	return new AsmCodeGen( args );
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args )
{
	FsmGbl *id = args.id;
	CodeGenData *codeGen = 0;
	BackendFeature feature = hostLang->feature;
	if ( args.forceVar )
		feature = VarFeature;

	switch ( args.codeStyle ) {
	case GenBinaryLoop:
		if ( feature == GotoFeature )
			codeGen = new BinGotoLoop( args );
		else if ( feature == BreakFeature )
			codeGen = new BinBreakLoop( args );
		else
			codeGen = new BinVarLoop( args );
		break;

	case GenBinaryExp:
		if ( feature == GotoFeature )
			codeGen = new BinGotoExp( args );
		else if ( feature == BreakFeature )
			codeGen = new BinBreakExp( args );
		else
			codeGen = new BinVarExp( args );
		break;

	case GenFlatLoop:
		if ( feature == GotoFeature )
			codeGen = new FlatGotoLoop( args );
		else if ( feature == BreakFeature )
			codeGen = new FlatBreakLoop( args );
		else
			codeGen = new FlatVarLoop( args );
		break;

	case GenFlatExp:
		if ( feature == GotoFeature )
			codeGen = new FlatGotoExp( args );
		else if ( feature == BreakFeature )
			codeGen = new FlatBreakExp( args );
		else
			codeGen = new FlatVarExp( args );
		break;
	case GenSwitchLoop:
		if ( feature == GotoFeature )
			codeGen = new SwitchGotoLoop( args );
		else if ( feature == BreakFeature )
			codeGen = new SwitchBreakLoop( args );
		else
			codeGen = new SwitchVarLoop( args );
		break;

	case GenSwitchExp:
		if ( feature == GotoFeature )
			codeGen = new SwitchGotoExp( args );
		else if ( feature == BreakFeature )
			codeGen = new SwitchBreakExp( args );
		else
			codeGen = new SwitchVarExp( args );
		break;


	case GenGotoLoop:
		if ( feature == GotoFeature )
			codeGen = new GotoLoop(args);
		else
			id->error() << "unsupported lang/style combination" << endp;
		break;
	case GenGotoExp:
		if ( feature == GotoFeature )
			codeGen = new GotoExp(args);
		else
			id->error() << "unsupported lang/style combination" << endp;
		break;

	case GenIpGoto:
		if ( feature == GotoFeature )
			codeGen = new IpGoto(args);
		else
			id->error() << "unsupported lang/style combination" << endp;
		break;
	}

	return codeGen;
}
