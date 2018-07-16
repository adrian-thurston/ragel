
option long l: -l --long;
option bool fetch: --fetch;

thread User;
thread Listen;

debug IP;
debug TCP;

message Shutdown
{
};

message Hello
{
};

packet BigPacket
{
	string  big1;
	long    l1;
	string  big2;
	long    l2;
	string  big3;
	long    l3;
	char(8) c1;
};

struct SmallRec
{
	long l1;
	char(8) c1;
};

packet SmallPacket
{
	long l1;
	long l2;
	long l3;
	list<SmallRec> sr;
};

Main starts User;
Main starts Listen;

Main sends Hello to User;

Main sends Shutdown to User;
Main sends Shutdown to Listen;

Listen sends BigPacket;
Listen sends SmallPacket;
Main receives BigPacket;
Main receives SmallPacket;

packet BigPacket 1;
packet SmallPacket 2;
