#include <pcap.h>

/*
 * Break up a dump file and strip headers, leaving just 
 * the dns portion of packets.
 */

char outname[1024];
char errbuf[PCAP_ERRBUF_SIZE];

int main( int argc, char **argv )
{
	int packet;
	pcap_t *p;
	if ( argc != 3 ) {
		fprintf( stderr, "usage: get <dumpfile> <rootname>\n" );
		return 1;
	}

	p = pcap_open_offline( argv[1], errbuf );

	for ( packet = 0; ; packet++ ) {
		FILE *file;
		unsigned long len;
		struct pcap_pkthdr h;
		const u_char *data;
		
		data = pcap_next( p, &h );
		if ( data == 0 )
			break;

		if ( h.caplen < h.len )
			fprintf( stderr, "warning: packet number %02d is short\n", packet );

		/* The magic number is the size of the headers we want to strip. */
		data += 42;
		len = h.caplen - 42;

		sprintf( outname, "%s-%04d", argv[2], packet );
		file = fopen( outname, "wb" );
		fwrite( data, 1, len, file );
		fclose( file );
	}

	pcap_close( p );

	return 0;
}
