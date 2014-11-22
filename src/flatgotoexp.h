/*
 *  Copyright 2004-2014 Adrian Thurston <thurston@complang.org>
 */

#ifndef FLAT_GOTO_EXP_H
#define FLAT_GOTO_EXP_H

#include <iostream>
#include "flat.h"

/* Forwards. */
struct CodeGenData;

/*
 * FlatGotoExp
 */
class FlatGotoExp
	: public Flat
{
public:
	FlatGotoExp( const CodeGenArgs &args ) 
		: Flat(args) {}

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
	virtual void COND_ACTION( RedCondAp *cond );
};

#endif
