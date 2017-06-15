The Rapicorn Toolkit
====================

[![License MPL2](https://testbit.eu/~timj/pics/license-mpl-2.svg)](https://github.com/tim-janik/rapicorn/blob/master/COPYING.MPL)
[![Build Status](https://travis-ci.org/tim-janik/rapicorn.svg)](https://travis-ci.org/tim-janik/rapicorn)
[![Tarball Download](https://testbit.eu/~timj/pics/tarball.svg)](https://dist.testbit.org/rapicorn/)


## DESCRIPTION

Rapicorn is a graphical user interface (UI) toolkit for rapid development
of user interfaces in C++ and Python. The user interface (UI) is designed
in declarative markup language and is connected to the programming logic
using data bindings and commands.

*   For a full description, visit the project website:
	https://rapicorn.testbit.org/

*   To submit bug reports and feature requests, visit:
	https://github.com/tim-janik/rapicorn/issues

*   Rapicorn is currently in the "prototype" phase. Features are still
	under heavy development. Details are provided in the roadmap:
	https://rapicorn.testbit.org/wiki/Rapicorn_Task_List


## REQUIREMENTS

Rapicorn has been successfully build on Ubuntu x86-32 and x86-64.
A number of dependency packages need to be installed:

    apt-get install intltool librsvg2-dev libpango1.0-dev libxml2-dev \
      libreadline6-dev python2.7-dev python-enum34 \
      xvfb cython doxygen graphviz texlive-binaries pandoc

## INSTALLATION

In short, Rapicorn needs to be built and installed with:

	./configure
	make -j`nproc`
	make -j`nproc` check		# run simple unit tests
	make install
	make -j`nproc` installcheck	# run module tests

Note that Rapicorn has to be fully installed to function properly.
For non-standard prefixes, Python module imports need proper search
path setups. The following commands shows two examples:

	make python-call-info -C cython/


## SUPPORT

If you have any issues, please let us know in the issue tracker or
the mailing list / web forum:

	https://groups.google.com/d/forum/rapicorn
	rapicorn@googlegroups.com

The developers can often be found chatting on IRC:

	#beast IRC channel on GimpNet: irc.gimp.org

The distribution tarball includes Python and C++ tests and examples:

	examples/  tests/

Documentation is provided online and locally (if installed in /usr):

*   https://testbit.eu/pub/docs/rapicorn/latest/

*   file:///usr/share/doc/rapicorn/html/index.html


## CONTINUOUS INTEGRATION

New source code pushed to the Rapicorn repository is automatically built
and tested through a travis-ci script. Here is the build history overview:

*   https://travis-ci.org/tim-janik/rapicorn/builds
