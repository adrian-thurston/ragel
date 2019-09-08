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

#ifndef RAGEL_SWITCHGOTO_H
#define RAGEL_SWITCHGOTO_H

#include "switch.h"
#include "actloop.h"
#include "actexp.h"

struct SwitchGoto
:
	public Switch, public TabGoto
{
	SwitchGoto( const CodeGenArgs &args, Switch::Type type )
	:
		Tables( args ),
		Switch( args, type ),
		TabGoto( args )
	{}

	void LOCATE_COND();
};

class SwitchGotoLoop
	: public SwitchGoto, public ActLoop
{
public:
	SwitchGotoLoop( const CodeGenArgs &args )
	:
		Tables( args ),
		SwitchGoto( args, Loop ),
		ActLoop( args )
	{}
};


class SwitchGotoExp
	: public SwitchGoto, public ActExp
{
public:
	SwitchGotoExp( const CodeGenArgs &args ) 
	:
		Tables( args ),
		SwitchGoto( args, Exp ),
		ActExp( args )
	{}
};


#endif
