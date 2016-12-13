/*
 * Copyright 2001-2012 Adrian Thurston <thurston@colm.net>
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

#ifndef _COLM_GVDOTGEN_H
#define _COLM_GVDOTGEN_H

#include <iostream>

#if 0

class GraphvizDotGen : public CodeGenData
{
public:
	GraphvizDotGen( ostream &out ) : CodeGenData(out) { }

	/* Print an fsm to out stream. */
	void writeTransList( RedState *state );
	void writeDotFile( );

	virtual void finishRagelDef();

private:
	/* Writing labels and actions. */
	std::ostream &ONCHAR( Key lowKey, Key highKey );
	std::ostream &TRANS_ACTION( RedState *fromState, RedTrans *trans );
	std::ostream &ACTION( RedAction *action );
	std::ostream &KEY( Key key );
};

#endif

#endif /* _COLM_GVDOTGEN_H */

