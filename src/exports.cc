/*
 * Copyright 2006-2012 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdbool.h>

#include <iostream>

#include "fsmcodegen.h"

using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;

void Compiler::openNameSpace( ostream &out, Namespace *nspace )
{
	if ( nspace == rootNamespace )
		return;
	
	openNameSpace( out, nspace->parentNamespace );
	out << "namespace " << nspace->name << " { ";
}

void Compiler::closeNameSpace( ostream &out, Namespace *nspace )
{
	if ( nspace == rootNamespace )
		return;
	
	openNameSpace( out, nspace->parentNamespace );
	out << " }";
}

void Compiler::generateExports()
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
		"inline void appendString( colm_print_args *args, const char *data, int length )\n"
		"{\n"
		"	std::string *str = (std::string*)args->arg;\n"
		"	*str += std::string( data, length );\n"
		"}\n"
		"\n";

	out << 
		"inline std::string printTreeStr( colm_program *prg, colm_tree *tree, bool trim )\n"
		"{\n"
		"	std::string str;\n"
		"	colm_print_args printArgs = { &str, 1, 0, trim, &appendString, \n"
		"			&colm_print_null, &colm_print_term_tree, &colm_print_null };\n"
		"	colm_print_tree_args( prg, colm_vm_root(prg), &printArgs, tree );\n"
		"	return str;\n"
		"}\n"
		"\n";

	/* Declare. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->isEOF )
			continue;

		openNameSpace( out, lel->nspace );
		out << "struct " << lel->fullName << ";";
		closeNameSpace( out, lel->nspace );
		out << "\n";
	}

	/* Class definitions. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->isEOF )
			continue;

		openNameSpace( out, lel->nspace );
		out << "struct " << lel->fullName << "\n";
		out << "{\n";
		out << "	std::string text() { return printTreeStr( __prg, __tree, true ); }\n";
		out << "	colm_location *loc() { return colm_find_location( __prg, __tree ); }\n";
		out << "	std::string text_notrim() { return printTreeStr( __prg, __tree, false ); }\n";
		out << "	std::string text_ws() { return printTreeStr( __prg, __tree, false ); }\n";
		out << "	colm_data *data() { return __tree->tokdata; }\n";
		out << "	operator colm_tree *() { return __tree; }\n";
		out << "	colm_program *__prg;\n";
		out << "	colm_tree *__tree;\n";

		if ( mainReturnUT != 0 && mainReturnUT->langEl == lel ) {
			out << "	" << lel->fullName <<
					"( colm_program *prg ) : __prg(prg), __tree(returnVal(prg)) {}\n";
		}

		out << "	" << lel->fullName <<
				"( colm_program *prg, colm_tree *tree ) : __prg(prg), __tree(tree) {}\n";

		if ( lel->objectDef != 0 ) {
			FieldList &fieldList = lel->objectDef->fieldList;
			for ( FieldList::Iter ofi = fieldList; ofi.lte(); ofi++ ) {
				ObjectField *field = ofi->value;
				if ( ( field->useOffset() && field->typeRef != 0 ) || field->isRhsGet() ) {
					UniqueType *ut = field->typeRef->resolveType( this );

					if ( ut != 0 && ut->typeId == TYPE_TREE  )
						out << "	" << ut->langEl->refName << " " << field->name << "();\n";
				}
			}
		}

		bool prodNames = false;
		for ( LelDefList::Iter prod = lel->defList; prod.lte(); prod++ ) {
			if ( prod->name.length() > 0 )
				prodNames = true;
		}

		if ( prodNames ) {
			out << "	enum prod_name {\n";
			for ( LelDefList::Iter prod = lel->defList; prod.lte(); prod++ ) {
				if ( prod->name.length() > 0 )
					out << "\t\t" << prod->name << " = " << prod->prodNum << ",\n";
			}
			out << "	};\n";
			out << "	enum prod_name prodName() " <<
					"{ return (enum prod_name)__tree->prod_num; }\n";
		}
		

		if ( lel->isRepeat ) {
			out << "	" << "int end() { return colm_repeat_end( __tree ); }\n";
			out << "	" << lel->refName << " next();\n";
			out << "	" << lel->repeatOf->refName << " value();\n";
		}

		if ( lel->isList ) {
			out << "	" << "int last() { return colm_list_last( __tree ); }\n";
			out << "	" << lel->refName << " next();\n";
			out << "	" << lel->repeatOf->refName << " value();\n";
		}


		out << "};";
		closeNameSpace( out, lel->nspace );
		out << "\n";
	}

	for ( FieldList::Iter of = globalObjectDef->fieldList; of.lte(); of++ ) {
		ObjectField *field = of->value;
		if ( field->isExport ) {
			UniqueType *ut = field->typeRef->resolveType(this);
			if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
				out << ut->langEl->refName << " " << field->name << "( colm_program *prg );\n";
			}
		}
	}
	
	out << "\n";

	for ( FunctionList::Iter func = functionList; func.lte(); func++ ) {
		if ( func->exprt ) {
			char *refName = func->typeRef->uniqueType->langEl->refName;
			int paramCount = func->paramList->length();
			out <<
				refName << " " << func->name << "( colm_program *prg";

			for ( int p = 0; p < paramCount; p++ )
				out << ", const char *p" << p;

			out << " );\n";
		}
	}

	out << "#endif\n";
}

void Compiler::generateExportsImpl()
{
	ostream &out = *outStream;

	if ( exportHeaderFn != 0 )  {
		out << "#include \"" << exportHeaderFn << "\"\n";
	}

	/* Function implementations. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->objectDef != 0 ) {
			FieldList &fieldList = lel->objectDef->fieldList;
			for ( FieldList::Iter ofi = fieldList; ofi.lte(); ofi++ ) {
				ObjectField *field = ofi->value;
				if ( field->useOffset() && field->typeRef != 0  ) {
					UniqueType *ut = field->typeRef->resolveType( this );

					if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
						out << ut->langEl->refName << " " << lel->declName <<
							"::" << field->name << "() { return " <<
							ut->langEl->refName << "( __prg, colm_get_attr( __tree, " <<
							field->offset << ") ); }\n";
					}
				}

				if ( field->isRhsGet() ) {
					UniqueType *ut = field->typeRef->resolveType( this );

					if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
						out << ut->langEl->refName << " " << lel->declName <<
							"::" << field->name << "() { static int a[] = {"; 

						/* Need to place the array computing the val. */
						out << field->rhsVal.length();
						for ( Vector<RhsVal>::Iter rg = field->rhsVal; rg.lte(); rg++ ) {
							out << ", " << rg->prodEl->production->prodNum;
							out << ", " << rg->prodEl->pos;
						}

						out << "}; return " << ut->langEl->refName << 
							"( __prg, colm_get_rhs_val( __prg, __tree, a ) ); }\n";
					}
				}
			}
		}

		if ( lel->isRepeat ) {
			out << lel->refName << " " << lel->declName << "::" << " next"
				"() { return " << lel->refName << 
				"( __prg, colm_get_repeat_next( __tree ) ); }\n";

			out << lel->repeatOf->refName << " " << lel->declName << "::" << " value"
				"() { return " << lel->repeatOf->refName << 
				"( __prg, colm_get_repeat_val( __tree ) ); }\n";
		}

		if ( lel->isList ) {
			out << lel->refName << " " << lel->declName << "::" << " next"
				"() { return " << lel->refName << 
				"( __prg, colm_get_repeat_next( __tree ) ); }\n";

			out << lel->repeatOf->refName << " " << lel->declName << "::" << " value"
				"() { return " << lel->repeatOf->refName << 
				"( __prg, colm_get_repeat_val( __tree ) ); }\n";
		}
	}

	out << "\n";

	for ( FieldList::Iter of = globalObjectDef->fieldList; of.lte(); of++ ) {
		ObjectField *field = of->value;
		if ( field->isExport ) {
			UniqueType *ut = field->typeRef->resolveType(this);
			if ( ut != 0 && ut->typeId == TYPE_TREE  ) {
				out << 
					ut->langEl->refName << " " << field->name << "( colm_program *prg )\n"
					"{ return " << ut->langEl->refName << "( prg, colm_get_global( prg, " << 
					field->offset << ") ); }\n";
			}
		}
	}

	out << "\n";

	for ( FunctionList::Iter func = functionList; func.lte(); func++ ) {
		if ( func->exprt ) {
			char *refName = func->typeRef->uniqueType->langEl->refName;
			int paramCount = func->paramList->length();
			out <<
				refName << " " << func->name << "( colm_program *prg";

			for ( int p = 0; p < paramCount; p++ )
				out << ", const char *p" << p;

			out << " )\n"
				"{\n"
				"	int funcId = " << func->funcId << ";\n"
				"	const char *params[" << paramCount << "];\n";

			for ( int p = 0; p < paramCount; p++ )
				out << "	params[" << p << "] = p" << p << ";\n";

			out << 
				"	return " << refName <<
						"( prg, colm_run_func( prg, funcId, params, " << paramCount << " ));\n"
				"}\n";
		}
	}
}
