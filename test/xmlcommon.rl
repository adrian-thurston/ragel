/*
 * This file is included by xml.rl
 *
 * @IGNORE: yes
 */

%%{

  #
  # Common XML grammar rules based on the XML 1.0 BNF from:
  # http://www.jelks.nu/XML/xmlebnf.html
  #

  machine CommonXml;
  
	S = (0x20 | 0x9 | 0xD | 0xA)+;

	# WAS PubidChar = 0x20 | 0xD | 0xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%];
	PubidChar = 0x20 | 0xD | 0xA | [a-zA-Z0-9] | [\-'()+,./:=?;!*#@$_%];

	PubidLiteral = '"' PubidChar* '"' | "'" (PubidChar - "'")* "'";

	Name = (Letter | '_' | ':') (NameChar)*;

	Comment = '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->';

  # Used strong subtraction operator, and replaced * with +. Ragel complained since using
  # * results in a machine that accepts 0 length strings, and later it's only used in an
  # optional construct anyway.
  #
  CharData_Old = [^<&]* - ([^<&]* ']]>' [^<&]*);
	CharData = [^<&]+ -- ']]>';

	SystemLiteral = ('"' [^"]* '"') | ("'" [^']* "'");

	Eq = S? '=' S?;

	VersionNum = ([a-zA-Z0-9_.:] | '-')+;

  # WAS S 'version' Eq (' VersionNum ' | " VersionNum ") - fixed quotes
	VersionInfo = S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"');

	ExternalID = 'SYSTEM' S SystemLiteral | 'PUBLIC' S PubidLiteral S SystemLiteral;

	PublicID = 'PUBLIC' S PubidLiteral;

	NotationDecl = '<!NOTATION' S Name S (ExternalID |  PublicID) S? '>';

	EncName = [A-Za-z] ([A-Za-z0-9._] | '-')*;

	EncodingDecl = S 'encoding' Eq ('"' EncName  '"' |  "'" EncName "'" );

	# UNUSED TextDecl = '<?xml' VersionInfo? EncodingDecl S? '?>';

	NDataDecl = S 'NDATA' S Name;

	PEReference = '%' Name ';';

	EntityRef = '&' Name ';';

	CharRef = '&#' [0-9]+ ';' | '&0x' [0-9a-fA-F]+ ';';

	Reference = EntityRef | CharRef;

	EntityValue = '"' ([^%&"] | PEReference | Reference)* '"' |  "'" ([^%&'] | PEReference | Reference)* "'";

	PEDef = EntityValue | ExternalID;

	EntityDef = EntityValue | (ExternalID NDataDecl?);

	PEDecl = '<!ENTITY' S '%' S Name S PEDef S? '>';

	GEDecl = '<!ENTITY' S Name S EntityDef S? '>';

	EntityDecl = GEDecl | PEDecl;

	Mixed = '(' S? '#PCDATA' (S? '|' S? Name)* S? ')*' | '(' S? '#PCDATA' S? ')';

	# WAS cp = (Name | choice | seq) ('?' | '*' | '+')?;

	# WAS seq = '(' S? cp ( S? ',' S? cp )* S? ')';

	# WAS choice = '(' S? cp ( S? '|' S? cp )* S? ')';

  # WAS children = (choice | seq) ('?' | '*' | '+')?;

  # TODO put validation for this in and make it clearer
  alt = '?' | '*' | '+';
	children = '(' S? 
	           ( ( Name alt? )  | 
	             '(' | 
	              ( ')' alt? ) | 
	              [,|] |
	              S )
	            ')' alt?;

	contentspec = 'EMPTY' | 'ANY' | Mixed | children;

	elementdecl = '<!ELEMENT' S Name S contentspec S? '>';

	AttValue = '"' ([^<&"] | Reference)* '"' |  "'" ([^<&'] | Reference)* "'";

	Attribute = Name Eq AttValue;

	Nmtoken = (NameChar)+;

	# UNUSED Nmtokens = Nmtoken (S Nmtoken)*;

	Enumeration = '(' S? Nmtoken (S? '|' S? Nmtoken)* S? ')';

	NotationType = 'NOTATION' S '(' S? Name (S? '|' S? Name)* S? ')';

	EnumeratedType = NotationType | Enumeration;

	TokenizedType = 'ID' | 'IDREF' | 'IDREFS' | 'ENTITY' | 'ENTITIES' | 'NMTOKEN' | 'NMTOKENS';

	StringType = 'CDATA';

	AttType = StringType | TokenizedType | EnumeratedType;

	DefaultDecl = '#REQUIRED' | '#IMPLIED' | (('#FIXED' S)? AttValue);

	AttDef = S Name S AttType S DefaultDecl;

	AttlistDecl = '<!ATTLIST' S Name AttDef* S? '>';

	EmptyElemTag = '<' Name (S Attribute)* S? '/>';

	ETag = '</' Name S? '>';

  PITarget_Old = Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'));
	PITarget = Name -- "xml"i;

	PI = '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>';

	markupdecl = elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment;

	doctypedecl = '<!DOCTYPE' S Name (S ExternalID)? S? ('[' (markupdecl | PEReference | S)* ']' S?)? '>';

	# TODO extSubsetDecl = ( markupdecl | conditionalSect | PEReference | S )*;
	# UNUSED extSubsetDecl = ( markupdecl | PEReference | S )*;

	# UNUSED extSubset = TextDecl? extSubsetDecl;

	# UNUSED Ignore = Char* - (Char* ('<![' | ']]>') Char*);

	# TODO: ignoreSectContents = Ignore ('<![' ignoreSectContents ']]>' Ignore)*;
	# UNUSED ignoreSectContents = Ignore ('<![' ']]>' Ignore)*;

	# UNUSED ignoreSect = '<![' S? 'IGNORE' S? '[' ignoreSectContents* ']]>';

	# UNUSED includeSect = '<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>';

	# UNUSED conditionalSect = includeSect | ignoreSect;

	STag = '<' Name (S Attribute)* S? '>';

	CDStart = '<![CDATA[';

	CDEnd = ']]>';

	# WAS CData = (Char* - (Char* ']]>' Char*));
	CData = (Char* -- CDEnd);

	CDSect = CDStart CData CDEnd;

	# UNUSED Subcode = ([a-z] | [A-Z])+;

	# UNUSED UserCode = ('x' | 'X') '-' ([a-z] | [A-Z])+;

	# UNUSED IanaCode = ('i' | 'I') '-' ([a-z] | [A-Z])+;

	# UNUSED ISO639Code = ([a-z] | [A-Z]) ([a-z] | [A-Z]);

	# UNUSED Langcode = ISO639Code |  IanaCode |  UserCode;

	# UNUSED LanguageID = Langcode ('-' Subcode)*;

	SDDecl = S 'standalone' Eq (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"'));

	# UNUSED extPE = TextDecl? extSubsetDecl;

	Misc = Comment | PI |  S;

	XMLDecl = '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>';

	prolog = XMLDecl? Misc* (doctypedecl Misc*)?;

	# UNUSED Names = Name (S Name)*;

  # Added fcall - TODO check logic is correct
	# UNUSED extParsedEnt = TextDecl? @{fcall content;};

  # TODO tag stack validation
  
  # WAS element = EmptyElemTag | STag content ETag
	# WAS content = (element | CharData | Reference | CDSect | PI | Comment)*;
	content = (EmptyElemTag | STag | ETag | CharData | Reference | CDSect | PI | Comment)*;

	# WAS document = prolog element Misc*;
	document = prolog ( EmptyElemTag | ( STag content ETag ) ) Misc*;

	main := document;

}%%
