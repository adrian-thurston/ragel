#include "main.h"
#include "genf.h"

struct Sleep
:
	public Process 
{
	Sleep( Thread *thread, const char *cmdline )
		: Process( thread, cmdline ) {}

	void writeReady( SelectFd *fd )
	{
		log_message( "write ready" );
		wantWriteSet( false );
	}

	void readReady( SelectFd *fd )
	{
		log_message( "read ready" );
		wantReadSet( false );
	}
};


int MainThread::main()
{
	log_message( "starting up" );
	
	Sleep sleep( this, "echo hi; sleep 5s" );
	createProcess( &sleep );

	sleep.wantWriteSet( true );
	sleep.wantReadSet( true );

	selectLoop();

	log_message( "exiting" );

	return 0;
}
