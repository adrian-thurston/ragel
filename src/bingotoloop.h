/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
 */

#ifndef BIN_GOTO_LOOP_H
#define BIN_GOTO_LOOP_H

#include <iostream>
#include "binary.h"
#include "vector.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;

class BinaryGotoLoop
	: public Binary
{
public:
	BinaryGotoLoop( const CodeGenArgs &args );

	void calcIndexSize();
	void tableDataPass();

	virtual void genAnalysis();
	virtual void writeData();
	virtual void writeExec();

	virtual void TO_STATE_ACTION( RedStateAp *state );
	virtual void FROM_STATE_ACTION( RedStateAp *state );
	virtual void EOF_ACTION( RedStateAp *state );
	virtual void COND_ACTION( RedCondPair *cond );

	std::ostream &TO_STATE_ACTION_SWITCH();
	std::ostream &FROM_STATE_ACTION_SWITCH();
	std::ostream &EOF_ACTION_SWITCH();
	std::ostream &ACTION_SWITCH();
};

namespace C
{
	class BinaryGotoLoop
	:
		public ::BinaryGotoLoop
	{
	public:
		BinaryGotoLoop( const CodeGenArgs &args )
			: ::BinaryGotoLoop( args )
		{}
	};
}

#endif
