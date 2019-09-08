/*
 * Copyright 2018-2018 Adrian Thurston <thurston@colm.net>
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

#ifndef RAGEL_FLATBREAK_H
#define RAGEL_FLATBREAK_H

#include "flat.h"
#include "actloop.h"
#include "actexp.h"

struct FlatBreak
:
	public Flat, public TabBreak
{
	FlatBreak( const CodeGenArgs &args, Flat::Type type )
	:
		Tables( args ),
		Flat( args, type ),
		TabBreak( args )
	{}

	void LOCATE_TRANS();
	void LOCATE_COND();
};

class FlatBreakLoop
	: public FlatBreak, public ActLoop
{
public:
	FlatBreakLoop( const CodeGenArgs &args )
	:
		Tables( args ),
		FlatBreak( args, Flat::Loop ),
		ActLoop( args )
	{}
};

/*
 * FlatBreakExp
 */
class FlatBreakExp
	: public FlatBreak, public ActExp
{
public:
	FlatBreakExp( const CodeGenArgs &args ) 
	:
		Tables( args ),
		FlatBreak( args, Flat::Exp ),
		ActExp( args )
	{}
};

#endif
