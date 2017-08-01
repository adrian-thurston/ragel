/*
 * Copyright 2005-2011 Adrian Thurston <thurston@colm.net>
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
