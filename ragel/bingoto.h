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

#ifndef RAGEL_BINGOTO_H
#define RAGEL_BINGOTO_H

#include "binary.h"
#include "actloop.h"
#include "actexp.h"

struct BinGoto
:
	public Binary, public TabGoto
{
	BinGoto( const CodeGenArgs &args, Binary::Type type )
	:
		Tables( args ),
		Binary( args, type ),
		TabGoto( args )
	{}

	void LOCATE_TRANS();
	void LOCATE_COND();
};

class BinGotoLoop
	: public BinGoto, public ActLoop
{
public:
	BinGotoLoop( const CodeGenArgs &args )
	:
		Tables( args ),
		BinGoto( args, Loop ),
		ActLoop( args )
	{}
};


class BinGotoExp
	: public BinGoto, public ActExp
{
public:
	BinGotoExp( const CodeGenArgs &args ) 
	:
		Tables( args ),
		BinGoto( args, Exp ),
		ActExp( args )
	{}
};


#endif
