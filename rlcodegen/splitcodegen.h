/*
 *  Copyright 2006 Adrian Thurston <thurston@cs.queensu.ca>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _SPLITCODEGEN_H
#define _SPLITCODEGEN_H

#include "ipgotocodegen.h"

class SplitCodeGen : public IpGotoCodeGen
{
public:
	bool ptOutLabelUsed;

	std::ostream &PART_MAP();
	std::ostream &EXIT_STATES( int partition );
	std::ostream &PART_TRANS( int partition );
	std::ostream &TRANS_GOTO( RedTransAp *trans, int level );
	void GOTO_HEADER( RedStateAp *state, bool stateInPartition );
	std::ostream &STATE_GOTOS( int partition );
	std::ostream &PARTITION( int partition );
	std::ostream &ALL_PARTITIONS();
	void writeOutData();
	void writeOutExec();
	void writeOutParts();

	void setLabelsNeeded( RedStateAp *fromState, InlineList *inlineList );
	void setLabelsNeeded( RedStateAp *fromState, RedTransAp *trans );
	void setLabelsNeeded();

	int currentPartition;
};

struct CSplitCodeGen
	: public SplitCodeGen, public CCodeGen
{
};

/*
 * class DIpGotoCodeGen
 */
struct DSplitCodeGen
	: public IpGotoCodeGen, public DCodeGen
{
};


#endif /* _SPLITCODEGEN_H */
