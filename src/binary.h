/*
 * Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
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

#ifndef _C_BINARY_H
#define _C_BINARY_H

#include <iostream>
#include "codegen.h"
#include "tables.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;

class Binary
	: public virtual Tables
{
protected:
	enum Type {
		Loop = 1, Exp
	};

public:
	Binary( const CodeGenArgs &args, Type type )
	:
		Tables( args ),
		type(type)
	{}

protected:
	Type type;

	std::ostream &COND_KEYS_v1();
	std::ostream &COND_SPACES_v1();
	std::ostream &INDICES();
	std::ostream &INDEX_OFFSETS();
	std::ostream &SINGLE_LENS();
	std::ostream &RANGE_LENS();
	std::ostream &TRANS_TARGS_WI();
	std::ostream &ACTIONS_ARRAY();

	void taKeyOffsets();
	void taSingleLens();
	void taRangeLens();
	void taIndexOffsets();
	void taIndices();
	void taTransCondSpacesWi();
	void taTransOffsetsWi();
	void taTransLengthsWi();
	void taTransCondSpaces();
	void taTransOffsets();
	void taTransLengths();
	void taCondTargs();
	void taCondActions();
	void taToStateActions();
	void taFromStateActions();
	void taEofTrans();
	void taEofConds();
	void taEofActions();
	void taKeys();
	void taActions();
	void taCondKeys();
	void taNfaTargs();
	void taNfaOffsets();
	void taNfaPushActions();
	void taNfaPopTrans();

	void setKeyType();

	void setTableState( TableArray::State );

	virtual void writeData();
	virtual void tableDataPass();
	virtual void genAnalysis();
};

#endif
