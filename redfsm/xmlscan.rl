/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@cs.queensu.ca>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#include <iostream>
#include <string.h>
#include "vector.h"
#include "xmlparse.h"
#include "buffer.h"

using std::istream;
using std::cout;
using std::cerr;
using std::endl;

#define BUFSIZE 4096

%%{
	machine Scanner;
	write data;
}%%

class Perfect_Hash
{
private:
	static inline unsigned int hash (const char *str, unsigned int len);

public:
	static struct XMLTagHashPair *in_word_set (const char *str, unsigned int len);
};

struct Scanner
{
	Scanner( char *fileName, istream &input ) : 
		fileName(fileName),
		input(input), 
		curline(1), 
		curcol(1),
		p(0), pe(0), 
		done(false),
		data(0), data_len(0),
		value(0)
	{
		%%{
			machine Scanner;
			write init;
		}%%
	}
	
	int scan();
	void adjustAttrPointers( int distance );
	std::ostream &error();

	char *fileName;
	istream &input;

	/* Scanner State. */
	int cs, act, have, curline, curcol;
	char *tokstart, *tokend;
	char *p, *pe;
	int done;

	/* Token data */
	char *data;
	int data_len;
	int value;
	AttrMkList attrMkList;
	Buffer buffer;
	char *tag_id_start;
	int tag_id_len;
	int token_col, token_line;

	char buf[BUFSIZE];
};


#define TK_NO_TOKEN (-1)
#define TK_ERR 1
#define TK_SPACE 2
#define TK_EOF 3
#define TK_OpenTag 4
#define TK_CloseTag 5

#define ret_tok( _tok ) token = (_tok); data = tokstart

void Scanner::adjustAttrPointers( int distance )
{
	for ( AttrMkList::Iter attr = attrMkList; attr.lte(); attr++ ) {
		attr->id -= distance;
		attr->value -= distance;
	}
}

/* There is no claim that this is a proper XML parser, but it is good
 * enough for our purposes. */
%%{
	machine Scanner;

	action colup { curcol++; }
	action start_tok { token_col = curcol; token_line = curline; }
	NL = '\n' @{ curcol = 0; curline++; };

	WS = [\r\t ] | NL;
	id = [_a-zA-Z][_a-zA-Z0-9]*;
	literal = '"' ( [^"] | NL )* '"';

	# Attribute identifiers.
	action start_attr_id { attr_id_start = p; }
	action leave_attr_id { attr_id_len = p - attr_id_start; }

	attr_id = id >start_attr_id %leave_attr_id;

	# Attribute values
	action start_attr_value { attr_value_start = p; }
	action leave_attr_value
	{
		attr_value_len = p - attr_value_start;

		AttrMarker newAttr;
		newAttr.id = attr_id_start;
		newAttr.idLen = attr_id_len;
		newAttr.value = attr_value_start;
		newAttr.valueLen = attr_value_len;
		attrMkList.append( newAttr );
	}

	attr_value = literal >start_attr_value %leave_attr_value;

	# Attribute list. 
	attribute = attr_id WS* '=' WS* attr_value WS*;

	# Tag identifiers.
	action tag_id_start { tag_id_start = p; }
	action leave_tag_id { tag_id_len = p - tag_id_start; }

	tag_id = id >tag_id_start %leave_tag_id;

	main := |*
		# Tags
		( '<' WS* tag_id ( WS+ attribute* )? '>' ) >start_tok $colup 
			=> { ret_tok( TK_OpenTag ); fbreak; };

		( '<' WS* '/' WS* tag_id WS* '>' ) >start_tok $colup 
			=> { ret_tok( TK_CloseTag ); fbreak; };

		# Data in between tags.
		( [^<&\0] | NL ) $colup 
			=> { buffer.append( *p ); };

		# Specials.
		"&amp;" $colup
			=> { buffer.append( '&' ); };
		"&lt;" $colup
			=> { buffer.append( '<' ); };
		"&gt;" $colup
			=> { buffer.append( '>' ); };
		
		# EOF
		0 >start_tok => { ret_tok( TK_EOF ); fbreak; };

	*|;
}%%

int Scanner::scan( )
{
	int token = TK_NO_TOKEN;
	int space = 0, readlen = 0;
	char *attr_id_start = 0;
	char *attr_value_start = 0;
	int attr_id_len = 0;
	int attr_value_len = 0;

	attrMkList.empty();
	buffer.clear();

	while ( 1 ) {
		if ( p == pe ) {
			//printf("scanner: need more data\n");

			if ( tokstart == 0 )
				have = 0;
			else {
				/* There is data that needs to be shifted over. */
				//printf("scanner: buffer broken mid token\n");
				have = pe - tokstart;
				memmove( buf, tokstart, have );

				int distance = tokstart - buf;
				tokend -= distance;
				tag_id_start -= distance;
				attr_id_start -= distance;
				attr_value_start -= distance;
				adjustAttrPointers( distance );
				tokstart = buf;
			}

			p = buf + have;
			space = BUFSIZE - have;

			if ( space == 0 ) {
				/* We filled up the buffer trying to scan a token. */
				return TK_SPACE;
			}

			if ( done ) {
				//printf("scanner: end of file\n");
				p[0] = 0;
				readlen = 1;
			}
			else {
				input.read( p, space );
				readlen = input.gcount();
				if ( input.eof() ) {
					//printf("scanner: setting done flag\n");
					done = 1;
				}
			}

			pe = p + readlen;
		}

		%% write exec;

		if ( cs == Scanner_error )
			return TK_ERR;

		if ( token != TK_NO_TOKEN ) {
			/* fbreak does not advance p, so we do it manually. */
			p = p + 1;
			data_len = p - data;
			return token;
		}
	}
}

int xml_parse( std::istream &input, char *fileName, 
		bool outputActive, bool wantComplete )
{
	Scanner scanner( fileName, input );
	Parser parser( fileName, outputActive, wantComplete );

	parser.init();

	while ( 1 ) {
		int token = scanner.scan();
		if ( token == TK_NO_TOKEN ) {
			cerr << "xmlscan: interal error: scanner returned NO_TOKEN" << endl;
			exit(1);
		}
		else if ( token == TK_EOF ) {
			parser.token( _eof, scanner.token_col, scanner.token_line );
			break;
		}
		else if ( token == TK_ERR ) {
			scanner.error() << "scanner error" << endl;
			break;
		}
		else if ( token == TK_SPACE ) {
			scanner.error() << "scanner is out of buffer space" << endl;
			break;
		}
		else {
			/* All other tokens are either open or close tags. */
			XMLTagHashPair *tagId = Perfect_Hash::in_word_set( 
					scanner.tag_id_start, scanner.tag_id_len );

			XMLTag *tag = new XMLTag( tagId, token == TK_OpenTag ? 
					XMLTag::Open : XMLTag::Close );

			if ( tagId != 0 ) {
				/* Get attributes for open tags. */
				if ( token == TK_OpenTag && scanner.attrMkList.length() > 0 ) {
					tag->attrList = new AttrList;
					for ( AttrMkList::Iter attr = scanner.attrMkList; 
							attr.lte(); attr++ )
					{
						Attribute newAttr;
						newAttr.id = new char[attr->idLen+1];
						memcpy( newAttr.id, attr->id, attr->idLen );
						newAttr.id[attr->idLen] = 0;

						/* Exclude the surrounding quotes. */
						newAttr.value = new char[attr->valueLen-1];
						memcpy( newAttr.value, attr->value+1, attr->valueLen-2 );
						newAttr.value[attr->valueLen-2] = 0;

						tag->attrList->append( newAttr );
					}
				}

				/* Get content for closing tags. */
				if ( token == TK_CloseTag ) {
					switch ( tagId->id ) {
					case TAG_host: case TAG_arg:
					case TAG_t: case TAG_alphtype:
					case TAG_text: case TAG_goto:
					case TAG_call: case TAG_next:
					case TAG_entry: case TAG_set_tokend:
					case TAG_set_act: case TAG_start_state:
					case TAG_error_state: case TAG_state_actions: 
					case TAG_action_table: case TAG_cond_space: 
					case TAG_c: case TAG_ex:
						tag->content = new char[scanner.buffer.length+1];
						memcpy( tag->content, scanner.buffer.data,
								scanner.buffer.length );
						tag->content[scanner.buffer.length] = 0;
						break;
					}
				}
			}

			#if 0
			cerr << "parser_driver: " << (tag->type == XMLTag::Open ? "open" : "close") <<
					": " << (tag->tagId != 0 ? tag->tagId->name : "<unknown>") << endl;
			if ( tag->attrList != 0 ) {
				for ( AttrList::Iter attr = *tag->attrList; attr.lte(); attr++ )
					cerr << "    " << attr->id << ": " << attr->value << endl;
			}
			if ( tag->content != 0 )
				cerr << "    content: " << tag->content << endl;
			#endif

			parser.token( tag, scanner.token_col, scanner.token_line );
		}
	}

	return 0;
}

std::ostream &Scanner::error()
{
	gblErrorCount += 1;
	cerr << fileName << ":" << curline << ":" << curcol << ": ";
	return cerr;
}
