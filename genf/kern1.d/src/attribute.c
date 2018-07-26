#include "attribute.h"

#include <linux/workqueue.h>

struct ring
{
	struct kobject kobj;
};

static int n;
static struct workqueue_struct *wk = 0;

struct notify_work
{
	struct work_struct work;
	
	int n;
};

static void handle_work( struct work_struct *work )
{
	struct notify_work *nw = container_of( work, struct notify_work, work );

	printk( "work: %d\n", nw->n );

	kfree( nw );
}

static void shuttle_work( int n )
{
	struct notify_work *work;

	work = kzalloc( sizeof(*work), GFP_ATOMIC );
	if (!work)
		return;

	INIT_WORK( &work->work, &handle_work );
	work->n = n;
	queue_work( wk, &work->work );
}

static ssize_t ring_foo_show( struct ring *obj, char *buf )
{
	return sprintf( buf, "%d\n", n );
}

static ssize_t ring_foo_store( struct ring *obj )
{
	shuttle_work( 22 );
	return 0;
}

static ssize_t ring_bar_show( struct ring *obj, char *buf )
{
	return sprintf( buf, "%d\n", n );
}

static ssize_t ring_bar_store( struct ring *obj )
{
	return 0;
}

static int ring_init(void)
{
	wk = alloc_workqueue( "name", 0, 1 );
	return 0;
}

static void ring_exit(void)
{
	destroy_workqueue( wk );
}
