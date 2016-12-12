/*
 * Copyright 2016 Adrian Thurston <thurston@colm.net>
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


#include <colm/tree.h> 
#include <colm/bytecode.h>

#include <iostream>
#include <map>
#include <set>

using std::map;
using std::set;
using std::string;
using std::pair;
using std::cout;
using std::endl;

extern "C" {
	colm_value_t cc_action_params_find( struct colm_program *prg, tree_t **sp, str_t *_machine, str_t *_action );
	colm_value_t cc_action_params_insert( struct colm_program *prg, tree_t **sp, str_t *_machine, str_t *_action );
}

typedef set<string> Set;
typedef map< string, Set > Map;
static Map machineMap;

value_t cc_action_params_find( struct colm_program *prg, tree_t **sp, str_t *_machine, str_t *_action )
{
	string machine( _machine->value->data, _machine->value->length );
	string action( _action->value->data, _action->value->length );

	// cout << "cc_action_params_find " << machine << " " << action << " ";

	long res = 0;
	Map::iterator M = machineMap.find( machine );
	if ( M != machineMap.end() )
		res = M->second.find( action ) != M->second.end();

	// cout << ( res ? "FOUND" : "NOT FOUND" ) << endl;

	colm_tree_downref( prg, sp, (tree_t*)_machine );
	colm_tree_downref( prg, sp, (tree_t*)_action );

	return (value_t) res;
}

value_t cc_action_params_insert( struct colm_program *prg, tree_t **sp, str_t *_machine, str_t *_action )
{
	string machine( _machine->value->data, _machine->value->length );
	string action( _action->value->data, _action->value->length );

	// cout << "cc_action_params_insert " << machine << " " << action << endl;

	Map::iterator M = machineMap.find( machine );
	if ( M == machineMap.end() )
		machineMap.insert( pair<string, Set>( machine, Set() )  );

	M = machineMap.find( machine );
	M->second.insert( action );

	colm_tree_downref( prg, sp, (tree_t*)_machine );
	colm_tree_downref( prg, sp, (tree_t*)_action );

	return 0;
}
