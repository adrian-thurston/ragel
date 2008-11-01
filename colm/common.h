/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <fstream>
#include <climits>

enum MarkType
{
	MarkEnter,
	MarkLeave
};

typedef unsigned long long Size;

struct Key
{
private:
	long key;

public:
	friend inline Key operator+(const Key key1, const Key key2);
	friend inline Key operator-(const Key key1, const Key key2);
	friend inline Key operator/(const Key key1, const Key key2);
	friend inline long operator&(const Key key1, const Key key2);

	friend inline bool operator<( const Key key1, const Key key2 );
	friend inline bool operator<=( const Key key1, const Key key2 );
	friend inline bool operator>( const Key key1, const Key key2 );
	friend inline bool operator>=( const Key key1, const Key key2 );
	friend inline bool operator==( const Key key1, const Key key2 );
	friend inline bool operator!=( const Key key1, const Key key2 );

	friend struct KeyOps;
	
	Key( ) {}
	Key( const Key &key ) : key(key.key) {}
	Key( long key ) : key(key) {}

	/* Returns the value used to represent the key. This value must be
	 * interpreted based on signedness. */
	long getVal() const { return key; };

	/* Returns the key casted to a long long. This form of the key does not
	 * require and signedness interpretation. */
	long long getLongLong() const;

	bool isUpper() const { return ( 'A' <= key && key <= 'Z' ); }
	bool isLower() const { return ( 'a' <= key && key <= 'z' ); }
	bool isPrintable() const { return ( 32 <= key && key < 127 ); }

	Key toUpper() const
		{ return Key( 'A' + ( key - 'a' ) ); }
	Key toLower() const
		{ return Key( 'a' + ( key - 'A' ) ); }

	void operator+=( const Key other )
	{
		/* FIXME: must be made aware of isSigned. */
		key += other.key;
	}

	void operator-=( const Key other )
	{
		/* FIXME: must be made aware of isSigned. */
		key -= other.key;
	}

	void operator|=( const Key other )
	{
		/* FIXME: must be made aware of isSigned. */
		key |= other.key;
	}

	/* Decrement. Needed only for ranges. */
	inline void decrement();
	inline void increment();
};

struct HostType
{
	char *data1;
	char *data2;
	bool isSigned;
	long long minVal;
	long long maxVal;
	unsigned int size;
};

struct HostLang
{
	HostType *hostTypes;
	int numHostTypes;
	HostType *defaultAlphType;
	bool explicitUnsigned;
};


/* Target language. */
enum HostLangType
{
	CCode,
	DCode,
	JavaCode,
	RubyCode
};

extern HostLang *hostLang;
extern HostLangType hostLangType;

extern HostLang hostLangC;
extern HostLang hostLangD;
extern HostLang hostLangJava;
extern HostLang hostLangRuby;

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
			if ( ( isSigned && hostLang->hostTypes[i].isSigned || !isSigned ) &&
					maxVal <= hostLang->hostTypes[i].maxVal )
				return hostLang->hostTypes + i;
		}
		return 0;
	}
};

extern KeyOps *keyOps;

inline bool operator<( const Key key1, const Key key2 )
{
	return keyOps->isSigned ? key1.key < key2.key : 
		(unsigned long)key1.key < (unsigned long)key2.key;
}

inline bool operator<=( const Key key1, const Key key2 )
{
	return keyOps->isSigned ?  key1.key <= key2.key : 
		(unsigned long)key1.key <= (unsigned long)key2.key;
}

inline bool operator>( const Key key1, const Key key2 )
{
	return keyOps->isSigned ? key1.key > key2.key : 
		(unsigned long)key1.key > (unsigned long)key2.key;
}

inline bool operator>=( const Key key1, const Key key2 )
{
	return keyOps->isSigned ? key1.key >= key2.key : 
		(unsigned long)key1.key >= (unsigned long)key2.key;
}

inline bool operator==( const Key key1, const Key key2 )
{
	return key1.key == key2.key;
}

inline bool operator!=( const Key key1, const Key key2 )
{
	return key1.key != key2.key;
}

/* Decrement. Needed only for ranges. */
inline void Key::decrement()
{
	key = keyOps->isSigned ? key - 1 : ((unsigned long)key)-1;
}

/* Increment. Needed only for ranges. */
inline void Key::increment()
{
	key = keyOps->isSigned ? key+1 : ((unsigned long)key)+1;
}

inline long long Key::getLongLong() const
{
	return keyOps->isSigned ? (long long)key : (long long)(unsigned long)key;
}

inline Key operator+(const Key key1, const Key key2)
{
	/* FIXME: must be made aware of isSigned. */
	return Key( key1.key + key2.key );
}

inline Key operator-(const Key key1, const Key key2)
{
	/* FIXME: must be made aware of isSigned. */
	return Key( key1.key - key2.key );
}

inline long operator&(const Key key1, const Key key2)
{
	/* FIXME: must be made aware of isSigned. */
	return key1.key & key2.key;
}

inline Key operator/(const Key key1, const Key key2)
{
	/* FIXME: must be made aware of isSigned. */
	return key1.key / key2.key;
}

const char *findFileExtension( const char *stemFile );
char *fileNameFromStem( const char *stemFile, const char *suffix );

#endif /* _COMMON_H */
