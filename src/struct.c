#include <colm/program.h>
#include <colm/struct.h>

struct colm_struct *colm_new_struct( Program *prg, int id )
{
	int structSize = prg->rtd->selInfo[id].size;
	size_t memsize = sizeof(struct colm_struct) + ( sizeof(Tree*) * structSize );
	struct colm_struct *item = (struct colm_struct*) malloc( memsize );
	memset( item, 0, memsize );
	item->id = id;

	if ( prg->heap.head == 0 ) {
		prg->heap.head = prg->heap.tail = item;
		item->prev = item->next = 0;
	}
	else {
		item->prev = prg->heap.tail;
		item->next = 0;
		prg->heap.tail->next = item;
		prg->heap.tail = item;
	}
	
	return item;
}
