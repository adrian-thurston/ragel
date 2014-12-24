/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
 */

#ifndef SWITCH_GOTO_LOOP_H
#define SWITCH_GOTO_LOOP_H

#include <iostream>
#include "goto.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;
struct GenStateCond;

class SwitchGotoLoop
	: public Goto
{
public:
	SwitchGotoLoop( const CodeGenArgs &args ) 
		: Goto(args) {}

	void tableDataPass();

	std::ostream &ACTION_SWITCH();
	std::ostream &EXEC_FUNCS();
	std::ostream &TO_STATE_ACTION_SWITCH();
	std::ostream &FROM_STATE_ACTION_SWITCH();
	std::ostream &EOF_ACTION_SWITCH();

	/* Interface. */
	virtual void genAnalysis();
	virtual void writeData();
	virtual void writeExec();
};

namespace C
{
	class SwitchGotoLoop
	:
		public ::SwitchGotoLoop
	{
	public:
		SwitchGotoLoop( const CodeGenArgs &args )
			: ::SwitchGotoLoop( args )
		{}
	};
}

#endif
