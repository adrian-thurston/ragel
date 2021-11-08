#ifndef _NRAGEL_H
#define _NRAGEL_H

#include <libfsm/common.h>
#include <libfsm/ragel.h>

struct HostLang;

HostType *findAlphType( const HostLang *hostLang, const char *s1 );
HostType *findAlphType( const HostLang *hostLang, const char *s1, const char *s2 );
HostType *findAlphTypeInternal( const HostLang *hostLang, const char *s1 );

CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args );
CodeGenData *makeCodeGenAsm( const HostLang *hostLang, const CodeGenArgs &args );

/* Selects and constructs the codegen based on the output options. */
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args );
CodeGenData *asm_makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args );

typedef CodeGenData *(*MakeCodeGenT)( const HostLang *hostLang, const CodeGenArgs &args );

typedef void (*GenLineDirectiveT)( std::ostream &out, bool nld, int line, const char *file );
typedef const char *(*DefaultOutFnT)( const char *inputFileName );

struct HostLang
{
	HostType *hostTypes;
	int numHostTypes;
	int defaultAlphType;
	bool explicitUnsigned;
	bool loopLabels;

	RagelBackend backend;
	BackendFeature feature;

	MakeCodeGenT makeCodeGen;
	DefaultOutFnT defaultOutFn;
	GenLineDirectiveT genLineDirective;
};

void genLineDirectiveC( std::ostream &out, bool nld, int line, const char *file );
void genLineDirectiveAsm( std::ostream &out, bool nld, int line, const char *file );
void genLineDirectiveTrans( std::ostream &out, bool nld, int line, const char *file );

/* Filter on the output stream that keeps track of the number of lines
 * output. */
class output_filter
:	
	public std::filebuf
{
public:
	output_filter( const char *fileName )
	:
		fileName(fileName),
		line(1),
		level(0),
		indent(false),
		singleIndent(false)
	{}

	virtual int sync();
	virtual std::streamsize xsputn( const char* s, std::streamsize n );

	std::streamsize countAndWrite( const char* s, std::streamsize n );

	const char *fileName;
	int line;
	int level;
	bool indent;
	bool singleIndent;
};

class cfilebuf : public std::streambuf
{
public:
	cfilebuf( char *fileName, FILE* file ) : fileName(fileName), file(file) { }
	char *fileName;
	FILE *file;

	int sync()
	{
		fflush( file );
		return 0;
	}

	int overflow( int c )
	{
		if ( c != EOF )
			fputc( c, file );
		return 0;
	}

	std::streamsize xsputn( const char* s, std::streamsize n )
	{
		std::streamsize written = fwrite( s, 1, n, file );
		return written;
	}
};

class costream : public std::ostream
{
public:
	costream( cfilebuf *b ) : 
		std::ostream(b), b(b) {}
	
	~costream()
		{ delete b; }

	void fclose()
		{ ::fclose( b->file ); }

	cfilebuf *b;
};

const char *findFileExtension( const char *stemFile );
const char *fileNameFromStem( const char *stemFile, const char *suffix );

#endif
