#include <string>
#include <colm/pdarun.h>

/* A token's text isn't NUL-terminated, so read it with its length. */
inline long tokenValue( const colm_data *token )
{
	return std::stol( std::string( token->data, token->length ) );
}

struct Sum
{
	Sum() : total( 0 ) {}

	long total;

	/* The commit code defines this. */
	void commit_reduce_forward( program_t *prg, tree_t **root,
			struct pda_run *pda_run, parse_tree_t *pt );
};
