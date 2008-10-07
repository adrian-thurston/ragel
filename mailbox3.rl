/*
 * @LANG: c++
 * @CFLAGS: -I../aapl
 */

#include <iostream>
#include <string.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

%%{
	machine mailbox;

	action prn_char { cout << *p; }
	action prn_space { cout << ' '; }
	action prn_word { cout.write(ws, p-ws); cout << ' '; }
	action prn_addr1 { cout << "| "; cout.write(ws+1, p-ws-2); }
	action prn_addr2 { cout << "| "; cout.write(ws, p-ws); }
	action prn_tab { cout << '\t'; }
	action prn_nl { cout << '\n'; }
	action prn_separator { cout << "------\n"; } 
	action prn_from { cout << "FROM\n"; }
	action prn_to { cout << "TO\n"; }
	action prn_subj { cout << "SUBJECT\n"; }

	action start_word { ws = p; }
	action start_headers { preserve = p; }
	action end_headers {preserve = 0;}

	day = upper lower{2};
	month = upper lower{2};
	year = digit{4};
	time = digit{2} ':' digit{2} 
		( ':' digit{2} )?;
	letterZone = upper{3};
	numZone = [+\-] digit{4};
	zone = letterZone | numZone;
	dayNum = ( digit | ' ' ) digit;

	date = day ' ' month ' ' 
		dayNum ' ' time ' '
		( 
			year | 
			year ' ' zone | 
			zone ' ' year
		);

	fromLine = 'From ' [^\n]* ' ' 
		date '\n' @start_headers;

	headerChar = print - [ :];
	headersToPrint = 'From' | 
		'To' | 'Subject';
	headersToConsume = 
		headerChar+ - headersToPrint;

	action init_hlen {hlen = 0;}
	action hlen {hlen++ < 50}

	consumeHeaderBody = 
		':' @init_hlen
		( 
			[^\n] | 
			( '\n' [ \t] )
		)* when hlen
		'\n';

	consumeHeader = 
		headersToConsume consumeHeaderBody;
		
	addrWS = ( [ \t]+ | '\n' [ \t]+ );
	addrComment = '(' [^)]* ')';
	addrWord = [^"'@,<>() \t\n]+;
	addrAddr1 = '<' [^>]* '>';
	addrAddr2 = addrWord '@' addrWord;
	addrString = 
		'"' [^"]* '"' |
		"'" [^']* "'";

	addrItem = (
			addrAddr1 %prn_addr1 |
			addrAddr2 %prn_addr2 |
			addrWord %prn_word |
			addrString %prn_word
		) >start_word;

	address = ( 
			addrWS |
			addrComment |
			addrItem 
		)** >prn_tab;

	addrHeader = ( 
			'From' %prn_from | 
			'To' %prn_to
		) ':' @init_hlen
		( address ( ',' @prn_nl address )* ) when hlen
		'\n' %prn_nl;

	subjectHeader = 
		'Subject:' @prn_subj @prn_tab @init_hlen
		(
			' '* <:
			( 
				[^\n] @prn_char  | 
				( '\n' [ \t]+ ) %prn_space
			)**
		) when hlen
		'\n' %prn_nl;

	header = consumeHeader | 
		addrHeader | subjectHeader;

	messageLine = 
		( [^\n]* when hlen '\n' @init_hlen ) - fromLine;

	main := (
			fromLine %prn_separator
			header* 
			'\n' @end_headers @init_hlen
			messageLine*
		)*;
 }%%
 
%% write data;

#define BUFSIZE 8192

void test( const char *buf )
{
	int cs, len = strlen( buf );
	const char *preserve = 0, *ws = 0;
	int hlen = 0;

	%% write init;
	const char *p = buf;
	const char *pe = p + len;
	%% write exec;

	if ( cs < mailbox_first_final ) {
		cout << endl << endl;
		cout << "DID NOT FINISH IN A FINAL STATE" << endl;
	}
}

int main()
{
	test(
		"From user@host.com Wed Nov 28 13:30:05 2001\n"
		"From: \"Adrian D. Thurston\" <thurston@complang.org>\n"
		"Subject:   the squirrel has landed\n"
		"\n"
		"Message goes here. \n"
		"From (trick from line).\n"
		"From: not really a header\n"
		"\n"
		"From user2@host2.com Wed Nov 28 13:30:05 2001\n"
		"To: \"(kill 1)\" Edgar Allen Poe <ep@net.com> (da man)\n"
		"Subject:   (no subject) this is a really long subject which should fail the length constraint \n"
		"Other: 0123456789\n"
		"\n"
		"Message goes here. \n"
		"\n"
	);
	test(
		"From user@host.com Wed Nov 28 13:30:05 2001\n"
		"To: \"(kill 2)\" some guy <sg@net.com>\n"
		"From: \"Adrian D. Thurston this name is far too long\" <thurston@complang.org>\n"
		"Subject:   the squirrel has landed\n"
		"\n"
		"From user2@host2.com Wed Nov 28 13:30:05 2001\n"
		"To: Edgar Allen Poe <ep@net.com> (da man)\n"
		"Subject:   (no subject) \n"
		"\n"
	);
	test(
		"From user@host.com Wed Nov 28 13:30:05 2001\n"
		"To: \"(kill 3)\" some guy <sg@net.com>\n"
		"From: \"Adrian D. Thurston This name is fore sure absolutely too long\" <t@cs.ca>\n"
		"Subject:   the squirrel has landed\n"
		"\n"
	);
	test(
		"From user@host.com Wed Nov 28 13:30:05 2001\n"
		"From: \"Adrian D. Thurston \" <t@cs.ca>\n"
		"Subject:   (kill 4) the squirrel has landed\n"
		"Other: This is another header field, not interpreted, that is too long\n"
		"\n"
	);
	test(
		"From user@host.com Wed Nov 28 13:30:05 2001\n"
		"From: \"Adrian D. Thurston \" <t@cs.ca>\n"
		"Subject:   (kill 5)the squirrel has landed\n"
		"\n"
		"This message line is okay.\n"
		"But this message line is far too long and will cause an error.\n"
	);
	return 0;
}

#ifdef _____OUTPUT_____
------
FROM
	"Adrian D. Thurston" | thurston@complang.org
SUBJECT
	the squirrel has landed
------
TO
	"(kill 1)" Edgar Allen Poe | ep@net.com
SUBJECT
	(no subject) this is a really long subject whic

DID NOT FINISH IN A FINAL STATE
------
TO
	"(kill 2)" some guy | sg@net.com
FROM
	"Adrian D. Thurston this name is far too long" 

DID NOT FINISH IN A FINAL STATE
------
TO
	"(kill 3)" some guy | sg@net.com
FROM
	

DID NOT FINISH IN A FINAL STATE
------
FROM
	"Adrian D. Thurston " | t@cs.ca
SUBJECT
	(kill 4) the squirrel has landed


DID NOT FINISH IN A FINAL STATE
------
FROM
	"Adrian D. Thurston " | t@cs.ca
SUBJECT
	(kill 5)the squirrel has landed


DID NOT FINISH IN A FINAL STATE
#endif
