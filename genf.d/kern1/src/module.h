#ifndef _KERN1_MODULE_H
#define _KERN1_MODULE_H

int ring_init( void );
int ring_exit( void );

struct ring
{
	struct kobject kobj;
};

#endif
