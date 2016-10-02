% RAPIDRES(1) Rapicorn-@BUILDID@ | Rapicorn Manual
%
% @FILE_REVISION@


# NAME
rapidres - A Rapicorn binary resource file to C++ code converter


# SYNOPSIS
**rapidres** [**-h**] [**-v**] [*files*...]


# DESCRIPTION

Rapidres reads binary files and generates C++ source code to compile the
binary contents into a C++ program. The generated source code relies on
the definition of the following two macros:

**RAPICORN_RES_STATIC_DATA**(*symbol*)
:   Defines a static variable to hold a char array. By default, the data
    is read out of an input file, compressed (with zlib) and written out
    in C string syntax.

**RAPICORN_RES_STATIC_ENTRY**(*symbol*, *path*, *size*)
:   Defines a static variable with metadata about *symbol*.
    The *symbol* is the same as passed into **RAPICORN_RES_STATIC_DATA()**,
    the *path* designates the filename of the input file to identify the
    binary contents at runtime. The *size* argument corresponds to the
    actual uncompressed size of the data stored
    with **RAPICORN_RES_STATIC_DATA()**.


# OPTIONS

**Rapidres** follows the usual GNU command line syntax, with long options starting with two dashes ('-').

**-h**, **\--help**
:   Print usage information.

**-v**, **\--version**
:   Print version and file paths.


# EXAMPLES

Generate source code to compile a simple file into an executable:

**echo** Hello World > hello.txt	\
**rapidres** hello.txt

This generates something along the lines of:

    /* rapidres file dump of hello.txt */
    RAPICORN_RES_STATIC_DATA (hello_txt) = "Hello World\n"; // 12 + 1
    RAPICORN_RES_STATIC_ENTRY (hello_txt, "hello.txt", 12);


# AUTHORS
Tim Janik.


# SEE ALSO

**rapidrun**(1), [**Rapicorn Website**](http://rapicorn.org)
