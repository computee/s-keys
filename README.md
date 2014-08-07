s-keys
======

Port of linux kernel jump labels to userspace. Uses GCC goto asm extension and
 some other gcc/gas specific extensions (always online, .pushsection, section attributes).

Jump labels described in the kernel's jump_label.h:

Jump labels provide an interface to generate dynamic branches using
self-modifying code. Assuming toolchain and architecture support the result
of a "if (static_key_false(&key))" statement is a unconditional branch (which
defaults to false - and the true block is placed out of line).

Original authors Jason Baron and Peter Zijlstra.

See these two LWN articles for an introduction:

http://lwn.net/Articles/412072/

http://lwn.net/Articles/436041/

Another good one on kernel elf special sections:

http://lwn.net/Articles/531148/

And the kernel documentation:

https://www.kernel.org/doc/Documentation/static-keys.txt

Installing:
    git clone https://github.com/computee/s-keys.git
    cd s-keys
    make

To use static keys, just include the header and initialize like this:

Todo
====

1)
2)
3)
