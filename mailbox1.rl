/*
 * @LANG: c++
 * @CFLAGS: -I../aapl
 *
 * Test works with split code gen.
 */

/*
 * Parses unix mail boxes into headers and bodies.
 */

#include "mailbox1.h"

%%{
	machine MBox;

	# Buffer the header names.
	action bufHeadName { fsm->headName.append(fc); }

	# Buffer the header content.
	action bufHeadContent { fsm->headContent.append(fc); }

	# Terminate a header. If it is an interesting header then prints it.
	action finBufHeadContent {
		/* Terminate the buffers. */
		fsm->headName.append(0);
		fsm->headContent.append(0);

		/* Print the header. Interesting headers. */
		printf("%s:%s\n", fsm->headName.data, fsm->headContent.data);
		
		/* Clear for the next time we use them. */
		fsm->headName.empty();
		fsm->headContent.empty();
	}

	action msgstart{
		printf("NEW MESSAGE\n");
	}

	# Prints a blank line after the end of the headers of each message.
	action blankLine {
		printf("\n");
	}
	
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

	# Note the priority assignment on the end of the from line. While we
	# matching the body of a message we may enter into this machine. We will
	# not leave the body of the previous message until this entire from line is
	# matched. 
	fromLine = 'From ' . /[^\n]/* . ' ' . date . '\n' @(new_msg,1) @msgstart;

	# The types of characters that can be used as a header name.
	hchar = print - [ :];

	header =
		# The name of the header.
		hchar+ $bufHeadName . ':' 
		# The content of the header. Look out for continuations.
		. ( (extend - '\n') $bufHeadContent | '\n'. [ \t] @bufHeadContent )*
		# Buffer must end with a newline that does not continue.
		. '\n' %finBufHeadContent;

	messageLine = ( extend - '\n' )* . '\n' @(new_msg, 0);

	# When we get to the last newline we are still matching messageLine
	# so on the last newline it will think we are still in the message.
	# We need this because we can't assume that every newline means
	# the end of the current message, whereas at the same time we requre
	# that there be a newline before the fromLine of the next message.
	message = ( fromLine .  header* .  '\n' @blankLine .  messageLine* . '\n' );

	# Its important that the priority in the fromLine gets bumped up
	# so that we are able to move to new messages. Otherwise we
	# will always stay in the message body of the first message.
	main := message*;
}%%

%% write data;

void MBox::init( )
{
	MBox *fsm = this;
	%% write init;
}

void MBox::execute( const char *data, int len )
{
	MBox *fsm = this;
	const char *p = data;
	const char *pe = data + len;
	%%{
		access fsm->;
		write exec;
	}%%
}

int MBox::finish( )
{
	if ( cs == MBox_error )
		return -1;
	if ( cs >= MBox_first_final )
		return 1;
	return 0;
}

MBox mbox;

void test( const char *buf )
{
	int len = strlen( buf );
	mbox.init();
	mbox.execute( buf, len );
	if ( mbox.finish() > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}


int main()
{
	test(
		"From email address goes here Wed Nov 28 13:30:05 2001 -0500\n"
		"Header1: this is the header contents\n"
		" there is more on the second line\n"
		"	and more on the third line.\n"
		"Header2: slkdj\n"
		"\n"
		"This is the message data\n"
		"\n"
		"From email Wed Nov 28 13:30:05 2001 -0500\n"
		"Header: \n"
		"\n"
		"mail message\n"
		"\n"
	);

	test(
		"From user@host.dom Wed Nov 28 13:30:05 2001\n"
		"\n"
		"There are no headers. \n"
		"\n"
		"From email Wed Nov 28 13:30:05 EST 2000\n"
		"\n"
		"There are no headers.\n"
		"\n"
	);

	test(
		"From user@host.dom Wed Nov 28 13:30:05 2001\n"
		"Header:alsdj\n"
		"\n"
		"Header:\n"
		"salkfj\n"
		"\n"
		"There are no headers. \n"
		"\n"
	);

	test(
		"From user@host.dom Wed Nov 28 13:30:05 2001\n"
		"Header:alsdj\n"
		"\n"
		"Header:\n"
		"salkfj\n"
		"\n"
		"There are no headers. \n"
		"\n"
		">From user@host.dom Wed Nov 28 13:30:05 2001\n"
		"\n"
	);

	test(
		"From user@host.dom Wed Nov 28 13:30:05 2001\n"
		"Header:alsdj\n"
		"\n"
		"Header:\n"
		"salkfj\n"
		"\n"
		"There are no headers. \n"
		"\n"
		"From user@host.dom Wed Nov 28 13:30:05 2001\n"
		"\n"
	);

	test(
		"From user@host.dom Wed Nov 28 13:30:05 2001\n"
		"Header:alsdj\n"
		"\n"
		"Header:\n"
		"salkfj\n"
		"\n"
		"There are no headers. \n"
		"\n"
		"From user@host.dom Wed Nov 28 13:30:05 2001\n"
		"\n"
		"\n"
	);

	return 0;
}

#ifdef _____OUTPUT_____
NEW MESSAGE
Header1: this is the header contents there is more on the second line	and more on the third line.
Header2: slkdj

NEW MESSAGE
Header: 

ACCEPT
NEW MESSAGE

NEW MESSAGE

ACCEPT
NEW MESSAGE
Header:alsdj

ACCEPT
NEW MESSAGE
Header:alsdj

ACCEPT
NEW MESSAGE
Header:alsdj

NEW MESSAGE

FAIL
NEW MESSAGE
Header:alsdj

NEW MESSAGE

ACCEPT
#endif
