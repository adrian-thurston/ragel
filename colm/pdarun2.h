#ifndef _PDARUN2_H
#define _PDARUN2_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _File
{
	struct _File *prev;
	struct _File *next;
} File;

typedef struct _Location
{
	File *file;
	long line;
	long column;
	long byte;
} Location;

/* Header located just before string data. */
typedef struct _Head
{
	const char *data;
	long length;
	Location *location;
} Head;

typedef struct _Kid
{
	/* The tree needs to be first since pointers to kids are used to reference
	 * trees on the stack. A pointer to the word that is a Tree* is cast to
	 * a Kid*. */
	struct _Tree *tree;
	struct _Kid *next;
} Kid;

typedef struct _Ref
{
	Kid *kid;
	struct _Ref *next;
} Ref;

typedef struct _Tree
{
	/* First four will be overlaid in other structures. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *tokdata;
} Tree;

typedef struct _ParseTree
{
	/* Entire structure must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *tokdata;

	/* Parsing algorithm. */
	long state;
	long region;
	char causeReduce;
	char retry_lower;
	char retry_upper;
} ParseTree;

typedef struct _Int
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	long value;
} Int;

typedef struct _Pointer
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Kid *value;
} Pointer;

typedef struct _Str
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *value;
} Str;

typedef struct _ListEl
{
	/* Must overlay kid. */
	Tree *value;
	struct _ListEl *next;
	struct _ListEl *prev;
} ListEl;

/*
 * Maps
 */
typedef struct _GenericInfo
{
	long type;
	long typeArg;
	long keyOffset;
	long keyType;
	long langElId;
	long parserId;
} GenericInfo;

typedef struct _List
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	ListEl *head;

	ListEl *tail;
	long listLen;
	GenericInfo *genericInfo;

} List;


void listAddAfter( List *list, ListEl *prev_el, ListEl *new_el );
void listAddBefore( List *list, ListEl *next_el, ListEl *new_el );

inline void listPrepend( List *list, ListEl *new_el) { listAddBefore(list, list->head, new_el); }
inline void listAppend( List *list, ListEl *new_el)  { listAddAfter(list, list->tail, new_el); }

ListEl *listDetach( List *list, ListEl *el );
inline ListEl *listDetachFirst(List *list )        { return listDetach(list, list->head); }
inline ListEl *listDetachLast(List *list )         { return listDetach(list, list->tail); }

inline long listLength(List *list)
	{ return list->listLen; }

#ifdef __cplusplus
}
#endif

#endif
