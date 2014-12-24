/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
 */

#ifndef SWITCH_GOTO_EXP_H
#define SWITCH_GOTO_EXP_H

#include <iostream>
#include "goto.h"

/* Forwards. */
struct CodeGenData;

/*
 * class SwitchGotoExp
 */
class SwitchGotoExp
	: public Goto
{
public:
	SwitchGotoExp( const CodeGenArgs &args )
		: Goto(args) {}

	std::ostream &EXEC_ACTIONS();
	std::ostream &TO_STATE_ACTION_SWITCH();
	std::ostream &FROM_STATE_ACTION_SWITCH();
	std::ostream &FINISH_CASES();
	std::ostream &EOF_ACTION_SWITCH();
	unsigned int TO_STATE_ACTION( RedStateAp *state );
	unsigned int FROM_STATE_ACTION( RedStateAp *state );
	unsigned int EOF_ACTION( RedStateAp *state );

	void tableDataPass();

	virtual void genAnalysis();
	virtual void writeData();
	virtual void writeExec();
};

namespace C
{
	class SwitchGotoExp
	:
		public ::SwitchGotoExp
	{
	public:
		SwitchGotoExp( const CodeGenArgs &args )
			: ::SwitchGotoExp( args )
		{}
	};
}


#endif
