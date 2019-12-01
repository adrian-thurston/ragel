#include <string.h>
#include <iostream>
#include <stdio.h>
#include <pcap.h>

#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <arpa/inet.h>


struct Node
{
	enum Type
	{
		DnsPacket = 1,
		DnsQuestion,
		DnsAnswer,
		DnsAuthority,
		DnsAdditional,
		DnsRrA,
		DnsRrAAAA,
		DnsRrCname,

		DnsName,
		DnsAddr,
	};
};

const char *names[] = {
	"<>",
	"DnsPacket",
	"DnsQuestion",
	"DnsAnswer",
	"DnsAuthority",
	"DnsAdditional",
	"DnsRrA",
	"DnsRrAAAA",
	"DnsRrCname",
	"DnsName",
	"DnsAddr",
};

struct VPT
{
	void push( Node::Type type )
	{
		std::cout << "push " << names[type] << std::endl;
	}

	void pop( Node::Type type )
	{
		std::cout << "pop " << names[type] << std::endl;
	}

	void setText( const char *txt )
	{
		std::cout << "setText: " << txt << std::endl;
	}

	void setData( const unsigned char *d, int len )
	{
		std::cout << "setData: ";
		for ( int i = 0; i < len; i++ )
			std::cout << std::hex << (unsigned int)d[i] << ' ';
		std::cout << std::dec << std::endl;
	}
};

struct Context
{
	VPT vpt;	
};

struct Packet
{

};

typedef unsigned char wire_t;

struct DnsSection
{
	unsigned long count;
	Node::Type tree;
};

struct DnsParser
{
	DnsParser() {}

	int cs;

	unsigned int names[32];
	int n;
	const unsigned char *packet_start;

	unsigned long id;
	unsigned long options;

	/* Current section type. indexes into sect and rr arrays. */
	int s;

	static const int nsect = 4;
	static const int qd = 0, an = 1;
	static const int ns = 2, ar = 3;

	DnsSection sect[nsect];
	static const char *rr[nsect];

	Context ctx;

	int copyName( char *nb, int max, const unsigned char *ptr );

	const unsigned char *header( Context *ctx, Packet *packet,
			const unsigned char *p, const unsigned char *pe );
	const unsigned char *question( Context *ctx, Packet *packet,
			const unsigned char *p, const unsigned char *pe );
	const unsigned char *resource_record( Context *ctx, Packet *packet,
			const unsigned char *p, const unsigned char *pe );

	int data( Context *ctx, Packet *packet, const wire_t *data, int length );
};


const char *DnsParser::rr[] = {
	"QD", "AN", "NS", "AR"
};

enum RR
{
	A = 1, CNAME, OTHER
};


%%{
	machine dns;

	alphtype unsigned char;

	octet = any;

	# Tokens generated from RR_UNKNOWN. Used to pick the kind 
	# of resource record to attempt to parse.
	# RR_A     = 1;   # 1 a host address
	# RR_NS    = 2;   # 2 an authoritative name server
	# RR_MD    = 3;   # 3 a mail destination (Obsolete - use MX)
	# RR_MF    = 4;   # 4 a mail forwarder (Obsolete - use MX)
	# RR_CNAME = 5;   # 5 the canonical name for an alias
	# RR_SOA   = 6;   # 6 marks the start of a zone of authority
	# RR_MB    = 7;   # 7 a mailbox domain name (EXPERIMENTAL)
	# RR_MG    = 8;   # 8 a mail group member (EXPERIMENTAL)
	# RR_MR    = 9;   # 9 a mail rename domain name (EXPERIMENTAL)
	# RR_NULL  = 10;  # 10 a null RR (EXPERIMENTAL)
	# RR_WKS   = 11;  # 11 a well known service description
	# RR_PTR   = 12;  # 12 a domain name pointer
	# RR_HINFO = 13;  # 13 host information
	# RR_MINFO = 14;  # 14 mailbox or mail list information
	# RR_MX    = 15;  # 15 mail exchange
	# RR_TXT   = 16;  # 16 text strings

	RR_A      = 0x00 0x01;
	RR_NS     = 0x00 0x02;
	RR_CNAME  = 0x00 0x05;
	RR_PTR    = 0x00 0x0c;
	RR_MX     = 0x00 0x0f;
	RR_AAAA   = 0x00 0x1c;

	RR_ALL     = RR_A | RR_NS | RR_CNAME | RR_PTR | RR_MX | RR_AAAA;
	RR_UNKNOWN = ( octet octet ) - RR_ALL;

	#
	# Names
	#

	action part_len {
		nbytes = *p;
	}

	part_len =
		( 0x1 .. 0x3f ) @part_len;

	action nb_init { b = 0; }
	action nb_inc { b++; }
	action nb_min { b >= nbytes }
	action nb_max { b < nbytes }

	nbytes = :condplus( octet, nb_init, nb_inc, nb_min, nb_max ):;

	name_part =
		part_len nbytes;

	# Name part lists are terminated by a zero length or a pointer.
	name_end =
		# Zero length ending
		0

		# Pointer ending
		#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
		#   | 1  1|                OFFSET                   |
		#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|	( 0xc0 .. 0xff ) octet;

	action name_begin
	{
		name_start = p;
	}

	# Copy names into 'namebuf'. This can be a pointer into a current buffer,
	# in case there are multiple names we need.
	action name_end {
		if ( namebuf != 0 ) {
			int r = copyName( namebuf, nblen, name_start );

			/* Print names from the answer section. */
			if ( r < 0 ) {
				std::cerr << "error copying name" << std::endl;
				std::cerr << "  " << rr[s] << " name: " <<
						namebuf << " off: " << name_start - packet_start << std::endl;
			}
		}

		/* Register the parts of the name. Doing this after the above copy
		 * prevents a self-reference. */
		const unsigned char *s = name_start;
		while ( true ) {
			unsigned int sl = *s;
			if ( *s < 1 || *s > 63 )
				break;
			else {
				unsigned int off = s - packet_start;
				names[n++] = off;
				s += 1 + sl;
			}

		}
	}

	name =
		( name_part** <: name_end ) >name_begin @name_end;


	#    Message Header
	#
	#                                    1  1  1  1  1  1
	#      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
	#    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#    |                      ID                       |
	#    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
	#    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#    |                    QDCOUNT                    |
	#    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#    |                    ANCOUNT                    |
	#    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#    |                    NSCOUNT                    |
	#    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#    |                    ARCOUNT                    |
	#    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

	header_id =
		octet @{ id = (unsigned long)*p << 8; }
		octet @{ id |= (unsigned long)*p; };

	header_fields =
		octet @{ options = (unsigned long)*p << 8; }
		octet @{ options |= (unsigned long)*p; };

	count =
		octet @{ count = (unsigned long)*p << 8; }
		octet @{ count |= (unsigned long)*p; };

	header =
		header_id header_fields
		count @{ sect[qd].count = count; sect[qd].tree = Node::DnsQuestion; }
		count @{ sect[an].count = count; sect[an].tree = Node::DnsAnswer; }
		count @{ sect[ns].count = count; sect[ns].tree = Node::DnsAuthority; }
		count @{ sect[ar].count = count; sect[ar].tree = Node::DnsAdditional; }
		@{
			std::cout << "dns header:"
				" id: " << id << " opts: " << options <<
				" qd: " << sect[qd].count << " an: " << sect[an].count <<
				" ns: " << sect[ns].count << " ar: " << sect[ar].count << std::endl;
			fbreak;
		};

	#
	#   Question
	#

	#                                   1  1  1  1  1  1
	#     0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#   |                                               |
	#   /                     QNAME                     /
	#   /                                               /
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#   |                     QTYPE                     |
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#   |                     QCLASS                    |
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

	qtype =
		octet octet;

	qclass =
		octet octet;

	question =
		name qtype qclass
		@{
			std::cout << "  QD " << namebuf << std::endl;
			fbreak;
		};

	#
	# Address
	#
	address4 =
		octet @{ addr[0] = *p; }
		octet @{ addr[1] = *p; }
		octet @{ addr[2] = *p; }
		octet @{ addr[3] = *p; }
	;

	address6 =
		octet @{ addr[ 0] = *p; } octet @{ addr[ 1] = *p; }
		octet @{ addr[ 2] = *p; } octet @{ addr[ 3] = *p; }
		octet @{ addr[ 4] = *p; } octet @{ addr[ 5] = *p; }
		octet @{ addr[ 6] = *p; } octet @{ addr[ 7] = *p; }
		octet @{ addr[ 8] = *p; } octet @{ addr[ 9] = *p; }
		octet @{ addr[10] = *p; } octet @{ addr[11] = *p; }
		octet @{ addr[12] = *p; } octet @{ addr[13] = *p; }
		octet @{ addr[14] = *p; } octet @{ addr[15] = *p; }
	;


	#
	#   Resource Records
	#

	#                                   1  1  1  1  1  1
	#     0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#   |                                               |
	#   /                                               /
	#   /                      NAME                     /
	#   |                                               |
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#   |                      TYPE                     |
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#   |                     CLASS                     |
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#   |                      TTL                      |
	#   |                                               |
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	#   |                   RDLENGTH                    |
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
	#   /                     RDATA                     /
	#   /                                               /
	#   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

	rr_type =
		octet octet;

	rr_class =
		octet octet;

	ttl =
		octet octet octet octet;

	rdlength =
		octet @{ rdlength = (unsigned long)*p << 8; }
		octet @{ rdlength |= (unsigned long)*p; };

	rdata_bytes =
		/''/;

	rr_a =
		RR_A rr_class ttl rdlength
		address4
		@{
			std::cout << "  " << rr[s] << " A record: " << namebuf1 << " " <<
					(unsigned int)addr[0] << '.' <<
					(unsigned int)addr[1] << '.' <<
					(unsigned int)addr[2] << '.' <<
					(unsigned int)addr[3] << std::endl;
			fbreak;
		};

	rr_aaaa =
		RR_AAAA rr_class ttl rdlength
		address6
		@{
			std::cout << "  " << rr[s] << " AAAA record: " << namebuf1 << " " << std::hex <<
					(uint)addr[ 0] << ':' << (uint)addr[ 1] << ':' <<
					(uint)addr[ 2] << ':' << (uint)addr[ 3] << ":" <<
					(uint)addr[ 4] << ':' << (uint)addr[ 5] << ':' <<
					(uint)addr[ 6] << ':' << (uint)addr[ 7] << ":" <<
					(uint)addr[ 8] << ':' << (uint)addr[ 9] << ':' <<
					(uint)addr[10] << ':' << (uint)addr[11] << ":" <<
					(uint)addr[12] << ':' << (uint)addr[13] << ':' <<
					(uint)addr[14] << ':' << (uint)addr[15] << std::dec << std::endl;

			fbreak;
		};


	# Skip collection of second name by setting namebuf = 0; */
	rr_ns =
		RR_NS @{ namebuf = 0; } rr_class ttl rdlength
		name
		@{ fbreak; };

	rr_cname =
		RR_CNAME @{ namebuf = namebuf2; } rr_class ttl rdlength
		name
		@{
			std::cout << "  " << rr[s] << " CNAME record: " << namebuf1 << " " << namebuf2 << std::endl;
			fbreak;
		};

	rr_ptr =
		RR_PTR @{ namebuf = namebuf2; } rr_class ttl rdlength
		name
		@{
			std::cout << "  " << rr[s] << " PTR record: " << namebuf1 << " " << namebuf2 << std::endl;
			fbreak;
		};

	preference = octet octet;

	rr_mx =
		RR_MX @{ namebuf = 0; } rr_class ttl rdlength
		preference name
		@{ fbreak; };

	rr_unknown =
		RR_UNKNOWN rr_class ttl rdlength
		@{
			std::cout << "skipping other section" << std::endl;

			if ( p + rdlength < pe ) {
				/* Position ourselves on the last item and break. Implied
				 * advance of fbreak will take p to the next char to parse. */
				p = p + rdlength;
				fbreak;
			}
			else {
				fgoto *resource_record_error;
			}
		};

	resource_record =
		name ( rr_a | rr_ns | rr_cname | rr_ptr | rr_mx | rr_aaaa | rr_unknown );
}%%

/*
 * header
 */

%%{
	machine header;
	include dns;

	main := header;
}%%

%% write data;

const unsigned char *DnsParser::header( Context *ctx, Packet *packet, const unsigned char *p, const unsigned char *pe )
{
	unsigned long count;

	%% write init;
	%% write exec;

	if ( cs == %%{ write error; }%% ) {
		std::cerr << "header: DNS PARSE FAILED" << std::endl;
		p = 0;
	}

	return p;
}

/*
 * question
 */

%%{
	machine question;
	include dns;

	main := question;
}%%

%% write data;

const unsigned char *DnsParser::question( Context *ctx, Packet *packet,
		const unsigned char *p, const unsigned char *pe )
{
	unsigned long nbytes, b;

	const int nblen = 256;
	char namebuf[nblen];
	const unsigned char *name_start;

	%% write init;
	%% write exec;

	if ( cs == %%{ write error; }%% ) {
		// log_debug( DBG_DNS, "question: DNS PARSE FAILED" );
		return 0;
	}

	return p;
}

/*
 * resource_record
 */

%%{
	machine resource_record;
	include dns;

	main := resource_record;
}%%

%% write data;

const unsigned char *DnsParser::resource_record( Context *ctx, Packet *packet,
		const unsigned char *p, const unsigned char *pe )
{
	unsigned long nbytes, b;
	unsigned long rdlength;
	unsigned char addr[16];

	const int nblen = 256;
	char namebuf1[nblen], namebuf2[nblen];
	const unsigned char *name_start;

	char *namebuf = namebuf1;

	%% write init;
	%% write exec;
	
	if ( cs == %%{ write error; }%% ) {
		// log_debug( DBG_DNS, "resource_record: DNS PARSE FAILED: " <<
		//		p - packet_start << " " << (unsigned int)*p );
		return 0;
	}

	return p;
}

int DnsParser::copyName( char *nb, int max, const unsigned char *ptr )
{
	int dlen = 0;

	while ( true ) {
		if ( *ptr == 0 ) {
			/* done */
			nb[dlen++] = 0;
			return 0;
		}
		else if ( *ptr >= 1 && *ptr <= 63 ) {
			/* name section. */
			unsigned int pl = *ptr++;
			if ( dlen > 0 )
				nb[dlen++] = '.';
			memcpy( nb + dlen, ptr, pl );
			ptr += pl;
			dlen += pl;
		}

		else if ( *ptr >= 192 ) {
			unsigned int off = ( *ptr++ & 0x3f ) << 8;
			off |= *ptr;

			/* find in the set of names. */
			bool good = false;
			for ( int i = 0; i < n; i++ ) {
				if ( names[i] == off ) {
					/* good name. */
					good = true;
				}
			}

			if ( !good ) {
				nb[0] = 0;
				return -1;
			}

			ptr = packet_start + off;
		}
	}
	
	return 0;	
}

int DnsParser::data( Context *ctx, Packet *packet, const unsigned char *data, int length )
{
	const unsigned char *p = data;
	const unsigned char *pe = data + length;

	//ctx->vpt.push( Node::DnsPacket );

	packet_start = data;
	n = 0;

	p = header( ctx, packet, p, pe );

	if ( p == 0 )
		return -1;

	/* Iterate over the section type. */
	for ( s = 0; s < 4; s++ ) {
		if ( sect[s].count > 0 ) {

			//ctx->vpt.push( sect[s].tree );

			while ( p != 0 && sect[s].count > 0 ) {
				p = s == qd ? question( ctx, packet, p, pe ) : resource_record( ctx, packet, p, pe );
				sect[s].count -= 1;
			}

			//ctx->vpt.pop( sect[s].tree );
		}
	}

	//ctx->vpt.pop( Node::DnsPacket );

	return 0;
}

void handler( u_char *user, const struct pcap_pkthdr *h, const u_char *bytes )
{
	DnsParser *dnsParser = reinterpret_cast<DnsParser*>( user );

	const ethhdr *eh = reinterpret_cast<const ethhdr*>( bytes );

//	dnsParser->data( &dnsParser->ctx, 0, bytes,  Packet::UnknownDir, h, bytes );
	//std::cerr << "handler" << std::endl;

	if ( eh->h_proto == htons( ETH_P_IP ) ) {
		 const iphdr *ih = reinterpret_cast<const struct iphdr*>(bytes + sizeof(struct ethhdr));
		 const int ihlen = ih->ihl * 4;

		 if ( ih->protocol == IPPROTO_UDP ) {

		 	const udphdr *uh = reinterpret_cast<const struct udphdr*>( (u_char*)ih + ihlen );
			const int uhlen = sizeof( struct udphdr );

			/* TCP data and data length. */
			u_char *data = (u_char*)uh + uhlen;
			int dlen = ntohs(ih->tot_len) - uhlen - ihlen;
			int caplen = h->caplen - uhlen - ihlen - sizeof(struct ethhdr);

			if ( ntohs(uh->source) == 53 || ntohs(uh->dest) == 53 ) {
				dnsParser->data( &dnsParser->ctx, 0, data, dlen );
			}
		}
	}
}

int main( int argc, const char **argv )
{
	char errbuf[PCAP_ERRBUF_SIZE];
	DnsParser dnsParser;

	if ( argc < 2 ) {
		std::cerr << "usage: ./dns <pcap-file>" << std::endl;
		return -1;
	}

	pcap_t *pcap = pcap_open_offline( argv[1], errbuf );
	if ( pcap == NULL ) {
		std::cerr << "couldn't open file " << argv[1] << ": " << errbuf << std::endl;
		return -1;
	}

	int datalink = pcap_datalink( pcap );
	if ( datalink != DLT_EN10MB ) {
		std::cerr << "expected EN10MB datalink\n" << std::endl;
		return  -1;
	}

	int packets = pcap_dispatch( pcap, -1, &handler, (u_char*)&dnsParser );
	if ( packets < 0 ) {
		std::cerr << "error from pcap_dispatch: " << packets << std::endl;
	}

	pcap_close( pcap );
	return 0;
}
