# Strategy regression test

Exercises `Strategy::minimax` directly, with no SDL 1.2 install needed: only
`strategy.cc` and `bidiarray.cc` are compiled, and `shim/SDL_image.h` is an
empty stub so `common.h`'s `#include <SDL_image.h>` resolves without pulling
in the actual image library, which these two files never call.

```bash
cd tests
g++ -I shim -I/usr/include/SDL2 -D_REENTRANT -Wall -O2 -c test_strategy.cc -o test_strategy.o
g++ -I shim -I/usr/include/SDL2 -D_REENTRANT -Wall -O2 -c ../strategy.cc -o strategy.o
g++ -I shim -I/usr/include/SDL2 -D_REENTRANT -Wall -O2 -c ../bidiarray.cc -o bidiarray.o
g++ test_strategy.o strategy.o bidiarray.o -lpthread -lSDL2 -o test_strategy
./test_strategy
```

If `libsdl2-dev` is not installed either, install it first
(`sudo apt install libsdl2-dev`): only its headers are used here, for the
handful of integer typedefs (`Sint16`, `Sint32`, ...) that SDL 1.2 and SDL2
both define under the same names.
