# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with
code in this repository.

## Project Overview

This repository contains two closely related projects:

- **Colm** (COmputer Language Machinery): A programming language and compiler
  construction toolkit designed for analysis and transformation of computer
  languages. Written in C/C++, it generates C code that compiles to native
  executables.

- **Ragel**: A state machine compiler that generates executable finite state
  machines from regular expressions and state machine specifications. It targets
  13+ languages: C, C++, D, Java, Ruby, C#, Go, OCaml, Rust, Julia, Zig, JavaScript,
  GNU ASM x86-64, and Crack.

Colm bootstraps Ragel -- Ragel's parser is written in Colm (`src/ragel/*.lm`).

## Build Commands

```bash
# Initial setup (only needed once)
./autogen.sh
./configure

# Build the project (both Colm and Ragel)
make

# Run tests
make check

# Install
make install

# Clean build artifacts
make clean
```

CMake (3.16+) is supported as an alternative build system. It covers colm,
libfsm and ragel (including all host-language backends), but not the test
suite. Out-of-source builds only.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
cmake --install build
```

Colm and ragel install together by default. Either half can be installed on its
own; the whole tree is built either way.

```bash
# autotools
./configure --disable-install-colm     # ragel only
./configure --disable-install-ragel    # colm only

# cmake
cmake -S . -B build -DCOLM_INSTALL_COLM=OFF
cmake -S . -B build -DCOLM_INSTALL_RAGEL=OFF
```

## Development Commands

```bash
# Compile a Colm program
colm file.lm

# Compile and run immediately
colm -r file.lm

# Compile only (don't produce binary)
colm -c file.lm

# Generate dot format for visualization
colm -V file.lm

# Compile a Ragel state machine
ragel -o output.c input.rl

# Code generation backends: -T0 (table), -T1, -F0 (flat), -F1, -G0 (goto), -G1, -G2
ragel -T0 -o output.c input.rl

# Host language selection via language-specific binaries
ragel-c input.rl
ragel-go input.rl
ragel-rust input.rl
ragel-zig input.rl
```

## Testing

```bash
# Run all tests
make check

# Run the harness directly (after make check has built the test programs)
./test/harness/harness

# Run specific suites or cases
./test/harness/harness --suite colm.d              # one suite
./test/harness/harness colm.d/argv1.lm ragel.d/atoi1.rl
cd test/ragel.d && ../harness/harness atoi1.rl     # from inside a suite
./test/harness/harness --lang java --genflags -T0  # ragel.d matrix selection
./test/harness/harness --suite aapl.d --stress 60  # aapl.d stress programs, 60s each
./test/harness/harness --suite colm.d --valgrind   # colm.d programs under valgrind
./test/harness/harness --suite manual              # the manual's example programs
./test/harness/harness -v --tap results.tap        # every result, TAP output
./test/harness/harness -j 4 --suite colm.d         # 4 cases at once
```

The harness (`test/harness/`) enumerates all eight suites into one work queue
and runs them in parallel, `-j N` cases at once (by default the `-j` of
`make -j N check`, else the CPU count). Failures leave a `.diff` file in the
suite's `working/` directory with the differences and the commands run.

Seven suites are the directories under `test/`. The eighth, `manual`, runs
the colm manual's example programs, `doc/colm/code/NAME.lm`, with `NAME.in`
on stdin if there is one, and compares the output with `NAME.exp`. An example
can also have colm options in `NAME.comp`, a host program it is linked into,
`NAME.host.c` or `NAME.host.cc`, and the `reducer.h` a program's commit code
includes, `NAME.reducer.h`, which needs `NAME.host.cc`. The chapters include
those files, so a new example needs its `.exp` (and any of the others) there
too, and in `CODE` and `MANUAL_CODE` in `doc/colm/`'s build files. Each
example is compiled with `-I` naming `doc/colm/code`, so it can include a file
kept there. Give that file a suffix other than `.lm`, such as `.lmi`, since
every `.lm` there is run as an example, and list it in `CODE` and
`MANUAL_CODE` too. `NAME.args`, if there is one, holds the program's
arguments, and goes in the build files too. No chapter includes it: a page
that shows the command line types it by hand, so keep the two the same.
`NAME.exit`, if there is one, holds the exit status the example must end
with. Without it, the example must exit 0. It goes in `CODE` and
`MANUAL_CODE` like the others. The suite's results go to
`test/manual/working/`.

The manual itself is one book, `doc/colm/colm-manual.adoc`, which includes
each chapter. A new chapter goes in its includes, and in `CHAPTERS` and
`MANUAL_CHAPTERS` in `doc/colm/`'s build files. A link to a chapter names the
id above the chapter's title, as in `xref:3_01_lex.adoc#lexing[...]`, since a
link without it leads to the top of the book. The book's build fails on such
a link, and on a link to an id or a page the book doesn't have
(`doc/colm/colm-links.rb`). Colm code goes in a `[source,colm]` block, which
the build highlights with its own Rouge lexer, `doc/colm/colm-rouge.rb`.

Each colm.d and manual run sets `COLM_LEAK_CHECK`, so a program that loses
pool items (kids, trees, heads, ...) fails its case. A colm.d case's `LOST`
section lists known losses that don't. `--valgrind` also catches memory errors
and malloc leaks.

A colm.d case with a `COMP_ERR` section is a program colm must reject. It
passes when colm exits with 1 and its error output is the section, read
without the `working/NAME.lm:` colm prints in front of each `line:col`. Colm
must also leave the program, `working/NAME.lm`, as it was. It has no runs, so
it takes no `ARGS`, `IN`, `EXP`, `EXIT`, `LOST` or `HOST`.

A fix comes with a test case that fails without it: Colm cases go in
`test/colm.d`, Ragel cases in `test/ragel.d`. For a change to the runtime's
memory handling (`src/colm/*.c`), also run the cases it touches with
`--valgrind`.

## Code Architecture

### Colm

1. **Compiler Pipeline** (src/colm/):
   - `parsedata.cc/h`: Main parser data structures
   - `parsetree.cc/h`: Parse tree construction
   - `synthesis.cc`: Code synthesis and generation
   - `bytecode.cc/h`: Bytecode generation for the VM
   - `pdarun.c`: Runtime parser driver

2. **Runtime System** (src/colm/):
   - `colm.c`: Main runtime entry point
   - `tree.c`: Tree manipulation runtime
   - `input.c`: Input stream handling
   - `program.c`: Program execution runtime

3. **Code Generation** (src/):
   - `cgil/`: Colm sources for code generation intermediate language

### Ragel

1. **Core** (src/ragel/):
   - `inputdata.cc/h`: Central orchestrator managing the compilation pipeline
   - `parsedata.cc/h`: Manages parsing of Ragel state machine definitions
   - `reducer.cc/h`: Transforms parsed data into intermediate representations
   - `parsetree.cc/h`: Parse tree structures for Ragel

2. **Parser Specs** (src/ragel/):
   - `ragel.lm`: Main Ragel grammar (written in Colm)
   - `rlparse.lm`: Parser definitions
   - `rlreduce.lm`: Reduction rules

3. **Host Language Targets** (src/ragel/host-*/):
   - 13 directories: `host-asm/`, `host-c/`, `host-crack/`, `host-csharp/`,
     `host-d/`, `host-go/`, `host-java/`, `host-js/`, `host-julia/`,
     `host-ocaml/`, `host-ruby/`, `host-rust/`, `host-zig/`
   - Each contains language-specific parser grammar and code generation

4. **Ragel Compilation Pipeline**:
   - Input processing: read and preprocess `.rl` source files
   - Parsing: Colm-based parsers build parse trees
   - Reduction: transform parse trees into FSM intermediate representation
   - FSM construction: build and optimize finite state machine graphs
   - Code generation: generate target language code using selected backend

### Shared

- **libfsm** (src/libfsm/): Finite state machine library used by both Colm and Ragel

### Key Abstractions

- **Parse Trees**: Central data structure for language processing
- **Virtual Machine**: Stack-based VM for executing Colm transformations
- **Input Streams**: Abstraction for handling various input sources
- **Patterns**: Grammar-based pattern matching for transformations
- **FsmGraph**: Core finite state machine representation and manipulation

### File Extensions

- `.lm`: Colm language source files
- `.cgil`: Code generation intermediate language files
- `.rl`: Ragel state machine definition files

## Coding Conventions

- Use tabs for indentation
- Spaces for alignment after first non-whitespace character
- Maximum 100 characters per line
- No trailing whitespace
- Function blocks start on new line
- C-style comments for documentation
- C++ style comments for disabled code

## Commits and Issues

- Commit subjects name the area, then what changed, e.g.
  `colm: detach the right ignore under a nonterminal after unreducing it`.
- Issue titles name the area, then what is wrong, not the fix, e.g.
  `colm: calling exit inside a token action segfaults`.

## Changing the Runtime

- Runtime bugs hide in three places: reference counts (`refs`), who owns
  ignore lists and trees, and the reverse code (`rcode`) and `undo_*` paths
  that backtracking runs. Check all three for any change.
- Installed headers and the libraries' exported symbols are public interface:
  change them only on purpose, and say so in the PR.
- An error a program makes at run time, such as popping an empty list, stops
  it through `runtime_error()` in `src/colm/bytecode.c`: it prints `fatal:`
  and the message after the program's output, and exits with status 1, ending
  a host too. Use it for new checks of this kind, rather than crashing or
  returning nil.

## Important Notes

- The project uses GNU autotools (autoconf, automake, libtool)
- Colm is self-hosting -- it uses itself to build parts of the compiler
- Ragel's parsers are written in Colm, so Colm must build first
- The test suite is comprehensive and uses expected output comparison
- Runtime requires GCC as Colm generates C code
- The project uses both C++ and Colm extensively
- When modifying parsers, changes to `.lm` files require rebuilding
- Multiple Ragel code generation backends are available -- choose based on performance/size requirements
- The project includes example grammars for C++, Go, Python, Rust, and other languages in the `grammar/` directory
