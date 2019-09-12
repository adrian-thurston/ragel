/*
 * Parses unix mail boxes into headers and bodies.
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#define BUFSIZE 2048

/* A growable buffer for collecting headers. */
struct Buffer
{
	Buffer() : data(0), allocated(0), length(0) { }
	~Buffer() { empty(); }

	void append( char p ) {
		if ( ++length > allocated )
			upAllocate( length*2 );
		data[length-1] = p;
	}
		
	void clear() { length = 0; }
	void upAllocate( int len );
	void empty();

	char *data;
	int allocated;
	int length;
};


struct MailboxScanner
{
	Buffer headName;
	Buffer headContent;

	int cs, top, stack[1];

	int init( );
	int execute( const char *data, int len, bool isEof );
	int finish( );
};

%%{
	machine MailboxScanner;

	# Buffer the header names.
	action bufHeadName { headName.append(fc); }

	# Prints a blank line after the end of the headers of each message.
	action blankLine { cout << endl; }
	
	# Helpers we will use in matching the date section of the from line.
	day = /[A-Z][a-z][a-z]/;
	month = /[A-Z][a-z][a-z]/;
	year = /[0-9][0-9][0-9][0-9]/;
	time = /[0-9][0-9]:[0-9][0-9]/ . ( /:[0-9][0-9]/ | '' );
	letterZone = /[A-Z][A-Z][A-Z]/;
	numZone = /[+\-][0-9][0-9][0-9][0-9]/;
	zone = letterZone | numZone;
	dayNum = /[0-9 ][0-9]/;

	# These are the different formats of the date minus an obscure
	# type that has a funny string 'remote from xxx' on the end. Taken
	# from c-client in the imap-2000 distribution.
	date = day . ' ' . month . ' ' . dayNum . ' ' . time . ' ' .
		( year | year . ' ' . zone | zone . ' ' . year );

	# From lines separate messages. We will exclude fromLine from a message
	# body line.  This will cause us to stay in message line up until an
	# entirely correct from line is matched.
	fromLine = 'From ' . (any-'\n')* . ' ' . date . '\n';

	# The types of characters that can be used as a header name.
	hchar = print - [ :];

	# Simply eat up an uninteresting header. Return at the first non-ws
	# character following a newline.
	consumeHeader := ( 
			[^\n] | 
			'\n' [ \t] |
			'\n' [^ \t] @{fhold; fret;}
		)*;

	action hchar {headContent.append(fc);}
	action hspace {headContent.append(' ');}

	action hfinish {
		headContent.append(0);
		cout << headContent.data << endl;
		headContent.clear();
		fhold;
		fret;
	}

	# Display the contents of a header as it is consumed. Collapses line
	# continuations to a single space. 
	printHeader := ( 
		[^\n] @hchar  | 
		( '\n' ( [ \t]+ '\n' )* [ \t]+ ) %hspace
	)** $!hfinish;

	action onHeader 
	{
		headName.append(0);
		if ( strcmp( headName.data, "From" ) == 0 ||
				strcmp( headName.data, "To" ) == 0 ||
				strcmp( headName.data, "Subject" ) == 0 )
		{
			/* Print the header name, then jump to a machine the will display
			 * the contents. */
			cout << headName.data << ":";
			headName.clear();
			fcall printHeader;
		}

		headName.clear();
		fcall consumeHeader;
	}

	header = hchar+ $bufHeadName ':' @onHeader;

	# Exclude fromLine from a messageLine, otherwise when encountering a
	# fromLine we will be simultaneously matching the old message and a new
	# message.
	messageLine = ( [^\n]* '\n' - fromLine );

	# An entire message.
	message = ( fromLine .  header* .  '\n' @blankLine .  messageLine* );

	# File is a series of messages.
	main := message*;
}%%

%% write data;

int MailboxScanner::init( )
{
	%% write init;
	return 1;
}

int MailboxScanner::execute( const char *data, int len, bool isEof )
{
	const char *p = data;
	const char *pe = data + len;
	const char *eof = isEof ? pe : 0;

	%% write exec;

	if ( cs == MailboxScanner_error )
		return -1;
	if ( cs >= MailboxScanner_first_final )
		return 1;
	return 0;
}

int MailboxScanner::finish( )
{
	if ( cs == MailboxScanner_error )
		return -1;
	if ( cs >= MailboxScanner_first_final )
		return 1;
	return 0;
}


void Buffer::empty()
{
	if ( data != 0 ) {
		free( data );

		data = 0;
		length = 0;
		allocated = 0;
	}
}

void Buffer::upAllocate( int len )
{
	if ( data == 0 )
		data = (char*) malloc( len );
	else
		data = (char*) realloc( data, len );
	allocated = len;
}

MailboxScanner mailbox;
char buf[BUFSIZE];

int main()
{
	mailbox.init();
	while ( 1 ) {
		int len = fread( buf, 1, BUFSIZE, stdin );
		mailbox.execute( buf, len, len != BUFSIZE );
		if ( len != BUFSIZE )
			break;
	}
	if ( mailbox.finish() <= 0 )
		cerr << "mailbox: error parsing input" << endl;
	return 0;
}
