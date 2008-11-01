namespace ns1
{
	namespace sub1 { class A {}; }
	namespace sub2 { class B {}; }
}

namespace ns2
{
//	int i = b;
	class C
	{
	};

	using namespace ns1;
}

ns2::sub1::A a;
