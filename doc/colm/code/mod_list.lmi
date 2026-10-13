# A list of items, and a function that counts them. The namespace that
# includes this file defines the token item.
def items
	[item*]

int count( L: items )
{
	N: int = 0
	for I: item in L
		N = N + 1
	return N
}
