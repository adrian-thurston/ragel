#ifndef RAGEL_BINVAR_H
#define RAGEL_BINVAR_H

#include "binary.h"

struct BinaryVar
:
	public Binary
{
	BinaryVar( const CodeGenArgs &args )
	:
		Binary( args )
	{}

	void LOCATE_TRANS();
	void LOCATE_COND();

	void GOTO( ostream &ret, int gotoDest, bool inFinish );
	void GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish );
	void CALL( ostream &ret, int callDest, int targState, bool inFinish );
	void NCALL( ostream &ret, int callDest, int targState, bool inFinish );
	void CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish );
	void RET( ostream &ret, bool inFinish );
	void BREAK( ostream &ret, int targState, bool csForced );
	void NRET( ostream &ret, bool inFinish );
	void NBREAK( ostream &ret, int targState, bool csForced );

	void NFA_POP();
};


#endif
