int main()
{
    DListVal<int> list;
    list.append( 1 );
	list.append( 2 );
	list.append( 3 );

	DListVal<int>::Iter it = list;
	while ( it.lte() && it->value != 2 )
		it++;
	
	list.remove( it.ptr );

    return 0;
}
