#include "main.h"
#include "listen.h"

#include "packet_gen.h"
#include "genf.h"

#include <errno.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/ssl.h>

#include <sys/socket.h>
#include <arpa/inet.h>

struct BpConnection
:
	public PacketConnection
{
	BpConnection( ListenThread *listenThread, SelectFd *selectFd )
	:
		PacketConnection( listenThread ),
		listenThread(listenThread)
	{}

	void notifyAccept()
	{
		log_message( "test packet connection: notify accept" );

		/* Immediately try to read. Is this necessary? IE will the connect
		 * leave data on the FD or buffer it in? */
		// sslReadReady( fd );
		PacketWriter writer( this );

		for ( int i = 0; i < 100; i++ ) {
			Packer::BigPacket msg( this );
			msg.set_big3( ::data3 );
			msg.set_big2( ::data2 );
			msg.set_big1( ::data1 );
			msg.set_l1( ::l1 );
			msg.set_l2( ::l2 );
			msg.set_l3( ::l3 );
			msg.send();
		}

		listenThread->bpConnection = this;
	}

	ListenThread *listenThread;
};

struct BpListener
:
	public PacketListener
{
	BpListener( ListenThread *listenThread )
	:
		PacketListener( listenThread ),
		listenThread(listenThread)
	{}

	virtual BpConnection *connectionFactory( int fd )
		{ return new BpConnection( listenThread, 0 ); }

	ListenThread *listenThread;
};


void ListenThread::recvShutdown( Message::Shutdown *msg )
{
	breakLoop();
}

void ListenThread::handleTimer()
{
}

void ListenThread::recvPassthru( Message::PacketPassthru *msg )
{
	if ( bpConnection == 0 ) {
		log_message( "received passthrough, but no BpConnection" );
	}
	else {
		log_message( "received passthrough, forwarding" );
		PacketWriter writer( bpConnection );
		Rope *rope = (Rope*) msg->rope;
		PacketBase::send( &writer, *rope, true );
	}
}

int ListenThread::main()
{
	log_message("starting up" );

	BpListener *listener = new BpListener( this );

	listener->tlsAccept = true;
	SSL_CTX *sslCtx = sslCtxServerInternal();

	listener->startListen( 44726, true, sslCtx, false );

	struct timeval t;
	t.tv_sec = 1;
	t.tv_usec = 0;

	selectLoop( &t );

	::close( listener->selectFd->fd );

	log_message( "exiting" );

	return 0;
}
