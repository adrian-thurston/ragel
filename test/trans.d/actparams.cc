
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
