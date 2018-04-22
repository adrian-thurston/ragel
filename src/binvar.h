/*
 * Copyright 2014-2018 Adrian Thurston <thurston@colm.net>
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

#ifndef RAGEL_BINVAR_H
#define RAGEL_BINVAR_H

#include "binary.h"
#include "actloop.h"
#include "actexp.h"

struct BinaryVar
:
	public Binary
{
	BinaryVar( const CodeGenArgs &args, Binary::Type type )
	:
		Tables( args ),
		Binary( args, type )
	{}

	void COND_BIN_SEARCH( TableArray &keys, std::string ok, std::string error );

	void LOCATE_TRANS();
	void LOCATE_COND();
	void VARS();

	void GOTO( ostream &ret, int gotoDest, bool inFinish );
	void GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void CALL( ostream &ret, int callDest, int targState, bool inFinish );
	void NCALL( ostream &ret, int callDest, int targState, bool inFinish );
	void CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void RET( ostream &ret, bool inFinish );
	void BREAK( ostream &ret, int targState, bool csForced );
	void NRET( ostream &ret, bool inFinish );
	void NBREAK( ostream &ret, int targState, bool csForced );

	void NFA_POP();

	void genAnalysis();

	virtual void tableDataPass();
	virtual void writeData();
	virtual void writeExec();


};

class BinVarLoop
	: public BinaryVar, public ActLoop
{
public:
	BinVarLoop( const CodeGenArgs &args )
	:
		Tables( args ),
		BinaryVar( args, Loop ),
		ActLoop( args )
	{}
};

class BinVarExp
:
	public BinaryVar, public ActExp
{
public:
	BinVarExp( const CodeGenArgs &args ) 
	:
		Tables( args ),
		BinaryVar( args, Exp ),
		ActExp( args )
	{}
};

#endif
