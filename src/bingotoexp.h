/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
 */

#ifndef BIN_GOTO_EXP_H
#define BIN_GOTO_EXP_H

#include <iostream>
#include "binary.h"

/* Forwards. */
struct CodeGenData;

class BinaryGotoExp : public Binary
{
public:
	BinaryGotoExp( const CodeGenArgs &args );

	void calcIndexSize();
	void tableDataPass();

	virtual void genAnalysis();
	virtual void writeData();
	virtual void writeExec();

protected:
	std::ostream &TO_STATE_ACTION_SWITCH();
	std::ostream &FROM_STATE_ACTION_SWITCH();
	std::ostream &EOF_ACTION_SWITCH();
	std::ostream &ACTION_SWITCH();

	virtual void TO_STATE_ACTION( RedStateAp *state );
	virtual void FROM_STATE_ACTION( RedStateAp *state );
	virtual void EOF_ACTION( RedStateAp *state );
	virtual void COND_ACTION( RedCondPair *cond );

};

namespace C
{
	class BinaryGotoExp
	:
		public ::BinaryGotoExp
	{
	public:
		BinaryGotoExp( const CodeGenArgs &args )
			: ::BinaryGotoExp( args )
		{}
	};
}

#endif
