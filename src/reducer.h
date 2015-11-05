#include <colm/pdarun.h>
#include <colm/bytecode.h>
#include <colm/defs.h>
#include <colm/input.h>
#include <colm/tree.h>
#include <colm/program.h>
#include <colm/colm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <iostream>

#include "vector.h"
#include "inputdata.h"
#include "parsedata.h"

#ifndef _REDUCER_H
#define _REDUCER_H

struct Reducer
{
	Reducer( InputData &id, const HostLang *hostLang,
			MinimizeLevel minimizeLevel, MinimizeOpt minimizeOpt )
	:
		id(id),
		section(0),
		pd(0),
		machineSpec(0),
		machineName(0),
		includeDepth(0),
		hostLang(hostLang),
		minimizeLevel(minimizeLevel),
		minimizeOpt(minimizeOpt),
		
		/* Should be passed into the load, somehow. */
		targetMachine(0),
		searchMachine(0),

		current(0),

		exprLeft(0)
	{
		exportContext.append( false );
	}

	InputData &id;
	Section *section;
	ParseData *pd;
	char *machineSpec;
	char *machineName;
	int includeDepth;
	const HostLang *hostLang;
	MinimizeLevel minimizeLevel;
	MinimizeOpt minimizeOpt;
	Vector<std::string> writeArgs;

	/* Should this go in the parse data? Probably. */
	Vector<bool> exportContext;

	const char *targetMachine;
	const char *searchMachine;

	int current;

	/* Expression reduction. Grammar is right recursive. Output tree is left.
	 * This keeps the left tree. */
	Expression *exprLeft;

	/* Term reduction. Also right recursive grammar. */
	Term *termLeft;

	void commit_reduce_forward( program_t *prg, tree_t **root,
		struct pda_run *pda_run, parse_tree_t *pt );

	void loadMachineName( string data );
	void tryMachineDef( InputLoc &loc, std::string name, 
			MachineDef *machineDef, bool isInstance );

	void reduceFile( const char *inputFileName, const char *targetMachine,
			const char *searchMachine );

	void topReduce( const char *inputFileName, const char *targetMachine,
			const char *searchMachine );
};

char *unescape( const char *s, int slen );
char *unescape( const char *s );

#endif
