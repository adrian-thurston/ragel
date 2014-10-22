/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
 */

#ifndef _C_GOTOLOOP_H
#define _C_GOTOLOOP_H

#include <iostream>
#include "goto.h"

/* Forwards. */
struct CodeGenData;
struct NameInst;
struct RedTransAp;
struct RedStateAp;
struct GenStateCond;

class GotoLooped
	: public Goto
{
public:
	GotoLooped( const CodeGenArgs &args ) 
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

#endif
