# Colm Suite

The Colm Suite is a unified distribution of two closely related language tools:

- **Colm** (COmputer Language Machinery): A programming language designed for the analysis and transformation of computer languages. Influenced primarily by [TXL](https://www.txl.ca/).

- **Ragel**: A state machine compiler that generates executable finite state machines from regular expressions and state machine specifications.

The two projects share components and have a build dependency, so a unified
repository simplifies development and building. The significant links are:

- The **colm compiler and runtime** (`src/colm/`) -- one sub-package of the suite, alongside the others below.
- The **FSM library** (`src/libfsm/`) -- finite state machine construction and manipulation, used by both Colm and Ragel.
- The **code generation intermediate language** (`src/cgil/`) -- written in Colm, used by both projects for target language code generation.
- Ragel's **frontend is written in Colm** (`src/ragel/*.lm`), so Colm must build first to bootstrap Ragel.

## Versioning

Every shipped component is versioned independently: colm, ragel, libfsm,
cgil and aapl each carry their own number, declared in `configure.ac`.
Suite releases are calendar-versioned snapshots of the components:
`colm-suite-YYYY.MM`, zero padded, with a micro appended for fixup
releases (`2026.08`, then `2026.08.1`).


## Colm

Colm is a programming language for the analysis and transformation of computer
languages. Where other languages define classes or data structures, a colm
program defines grammars, and colm builds a parser from them that reads both the
program's input and the patterns the program looks for in it, so the two parse
into trees of the same types. Colm compiles a program to C, and the C to an
executable.

This is how Colm greets the world
([`hello_world.lm`](doc/colm/code/hello_world.lm)):

```colm
print "hello world\n"
```

`-r` runs the program once colm has built it, and `colm --help` lists the other
options:

```
$ colm -r hello_world.lm
hello world
```

The [colm manual](doc/colm/colm-manual.adoc) covers the rest. Places to start:

- [Installing](doc/colm/0_01_installing.adoc): getting colm, building it and
  running a program.
- [Commandline](doc/colm/0_03_commandline.adoc): the output of `colm --help`.
- A tutorial, from [Hello world](doc/colm/0_04_hello_world.adoc) to
  [A complete transformation](doc/colm/0_07_transformation.adoc).
- [Contributing](doc/colm/8_05_contributing.adoc): working on colm itself.
- [Q and A](doc/colm/9_00_q_and_a.adoc).

More real-world programs parsing several languages implemented in Colm can be
found in the [`grammar/`](grammar/) folder.


## Ragel

Ragel compiles regular expressions and state charts to executable finite state
machines. The generated code can be output in a variety of host languages.

### Supported target languages

C, C++, D, Java, Ruby, C#, Go, OCaml, Rust, Julia, Zig, JavaScript, GNU ASM x86-64, and
Crack.

### Code generation backends

| Flag | Style |
|------|-------|
| `-T0`, `-T1` | Table-driven |
| `-F0`, `-F1` | Flat table-driven |
| `-G0`, `-G1`, `-G2` | Goto-driven |

Language-specific binaries are also available: `ragel-c`, `ragel-go`, `ragel-rust`,
`ragel-zig`, etc.

See the [`examples/`](examples/) directory for sample Ragel programs.


## Building

### Dependencies

- make
- libtool
- gcc
- g++
- autoconf
- automake

For the documentation (`./configure --enable-manual`), install
[Asciidoctor](https://asciidoctor.org/) for the colm manual, and
[`asciidoc`](https://asciidoc-py.github.io/),
[`fig2dev`](https://github.com/getlarky/fig2dev) and
[dblatex](https://dblatex.sourceforge.net/) for the ragel guide, as well. The
guide's PDF is made by `a2x`, which comes with `asciidoc`, using dblatex. Each
manual is built only when its half of the suite is installed (see [Installing
one half of the suite](#installing-one-half-of-the-suite)), and configure checks
only for the tools of the manuals it will build.
With [Rouge](https://rouge.jneen.net/) (`ruby-rouge`) installed, the manual's
examples are highlighted. Rouge has no lexer for colm, so the build loads its
own, `doc/colm/colm-rouge.rb`.

### Build instructions

```
$ ./autogen.sh
$ ./configure
$ make
$ make install
```

Without `--prefix`, the suite installs under `/usr/local`. On Debian, Ubuntu and
other systems that list `/usr/local/lib` in `/etc/ld.so.conf`, run
`sudo ldconfig` after `make install`, or colm and ragel won't find their
libraries. The colm manual's
[FAQ](doc/colm/9_00_q_and_a.adoc#libcolm-not-found) explains why.

### Installing one half of the suite

Colm and ragel install together by default. The two halves can be installed
separately, which is useful for a ragel user who has no interest in colm, or a
colm user who has no interest in ragel:

```
$ ./configure --disable-install-colm    # ragel only
$ ./configure --disable-install-ragel   # colm only
```

The whole tree is still built either way. Ragel's parsers are written in colm,
so colm has to be built before ragel can be, and building everything keeps the
test suite runnable from the build tree. Only the install step is narrowed.
The manuals are the exception: with `--enable-manual`, only the installed
half's manual is built, and configure checks only for its tools.

Two things go out in both cases and cannot be excluded from a ragel-only
install: the colm runtime library, which the ragel programs link, and the aapl
headers, which the installed libfsm headers include.

### Building with CMake

CMake (3.16 or later) is supported as an alternative to autotools. It builds the
same set of programs and libraries and reads the version numbers out of
`configure.ac`, so there is no need to run `autogen.sh` or `configure` first.
Only out-of-source builds are supported.

```
$ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
$ cmake --build build -j$(nproc)
$ cmake --install build
```

Options:

| Option | Default | Meaning |
| --- | --- | --- |
| `COLM_MAKE_INSTALL` | `ON` | Generate install rules. |
| `COLM_INSTALL_COLM` | `ON` | Install the colm program and its development files. |
| `COLM_INSTALL_RAGEL` | `ON` | Install ragel, its host backends, libragel, libfsm and cgil. |
| `COLM_BUILD_EXAMPLES` | `OFF` | Build the ragel examples under `examples/`. |
| `COLM_BUILD_MANUAL` | `OFF` | Build the colm manual under `doc/colm/` with Asciidoctor. |
| `BUILD_STANDALONE` | `ON` on Windows | Link the executables statically. |
| `BUILD_SHARED_LIBS` | `OFF` | Build libcolm, libfsm and libragel as shared libraries. |

`COLM_INSTALL_COLM` and `COLM_INSTALL_RAGEL` are the cmake spelling of
`--disable-install-colm` and `--disable-install-ragel`; see [Installing one half
of the suite](#installing-one-half-of-the-suite) for what each one covers.

All executables are written to `build/bin`, which is where the `ragel` driver
expects to find the per-host-language backends (`ragel-c`, `ragel-go`, ...).

The install exports cmake packages, so a dependent project can do:

```cmake
find_package(colm REQUIRED)   # colm::colm, colm::libcolm
find_package(ragel REQUIRED)  # ragel::ragel, ragel::libfsm, ragel::libragel
```

A version asked for in `find_package` must match the installed major and minor
version, since a minor release of either component can break a dependent
project. `find_package(colm 0.15)` accepts colm 0.15.x but not 0.16, and
`find_package(ragel 7.1)` accepts ragel 7.1.x but not 7.2 or 8.0. A major
version alone means `.0`, so `find_package(ragel 7)` does not find 7.1. Both
ends of a version range must be in the installed minor version: `7.1...<7.2`
finds 7.1.x, `7.1...<8` does not.

`ragel::libragel` links the colm runtime, so `find_package(ragel)` loads the
colm package too, at exactly the colm version ragel was built with. It passes
over any other colm on the search path. It also looks for colm beside the ragel
package, so setting `ragel_DIR` to an install or a build tree is enough.

Once a project has loaded colm, through `find_package(colm)` or
`find_package(ragel)`, its colm targets stay, even when it was found inside a
function or under the name `Colm`. So a later `find_package(colm)` fails if the
colm already loaded is a different version, and `find_package(ragel)` fails if
it isn't the colm ragel was built with.

The autotools build remains the reference build. Known differences:

- The run-from-the-build-tree detection described below relies on libtool, so a
  colm built by cmake always uses the install location to find its includes and
  runtime library. Install it before using it to compile colm programs.
- The test suite under `test/`, the ragel guide and man page under
  `doc/ragel/`, and `colm-wrap` are autotools-only. A cmake install therefore
  cannot serve as the `--with-colm` target of an autotools build. The colm
  manual builds either way, with `--enable-manual` or `COLM_BUILD_MANUAL`.
- Libtool builds both a static and a shared library and versions all three
  with `-release` (`libcolm-<version>.so`). CMake builds one flavour, selected
  by `BUILD_SHARED_LIBS`, and versions all three with a soname that carries the
  whole version (`libcolm.so.<version>`). Either way the name changes with
  every release of the library's own component: libcolm takes colm's version,
  libragel ragel's, and libfsm its own.
- `--enable-pool-malloc`, `--with-ragel-kelbt`, `--with-colm` and the
  large-file-support checks have no cmake equivalent.

### Run-time dependencies

Colm compiles a program to C, then compiles the C with `gcc`, or the compiler
named by the `CC` environment variable, passing it `CFLAGS` from the
environment as well. So colm needs a C compiler wherever it is used, not only
where it is built. The program is linked with the colm runtime library, libcolm.

An installed colm compiles against the installed headers and libcolm. A colm run
from an autotools build tree uses the tree's headers instead, and links libcolm
statically through the tree's libtool, so nothing has to be installed. Colm knows
that it is in the build tree when its own executable is the one the build made,
a check that works on Linux and macOS only. When the check fails, `-B <path>`
names the build tree, the directory that holds `src`. A colm built by CMake
always uses the installed headers and libcolm, and `-B` doesn't work with it.

The manual's [Running colm](doc/colm/0_01_installing.adoc#running-colm) and
[Building from source](doc/colm/8_05_contributing.adoc#building-from-source)
say more.


## Testing

```
$ make check
```

Test suites are under `test/` with subdirectories for each component (`colm.d`, `ragel.d`, `aapl.d`, etc.).
One more, `manual`, runs the colm manual's example programs,
`doc/colm/code/NAME.lm`, with `NAME.in` on stdin if there is one, and compares
their output with `NAME.exp`. An example can also have colm options in
`NAME.comp`, a host program it is linked into, `NAME.host.c` or
`NAME.host.cc`, and the `reducer.h` a program's commit code includes,
`NAME.reducer.h`, which needs `NAME.host.cc`. The chapters include those
files, so a new example needs its `.exp` (and any of the others) there too,
listed in `doc/colm/Makefile.am` and `doc/colm/CMakeLists.txt`. Each example is
compiled with `-I` naming `doc/colm/code`, so it can include a file kept there.
Give that file a suffix other than `.lm`, such as `.lmi`, since every `.lm`
there is run as an example, and list it in those two build files too.
`NAME.args`, if there is one, holds the program's arguments, and is listed in
them too. A page that shows the command line types it by hand, so keep the two
the same. `NAME.exit`, if there is one, holds the exit status the example must
end with. Without it, the example must exit 0. It is listed in the two build
files like the others.


## Syntax highlighting

There are vim syntax definition files [colm.vim](/colm.vim) and [ragel.vim](/ragel.vim).


## License

Colm and Ragel are free software under the MIT license.<br>
Please see the COPYING file for more details.
