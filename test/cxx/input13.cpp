
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
