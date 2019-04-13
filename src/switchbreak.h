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

#ifndef RAGEL_SWITCHBREAK_H
#define RAGEL_SWITCHBREAK_H

#include "switch.h"
#include "actloop.h"
#include "actexp.h"

struct SwitchBreak
:
	public Switch, public TabBreak
{
	SwitchBreak( const CodeGenArgs &args, Switch::Type type )
	:
		Tables( args ),
		Switch( args, type ),
		TabBreak( args )
	{}

	void LOCATE_COND();
};

class SwitchBreakLoop
	: public SwitchBreak, public ActLoop
{
public:
	SwitchBreakLoop( const CodeGenArgs &args )
	:
		Tables( args ),
		SwitchBreak( args, Loop ),
		ActLoop( args )
	{}
};


class SwitchBreakExp
	: public SwitchBreak, public ActExp
{
public:
	SwitchBreakExp( const CodeGenArgs &args ) 
	:
		Tables( args ),
		SwitchBreak( args, Exp ),
		ActExp( args )
	{}
};


#endif
