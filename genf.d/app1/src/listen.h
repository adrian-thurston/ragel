#ifndef _LISTEN_H
#define _LISTEN_H

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/ssl.h>

#include "listen_gen.h"
#include <aapl/dlistval.h>
#include <aapl/avlmap.h>
#include <list>

struct BpConnection;

struct ListenThread
	: public ListenGen
{
	ListenThread()
	{
		recvRequiresSignal = true;
		bpConnection = 0;
	}

	int main();

	void handleTimer();
	void recvShutdown( Message::Shutdown *msg );
	void recvPassthru( Message::PacketPassthru *msg );

	BpConnection *bpConnection;
};

#endif /* _LISTEN_H */
