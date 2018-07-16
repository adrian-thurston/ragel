#include "user.h"
#include "itq_gen.h"

void cb( void *arg, int status, int timeouts, unsigned char *abuf, int alen )
{
	static_cast<UserThread*>(arg)->cb( status, timeouts, abuf, alen );
}


UserThread::UserThread()
{
	recvRequiresSignal = true;
}

void UserThread::recvHello( Message::Hello *msg )
{
	log_message( "received hello" );
	ares_query( ac, "www.google.ca", ns_c_in, ns_t_a, ::cb, this );
	// sleep(1);
}

void UserThread::handleTimer()
{
	// log_message( "timer" );
}

void UserThread::recvShutdown( Message::Shutdown *msg )
{
	log_message( "received shutdown" );
	breakLoop();
}

#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void print_inet( char **listptr, int length )
{
}

void *UserThread::cb( int status, int timeouts, unsigned char *abuf, int alen )
{
	if ( status == ARES_SUCCESS ) {
		log_message( "query succeeded" );

		hostent *h;
		ares_parse_a_reply( abuf, alen, &h, 0, 0 );

		log_message( "official host name: " << h->h_name );

		/* Go through the list of aliases. */
		char **ptr = h->h_aliases;
		for ( int i = 0; ptr[i] != 0; i++ ) {
			log_message( "  alias: " << ptr[i] );
		}

		log_message( "  addr type = " << h->h_addrtype << ", addr length = " << h->h_length );

		switch ( h->h_addrtype ) {
			case AF_INET: {
				/* Print the list of internet address responses. */
				struct in_addr **ptr = (struct in_addr**) h->h_addr_list;
				for ( int i = 0; ptr[i] != 0; i++ ) {
					log_message( "  internet address: " << inet_ntoa( *ptr[i] ) );
				}
				break;
			}
			default: {
				log_message("unknown address type");
				break;
			}
		}

		ares_free_hostent( h );
	}
	else {
		log_message( "query failed" );
	}

	return 0;
}

int UserThread::main()
{
	log_message( "starting up" );

	struct timeval t;
	t.tv_sec = 2;
	t.tv_usec = 0;

	ares_query( ac, "www.google.com", ns_c_in, ns_t_a, ::cb, this );

	selectLoop( &t );
	log_message( "exiting" );
	return 0;
}
