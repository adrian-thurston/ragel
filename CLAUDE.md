# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with
code in this repository.

## Project Overview

Ragel is a State Machine Compiler that generates executable finite state
machines from regular expressions and state machine specifications. It compiles
high-level state machine descriptions into efficient code in multiple target
languages including C, C++, D, Java, Ruby, C#, Go, OCaml, Rust, Julia,
JavaScript, GNU ASM x86-64, and Crack.

## Dependencies

- **Colm Programming Language**: Required dependency (version 0.14.7)
  - When building from master, use master branch of Colm
  - For releases, check EXPECTED_COLM_VER in configure.ac

## Build Commands

### Using Autotools (Primary Build System)
```bash
./autogen.sh
./configure --prefix=$PREFIX --with-colm=$COLM_INSTALL_PATH
make
make install
```

### Running Tests
```bash
# Run the full test suite
bash test/runtests.sh

# Run tests from the build directory
make check
```

## Architecture Overview

### Core Components

- **InputData** (`src/inputdata.h/cc`): Central orchestrator managing the compilation pipeline
- **ParseData** (`src/parsedata.h/cc`): Manages parsing of Ragel state machine definitions
- **FsmGraph** (libfsm): Core finite state machine representation and manipulation
- **Reducer** (`src/reducer.h/cc`): Transforms parsed data into intermediate representations

### Language Support

Each target language has a dedicated directory under `src/host-*/` containing:
- Language-specific parser grammar (`rlparse.lm`)
- Main entry point configuring the host language
- Code generation specializations

### Compilation Pipeline

1. **Input Processing**: Read and preprocess Ragel source files
2. **Parsing**: Colm-based parsers build parse trees from `.rl` files
3. **Reduction**: Transform parse trees into FSM intermediate representation
4. **FSM Construction**: Build and optimize finite state machine graphs
5. **Code Generation**: Generate target language code using selected backend (binary, flat, switch, goto)

### Key File Types

- `.rl` - Ragel source files containing state machine definitions
- `.lm` - Colm language files for parser definitions
- `.cc/.h` - C++ implementation files

### Testing

Test cases are located in `test/ragel.d/` with numerous `.rl` files covering various features. The test system uses the Colm test driver framework.

## Important Notes

- Always check that Colm is properly installed before building
- The project uses both C++ and Colm extensively
- When modifying parsers, changes to `.lm` files require rebuilding
- Multiple code generation backends are available - choose based on performance/size requirements
