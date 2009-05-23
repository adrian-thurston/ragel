#ifndef _CPPSCAN1_H
#define _CPPSCAN1_H

#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace std;

#define BUFSIZE 2048

#define TK_Dlit 192
#define TK_Slit 193
#define TK_Float 194
#define TK_Id 195
#define TK_NameSep 197
#define TK_Arrow 211
#define TK_PlusPlus 212
#define TK_MinusMinus 213
#define TK_ArrowStar 214
#define TK_DotStar 215
#define TK_ShiftLeft 216
#define TK_ShiftRight 217
#define TK_IntegerDecimal 218
#define TK_IntegerOctal 219
#define TK_IntegerHex 220
#define TK_EqualsEquals 223
#define TK_NotEquals 224
#define TK_AndAnd 225
#define TK_OrOr 226
#define TK_MultAssign 227
#define TK_DivAssign 228
#define TK_PercentAssign 229
#define TK_PlusAssign 230
#define TK_MinusAssign 231
#define TK_AmpAssign 232
#define TK_CaretAssign 233
#define TK_BarAssign 234
#define TK_DotDotDot 240

/* A growable buffer for collecting headers. */
struct Buffer
{
	Buffer() : data(0), allocated(0), length(0) { }
	Buffer( const Buffer &other ) {
		data = (char*)malloc( other.allocated );
		memcpy( data, other.data, other.length );
		allocated = other.allocated;
		length = other.length;
	}
	~Buffer() { empty(); }

	void append( char p ) {
		if ( ++length > allocated )
			upAllocate( length*2 );
		data[length-1] = p;
	}
	void append( char *str, int len ) {
		if ( (length += len) > allocated )
			upAllocate( length*2 );
		memcpy( data+length-len, str, len );
	}
		
	void clear() { length = 0; }
	void upAllocate( int len );
	void empty();

	char *data;
	int allocated;
	int length;
};


struct Scanner
{
	Scanner( std::ostream &out )
		: out(out) { }

	std::ostream &out;

	int line, col;
	int tokStart;
	int inlineDepth;
	int count;
	Buffer tokBuf;
	Buffer nonTokBuf;

	void pass(char c) { nonTokBuf.append(c); }
	void buf(char c) { tokBuf.append(c); }
	void token( int id );

	int cs, stack, top;

	// Initialize the machine. Invokes any init statement blocks. Returns 0
	// if the machine begins in a non-accepting state and 1 if the machine
	// begins in an accepting state.
	void init( );

	// Execute the machine on a block of data. Returns -1 if after processing
	// the data, the machine is in the error state and can never accept, 0 if
	// the machine is in a non-accepting state and 1 if the machine is in an
	// accepting state.
	int execute( const char *data, int len );

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( );
};

#endif
