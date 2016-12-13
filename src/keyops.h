/*
 * Copyright 2001-2012 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#ifndef _COLM_KEYOPS_H
#define _COLM_KEYOPS_H

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

#endif /* _COLM_KEYOPS_H */

