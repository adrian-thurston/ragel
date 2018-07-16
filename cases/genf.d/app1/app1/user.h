#ifndef _USER_H
#define _USER_H

#include "user_gen.h"

struct UserThread
	: public UserGen
{
	UserThread();

	int main();

	void *cb( int status, int timeouts, unsigned char *abuf, int alen );

	void handleTimer();
	void recvShutdown( Message::Shutdown *msg );
	void recvHello( Message::Hello *msg );
};

#endif

