/*
 *  Copyright 2007-2015 Adrian Thurston <thurston@complang.org>
 */

#ifndef _COLM_TYPE_H
#define _COLM_TYPE_H

enum TYPE
{
	TYPE_NOTYPE      = 0x00,
	TYPE_NIL         = 0x01,
	TYPE_TREE        = 0x02,
	TYPE_REF         = 0x03,
	TYPE_ITER        = 0x04,
	TYPE_STRUCT      = 0x05,
	TYPE_GENERIC     = 0x06,
	TYPE_INT         = 0x07,
	TYPE_BOOL        = 0x08,
	TYPE_LIST_PTRS   = 0x09,
	TYPE_MAP_PTRS    = 0x0a,
};

#endif
