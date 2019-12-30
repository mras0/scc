# SCC

## Introduction

[SCC](scc.c) is a limited C compiler producing tiny-model (i.e. single segment) DOS COM files. It should run on most operating systems and can self-host under DOS2.0+ compatible systems (e.g. [SDOS](https://github.com/mras0/sasm)) with 128K RAM (the compiler only uses 64K, but the operating system needs to live as well :)).

Also included are a small set of supporting software components:

  - [SCPP](scpp.c) - Pre-processor (Note: not optimized yet)
  - [SAS](sas.c) - Assembler that can bootstrap [SDOS/SASM](https://github.com/mras0/sasm)
  - [SIM86](sim86.c) - Basic x86 simulator (Just enough to simulate the ecosystem)

## Trying

Download a [release](https://github.com/mras0/scc/releases) or build the software yourself (see below).

I recommend using one of the disk images:

  - `small.img` is a 360K disk image containing the bare minimum. It's suitable for use with really old machines.
  - `large.img` is a 1440K disk image. You'll want to use this if you can.

Fire up an emulator (or the real thing if you're lucky) e.g. [PCem](https://pcem-emulator.co.uk/), [QEMU](https://www.qemu.org/) or the excellent online [PCjs](https://www.pcjs.org/devices/pcx86/machine/).

For example you can use the [IBM PC (Model 5150), 256Kb RAM, Color Display](https://www.pcjs.org/devices/pcx86/machine/5150/cga/256kb/).
1. Press the "Browse..." button and select the small disk image.
2. Press "Mount" and then "Ctrl-Alt-Del"
3. (Optional) At the SDOS prompt (`# `) type `build` and press enter to build the included software. Wait a couple of minutes for the software to build (Or press the 4.77MHz button a couple of times to speed it up).
4. Use `uvi` (a tiny vi clone) to edit source code, `scpp` to pre-process, `scc` to compile, `debug` to debug and `sim86` to simulate.

Consider the classic hello world adapted to SCC:

    // hello.c
    #include "lib.h"
    void main() {
      puts("Hello world! From DOS!");
    }

To build and run (don't actually type the comments):

    uvi hello.c       # Edit (press "i" to enter insert mode and press escape to exit it. ":wq" to write & quit)
    scpp hello.c      # Pre-process to produce hello.i
    scc hello.i       # Compile
    hello.com         # Run

Or on your normal system:

    favorite-editor hello.c && scpp hello.c && scc hello.i && sim86 hello.com

Some notes:

  - On your host system, you can use `sim86 -p hello.com` to get basic performance information (basically the number of memory accesses performed)
  - `lib.h` contains an extremely bare-bones standard library and start-up code that calls `main`. On non-SCC platforms it includes the corresponding headers instead, allowing the same code to compile unmodified on other systems.
  - You'll probably want to create a batch file with those steps if you plan on running them more than once.
  - If you're doing anything remotely useful, you'll probably want to ditch the pre-processing step and build your own standard library.

A more realistic hello world example would be:

    void PutChar(int ch) {
        _emit 0xB4 _emit 0x02            // MOV AH, 2
        _emit 0x8A _emit 0x56 _emit 0x04 // MOV DL, [BP+4]
        _emit 0xCD _emit 0x21            // INT 0x21
    }

    void _start() {
        const char* s = "Hello DOS!\r\n";
        while (*s) PutChar(*s++);
    }

Which should give you a (vague) idea of how to overcome the limitations of SCC (you can see more examples in [lib.h](lib.h) and [sim86.c](sim86.c) by searching for `_emit`).

## Building

Compile `scc.c` using a C99 compiler and run the output on `scc.c`, this produces `scc.com` which can then be run under DOS (or using e.g. [DOSBox](https://www.dosbox.com/)) where it can self-compile: `scc.com scc.c`. These steps can then be repeated for the remainig software components.

Or using CMake:

    mkdir build && cd build && cmake -DMAKE_DISKS:BOOL=TRUE && cmake --build .

You can omit `-DMAKE_DISKS:BOOL=TRUE` if you don't want the disk images built.

Running the test suite (will use DOSBox if available):

    cmake --build . --target test

If building the disk images is enabled and [QEMU](https://www.qemu.org/) was found, you can build the `qemu_test` target to start QEMU with the large disk image attached. Use the CMake variable `QEMU_EXTRA_ARGS` to pass in extra arguments to QMEU.

## Q&A

#### Why would I want this?

You probably don't. If you're actually looking to compile C code for DOS look into [DJGPP](http://www.delorie.com/djgpp/) or [OpenWatcom](http://openwatcom.org/).
This is just a hobby project to create a compiler for another [NIH](https://en.wikipedia.org/wiki/Not_invented_here) project of mine: [SDOS/SASM](https://github.com/mras0/sasm).

#### Why is the user experience so terrible?

Good UX uses precious bytes. Sorry :(

#### Why not support other memory models?

I wanted to keep it simple and not have to rely too much on inline "assembly" (read: `_emit` statements) magic.
Out of necessity [SIM86](sim86.c) has to use more than 64K, so there's a clear path to allowing larger output files, but for now I prefer the challenge of keeping the memory usage down :)

#### What are some similar projects?

  - [TCC](https://bellard.org/tcc/)
  - [8cc](https://github.com/rui314/8cc)
  - [c4](https://github.com/rswier/c4/)

## Limitations

A non-exhaustive list of the current limitations of SCC:

  - Arbitrary limits to e.g. the number of struct members (you can to some degree change these)
  - Very limited error checking/information (see the debugging section for help)
  - Integers are limited to 16-bits (and no support for `long`)
  - Tiny memory model (i.e. all code and data must live within a single 64K segment). `CS=DS=ES=SS` is assumed. (See [SIM86](sim86.c) for an example of how to work around this limitation)
  - Only basic support for initializers (rules are complex, but if it's not in the test suite it's not supported...)
  - No floating point support
  - No bitfields
  - Uncountably many bugs :)

## Debugging

First make sure the source code (to the greatest degree possible) compiles and works with another C compiler.
SCC assumes the source code works and only does a bare minimum of checking. Many invalid C programs are accepted and many valid C programs are rejected (or miscompiled!).

Try simplifying the code. SCC isn't clever yet.

Use the simulator ([SIM86](sim86.c)) - on your host system `-p` also provides a convinient disassembly of all reached instructions.

Change the code of the simulator to debug hard problems!

#### Stack traces

Some errors while be reported as a stack trace (as below). For these cases you'll need to refer to the map file produced in addition to the com file (i.e. `scc.map` for `scc.com`).

Example:

    BP   Return address
    FFBE 07A5
    FFC8 4844
    FFD6 48E4
    FFDA 4920
    FFDE 51FE
    FFE8 5892
    FFFC 03D5
    In line 48: Check failed

Consulting the map file for the closest (not greater than) symbol:

    07A5 # 0786 Fail
    4844 # 4666 ParseDeclarator
    48E4 # 48D5 DoDecl
    4920 # 4908 ParseFirstDecl
    51FE # 51F8 ParseExternalDefition
    5892 # 55F3 main
    03D5 # 03A2 _start

#### Compile errors

Note that:

  - The reported line number is past the current token (meaning lots of whitespace/comments may have been skipped).
  - If the pre-processor was used you need to manually reverse engineer the real line from the pre-processed file (since that's where the line number points)
  - Running SCC on your host system may provide a line number to look up in `scc.c` for check failures.

If you can reproduce the problem on your host system you should use a debugger attached to `scc` to get an idea of where and why the issue happens.

#### Other problems

If you suspect a compiler bug, you'll probably have to isolate the issue and look manually inspect the generated code.

## Technical Notes

Apart from some minor exceptions SCC is a single pass compiler. It tries really hard to only consider the current character/token/(sub-)expression to preserve memory.
An [AST](https://en.wikipedia.org/wiki/Abstract_syntax_tree) isn't built, rather code is outputted directly in response to expressions and statements.

Within these constraints SCC tries to generate somewhat optimized code. Many possible optimizations aren't performed because they don't pay off (meaning they don't optimize SCC itself either in time or size).

There's some limited optimizations done while doing address fixups to compensate for the limited optimizations. These are the only backward looking optimizations.

More to come... (maybe).
