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

	consumeHeader = 
		headersToConsume ':'
		( 
			[^\n] | 
			( '\n' [ \t] )
		)*
		'\n';

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
		) ':'
		address ( ',' @prn_nl address )*
		'\n' %prn_nl;

	subjectHeader = 
		'Subject:' @prn_subj @prn_tab
		' '* <:
		( 
			[^\n] @prn_char  | 
			( '\n' [ \t]+ ) %prn_space
		)**
		'\n' %prn_nl;

	header = consumeHeader | 
		addrHeader | subjectHeader;

	messageLine = 
		( [^\n]* '\n' - fromLine );

	main := (
			fromLine %prn_separator
			header* 
			'\n' @end_headers
			messageLine*
		)*;
 }%%
 
%% write data;

#define BUFSIZE 8192

void test( const char *buf )
{
	int cs, len = strlen( buf );
	const char *preserve = 0, *ws = 0;

	%% write init;
	const char *p = buf;
	const char *pe = p + len;
	%% write exec;

	if ( cs == mailbox_error )
		cerr << "ERROR" << endl;

	if ( cs < mailbox_first_final )
		cerr << "DID NOT FINISH IN A FINAL STATE" << endl;
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
		"To: Edgar Allen Poe <ep@net.com> (da man)\n"
		"Subject:   (no subject) \n"
		"\n"
		"Message goes here. \n"
		"\n"
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
	Edgar Allen Poe | ep@net.com
SUBJECT
	(no subject) 
#endif
