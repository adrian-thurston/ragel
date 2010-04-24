/*
 *  Copyright 2008 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "bytecode.h"
#include "pdarun.h"
#include "fsmrun.h"
#include "pdarun.h"
#include <iostream>
#include <string.h>
#include <stdlib.h>

using std::cout;
using std::cerr;
using std::endl;
using std::ostream;

void printStr( ostream &out, Head *str )
{
	out.write( (char*)(str->data), str->length );
}

/* Note that this function causes recursion, thought it is not a big
 * deal since the recursion it is only caused by nonterminals that are ignored. */
void printIgnoreList( ostream &out, Tree **sp, Program *prg, Tree *tree )
{
	Kid *ignore = treeIgnore( prg, tree );

	/* Record the root of the stack and push everything. */
	Tree **root = vm_ptop();
	while ( ignore != 0 ) {
		vm_push( (SW)ignore );
		ignore = ignore->next;
	}

	/* Pop them off and print. */
	while ( vm_ptop() != root ) {
		ignore = (Kid*) vm_pop();
		printTree( out, sp, prg, ignore->tree );
	}
}

void printKid( ostream &out, Tree **&sp, Program *prg, Kid *kid, bool printIgnore )
{
	Tree **root = vm_ptop();
	Kid *child;

rec_call:
	/* If not currently skipping ignore data, then print it. Ignore data can
	 * be associated with terminals and nonterminals. */
	if ( printIgnore && treeIgnore( prg, kid->tree ) != 0 ) {
		/* Ignorelists are reversed. */
		printIgnoreList( out, sp, prg, kid->tree );
		printIgnore = false;
	}

	if ( kid->tree->id < prg->rtd->firstNonTermId ) {
		/* Always turn on ignore printing when we get to a token. */
		printIgnore = true;

		if ( kid->tree->id == LEL_ID_INT )
			out << ((Int*)kid->tree)->value;
		else if ( kid->tree->id == LEL_ID_BOOL ) {
			if ( ((Int*)kid->tree)->value )
				out << "true";
			else
				out << "false";
		}
		else if ( kid->tree->id == LEL_ID_PTR )
			out << '#' << (void*) ((Pointer*)kid->tree)->value;
		else if ( kid->tree->id == LEL_ID_STR )
			printStr( out, ((Str*)kid->tree)->value );
		else if ( kid->tree->id == LEL_ID_STREAM )
			out << '#' << (void*) ((Stream*)kid->tree)->file;
		else if ( kid->tree->tokdata != 0 && 
				stringLength( kid->tree->tokdata ) > 0 )
		{
			out.write( stringData( kid->tree->tokdata ), 
					stringLength( kid->tree->tokdata ) );
		}
	}
	else {
		/* Non-terminal. */
		child = treeChild( prg, kid->tree );
		if ( child != 0 ) {
			vm_push( (SW)kid );
			kid = child;
			while ( kid != 0 ) {
				goto rec_call;
				rec_return:
				kid = kid->next;
			}
			kid = (Kid*)vm_pop();
		}
	}

	if ( vm_ptop() != root )
		goto rec_return;
}

void printTree( ostream &out, Tree **&sp, Program *prg, Tree *tree )
{
	if ( tree == 0 )
		out << "NIL";
	else {
		Kid kid;
		kid.tree = tree;
		kid.next = 0;
		printKid( out, sp, prg, &kid, false );
	}
}


void xmlEscapeData( const char *data, long len )
{
	for ( int i = 0; i < len; i++ ) {
		if ( data[i] == '<' )
			cout << "&lt;";
		else if ( data[i] == '>' )
			cout << "&gt;";
		else if ( data[i] == '&' )
			cout << "&amp;";
		else if ( 32 <= data[i] && data[i] <= 126 )
			cout << data[i];
		else
			cout << "&#" << ((unsigned)data[i]) << ';';
	}
}

/* Might be a good idea to include this in the printXmlKid function since
 * it is recursive and can eat up stac, however it's probably not a big deal
 * since the additional stack depth is only caused for nonterminals that are
 * ignored. */
void printXmlIgnoreList( Tree **sp, Program *prg, Tree *tree, long depth )
{
	Kid *ignore = treeIgnore( prg, tree );
	while ( ignore != 0 ) {
		printXmlKid( sp, prg, ignore, true, depth );
		ignore = ignore->next;
	}
}

void printXmlKid( Tree **&sp, Program *prg, Kid *kid, bool commAttr, int depth )
{
	Kid *child;
	Tree **root = vm_ptop();
	long i, objectLength;
	LangElInfo *lelInfo = prg->rtd->lelInfo;

	long kidNum = 0;;

rec_call:

	if ( kid->tree == 0 ) {
		for ( i = 0; i < depth; i++ )
			cout << "  ";

		cout << "NIL" << endl;
	}
	else {
		/* First print the ignore tokens. */
		if ( commAttr )
			printXmlIgnoreList( sp, prg, kid->tree, depth );

		for ( i = 0; i < depth; i++ )
			cout << "  ";

		/* Open the tag. Afterwards we will either print data underneath it or
		 * we will close it off immediately. */
		cout << '<' << lelInfo[kid->tree->id].name;

		/* If this is an attribute then give the node an attribute number. */
		if ( vm_ptop() != root ) {
			objectLength = lelInfo[((Kid*)vm_top())->tree->id].objectLength;
			if ( kidNum < objectLength )
				cout << " an=\"" << kidNum << '"';
		}

		objectLength = lelInfo[kid->tree->id].objectLength;
		child = treeChild( prg, kid->tree );
		if ( (commAttr && objectLength > 0) || child != 0 ) {
			cout << '>' << endl;
			vm_push( (SW) kidNum );
			vm_push( (SW) kid );

			kidNum = 0;
			kid = kid->tree->child;

			/* Skip over attributes if not printing comments and attributes. */
			if ( ! commAttr )
				kid = child;

			while ( kid != 0 ) {
				/* FIXME: I don't think we need this check for ignore any more. */
				if ( kid->tree == 0 || !lelInfo[kid->tree->id].ignore ) {
					depth++;
					goto rec_call;
					rec_return:
					depth--;
				}
				
				kid = kid->next;
				kidNum += 1;

				/* If the parent kid is a repeat then skip this node and go
				 * right to the first child (repeated item). */
				if ( lelInfo[((Kid*)vm_top())->tree->id].repeat )
					kid = kid->tree->child;

				/* If we have a kid and the parent is a list (recursive prod of
				 * list) then go right to the first child. */
				if ( kid != 0 && lelInfo[((Kid*)vm_top())->tree->id].list )
					kid = kid->tree->child;
			}

			kid = (Kid*) vm_pop();
			kidNum = (long) vm_pop();

			for ( i = 0; i < depth; i++ )
				cout << "  ";
			cout << "</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_PTR ) {
			cout << '>' << (void*)((Pointer*)kid->tree)->value << 
					"</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_BOOL ) {
			if ( ((Int*)kid->tree)->value )
				cout << ">true</";
			else
				cout << ">false</";
			cout << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_INT ) {
			cout << '>' << ((Int*)kid->tree)->value << 
					"</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( kid->tree->id == LEL_ID_STR ) {
			Head *head = (Head*) ((Str*)kid->tree)->value;

			cout << '>';
			xmlEscapeData( (char*)(head->data), head->length );
			cout << "</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else if ( 0 < kid->tree->id && kid->tree->id < prg->rtd->firstNonTermId &&
				kid->tree->tokdata != 0 && 
				stringLength( kid->tree->tokdata ) > 0 && 
				!lelInfo[kid->tree->id].literal )
		{
			cout << '>';
			xmlEscapeData( stringData( kid->tree->tokdata ), 
					stringLength( kid->tree->tokdata ) );
			cout << "</" << lelInfo[kid->tree->id].name << '>' << endl;
		}
		else
			cout << "/>" << endl;
	}

	if ( vm_ptop() != root )
		goto rec_return;
}

void printXmlTree( Tree **&sp, Program *prg, Tree *tree, bool commAttr )
{
	Kid kid;
	kid.tree = tree;
	kid.next = 0;
	printXmlKid( sp, prg, &kid, commAttr, 0 );
}
