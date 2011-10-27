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
		"inline void appendString( PrintArgs *args, const char *data, int length )\n"
		"{\n"
		"	std::string *str = (std::string*)args->arg;\n"
		"	*str += std::string( data, length );\n"
		"}\n"
		"\n";

	out << 
		"inline std::string printTreeStr( Program *prg, Tree *tree )\n"
		"{\n"
		"	std::string str;\n"
		"	PrintArgs printArgs = { &str, 1, 0, &appendString, \n"
		"			&printNull, &printTermTree, &printNull };\n"
		"	printTreeArgs( &printArgs, prg->vm_root, prg, tree );\n"
		"	return str;\n"
		"}\n"
		"\n";

	/* Declare. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ )
		out << "struct " << lel->fullName << ";\n";

	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		out << "struct " << lel->fullName << "\n";
		out << "{\n";
		out << "	Head *data() { return ((Tree*)this)->tokdata; }\n";
		out << "	std::string text( Program *prg ) { return printTreeStr( prg, (Tree*)this ); }\n";

		if ( lel->objectDef != 0 && lel->objectDef->objFieldList != 0 ) {
			ObjFieldList *objFieldList = lel->objectDef->objFieldList;
			for ( ObjFieldList::Iter ofi = *objFieldList; ofi.lte(); ofi++ ) {
				ObjField *field = ofi->value;
				if ( field->useOffset && field->typeRef != 0  ) {
					UniqueType *ut = field->typeRef->lookupType( this );

					if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
						out << "	" << ut->langEl->fullName << " *" << field->name << 
							"() { return (" << ut->langEl->fullName << 
							"*)getAttr( (Tree*)this, " << field->offset << "); }\n";
					}
				}

				if ( field->isRhsGet ) {
					UniqueType *ut = field->typeRef->lookupType( this );

					if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
						out << "	" << ut->langEl->fullName << " *" << field->name << 
							"(Program *prg) { static int a[] = {"; 

						/* Need to place the array computing the val. */
						out << field->rhsVal.length();
						for ( Vector<RhsVal>::Iter rg = field->rhsVal; rg.lte(); rg++ ) {
							out << ", " << rg->prodNum;
							out << ", " << rg->childNum;
						}

						out << "}; return (" << ut->langEl->fullName << 
							"*)getRhsVal( prg, (Tree*)this, a ); }\n";
					}
				}
			}
		}

		if ( lel->isRepeat ) {
			out << "	" << lel->fullName << " *next"
				"() { return (" << lel->fullName << 
				"*)getRepeatNext( (Tree*)this ); }\n";

			out << "	" <<
				"int end() { return repeatEnd( (Tree*)this ); }\n";

			out << "	" << lel->repeatOf->fullName << " *value"
				"() { return (" << lel->repeatOf->fullName << 
				"*)getRepeatVal( (Tree*)this ); }\n";
		}

		if ( lel->isList ) {
			out << "	" << lel->fullName << " *next"
				"() { return (" << lel->fullName << 
				"*)getRepeatNext( (Tree*)this ); }\n";

			out << "	" <<
				"int last() { return listLast( (Tree*)this ); }\n";

			out << "	" << lel->repeatOf->fullName << " *value"
				"() { return (" << lel->repeatOf->fullName << 
				"*)getRepeatVal( (Tree*)this ); }\n";
		}
		out << "};\n";
	}
	out << "#endif\n";
}

void FsmCodeGen::writeMain()
{
	out << 
		"int main( int argc, const char **argv )\n"
		"{\n"
		"	Program program;\n"
		"	initColm( 0 );\n"
		"	initProgram( &program, argc, argv, 1, &main_runtimeData );\n"
		"	runProgram( &program );\n"
		"	clearProgram( &program );\n"
		"	return 0;\n"
		"}\n"
		"\n";

	out.flush();
}

