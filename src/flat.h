/*
 * Copyright 2004-2018 Adrian Thurston <thurston@colm.net>
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

#ifndef _C_FLAT_H
#define _C_FLAT_H

#include <iostream>
#include "codegen.h"
#include "tables.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;

class Flat
	: public virtual Tables
{
protected:
	enum Type {
		Loop = 1, Exp
	};

public:
	Flat( const CodeGenArgs &args, Type type ) 
	:
		Tables( args ),
		type(type)
	{}

	virtual ~Flat() { }

protected:
	Type type;

	void taKeys();
	void taKeySpans();
	void taCharClass();
	void taActions();
	void taFlatIndexOffset();
	void taIndices();
	void taIndexDefaults();
	void taTransCondSpaces();
	void taTransOffsets();
	void taCondTargs();
	void taCondActions();
	void taToStateActions();
	void taFromStateActions();
	void taEofActions();
	void taEofTrans();
	void taEofConds();
	void taNfaTargs();
	void taNfaOffsets();
	void taNfaPushActions();
	void taNfaPopTrans();

	void setKeyType();

	std::ostream &INDICES();
	std::ostream &TRANS_COND_SPACES();
	std::ostream &TRANS_OFFSETS();
	std::ostream &TRANS_LENGTHS();
	std::ostream &COND_KEYS();
	std::ostream &COND_TARGS();
	std::ostream &COND_ACTIONS();

	virtual void setTableState( TableArray::State );

	virtual void genAnalysis();
	virtual void tableDataPass();
	virtual void writeData();
};

#endif
