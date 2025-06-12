# Project: Assembler, Linker, and Emulator for an Abstract Computer System

The goal of this project is to implement a toolchain (assembler and linker) and an emulator for an abstract computer system.
The assembler is specific to the defined abstract computer system, while the linker is designed to be target-architecture independent.

## Building

To build the project, use the provided [Makefile](Makefile):

```sh
make
```

This will create the following executables:
- `asembler`
- `linker`
- `emulator`

## Running

The [start.sh](start.sh) script demonstrates how to assemble, link, and emulate a sample program:

```sh
./start.sh
```

This script will:
1. Assemble the `.s` files in the `tests/` directory (e.g., [tests/main.s](tests/main.s), [tests/handler.s](tests/handler.s), [tests/isr_terminal.s](tests/isr_terminal.s), [tests/isr_timer.s](tests/isr_timer.s)) into object files (`.o`).
2. Link the object files into a `program.hex` file.
3. Run the `program.hex` file with the emulator.