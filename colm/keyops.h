/*
 *  Copyright 2001-2012 Adrian Thurston <thurston@complang.org>
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

#ifndef _KEYOPS_H
#define _KEYOPS_H

#include <fstream>
#include <climits>

enum MarkType
{
	MarkNone = 0,
	MarkMark
};

typedef unsigned long long Size;

struct Key
{
private:
	long key;

public:
	friend inline Key operator+(const Key key1, const Key key2);
	friend inline Key operator-(const Key key1, const Key key2);

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
		{ key += other.key; }

	void operator-=( const Key other )
		{ key -= other.key; }

	void operator|=( const Key other )
		{ key |= other.key; }

	/* Decrement. Needed only for ranges. */
	inline void decrement();
	inline void increment();
};

struct HostType
{
	const char *data1;
	const char *data2;
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

extern HostLang *hostLang;
extern HostLang hostLangC;

/* An abstraction of the key operators that manages key operations such as
 * comparison and increment according the signedness of the key. */
struct KeyOps
{
	/* Default to signed alphabet. */
	KeyOps() : alphType(0) {}

	Key minKey, maxKey;
	HostType *alphType;

	void setAlphType( HostType *alphType )
	{
		this->alphType = alphType;
		minKey = (long) alphType->minVal;
		maxKey = (long) alphType->maxVal;
	}

	/* Compute the distance between two keys. */
	Size span( Key key1, Key key2 )
	{
		return (unsigned long long)( (long long)key2.key - (long long)key1.key + 1) ;
	}

	Size alphSize()
		{ return span( minKey, maxKey ); }
};

inline bool operator<( const Key key1, const Key key2 )
{
	return key1.key < key2.key;
}

inline bool operator<=( const Key key1, const Key key2 )
{
	return key1.key <= key2.key;
}

inline bool operator>( const Key key1, const Key key2 )
{
	return key1.key > key2.key;
}

inline bool operator>=( const Key key1, const Key key2 )
{
	return key1.key >= key2.key;
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
	key = key - 1;
}

/* Increment. Needed only for ranges. */
inline void Key::increment()
{
	key = key + 1;
}

inline long long Key::getLongLong() const
{
	return (long long) key;
}

inline Key operator+(const Key key1, const Key key2)
{
	return Key( key1.key + key2.key );
}

inline Key operator-(const Key key1, const Key key2)
{
	return Key( key1.key - key2.key );
}

const char *findFileExtension( const char *stemFile );
char *fileNameFromStem( const char *stemFile, const char *suffix );

#endif /* _KEYOPS_H */
