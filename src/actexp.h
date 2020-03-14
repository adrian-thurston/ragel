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

#ifndef _BINEXP_H
#define _BINEXP_H

#include "binary.h"

struct RedStateAp;
struct RedCondPair;

class ActExp
	: public virtual Tables
{
public:
	ActExp( const CodeGenArgs &args )
	:
		Tables( args )
	{}

	virtual void FROM_STATE_ACTION( RedStateAp *state );
	virtual void COND_ACTION( RedCondPair *cond );
	virtual void TO_STATE_ACTION( RedStateAp *state );
	virtual void EOF_ACTION( RedStateAp *state );

	virtual void NFA_PUSH_ACTION( RedNfaTarg *targ );
	virtual void NFA_POP_TEST( RedNfaTarg *targ );

	virtual std::ostream &FROM_STATE_ACTION_SWITCH();
	virtual std::ostream &ACTION_SWITCH();
	virtual std::ostream &TO_STATE_ACTION_SWITCH();
	virtual std::ostream &EOF_ACTION_SWITCH();

	virtual void TO_STATE_ACTIONS();
	virtual void REG_ACTIONS( std::string cond );
	virtual void FROM_STATE_ACTIONS();
	virtual void EOF_ACTIONS();

	virtual void NFA_FROM_STATE_ACTION_EXEC();
};

#endif

