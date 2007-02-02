%%{
	machine uri;

	action scheme {}
	action loc {}
	action item {}
	action query {}
	action last {}
	action nothing {}

	main :=
		# Scheme machine. This is ambiguous with the item machine. We commit
		# to the scheme machine on colon.
		( [^:/?#]+ ':' @(colon,1) @scheme )?

		# Location machine. This is ambiguous with the item machine. We remain
		# ambiguous until a second slash, at that point and all points after
		# we place a higher priority on staying in the location machine over
		# moving into the item machine.
		( ( '/' ( '/' [^/?#]* ) $(loc,1) ) %loc %/loc )? 

		# Item machine. Ambiguous with both scheme and location, which both
		# get a higher priority on the characters causing ambiguity.
		( ( [^?#]+ ) $(loc,0) $(colon,0) %item %/item )? 

		# Last two components, the characters that initiate these machines are
		# not supported in any previous components, therefore there are no
		# ambiguities introduced by these parts.
		( '?' [^#]* %query %/query)?
		( '#' any* %/last )?;
}%%
