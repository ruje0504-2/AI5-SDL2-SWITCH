libai5
======

This is a library to facilitate code sharing between elf-tools and a (future)
AI5\_Win implementation. It is not intended for wider use and as such the
interfaces are not guaranteed to be stable.

Building
--------

First install the dependencies (corresponding Debian package in parentheses):

* meson (meson)
* libpng (libpng-dev)

Then build the libai5.a static library with meson,

    mkdir build
    meson build
    ninja -C build

Usage
-----

This project is meant to be used as a subproject in meson. Otherwise you can
link it as you would any other static library.
