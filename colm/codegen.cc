/*
 *  Copyright 2006-2011 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "parsedata.h"
#include "fsmcodegen.h"
#include "redfsm.h"
#include "bstmap.h"
#include "fsmrun.h"
#include <sstream>
#include <string>
#include <assert.h>

using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;

void ParseData::generateExports()
{
	ostream &out = *outStream;

	out << 
		"#ifndef _EXPORTS_H\n"
		"#define _EXPORTS_H\n"
		"\n"
		"#include <colm/colm.h>\n"
		"#include <string>\n"
		"\n";
	
	out << 
		"inline void appendString( ColmPrintArgs *args, const char *data, int length )\n"
		"{\n"
		"	std::string *str = (std::string*)args->arg;\n"
		"	*str += std::string( data, length );\n"
		"}\n"
		"\n";

	out << 
		"inline std::string printTreeStr( ColmProgram *prg, ColmTree *tree )\n"
		"{\n"
		"	std::string str;\n"
		"	ColmPrintArgs printArgs = { &str, 1, 0, &appendString, \n"
		"			&printNull, &printTermTree, &printNull };\n"
		"	printTreeArgs( prg, vm_root(prg), &printArgs, tree );\n"
		"	return str;\n"
		"}\n"
		"\n";

	/* Declare. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ )
		out << "struct " << lel->fullName << ";\n";

	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		out << "struct " << lel->fullName << "\n";
		out << "{\n";
		out << "	std::string text() { return printTreeStr( prg, tree ); }\n";
		out << "	operator ColmTree *() { return tree; }\n";
		out << "	ColmProgram *prg;\n";
		out << "	ColmTree *tree;\n";

		if ( mainReturnUT != 0 && mainReturnUT->langEl == lel ) {
			out << "	" << lel->fullName << "( ColmProgram *prg ) : prg(prg), tree(returnVal(prg)) {}\n";
		}
		out << "	" << lel->fullName << "( ColmProgram *prg, ColmTree *tree ) : prg(prg), tree(tree) {}\n";

		if ( lel->objectDef != 0 && lel->objectDef->objFieldList != 0 ) {
			ObjFieldList *objFieldList = lel->objectDef->objFieldList;
			for ( ObjFieldList::Iter ofi = *objFieldList; ofi.lte(); ofi++ ) {
				ObjField *field = ofi->value;
				if ( field->useOffset && field->typeRef != 0  ) {
					UniqueType *ut = field->typeRef->lookupType( this );

					if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
						out << "	" << ut->langEl->fullName << " " << field->name << "();\n";
					}
				}

				if ( field->isRhsGet ) {
					UniqueType *ut = field->typeRef->lookupType( this );

					if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
						out << "	" << ut->langEl->fullName << " " << field->name << "();\n";
					}
				}
			}
		}

		if ( lel->isRepeat ) {
			out << "	" << "int end() { return repeatEnd( tree ); }\n";
			out << "	" << lel->fullName << " next();\n";
			out << "	" << lel->repeatOf->fullName << " value();\n";
		}

		if ( lel->isList ) {
			out << "	" << "int last() { return listLast( tree ); }\n";
			out << "	" << lel->fullName << " next();\n";
			out << "	" << lel->repeatOf->fullName << " value();\n";
		}
		out << "};\n";
	}

	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->objectDef != 0 && lel->objectDef->objFieldList != 0 ) {
			ObjFieldList *objFieldList = lel->objectDef->objFieldList;
			for ( ObjFieldList::Iter ofi = *objFieldList; ofi.lte(); ofi++ ) {
				ObjField *field = ofi->value;
				if ( field->useOffset && field->typeRef != 0  ) {
					UniqueType *ut = field->typeRef->lookupType( this );

					if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
						out << ut->langEl->fullName << " " << lel->fullName << "::" << field->name << 
							"() { return " << ut->langEl->fullName << 
							"( prg, getAttr( tree, " << field->offset << ") ); }\n";
					}
				}

				if ( field->isRhsGet ) {
					UniqueType *ut = field->typeRef->lookupType( this );

					if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
						out << ut->langEl->fullName << " " << lel->fullName << "::" << field->name << 
							"() { static int a[] = {"; 

						/* Need to place the array computing the val. */
						out << field->rhsVal.length();
						for ( Vector<RhsVal>::Iter rg = field->rhsVal; rg.lte(); rg++ ) {
							out << ", " << rg->prodNum;
							out << ", " << rg->childNum;
						}

						out << "}; return " << ut->langEl->fullName << 
							"( prg, getRhsVal( prg, tree, a ) ); }\n";
					}
				}
			}
		}

		if ( lel->isRepeat ) {
			out << lel->fullName << " " << lel->fullName << "::" << " next"
				"() { return " << lel->fullName << 
				"( prg, getRepeatNext( tree ) ); }\n";

			out << lel->repeatOf->fullName << " " << lel->fullName << "::" << " value"
				"() { return " << lel->repeatOf->fullName << 
				"( prg, getRepeatVal( tree ) ); }\n";

		}

		if ( lel->isList ) {
			out << lel->fullName << " " << lel->fullName << "::" << " next"
				"() { return " << lel->fullName << 
				"( prg, getRepeatNext( tree ) ); }\n";

			out << lel->repeatOf->fullName << " " << lel->fullName << "::" << " value"
				"() { return " << lel->repeatOf->fullName << 
				"( prg, getRepeatVal( tree ) ); }\n";
		}
	}
	out << "#endif\n";
}

void FsmCodeGen::writeMain()
{
	out << 
		"int main( int argc, const char **argv )\n"
		"{\n"
		"	struct ColmProgram *prg;\n"
		"	colmInit( 0 );\n"
		"	prg = colmNewProgram( &main_runtimeData, argc, argv );\n"
		"	colmRunProgram( prg );\n"
		"	colmDeleteProgram( prg );\n"
		"	return 0;\n"
		"}\n"
		"\n";

	out.flush();
}

