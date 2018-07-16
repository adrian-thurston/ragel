#ifndef _MAIN_H
#define _MAIN_H

#include "main_gen.h"

extern const char data1[];
extern const char data2[];
extern const char data3[];

extern const long l1;
extern const long l2;
extern const long l3;

struct MainThread
	: public MainGen
{
	void handleTimer();
	void recvSmallPacket( SelectFd *fd, Record::SmallPacket *pkt );
	void recvBigPacket( SelectFd *fd, Record::BigPacket *pkt );
	int main();
	SSL_CTX *sslCtx;
	SendsPassthru *sendsPassthruToListen;
};

#endif
