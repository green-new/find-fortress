#!/bin/bash
# excuse my Linux scripting skills...

make libcubiomes
gcc find_witherskeleton.c -o a.out libcubiomes.a -fwrapv -lm
./a.out