#include <iostream>
#include "emb_export.if.h"

int main( int argc, const char **argv )
{
	colm_program *prg = colm_new_program( &colm_object );
	colm_run_program( prg, argc, argv );

	config C = Config( prg );
	if ( C == 0 ) {
		std::cout << "error: " << Error( prg ).text() << std::endl;
	}
	else {
		for ( RepeatIter<pair> P( C ); !P.end(); P.next() )
			std::cout << P.value().Key().text() << " is "
					<< P.value().Value().text() << std::endl;

		std::string port = lookup( prg, "port" ).text();
		std::cout << "port: " << port << std::endl;
	}

	return colm_delete_program( prg );
}
