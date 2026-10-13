#include <colm/pdarun.h>

struct Trace
{
	void commit_reduce_forward( program_t *prg, tree_t **root,
			struct pda_run *pda_run, parse_tree_t *pt );
};
