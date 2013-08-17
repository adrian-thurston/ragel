/*
 *  Copyright 2001-2006, 2013 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <fstream>
#include <climits>
#include "dlist.h"

struct colm_location;

/* Location in an input file. */
struct InputLoc
{
	InputLoc( colm_location *pcloc );

	InputLoc() : fileName(0), line(-1), col(-1)  {}

	InputLoc( const InputLoc &loc )
	{
		fileName = loc.fileName;
		line = loc.line;
		col = loc.col;
	}

	InputLoc( const char *fileName, int line, int col )
		: fileName(fileName), line(line), col(col) {}

	const char *fileName;
	int line;
	int col;
};

extern InputLoc internal;

typedef unsigned long long Size;

struct Key
{
private:
	long key;

public:
	friend struct KeyOps;
	
	Key( ) {}
	Key( const Key &key ) : key(key.key) {}
	Key( long key ) : key(key) {}

	/* Returns the value used to represent the key. This value must be
	 * interpreted based on signedness. */
	long getVal() const { return key; };

	bool isUpper() const { return ( 'A' <= key && key <= 'Z' ); }
	bool isLower() const { return ( 'a' <= key && key <= 'z' ); }
	bool isPrintable() const
	{
	    return ( 7 <= key && key <= 13 ) || ( 32 <= key && key < 127 );
	}

	Key toUpper() const
		{ return Key( 'A' + ( key - 'a' ) ); }
	Key toLower() const
		{ return Key( 'a' + ( key - 'A' ) ); }
};

struct CondKey
{
private:
	long key;

public:
	friend inline bool operator<( const CondKey key1, const CondKey key2 );
	friend inline bool operator>( const CondKey key1, const CondKey key2 );
	friend inline bool operator==( const CondKey key1, const CondKey key2 );
	friend inline CondKey operator+( const CondKey key1, const CondKey key2 );
	friend inline CondKey operator-( const CondKey key1, const CondKey key2 );

	friend struct KeyOps;
	
	CondKey( ) {}
	CondKey( const CondKey &key ) : key(key.key) {}
	CondKey( long key ) : key(key) {}

	/* Returns the value used to represent the key. This value must be
	 * interpreted based on signedness. */
	long getVal() const { return key; };

	bool isUpper() const { return ( 'A' <= key && key <= 'Z' ); }
	bool isLower() const { return ( 'a' <= key && key <= 'z' ); }
	bool isPrintable() const
	{
	    return ( 7 <= key && key <= 13 ) || ( 32 <= key && key < 127 );
	}

	CondKey toUpper() const
		{ return CondKey( 'A' + ( key - 'a' ) ); }
	CondKey toLower() const
		{ return CondKey( 'a' + ( key - 'A' ) ); }

	/* Decrement. Needed only for ranges. */
	inline void decrement();
	inline void increment();
};

inline CondKey operator+(const CondKey key1, const CondKey key2)
{
	return CondKey( key1.key + key2.key );
}

inline CondKey operator-(const CondKey key1, const CondKey key2)
{
	return CondKey( key1.key - key2.key );
}


struct HostType
{
	const char *data1;
	const char *data2;
	const char *internalName;
	bool isSigned;
	bool isOrd;
	bool isChar;
	long long minVal;
	long long maxVal;
	unsigned int size;
};

struct HostLang
{
	/* Target language. */
	enum Lang
	{
		C, D, D2, Go, Java, Ruby, CSharp, OCaml, Crack
	};

	Lang lang;
	HostType *hostTypes;
	int numHostTypes;
	HostType *defaultAlphType;
	bool explicitUnsigned;
};

extern HostLang *hostLang;

extern HostLang hostLangC;
extern HostLang hostLangD;
extern HostLang hostLangD2;
extern HostLang hostLangGo;
extern HostLang hostLangJava;
extern HostLang hostLangRuby;
extern HostLang hostLangCSharp;
extern HostLang hostLangOCaml;
extern HostLang hostLangCrack;

HostType *findAlphType( const char *s1 );
HostType *findAlphType( const char *s1, const char *s2 );
HostType *findAlphTypeInternal( const char *s1 );

/* An abstraction of the key operators that manages key operations such as
 * comparison and increment according the signedness of the key. */
struct KeyOps
{
	/* Default to signed alphabet. */
	KeyOps() :
		isSigned(true),
		alphType(0)
	{}

	/* Default to signed alphabet. */
	KeyOps( bool isSigned ) 
		:isSigned(isSigned) {}

	bool isSigned;
	Key minKey, maxKey;
	HostType *alphType;

	void setAlphType( HostType *alphType )
	{
		this->alphType = alphType;
		isSigned = alphType->isSigned;
		if ( isSigned ) {
			minKey = (long) alphType->minVal;
			maxKey = (long) alphType->maxVal;
		}
		else {
			minKey = (long) (unsigned long) alphType->minVal; 
			maxKey = (long) (unsigned long) alphType->maxVal;
		}
	}

	/* Compute the distance between two keys. */
	Size span( Key key1, Key key2 )
	{
		return isSigned ? 
			(unsigned long long)(
				(long long)key2.key - 
				(long long)key1.key + 1) : 
			(unsigned long long)(
				(unsigned long)key2.key) - 
				(unsigned long long)((unsigned long)key1.key) + 1;
	}

	Size alphSize()
		{ return span( minKey, maxKey ); }

	HostType *typeSubsumes( long long maxVal )
	{
		for ( int i = 0; i < hostLang->numHostTypes; i++ ) {
			if ( maxVal <= hostLang->hostTypes[i].maxVal )
				return hostLang->hostTypes + i;
		}
		return 0;
	}

	HostType *typeSubsumes( bool isSigned, long long maxVal )
	{
		for ( int i = 0; i < hostLang->numHostTypes; i++ ) {
			if ( ( ( isSigned && hostLang->hostTypes[i].isSigned ) || !isSigned ) &&
					maxVal <= hostLang->hostTypes[i].maxVal )
				return hostLang->hostTypes + i;
		}
		return 0;
	}

	inline bool lt( const Key key1, const Key key2 )
	{
		return this->isSigned ? key1.key < key2.key : 
			(unsigned long)key1.key < (unsigned long)key2.key;
	}

	inline bool le( const Key key1, const Key key2 )
	{
		return this->isSigned ?  key1.key <= key2.key : 
			(unsigned long)key1.key <= (unsigned long)key2.key;
	}

	inline bool gt( const Key key1, const Key key2 )
	{
		return this->isSigned ? key1.key > key2.key : 
			(unsigned long)key1.key > (unsigned long)key2.key;
	}

	inline bool ge( const Key key1, const Key key2 )
	{
		return this->isSigned ? key1.key >= key2.key : 
			(unsigned long)key1.key >= (unsigned long)key2.key;
	}

	inline bool eq( const Key key1, const Key key2 )
	{
		return key1.key == key2.key;
	}

	inline bool ne( const Key key1, const Key key2 )
	{
		return key1.key != key2.key;
	}

	inline Key add(const Key key1, const Key key2)
	{
		/* FIXME: must be made aware of isSigned. */
		return Key( key1.key + key2.key );
	}

	inline Key sub(const Key key1, const Key key2)
	{
		/* FIXME: must be made aware of isSigned. */
		return Key( key1.key - key2.key );
	}

	/* Decrement. Needed only for ranges. */
	inline void decrement( Key &key )
	{
		key.key = this->isSigned ? key.key - 1 : ((unsigned long)key.key)-1;
	}

	/* Increment. Needed only for ranges. */
	inline void increment( Key &key )
	{
		key.key = this->isSigned ? key.key+1 : ((unsigned long)key.key)+1;
	}

	/* Returns the key casted to a long long. This form of the key does not
	 * require any signedness interpretation. */
	inline long long getLongLong( const Key &key )
	{
		return this->isSigned ? (long long)key.key : (long long)(unsigned long)key.key;
	}
};

/* CondKey */

inline bool operator<( const CondKey key1, const CondKey key2 )
{
	return key1.key < key2.key;
}

inline bool operator>( const CondKey key1, const CondKey key2 )
{
	return key1.key > key2.key;
}

inline bool operator==( const CondKey key1, const CondKey key2 )
{
	return key1.key == key2.key;
}

/* Increment. Needed only for ranges. */
inline void CondKey::increment()
{
	key = key + 1;
}


/* Filter on the output stream that keeps track of the number of lines
 * output. */
class output_filter : public std::filebuf
{
public:
	output_filter( const char *fileName ) : fileName(fileName), line(1) { }

	virtual int sync();
	virtual std::streamsize xsputn(const char* s, std::streamsize n);

	const char *fileName;
	int line;
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

struct Export
{
	Export( const char *name, Key key )
		: name(name), key(key) {}

	const char *name;
	Key key;

	Export *prev, *next;
};

typedef DList<Export> ExportList;

struct exit_object { };
extern exit_object endp;
void operator<<( std::ostream &out, exit_object & );

#endif
