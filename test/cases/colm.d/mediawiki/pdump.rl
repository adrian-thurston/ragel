#include <iostream>
#include <fstream>

using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ofstream;


#define RBS 65536

%%{
	machine pdump;
	write data;
}%%

int main( int argc, char **argv )
{
	std::ios::sync_with_stdio(false);

	if ( argc != 3 ) {
		cerr << "usage: pdump <dump-file> <article-index>" << endl;
		return -1;
	}

	ifstream input( argv[1] );
	if ( !input.is_open() ) {
		cerr << "error: unable to open " << argv[1] << " for reading" << endl;
		return -1;
	}

	ofstream output( argv[2] );
	if ( !output.is_open() ) {
		cerr << "error: unable to open " << argv[2] <<  " for writing" << endl;
		return -1;
	}

	long cs;
	%% write init;

	long long line = 1;
	long long total = 0;
	static char buf[RBS];
	while ( true ) {
		if ( input.eof() )
			break;

		input.read( buf, RBS );
		long ss = input.gcount();

		char *p = buf, *pe = buf + ss;

		%%{

		action newline { line++; }
		newline = '\n'@newline;
		any_nl = any | newline;

		sp_char = [ \t\n\r];
		sp = sp_char | newline;

		# Tag names.
		any_tag_name = [a-zA-Z:0-9]+ ;

		page_tag_name = 'page'
			%{
				long inbuf = p - buf;
				long long pos = total + inbuf;
				output.write( (char*)&pos, sizeof(long long) );
			};

		tag_name = any_tag_name | page_tag_name;

		attr_name = [a-zA-Z:0-9]+;

		# Attributes
		attr_val = '"' ( [^"\\] | newline | ( '\\' any_nl ) )* '"';
		attr = attr_name '=' attr_val;
		attrs = ( sp attr )*;

		# Tags
		tag = '<' tag_name attrs sp? ( '>' | '/>' );
		close_tag = '</' any_tag_name '>';

		# Character data, not spaces and not tag starts.
		char_data_char = ^(sp_char | '<');
		char_data = char_data_char+;

		main := ( 
				tag |
				close_tag |
				newline |
				sp |
				char_data
			)*
			;

		write exec;

		}%%

		if ( cs == pdump_error ) {
			cerr << "error:" << line << ": parse error" << endl;
			return -1;
		}

		total += ss;
	}

	input.close();
	output.close();
		
	return 0;
}
