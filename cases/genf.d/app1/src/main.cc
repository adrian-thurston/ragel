#include "main.h"
#include "user.h"
#include "listen.h"
#include "itq_gen.h"
#include "genf.h"

#include <unistd.h>

extern const char data1[];
extern const char data2[];

void MainThread::recvSmallPacket( SelectFd *fd, Record::SmallPacket *pkt )
{
	static int sign = 1;
	bool failed = false;

	log_message( "received SmallPacket: " << sign );

	log_message( "l1 check   ... " << ( ( pkt->l1 != ::l1 ) == 0 ? "OK" : (failed=true,"FAILED") ) );
	log_message( "l2 check   ... " << ( ( pkt->l2 != ::l2 ) == 0 ? "OK" : (failed=true,"FAILED") ) );
	log_message( "l3 check   ... " << ( ( pkt->l3 != ::l3 ) == 0 ? "OK" : (failed=true,"FAILED") ) );

	for ( Record::SmallRec sr( pkt->rope, pkt->head_sr ); sr.valid(); sr.advance() ) {

		log_message( "sr check   ... " <<
				( ( sr.l1 != ::l2 ) == 0 ? "OK" : (failed=true,"FAILED") ) );
	}

	std::cout << "small packet " << sign << " " << ( failed ? "FAILED" : "ok" ) << std::endl;

	sign += 1;

	if ( sign > 10 )
		breakLoop();
}

void MainThread::recvBigPacket( SelectFd *fd, Record::BigPacket *pkt )
{
	static int bign = 1;
	bool failed = false;

	log_message( "received BigPacket: " << bign );

	log_message( "big1 check ... " << ( strcmp( pkt->big1, ::data1 ) == 0 ? "OK" : (failed=true,"FAILED") ) );
	log_message( "big2 check ... " << ( strcmp( pkt->big2, ::data2 ) == 0 ? "OK" : (failed=true,"FAILED") ) );
	log_message( "big3 check ... " << ( strcmp( pkt->big3, ::data3 ) == 0 ? "OK" : (failed=true,"FAILED") ) );

	log_message( "l1 check   ... " << ( ( pkt->l1 != ::l1 ) == 0 ? "OK" : (failed=true,"FAILED") ) );
	log_message( "l2 check   ... " << ( ( pkt->l2 != ::l2 ) == 0 ? "OK" : (failed=true,"FAILED") ) );
	log_message( "l3 check   ... " << ( ( pkt->l3 != ::l3 ) == 0 ? "OK" : (failed=true,"FAILED") ) );

	std::cout << "big packet " << bign << " " << ( failed ? "FAILED" : "ok" ) << std::endl;

	bign += 1;
}

struct DelayedRead
	: public PacketConnection
{
	DelayedRead( Thread *thread )
		: PacketConnection( thread ) {}

	virtual void connectComplete()
	{
		log_message( "connection completed" );

		/* Disable the read for now. */
		selectFd->wantReadSet( false );
	}
};

void MainThread::handleTimer()
{
	static int tick = 0;
	static DelayedRead *pc;

	if ( tick == 2 ) {
		log_message( "connecting to broker" );

		/* Connection to broker. */
		pc = new DelayedRead( this );
		pc->initiate( "localhost", 44726, true, sslCtx, false );
	}

	if ( tick == 6 ) {
		log_message( "triggering read" );
		pc->selectFd->wantReadSet( true );
	}

	Packer::SmallPacket sp( sendsPassthruToListen->writer );

	sp.set_l1( ::l1 );
	sp.set_l2( ::l2 );
	sp.set_l3( ::l3 );

	Packer::SmallRec rec;

	sp.alloc_sr( rec );
	rec.set_l1( ::l2 );

	sp.alloc_sr( rec );
	rec.set_l1( ::l2 );

	sp.alloc_sr( rec );
	rec.set_l1( ::l2 );

	sp.send();

	tick += 1;
}

int MainThread::main()
{
	log_message( "starting up" );

	tlsStartup( PKGSTATEDIR "/rand" );

	sslCtx = sslCtxClientInternal();

	UserThread *bare = new UserThread;
	ListenThread *listen = new ListenThread;

	SendsToUser *sendsToUser = registerSendsToUser( bare );
	SendsToListen *sendsToListen = registerSendsToListen( listen );
	sendsPassthruToListen = registerSendsPassthru( listen );

	Writer::Hello hello( sendsToUser->consHello() );
	hello.send();

	create( bare );
	create( listen );

	struct timeval t;
	t.tv_sec = 1;
	t.tv_usec = 0;

	selectLoop( &t );

	sendsToUser->openShutdown();
	sendsToUser->send();

	sendsToListen->openShutdown();
	sendsToListen->send();

	join();

	tlsShutdown();
	log_message( "exiting" );

	return 0;
}
