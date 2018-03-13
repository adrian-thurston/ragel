#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>

using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ofstream;

%%{
	machine garticle;
	write data;
}%%


int main( int argc, char **argv )
{
	std::ios::sync_with_stdio(false);

	if ( argc != 5 ) {
		cerr << "usage: garticle <dump-file> <article-index> <section> <article>" << endl;
		return -1;
	}

	char *dumpFile = argv[1];
	char *articleIndex = argv[2];
	char *section = argv[3];
	char *article = argv[4];

	ifstream dump( dumpFile );
	if ( !dump.is_open() ) {
		cerr << "error: unable to open " << dumpFile << " for reading" << endl;
		return -1;
	}

	ifstream index( articleIndex );
	if ( !index.is_open() ) {
		cerr << "error: unable to open " << articleIndex <<  " for writing" << endl;
		return -1;
	}

	long long articleNum = atoll(article);
	index.seekg( articleNum * sizeof(long long) );

	long long start, end;
	index.read( (char*)&start, sizeof(long long) );
	index.read( (char*)&end, sizeof(long long) );

	long long len = end - start;
	char *buf = new char[len];
	dump.seekg( start-5 );
	dump.read( buf, len );

	char tn[2048];
	long ptn = 0;
	bool emit = false;

	char *p = buf, *pe = buf+len;
	int cs;

	%%{
		newline = '\n';
		sp = [ \t\n\r];

		name = [a-zA-Z:_0-9]+;

		# Tag names.
		tag_name = name
			>{ ptn = 0; }
			${ tn[ptn++] = *p; }
			%{ tn[ptn++] = 0; }
			;

		attr_name = name;

		# Attributes
		attr_val = '"' ( [^"\\] | newline | ( '\\' any ) )* '"';
		attr = attr_name '=' attr_val;
		attrs = ( sp attr )*;

		action maybe_open {
			if ( strcmp( tn, section ) == 0 )
				emit = true;
		}

		action maybe_close {
			if ( strcmp( tn, section ) == 0 )
				emit = false;
		}

		# Tags
		tag = '<' tag_name %maybe_open attrs sp? ( '>' | '/>' );
		close_tag = '</' tag_name %maybe_close '>';

		# Character data, not spaces and not tag starts.
		char_data_char = ^('<'|'&');
		char_data = char_data_char+
			${ 
				if ( emit )
					cout << *p;
			} ;

		defined_entities =
			'quot' %{if (emit) cout << '"';} |
			'amp' %{if (emit) cout << '&';} |
			'apos' %{if (emit) cout << '\'';} |
			'lt' %{if (emit) cout << '<';} |
			'gt' %{if (emit) cout << '>';};

		entity_ref = '&' defined_entities ';';

		main := ( 
				tag |
				close_tag |
				entity_ref |
				char_data
			)*
			;

		write init;
		write exec;
	}%%

	if ( cs < garticle_first_final ) {
		cerr << endl << endl << "garticle: error parsing dump file" << endl;
		return 1;
	}

	//cout.write( buf, len );
	cout << endl;

	return 0;
}
