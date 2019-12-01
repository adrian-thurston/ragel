namespace ns1
{
	namespace sub1 { class A {}; }
	namespace sub2 { class B {}; }
}

namespace ns2
{
	int i = b;
	class C
	{
	};

	using namespace ns1;
}

ns2::sub1::A a;

struct A
{
	struct B {};
};

struct C
{
	struct D : virtual public A {};
};

C::D::A d;

C c;

struct C
{

};

enum E
{
	C,
	b
};

E e;

enum E
{
	C,
	b
};


int i;
class C
{
	int j;
};

class D
{
	int ~D();
};

int C::k;
int C::~C;

typedef int Int;

class C {};
void ~C( );
void C::operator +( int i );

int i;

//void operator C( void k );

class C
{
	
};

int C::f( int i, int j( void v ) );
class C 
{
	class D {};

	typedef C I;

	I::D i;
};

C c;

void function( int i, int j )
{
	function();
}



class B { class Find {}; };

typedef B T;

class C : public T
{
	Find find;
};


template <class X> struct Y
{
	X t;
	void f();
};

template <class X> void Y<X>::f();
template <class X> struct Y
{
	class Z {};
};

class Y<int>
{
	int i;
};

//void f( class C<int> i, int j );

int f( int (*) [](), void );
void f();
class C
{
	class D {};
	void g();
};

//typename C c;

class C
{
	class D {};
	int f();
};

int f()
{
}

int C::f()
{
	D d;
}
