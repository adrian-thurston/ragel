/*
 * @LANG: c
 * @ALLOW_GENFLAGS: -T0 -T1 -F0 -F1
 * @ALLOW_MINFLAGS: -n -m -l
 *
 * Test works with split code gen.
 */

#include <string.h>
#include <stdio.h>

#include "strings2.h"

%%{
	machine strs;
	variable cs fsm->cs;

	main := 
		"/lib/ld-linux.so.2\n" |
		"libstdc++-libc6.2-2.so.3\n" |
		"cerr\n" |
		"__cp_push_exception\n" |
		"_DYNAMIC\n" |
		"endl__FR7ostream\n" |
		"__ls__7ostreamc\n" |
		"_._9exception\n" |
		"__vt_9bad_alloc\n" |
		"__rtti_user\n" |
		"__ls__7ostreamPFR7ostream_R7ostream\n" |
		"__rtti_si\n" |
		"_init\n" |
		"bad__C3ios\n" |
		"__throw\n" |
		"__ls__7ostreamPCc\n" |
		"__deregister_frame_info\n" |
		"terminate__Fv\n" |
		"__ls__7ostreamb\n" |
		"__ls__7ostreami\n" |
		"__8ofstreamiPCcii\n" |
		"__builtin_vec_new\n" |
		"_fini\n" |
		"__9exception\n" |
		"__builtin_vec_delete\n" |
		"_GLOBAL_OFFSET_TABLE_\n" |
		"__vt_9exception\n" |
		"__nw__FUiPv\n" |
		"_._9bad_alloc\n" |
		"__builtin_delete\n" |
		"__builtin_new\n" |
		"cout\n" |
		"__register_frame_info\n" |
		"__eh_alloc\n" |
		"__gmon_start__\n" |
		"libm.so.6\n" |
		"libc.so.6\n" |
		"strcpy\n" |
		"stdout\n" |
		"memmove\n" |
		"memcpy\n" |
		"malloc\n" |
		"strtoul\n" |
		"fprintf\n" |
		"stdin\n" |
		"ferror\n" |
		"strncpy\n" |
		"strcasecmp\n" |
		"realloc\n" |
		"_IO_getc\n" |
		"fread\n" |
		"memset\n" |
		"clearerr\n" |
		"__assert_fail\n" |
		"strcmp\n" |
		"stderr\n" |
		"fwrite\n" |
		"__errno_location\n" |
		"exit\n" |
		"fopen\n" |
		"atoi\n" |
		"_IO_stdin_used\n" |
		"__libc_start_main\n" |
		"strlen\n" |
		"free\n" |
		"_edata\n" |
		"__bss_start\n" |
		"_end\n" |
		"GLIBC_2.1\n" |
		"GLIBC_2.0\n" |
		"PTRh\n" |
		"QVhL\n" |
		"<WVS\n" |
		"LWVS\n" |
		"PHRW\n" |
		"<WVS\n" |
		"\WVS\n" |
		",WVS\n" |
		"@Phl\n" |
		"<WVS\n" |
		"jZjA\n" |
		"jzja\n" |
		"j9j0\n" |
		"j9j0\n" |
		"jZjA\n" |
		"jzja\n" |
		"jzja\n" |
		"jZjA\n" |
		"j~j!\n" |
		"j~j \n" |
		"j/j!\n" |
		"j@j:\n" |
		"j`j[\n" |
		"j~j{\n" |
		"j9j0\n" |
		"jFjA\n" |
		"jfja\n" |
		",WVS\n" |
		",WVS\n" |
		";C<|\n" |
		"<WVS\n" |
		"C ;C\n" |
		"C$;C\n" |
		"C$;C\n" |
		"C ;C\n" |
		",WVS\n" |
		";E uF\n" |
		"P ;U\n" |
		"P ;U\n" |
		"E$fP\n" |
		"E$fP\n" |
		"E$fP\n" |
		"E$fP\n" |
		"E$fP\n" |
		"E$fP\n" |
		"E$fP\n" |
		"E$fP\n" |
		"u!h@\n" |
		"PHRj\n" |
		"PHRj\n" |
		"P\	U\n" |
		"j]hY\n" |
		"johY\n" |
		"PHRj\n" |
		"PHRj\n" |
		"E fPj\n" |
		"E fP\n" |
		"E fP\n" |
		"E fP\n" |
		"E fP\n" |
		"E fP\n" |
		"E fPj\n" |
		"t$h`\n" |
		"F ;C }	\n" |
		"F ;C ~	\n" |
		"@X:BXt)\n" |
		"\WVS\n" |
		"\WVS\n" |
		"PPRS\n" |
		"F ;C }	\n" |
		"F ;C ~	\n" |
		"@X:BXt)\n" |
		";H(}:\n" |
		"@ fP\n" |
		";P |\n" |
		"<WVS\n" |
		";P |\n" |
		"bad_alloc\n" |
		"usage: ragel [options] file\n" |
		"general:\n" |
		"   -h, -H, -?   Disply this usage.\n" |
		"   -o <file>    Write output to <file>.\n" |
		"   -s           Print stats on the compiled fsm.\n" |
		"   -f           Dump the final fsm.\n" |
		"fsm minimization:\n" |
		"   -n           No minimization (default).\n" |
		"   -m           Find the minimal fsm accepting the language.\n" |
		"generated code language:\n" |
		"   -c           Generate c code (default).\n" |
		"   -C           Generate c++ code.\n" |
		"generated code style:\n" |
		"   -T0          Generate a table driven fsm (default).\n" |
		"   -T1          Generate a faster table driven fsm.\n" |
		"   -S0          Generate a switch driven fsm.\n" |
		"   -G0          Generate a goto driven fsm.\n" |
		"   -G1          Generate a faster goto driven fsm.\n" |
		"   -G2          Generate a really fast goto driven fsm.\n" |
		"char * FileNameFromStem(char *, char *)\n" |
		"main.cpp\n" |
		"len > 0\n" |
		"main\n" |
		"ragel: main graph not defined\n" |
		"graph states:      \n" |
		"graph transitions: \n" |
		"machine states:    \n" |
		"machine functions: \n" |
		"function array:    \n" |
		"T:S:G:Cco:senmabjkfhH?-:\n" |
		"ragel: zero length output file name given\n" |
		"ragel: output file already given\n" |
		"ragel: invalid param specified (try -h for a list of options)\n" |
		"help\n" |
		"ragel: zero length input file name given\n" |
		"ragel: input file already given\n" |
		"ragel: warning: -e given but minimization is not enabled\n" |
		"ragel: no input file (try -h for a list of options)\n" |
		" for reading\n" |
		"ragel: could not open \n" |
		" for writing\n" |
		"ragel: error opening \n" |
		" * Parts of this file are copied from Ragel source covered by the GNU\n" |
		" * GPL. As a special exception, you may use the parts of this file copied\n" |
		" * from Ragel source without restriction. The remainder is derived from\n" |
		"bad_alloc\n" |
		"%s:%i: unterminated literal\n" |
		"%s:%i: unterminated comment\n" |
		"%s:%i: bad character in literal\n" |
		"fatal flex scanner internal error--no action found\n" |
		"fatal flex scanner internal error--end of buffer missed\n" |
		"fatal error - scanner input buffer overflow\n" |
		"input in flex scanner failed\n" |
		"out of dynamic memory in yy_create_buffer()\n" |
		"out of dynamic memory in yy_scan_buffer()\n" |
		"out of dynamic memory in yy_scan_bytes()\n" |
		"bad buffer in yy_scan_bytes()\n" |
		"bad_alloc\n" |
		"%s:%i: warning: range gives null fsm\n" |
		"%s:%i: warning: literal used in range is not of length 1, using 0x%x\n" |
		"%s:%i: warning: overflow in byte constant\n" |
		"parse error\n" |
		"parser stack overflow\n" |
		"%s:%i: %s\n" |
		"bad_alloc\n" |
		"extend\n" |
		"ascii\n" |
		"alpha\n" |
		"digit\n" |
		"alnum\n" |
		"lower\n" |
		"upper\n" |
		"cntrl\n" |
		"graph\n" |
		"print\n" |
		"punct\n" |
		"space\n" |
		"xdigit\n" |
		"struct Fsm * FactorWithAugNode::Walk()\n" |
		"parsetree.cpp\n" |
		"false\n" |
		"bad_alloc\n" |
		"xx []()\n" |
		" df \n" |
		"StartState: \n" |
		"Final States:\n" |
		"void FsmGraph<State,int,Trans>::AttachStates(State *, State *, Trans *, FsmKeyType, int)\n" |
		"rlfsm/fsmattach.cpp\n" |
		"trans->toState == __null\n" |
		"trans->fromState == __null\n" |
		"void FsmGraph<State,int,Trans>::DetachStates(State *, State *, Trans *, FsmKeyType, int)\n" |
		"trans->toState == to\n" |
		"trans->fromState == from\n" |
		"inTel != __null\n" |
		"void Vector<BstMapEl<int,int>,ResizeExpn>::setAs(const Vector<BstMapEl<int,int>,ResizeExpn> &)\n" |
		"aapl/vectcommon.h\n" |
		"&v != this\n" |
		"void FsmGraph<State,int,Trans>::ChangeRangeLowerKey(Trans *, int, int)\n" |
		"inRangeEl != __null\n" |
		"void FsmGraph<State,int,Trans>::IsolateStartState()\n" |
		"rlfsm/fsmgraph.cpp\n" |
		"md.stateDict.nodeCount == 0\n" |
		"md.stfil.listLength == 0\n" |
		"struct State * FsmGraph<State,int,Trans>::DetachState(State *)\n" |
		"fromTel != __null\n" |
		"struct Trans * FsmGraph<State,int,Trans>::AttachStates(State *, State *, FsmKeyType, int, int)\n" |
		"outTel != __null\n" |
		"outTel1 != __null\n" |
		"from->defOutTrans == __null\n" |
		"void FsmGraph<State,int,Trans>::VerifyOutFuncs()\n" |
		"state->outTransFuncTable.tableLength == 0\n" |
		"!state->isOutPriorSet\n" |
		"state->outPriority == 0\n" |
		"void FsmGraph<State,int,Trans>::VerifyIntegrity()\n" |
		"rlfsm/fsmbase.cpp\n" |
		"outIt.trans->fromState == state\n" |
		"inIt.trans->toState == state\n" |
		"static int FsmTrans<State,Trans,int,CmpOrd<int> >::ComparePartPtr(FsmTrans<State,Trans,int,CmpOrd<int> > *, FsmTrans<State,Trans,int,CmpOrd<int> > *)\n" |
		"rlfsm/fsmstate.cpp\n" |
		"false\n" |
		"void FsmGraph<State,int,Trans>::InTransMove(State *, State *)\n" |
		"dest != src\n" |
		"static bool FsmTrans<State,Trans,int,CmpOrd<int> >::ShouldMarkPtr(MarkIndex<State> &, FsmTrans<State,Trans,int,CmpOrd<int> > *, FsmTrans<State,Trans,int,CmpOrd<int> > *)\n" |
		"bad_alloc\n" |
		"10FsmCodeGen\n" |
		"bad_alloc\n" |
		"	case \n" |
		"break;}\n" |
		"unsigned char\n" |
		"unsigned short\n" |
		"unsigned int\n" |
		"{0, \n" |
		"/* Forward dec state for the transition structure. */\n" |
		"struct \n" |
		"StateStruct;\n" |
		"/* A single transition. */\n" |
		"struct \n" |
		"TransStruct\n" |
		"	struct \n" |
		"StateStruct *toState;\n" |
		"	int *funcs;\n" |
		"typedef struct \n" |
		"TransStruct \n" |
		"Trans;\n" |
		"/* A single state. */\n" |
		"struct \n" |
		"StateStruct\n" |
		"	int lowIndex;\n" |
		"	int highIndex;\n" |
		"	void *transIndex;\n" |
		"	unsigned int dflIndex;\n" |
		"	int *outFuncs;\n" |
		"	int isFinState;\n" |
		"typedef struct \n" |
		"StateStruct \n" |
		"State;\n" |
		"/* Only non-static data: current state. */\n" |
		"struct \n" |
		"Struct\n" |
		"State *curState;\n" |
		"	int accept;\n" |
		"typedef struct \n" |
		"Struct \n" |
		"/* Init the fsm. */\n" |
		"void \n" |
		"Init( \n" |
		" *fsm );\n" |
		"/* Execute some chunk of data. */\n" |
		"void \n" |
		"Execute( \n" |
		" *fsm, char *data, int dlen );\n" |
		"/* Indicate to the fsm tha there is no more data. */\n" |
		"void \n" |
		"Finish( \n" |
		" *fsm );\n" |
		"/* Did the machine accept? */\n" |
		"int \n" |
		"Accept( \n" |
		" *fsm );\n" |
		"#define f \n" |
		"#define s \n" |
		"#define i \n" |
		"#define t \n" |
		"/* The array of functions. */\n" |
		"#if \n" |
		"static int \n" |
		"_f[] = {\n" |
		"#endif\n" |
		"/* The array of indicies into the transition array. */\n" |
		"#if \n" |
		"static \n" |
		"_i[] = {\n" |
		"#endif\n" |
		"/* The aray of states. */\n" |
		"static \n" |
		"State \n" |
		"_s[] = {\n" |
		"/* The array of transitions. */\n" |
		"static \n" |
		"Trans \n" |
		"_t[] = {\n" |
		"/* The start state. */\n" |
		"static \n" |
		"State *\n" |
		"_startState = s+\n" |
		"#undef f\n" |
		"#undef s\n" |
		"#undef i\n" |
		"#undef t\n" |
		"* Execute functions pointed to by funcs until the null function is found. \n" |
		"inline static void \n" |
		"ExecFuncs( \n" |
		" *fsm, int *funcs, char *p )\n" |
		"	int len = *funcs++;\n" |
		"	while ( len-- > 0 ) {\n" |
		"		switch ( *funcs++ ) {\n" |
		" * Init the fsm to a runnable state.\n" |
		"void \n" |
		" *fsm )\n" |
		"	fsm->curState = \n" |
		"_startState;\n" |
		"	fsm->accept = 0;\n" |
		" * Did the fsm accept? \n" |
		"int \n" |
		" *fsm )\n" |
		"	return fsm->accept;\n" |
		" * Execute the fsm on some chunk of data. \n" |
		"void \n" |
		" *fsm, char *data, int dlen )\n" |
		"	char *p = data;\n" |
		"	int len = dlen;\n" |
		"State *cs = fsm->curState;\n" |
		"	for ( ; len > 0; p++, len-- ) {\n" |
		"		int c = (unsigned char) *p;\n" |
		"Trans *trans;\n" |
		"		if ( cs == 0 )\n" |
		"			goto finished;\n" |
		"		/* If the character is within the index bounds then get the\n" |
		"		 * transition for it. If it is out of the transition bounds\n" |
		"		 * we will use the default transition. */\n" |
		"		if ( cs->lowIndex <= c && c < cs->highIndex ) {\n" |
		"			/* Use the index to look into the transition array. */\n" |
		"			trans = \n" |
		"_t + \n" |
		"				((\n" |
		"*)cs->transIndex)[c - cs->lowIndex];\n" |
		"		else {\n" |
		"			/* Use the default index as the char is out of range. */\n" |
		"			trans = \n" |
		"_t + cs->dflIndex;\n" |
		"		/* If there are functions for this transition then execute them. */\n" |
		"		if ( trans->funcs != 0 )\n" |
		"ExecFuncs( fsm, trans->funcs, p );\n" |
		"		/* Move to the new state. */\n" |
		"		cs = trans->toState;\n" |
		"finished:\n" |
		"	fsm->curState = cs;\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		" *fsm )\n" |
		"State *cs = fsm->curState;\n" |
		"	if ( cs != 0 && cs->isFinState ) {\n" |
		"		/* If finishing in a final state then execute the\n" |
		"		 * out functions for it. (if any). */\n" |
		"		if ( cs->outFuncs != 0 )\n" |
		"ExecFuncs( fsm, cs->outFuncs, 0 );\n" |
		"		fsm->accept = 1;\n" |
		"	else {\n" |
		"		/* If we are not in a final state then this\n" |
		"		 * is an error. Move to the error state. */\n" |
		"		fsm->curState = 0;\n" |
		"class \n" |
		"public:\n" |
		"	/* Forward dec state for the transition structure. */\n" |
		"	struct State;\n" |
		"	/* A single transition. */\n" |
		"	struct Trans\n" |
		"		State *toState;\n" |
		"		int *funcs;\n" |
		"	/* A single state. */\n" |
		"	struct State\n" |
		"		int lowIndex;\n" |
		"		int highIndex;\n" |
		"		void *transIndex;\n" |
		"		unsigned int dflIndex;\n" |
		"		int *outFuncs;\n" |
		"		int isFinState;\n" |
		"	/* Constructor. */\n" |
		"	void Init( );\n" |
		"	/* Execute some chunk of data. */\n" |
		"	void Execute( char *data, int dlen );\n" |
		"	/* Indicate to the fsm tha there is no more data. */\n" |
		"	void Finish( );\n" |
		"	/* Did the machine accept? */\n" |
		"	int Accept( );\n" |
		"	State *curState;\n" |
		"	int accept;\n" |
		"	inline void ExecFuncs( int *funcs, char *p );\n" |
		"/* The array of functions. */\n" |
		"#if \n" |
		"::State \n" |
		"/* The array of trainsitions. */\n" |
		"static \n" |
		"::Trans \n" |
		"/* The start state. */\n" |
		"static \n" |
		"::State *\n" |
		" * Execute functions pointed to by funcs until the null function is found. \n" |
		"inline void \n" |
		"::ExecFuncs( int *funcs, char *p )\n" |
		"	int len = *funcs++;\n" |
		"	while ( len-- > 0 ) {\n" |
		"		switch ( *funcs++ ) {\n" |
		" * Constructor\n" |
		"	Init();\n" |
		"Init\n" |
		"void \n" |
		"::Init( )\n" |
		"	curState = \n" |
		"_startState;\n" |
		"	accept = 0;\n" |
		"::Accept( )\n" |
		"	return accept;\n" |
		"::Execute( char *data, int dlen )\n" |
		"	char *p = data;\n" |
		"	int len = dlen;\n" |
		"	State *cs = curState;\n" |
		"	for ( ; len > 0; p++, len-- ) {\n" |
		"		int c = (unsigned char)*p;\n" |
		"		Trans *trans;\n" |
		"		if ( cs == 0 )\n" |
		"			goto finished;\n" |
		"		/* If the character is within the index bounds then get the\n" |
		"		 * transition for it. If it is out of the transition bounds\n" |
		"		 * we will use the default transition. */\n" |
		"		if ( cs->lowIndex <= c && c < cs->highIndex ) {\n" |
		"			/* Use the index to look into the transition array. */\n" |
		"			trans = \n" |
		"_t + cs->dflIndex;\n" |
		"		/* If there are functions for this transition then execute them. */\n" |
		"		if ( trans->funcs != 0 )\n" |
		"			ExecFuncs( trans->funcs, p );\n" |
		"		/* Move to the new state. */\n" |
		"		cs = trans->toState;\n" |
		"finished:\n" |
		"	curState = cs;\n" |
		"::Finish( )\n" |
		"	State *cs = curState;\n" |
		"	if ( cs != 0 && cs->isFinState ) {\n" |
		"		/* If finishing in a final state then execute the\n" |
		"		 * out functions for it. (if any). */\n" |
		"		if ( cs->outFuncs != 0 )\n" |
		"			ExecFuncs( cs->outFuncs, 0 );\n" |
		"		accept = 1;\n" |
		"	else {\n" |
		"		/* If we are not in a final state then this\n" |
		"		 * is an error. Move to the error state. */\n" |
		"		curState = 0;\n" |
		"10TabCodeGen\n" |
		"11CTabCodeGen\n" |
		"12CCTabCodeGen\n" |
		"10FsmCodeGen\n" |
		"bad_alloc\n" |
		"	case \n" |
		"	break;\n" |
		"/* Forward dec state for the transition structure. */\n" |
		"struct \n" |
		"StateStruct;\n" |
		"/* A single transition. */\n" |
		"struct \n" |
		"TransStruct\n" |
		"	struct \n" |
		"StateStruct *toState;\n" |
		"	int funcs;\n" |
		"typedef struct \n" |
		"TransStruct \n" |
		"Trans;\n" |
		"/* A single state. */\n" |
		"struct \n" |
		"StateStruct\n" |
		"	int lowIndex;\n" |
		"	int highIndex;\n" |
		"	void *transIndex;\n" |
		"	int dflIndex;\n" |
		"	int outFuncs;\n" |
		"	int isFinState;\n" |
		"typedef struct \n" |
		"StateStruct \n" |
		"State;\n" |
		"/* Only non-static data: current state. */\n" |
		"struct \n" |
		"Struct\n" |
		"State *curState;\n" |
		"	int accept;\n" |
		"typedef struct \n" |
		"Struct \n" |
		"/* Init the fsm. */\n" |
		"void \n" |
		"Init( \n" |
		" *fsm );\n" |
		"/* Execute some chunk of data. */\n" |
		"void \n" |
		"Execute( \n" |
		" *fsm, char *data, int dlen );\n" |
		"/* Indicate to the fsm tha there is no more data. */\n" |
		"void \n" |
		"Finish( \n" |
		" *fsm );\n" |
		"/* Did the machine accept? */\n" |
		"int \n" |
		"Accept( \n" |
		" *fsm );\n" |
		"#define s \n" |
		"#define i \n" |
		"#define t \n" |
		"/* The array of indicies into the transition array. */\n" |
		"#if \n" |
		"static \n" |
		"_i[] = {\n" |
		"#endif\n" |
		"/* The aray of states. */\n" |
		"static \n" |
		"State \n" |
		"_s[] = {\n" |
		"/* The array of trainsitions. */\n" |
		"static \n" |
		"Trans \n" |
		"_t[] = {\n" |
		"/* The start state. */\n" |
		"static \n" |
		"State *\n" |
		"_startState = s+\n" |
		"#undef f\n" |
		"#undef s\n" |
		"#undef i\n" |
		"#undef t\n" |
		"/***************************************************************************\n" |
		" * Execute functions pointed to by funcs until the null function is found. \n" |
		"inline static void \n" |
		"ExecFuncs( \n" |
		" *fsm, int funcs, char *p )\n" |
		"	switch ( funcs ) {\n" |
		"/****************************************\n" |
		"Init\n" |
		"void \n" |
		" *fsm )\n" |
		"	fsm->curState = \n" |
		"_startState;\n" |
		"	fsm->accept = 0;\n" |
		"/****************************************\n" |
		"Accept\n" |
		" * Did the fsm accept? \n" |
		"int \n" |
		" *fsm )\n" |
		"	return fsm->accept;\n" |
		"/**********************************************************************\n" |
		" * Execute the fsm on some chunk of data. \n" |
		"void \n" |
		" *fsm, char *data, int dlen )\n" |
		"	char *p = data;\n" |
		"	int len = dlen;\n" |
		"State *cs = fsm->curState;\n" |
		"	for ( ; len > 0; p++, len-- ) {\n" |
		"		int c = (unsigned char)*p;\n" |
		"Trans *trans;\n" |
		"		if ( cs == 0 )\n" |
		"			goto finished;\n" |
		"		/* If the character is within the index bounds then get the\n" |
		"		 * transition for it. If it is out of the transition bounds\n" |
		"		 * we will use the default transition. */\n" |
		"		if ( cs->lowIndex <= c && c < cs->highIndex ) {\n" |
		"			/* Use the index to look into the transition array. */\n" |
		"			trans = \n" |
		"_t + \n" |
		"				((\n" |
		"*)cs->transIndex)[c - cs->lowIndex];\n" |
		"		else {\n" |
		"			/* Use the default index as the char is out of range. */\n" |
		"			trans = \n" |
		"_t + cs->dflIndex;\n" |
		"		/* If there are functions for this transition then execute them. */\n" |
		"		if ( trans->funcs >= 0 )\n" |
		"ExecFuncs( fsm, trans->funcs, p );\n" |
		"		/* Move to the new state. */\n" |
		"		cs = trans->toState;\n" |
		"finished:\n" |
		"	fsm->curState = cs;\n" |
		"/**********************************************************************\n" |
		"Finish\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		" *fsm )\n" |
		"State *cs = fsm->curState;\n" |
		"	if ( cs != 0 && cs->isFinState ) {\n" |
		"		/* If finishing in a final state then execute the\n" |
		"		 * out functions for it. (if any). */\n" |
		"		if ( cs->outFuncs != 0 )\n" |
		"ExecFuncs( fsm, cs->outFuncs, 0 );\n" |
		"		fsm->accept = 1;\n" |
		"	else {\n" |
		"		/* If we are not in a final state then this\n" |
		"		 * is an error. Move to the error state. */\n" |
		"		fsm->curState = 0;\n" |
		"class \n" |
		"public:\n" |
		"	/* Function and index type. */\n" |
		"	typedef int Func;\n" |
		"	/* Forward dec state for the transition structure. */\n" |
		"	struct State;\n" |
		"	/* A single transition. */\n" |
		"	struct Trans\n" |
		"		State *toState;\n" |
		"		int funcs;\n" |
		"	/* A single state. */\n" |
		"	struct State\n" |
		"		int lowIndex;\n" |
		"		int highIndex;\n" |
		"		void *transIndex;\n" |
		"		int dflIndex;\n" |
		"		int outFuncs;\n" |
		"		int isFinState;\n" |
		"	/* Constructor. */\n" |
		"	void Init( );\n" |
		"	/* Execute some chunk of data. */\n" |
		"	void Execute( char *data, int dlen );\n" |
		"	/* Indicate to the fsm tha there is no more data. */\n" |
		"	void Finish( );\n" |
		"	/* Did the machine accept? */\n" |
		"	int Accept( );\n" |
		"	State *curState;\n" |
		"	int accept;\n" |
		"	inline void ExecFuncs( int funcs, char *p );\n" |
		"::State \n" |
		"::Trans \n" |
		"::State *\n" |
		"/***************************************************************************\n" |
		" * Execute functions pointed to by funcs until the null function is found. \n" |
		"inline void \n" |
		"::ExecFuncs( int funcs, char *p )\n" |
		"	switch ( funcs ) {\n" |
		"/****************************************\n" |
		" * Constructor\n" |
		"	Init();\n" |
		"/****************************************\n" |
		"::Init( )\n" |
		"	curState = \n" |
		"_startState;\n" |
		"	accept = 0;\n" |
		"/****************************************\n" |
		" * Did the fsm accept? \n" |
		"int \n" |
		"::Accept( )\n" |
		"	return accept;\n" |
		"/**********************************************************************\n" |
		" * Execute the fsm on some chunk of data. \n" |
		"void \n" |
		"::Execute( char *data, int dlen )\n" |
		"	char *p = data;\n" |
		"	int len = dlen;\n" |
		"	State *cs = curState;\n" |
		"	for ( ; len > 0; p++, len-- ) {\n" |
		"		int c = (unsigned char)*p;\n" |
		"		Trans *trans;\n" |
		"		if ( cs == 0 )\n" |
		"			goto finished;\n" |
		"		/* If the character is within the index bounds then get the\n" |
		"		 * transition for it. If it is out of the transition bounds\n" |
		"		 * we will use the default transition. */\n" |
		"		if ( cs->lowIndex <= c && c < cs->highIndex ) {\n" |
		"			/* Use the index to look into the transition array. */\n" |
		"			trans = \n" |
		"_t + cs->dflIndex;\n" |
		"		/* If there are functions for this transition then execute them. */\n" |
		"		if ( trans->funcs != 0 )\n" |
		"			ExecFuncs( trans->funcs, p );\n" |
		"		/* Move to the new state. */\n" |
		"		cs = trans->toState;\n" |
		"finished:\n" |
		"	curState = cs;\n" |
		"/**********************************************************************\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		"::Finish( )\n" |
		"	State *cs = curState;\n" |
		"	if ( cs != 0 && cs->isFinState ) {\n" |
		"		/* If finishing in a final state then execute the\n" |
		"		 * out functions for it. (if any). */\n" |
		"		if ( cs->outFuncs != 0 )\n" |
		"			ExecFuncs( cs->outFuncs, 0 );\n" |
		"		accept = 1;\n" |
		"	else {\n" |
		"		/* If we are not in a final state then this\n" |
		"		 * is an error. Move to the error state. */\n" |
		"		curState = 0;\n" |
		"11FTabCodeGen\n" |
		"12CFTabCodeGen\n" |
		"13CCFTabCodeGen\n" |
		"bad_alloc\n" |
		"cs = -1; \n" |
		"cs = \n" |
		"break;\n" |
		"		switch( cs ) {\n" |
		"		case \n" |
		"			switch ( c ) {\n" |
		"case \n" |
		"default: \n" |
		"			}\n" |
		"			break;\n" |
		"	switch( cs ) {\n" |
		"accept = 1; \n" |
		"/* Only non-static data: current state. */\n" |
		"struct \n" |
		"Struct\n" |
		"	int curState;\n" |
		"	int accept;\n" |
		"typedef struct \n" |
		"Struct \n" |
		"/* Init the fsm. */\n" |
		"void \n" |
		"Init( \n" |
		" *fsm );\n" |
		"/* Execute some chunk of data. */\n" |
		"void \n" |
		"Execute( \n" |
		" *fsm, char *data, int dlen );\n" |
		"/* Indicate to the fsm tha there is no more data. */\n" |
		"void \n" |
		"Finish( \n" |
		" *fsm );\n" |
		"/* Did the machine accept? */\n" |
		"int \n" |
		"Accept( \n" |
		" *fsm );\n" |
		"/* The start state. */\n" |
		"static int \n" |
		"_startState = \n" |
		"/****************************************\n" |
		"Init\n" |
		"void \n" |
		" *fsm )\n" |
		"	fsm->curState = \n" |
		"_startState;\n" |
		"	fsm->accept = 0;\n" |
		"/**********************************************************************\n" |
		" * Execute the fsm on some chunk of data. \n" |
		"void \n" |
		" *fsm, char *data, int dlen )\n" |
		"	char *p = data;\n" |
		"	int len = dlen;\n" |
		"	int cs = fsm->curState;\n" |
		"	for ( ; len > 0; p++, len-- ) {\n" |
		"		unsigned char c = (unsigned char)*p;\n" |
		"	fsm->curState = cs;\n" |
		"/**********************************************************************\n" |
		"Finish\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		" *fsm )\n" |
		"	int cs = fsm->curState;\n" |
		"	int accept = 0;\n" |
		"	fsm->accept = accept;\n" |
		"/*******************************************************\n" |
		"Accept\n" |
		" * Did the machine accept?\n" |
		"int \n" |
		" *fsm )\n" |
		"	return fsm->accept;\n" |
		"/* Only non-static data: current state. */\n" |
		"class \n" |
		"public:\n" |
		"	/* Init the fsm. */\n" |
		"	void Init( );\n" |
		"	/* Execute some chunk of data. */\n" |
		"	void Execute( char *data, int dlen );\n" |
		"	/* Indicate to the fsm tha there is no more data. */\n" |
		"	void Finish( );\n" |
		"	/* Did the machine accept? */\n" |
		"	int Accept( );\n" |
		"	int curState;\n" |
		"	int accept;\n" |
		"	/* The start state. */\n" |
		"	static int startState;\n" |
		"/* The start state. */\n" |
		"int \n" |
		"::startState = \n" |
		"	Init();\n" |
		"/****************************************\n" |
		"::Init\n" |
		"void \n" |
		"::Init( )\n" |
		"	curState = startState;\n" |
		"	accept = 0;\n" |
		"::Execute( char *data, int dlen )\n" |
		"	char *p = data;\n" |
		"	int len = dlen;\n" |
		"	int cs = curState;\n" |
		"	for ( ; len > 0; p++, len-- ) {\n" |
		"		unsigned char c = (unsigned char)*p;\n" |
		"	curState = cs;\n" |
		"/**********************************************************************\n" |
		"::Finish\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		"::Finish( )\n" |
		"	int cs = curState;\n" |
		"	int accept = 0;\n" |
		"	this->accept = accept;\n" |
		"/*******************************************************\n" |
		"::Accept\n" |
		" * Did the machine accept?\n" |
		"int \n" |
		"::Accept( )\n" |
		"	return accept;\n" |
		"10SelCodeGen\n" |
		"11CSelCodeGen\n" |
		"12CCSelCodeGen\n" |
		"10FsmCodeGen\n" |
		"bad_alloc\n" |
		"goto tr\n" |
		"goto st\n" |
		"goto err;\n" |
		"	case \n" |
		"break;}\n" |
		": goto st\n" |
		"		case \n" |
		"		default: return;\n" |
		"	goto st\n" |
		"	if ( --len == 0 )\n" |
		"		goto out\n" |
		"	switch( (alph) *++p ) {\n" |
		"case \n" |
		"		default: \n" |
		"	return;\n" |
		"curState = \n" |
		"	switch( cs ) {\n" |
		"accept = 1; \n" |
		"break;\n" |
		"err:\n" |
		"curState = -1;\n" |
		", p );\n" |
		"ExecFuncs( fsm, f+\n" |
		"fsm->\n" |
		"/* Only non-static data: current state. */\n" |
		"struct \n" |
		"Struct\n" |
		"	int curState;\n" |
		"	int accept;\n" |
		"typedef struct \n" |
		"Struct \n" |
		"/* Init the fsm. */\n" |
		"void \n" |
		"Init( \n" |
		" *fsm );\n" |
		"/* Execute some chunk of data. */\n" |
		"void \n" |
		"Execute( \n" |
		" *fsm, char *data, int dlen );\n" |
		"/* Indicate to the fsm tha there is no more data. */\n" |
		"void \n" |
		"Finish( \n" |
		" *fsm );\n" |
		"/* Did the machine accept? */\n" |
		"int \n" |
		"Accept( \n" |
		" *fsm );\n" |
		"/* The start state. */\n" |
		"static int \n" |
		"_startState = \n" |
		"#define f \n" |
		"#define alph unsigned char\n" |
		"/* The array of functions. */\n" |
		"#if \n" |
		"static int \n" |
		"_f[] = {\n" |
		"#endif\n" |
		"/****************************************\n" |
		"Init\n" |
		"void \n" |
		" *fsm )\n" |
		"	fsm->curState = \n" |
		"_startState;\n" |
		"	fsm->accept = 0;\n" |
		"/***************************************************************************\n" |
		" * Function exection. We do not inline this as in tab\n" |
		" * code gen because if we did, we might as well just expand \n" |
		" * the function as in the faster goto code generator.\n" |
		"static void \n" |
		"ExecFuncs( \n" |
		" *fsm, int *funcs, char *p )\n" |
		"	int len = *funcs++;\n" |
		"	while ( len-- > 0 ) {\n" |
		"		switch ( *funcs++ ) {\n" |
		"/**********************************************************************\n" |
		" * Execute the fsm on some chunk of data. \n" |
		"void \n" |
		" *fsm, char *data, int dlen )\n" |
		"	/* Prime these to one back to simulate entering the \n" |
		"	 * machine on a transition. */ \n" |
		"	register char *p = data - 1;\n" |
		"	register int len = dlen + 1;\n" |
		"	/* Switch statment to enter the machine. */\n" |
		"	switch ( \n" |
		"curState ) {\n" |
		"/**********************************************************************\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		" *fsm )\n" |
		"	int cs = fsm->curState;\n" |
		"	int accept = 0;\n" |
		"	fsm->accept = accept;\n" |
		"/*******************************************************\n" |
		" * Did the machine accept?\n" |
		"int \n" |
		" *fsm )\n" |
		"	return fsm->accept;\n" |
		"#undef f\n" |
		"#undef alph\n" |
		"	ExecFuncs( f+\n" |
		"/* Only non-static data: current state. */\n" |
		"class \n" |
		"public:\n" |
		"	/* Init the fsm. */\n" |
		"	void Init( );\n" |
		"	/* Execute some chunk of data. */\n" |
		"	void Execute( char *data, int dlen );\n" |
		"	/* Indicate to the fsm tha there is no more data. */\n" |
		"	void Finish( );\n" |
		"	/* Did the machine accept? */\n" |
		"	int Accept( );\n" |
		"	int curState;\n" |
		"	int accept;\n" |
		"	/* The start state. */\n" |
		"	static int startState;\n" |
		"	/* Function exection. We do not inline this as in tab code gen\n" |
		"	 * because if we did, we might as well just expand the function \n" |
		"	 * as in the faster goto code generator. */\n" |
		"	void ExecFuncs( int *funcs, char * );\n" |
		"/* The start state. */\n" |
		"int \n" |
		"::startState = \n" |
		"/* some defines to lessen the code size. */\n" |
		"#define f \n" |
		"#endif\n" |
		"/****************************************\n" |
		" * Make sure the fsm is initted.\n" |
		"	Init();\n" |
		"/****************************************\n" |
		" * Initialize the fsm.\n" |
		"void \n" |
		"::Init( )\n" |
		"	curState = startState;\n" |
		"	accept = 0;\n" |
		"/***************************************************************************\n" |
		" * Execute functions pointed to by funcs until the null function is found. \n" |
		"void \n" |
		"::ExecFuncs( int *funcs, char *p )\n" |
		"	int len = *funcs++;\n" |
		"	while ( len-- > 0 ) {\n" |
		"		switch ( *funcs++ ) {\n" |
		"/**********************************************************************\n" |
		" * Execute the fsm on some chunk of data. \n" |
		"void \n" |
		"::Execute( char *data, int dlen )\n" |
		"	/* Prime these to one back to simulate entering the \n" |
		"	 * machine on a transition. */ \n" |
		"	register char *p = data - 1;\n" |
		"	register int len = dlen + 1;\n" |
		"	/* Switch statment to enter the machine. */\n" |
		"	switch ( curState ) {\n" |
		"/**********************************************************************\n" |
		"::Finish\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		"::Finish( )\n" |
		"	int cs = curState;\n" |
		"	int accept = 0;\n" |
		"	this->accept = accept;\n" |
		"/*******************************************************\n" |
		"::Accept\n" |
		" * Did the machine accept?\n" |
		"int \n" |
		"::Accept( )\n" |
		"	return accept;\n" |
		"#undef f\n" |
		"#undef alph\n" |
		"11GotoCodeGen\n" |
		"12CGotoCodeGen\n" |
		"13CCGotoCodeGen\n" |
		"10FsmCodeGen\n" |
		"bad_alloc\n" |
		"	case \n" |
		"	break;\n" |
		", p );\n" |
		"ExecFuncs( fsm, \n" |
		"fsm->\n" |
		"/* Only non-static data: current state. */\n" |
		"struct \n" |
		"Struct\n" |
		"	int curState;\n" |
		"	int accept;\n" |
		"typedef struct \n" |
		"Struct \n" |
		"/* Init the fsm. */\n" |
		"void \n" |
		"Init( \n" |
		" *fsm );\n" |
		"/* Execute some chunk of data. */\n" |
		"void \n" |
		"Execute( \n" |
		" *fsm, char *data, int dlen );\n" |
		"/* Indicate to the fsm tha there is no more data. */\n" |
		"void \n" |
		"Finish( \n" |
		" *fsm );\n" |
		"/* Did the machine accept? */\n" |
		"int \n" |
		"Accept( \n" |
		" *fsm );\n" |
		"/* The start state. */\n" |
		"static int \n" |
		"_startState = \n" |
		"/****************************************\n" |
		"Init\n" |
		"void \n" |
		" *fsm )\n" |
		"	fsm->curState = \n" |
		"_startState;\n" |
		"	fsm->accept = 0;\n" |
		"/***************************************************************************\n" |
		" * Function exection. We do not inline this as in tab\n" |
		" * code gen because if we did, we might as well just expand \n" |
		" * the function as in the faster goto code generator.\n" |
		"static void \n" |
		"ExecFuncs( \n" |
		" *fsm, int func, char *p )\n" |
		"	switch ( func ) {\n" |
		"#define alph unsigned char\n" |
		"/**********************************************************************\n" |
		" * Execute the fsm on some chunk of data. \n" |
		"void \n" |
		" *fsm, char *data, int dlen )\n" |
		"	/* Prime these to one back to simulate entering the \n" |
		"	 * machine on a transition. */ \n" |
		"	register char *p = data-1;\n" |
		"	register int len = dlen+1;\n" |
		"	/* Switch statment to enter the machine. */\n" |
		"	switch ( \n" |
		"curState ) {\n" |
		"/**********************************************************************\n" |
		"Finish\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		" *fsm )\n" |
		"	int cs = fsm->curState;\n" |
		"	int accept = 0;\n" |
		"	fsm->accept = accept;\n" |
		"/*******************************************************\n" |
		"Accept\n" |
		" * Did the machine accept?\n" |
		"int \n" |
		" *fsm )\n" |
		"	return fsm->accept;\n" |
		"#undef alph\n" |
		"	ExecFuncs( \n" |
		"/* Only non-static data: current state. */\n" |
		"class \n" |
		"public:\n" |
		"	/* Init the fsm. */\n" |
		"	void Init( );\n" |
		"	/* Execute some chunk of data. */\n" |
		"	void Execute( char *data, int dlen );\n" |
		"	/* Indicate to the fsm tha there is no more data. */\n" |
		"	void Finish( );\n" |
		"	/* Did the machine accept? */\n" |
		"	int Accept( );\n" |
		"	int curState;\n" |
		"	int accept;\n" |
		"	/* The start state. */\n" |
		"	static int startState;\n" |
		"	/* Function exection. We do not inline this as in tab code gen\n" |
		"	 * because if we did, we might as well just expand the function \n" |
		"	 * as in the faster goto code generator. */\n" |
		"	void ExecFuncs( int func, char *p );\n" |
		"/* The start state. */\n" |
		"int \n" |
		"::startState = \n" |
		"	Init();\n" |
		"/****************************************\n" |
		"::Init\n" |
		"void \n" |
		"::Init( )\n" |
		"	curState = startState;\n" |
		"	accept = 0;\n" |
		"/***************************************************************************\n" |
		" * Execute functions pointed to by funcs until the null function is found. \n" |
		"void \n" |
		"::ExecFuncs( int func, char *p )\n" |
		"	switch ( func ) {\n" |
		"::Execute( char *data, int dlen )\n" |
		"	/* Prime these to one back to simulate entering the \n" |
		"	 * machine on a transition. */ \n" |
		"	register char *p = data-1;\n" |
		"	register int len = dlen+1;\n" |
		"	/* Switch statment to enter the machine. */\n" |
		"	switch ( curState ) {\n" |
		"::Finish\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		"::Finish( )\n" |
		"	int cs = curState;\n" |
		"	int accept = 0;\n" |
		"	this->accept = accept;\n" |
		"/*******************************************************\n" |
		"::Accept\n" |
		" * Did the machine accept?\n" |
		"int \n" |
		"::Accept( )\n" |
		"	return accept;\n" |
		"#undef alph\n" |
		"12FGotoCodeGen\n" |
		"13CFGotoCodeGen\n" |
		"14CCFGotoCodeGen\n" |
		"11GotoCodeGen\n" |
		"10FsmCodeGen\n" |
		"bad_alloc\n" |
		"fsm->\n" |
		"/* Only non-static data: current state. */\n" |
		"struct \n" |
		"Struct\n" |
		"	int curState;\n" |
		"	int accept;\n" |
		"typedef struct \n" |
		"Struct \n" |
		"/* Init the fsm. */\n" |
		"void \n" |
		"Init( \n" |
		" *fsm );\n" |
		"/* Execute some chunk of data. */\n" |
		"void \n" |
		"Execute( \n" |
		" *fsm, char *data, int dlen );\n" |
		"/* Indicate to the fsm tha there is no more data. */\n" |
		"void \n" |
		"Finish( \n" |
		" *fsm );\n" |
		"/* Did the machine accept? */\n" |
		"int \n" |
		"Accept( \n" |
		" *fsm );\n" |
		"/* The start state. */\n" |
		"static int \n" |
		"_startState = \n" |
		"/****************************************\n" |
		"Init\n" |
		"void \n" |
		" *fsm )\n" |
		"	fsm->curState = \n" |
		"_startState;\n" |
		"	fsm->accept = 0;\n" |
		"#define alph unsigned char\n" |
		"/**********************************************************************\n" |
		" * Execute the fsm on some chunk of data. \n" |
		"void \n" |
		" *fsm, char *data, int dlen )\n" |
		"	/* Prime these to one back to simulate entering the \n" |
		"	 * machine on a transition. */ \n" |
		"	register char *p = data-1;\n" |
		"	register int len = dlen+1;\n" |
		"	/* Switch statment to enter the machine. */\n" |
		"	switch ( \n" |
		"curState ) {\n" |
		"/**********************************************************************\n" |
		"Finish\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		" *fsm )\n" |
		"	int cs = fsm->curState;\n" |
		"	int accept = 0;\n" |
		"	fsm->accept = accept;\n" |
		"/*******************************************************\n" |
		"Accept\n" |
		" * Did the machine accept?\n" |
		"int \n" |
		" *fsm )\n" |
		"	return fsm->accept;\n" |
		"#undef alph\n" |
		"/* Only non-static data: current state. */\n" |
		"class \n" |
		"public:\n" |
		"	/* Init the fsm. */\n" |
		"	void Init( );\n" |
		"	/* Execute some chunk of data. */\n" |
		"	void Execute( char *data, int dlen );\n" |
		"	/* Indicate to the fsm tha there is no more data. */\n" |
		"	void Finish( );\n" |
		"	/* Did the machine accept? */\n" |
		"	int Accept( );\n" |
		"	int curState;\n" |
		"	int accept;\n" |
		"	/* The start state. */\n" |
		"	static int startState;\n" |
		"/* The start state. */\n" |
		"int \n" |
		"::startState = \n" |
		"	Init();\n" |
		"/****************************************\n" |
		"::Init\n" |
		"void \n" |
		"::Init( )\n" |
		"	curState = startState;\n" |
		"	accept = 0;\n" |
		"#define alph unsigned char\n" |
		"/**********************************************************************\n" |
		" * Execute the fsm on some chunk of data. \n" |
		"void \n" |
		"::Execute( char *data, int dlen )\n" |
		"	/* Prime these to one back to simulate entering the \n" |
		"	 * machine on a transition. */ \n" |
		"	register char *p = data-1;\n" |
		"	register int len = dlen+1;\n" |
		"	/* Switch statment to enter the machine. */\n" |
		"	switch ( curState ) {\n" |
		"::Finish\n" |
		" * Indicate to the fsm that the input is done. Does cleanup tasks.\n" |
		"void \n" |
		"::Finish( )\n" |
		"	int cs = curState;\n" |
		"	int accept = 0;\n" |
		"	this->accept = accept;\n" |
		"/*******************************************************\n" |
		"::Accept\n" |
		" * Did the machine accept?\n" |
		"int \n" |
		"::Accept( )\n" |
		"	return accept;\n" |
		"#undef alph\n" |
		"13IpGotoCodeGen\n" |
		"14CIpGotoCodeGen\n" |
		"15CCIpGotoCodeGen\n" |
		"11GotoCodeGen\n" |
		"10FsmCodeGen\n";
}%%

%% write data;
struct strs the_fsm;

void test( char *buf )
{
	struct strs *fsm = &the_fsm;
	char *p = buf;
	char *pe = buf + strlen( buf );

	%% write init;
	%% write exec;

	if ( fsm->cs >= strs_first_final )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}


int main()
{
	test( "stdin\n" );
	test( "bad_alloc\n" );
	test( "_GLOBAL_OFFSET_TABLE_\n" );
	test( "not in\n" );
	test(
		"isatty\n"
		"junk on end.\n"
	);

	return 0;
}

#ifdef _____OUTPUT_____
ACCEPT
ACCEPT
ACCEPT
FAIL
FAIL
#endif
