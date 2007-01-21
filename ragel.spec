Summary:      Ragel State Machine Compiler
Name:         ragel
Version:      5.16
Release:      1

URL:          http://www.cs.queensu.ca/home/thurston/ragel/
Vendor:       Adrian Thurston
Packager:     Adrian Thurston
Distribution: Any
Group:        Development/Other
License:      GPL

Source0:      http://www.cs.queensu.ca/home/thurston/ragel/%{name}-%{version}.tar.gz

Prefix:       /usr
BuildRoot:    %_tmppath/%name-%version-root
BuildPreReq:  gcc, make

%description
Ragel compiles finite state machines from regular languages into executable C,
C++, Objective-C or D code. Ragel state machines can not only recognize byte
sequences as regular expression machines do, but can also execute code at
arbitrary points in the recognition of a regular language. Using custom
operators, Ragel allows the user to embed code into a regular language in
arbitrary places without disrupting the regular language syntax. Ragel also
provides operators for controlling nondeterminism, constructing machines using
state charts and building scanners.

%prep
%setup -q -n %{name}-%{version}

%build
./configure --prefix=%{prefix}
make CFLAGS="-O2 -Wall"
cd doc && make ragel.1 rlcodegen.1

%install
# Rather than 'make install', let RPM choose where
# things are kept on this system:
install -d $RPM_BUILD_ROOT%_bindir
install -s ragel/ragel $RPM_BUILD_ROOT%_bindir/ragel
install -s rlcodegen/rlcodegen $RPM_BUILD_ROOT%_bindir/rlcodegen
install -d $RPM_BUILD_ROOT%_mandir/man1
install doc/ragel.1 $RPM_BUILD_ROOT%_mandir/man1/ragel.1
install doc/rlcodegen.1 $RPM_BUILD_ROOT%_mandir/man1/rlcodegen.1

%files
%defattr(-,root,root)
%_bindir/ragel
%_bindir/rlcodegen
%_mandir/man1/ragel.1
%_mandir/man1/rlcodegen.1

%clean
    rm -rf $RPM_BUILD_ROOT
