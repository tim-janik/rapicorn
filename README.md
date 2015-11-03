The Rapicorn Toolkit
====================

[![License MPL2](http://testbit.eu/~timj/pics/license-mpl-2.svg)](https://github.com/tim-janik/rapicorn/blob/master/COPYING.MPL)
[![Build Status](https://travis-ci.org/tim-janik/rapicorn.svg)](https://travis-ci.org/tim-janik/rapicorn)
[![Binary Download](https://api.bintray.com/packages/beast-team/deb/rapicorn/images/download.svg)](https://github.com/tim-janik/rapicorn/#binary-packages)


# DESCRIPTION

Rapicorn is a graphical user interface (UI) toolkit for rapid development
of user interfaces in C++ and Python. The user interface (UI) is designed
in declarative markup language and is connected to the programming logic
using data bindings and commands.

*   For a full description, visit the project website:
	http://rapicorn.org

*   To submit bug reports and feature requests, visit:
	https://github.com/tim-janik/rapicorn/issues

*   Rapicorn is currently in the "prototype" phase. Features are still
	under heavy development. Details are provided in the roadmap:
	http://rapicorn.org/wiki/Rapicorn_Task_List#Roadmap


# REQUIREMENTS

Rapicorn has been successfully build on Ubuntu x86-32 and x86-64.
A number of dependency packages need to be installed:

	apt-get install intltool libcairo2-dev libpango1.0-dev python2.7-dev \
	  libxml2-dev libgdk-pixbuf2.0-dev libreadline6-dev libcroco3-dev \
	  xvfb cython


# INSTALLATION

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


# SUPPORT

If you have any issues, please let us know in the issue tracker or
the mailing list / web forum:

	https://groups.google.com/d/forum/rapicorn
	rapicorn@googlegroups.com

The developers can often be found chatting on IRC:

	#beast IRC channel on GimpNet: irc.gimp.org

The distribution tarball includes Python and C++ examples directories:

	examples/  pytests/

And documentation is provided with the distribution and online:

*   https://testbit.eu/pub/docs/rapicorn/latest/

*   $prefix/share/doc/rapicorn-<VERSION>/html/apps.html

# BINARY PACKAGES

New source code pushed to the Rapicorn repository is automatically built
and tested through a Travis-CI script. Successful continuous integration
builds also create binary Debian packages
([latest version](https://bintray.com/beast-team/deb/rapicorn/_latestVersion))
which can be installed after adding an apt data source, example:

    # Enable HTTPS transports for apt
    apt-get -y install apt-transport-https ca-certificates
    # Add and trust the beast-team packages on bintray.com
    echo "deb [trusted=yes] https://dl.bintray.com/beast-team/deb vivid main" |
      sudo tee -a /etc/apt/sources.list.d/beast-team.list
    # Update package list and install Rapicorn
    apt-get update && apt-get -y install rapicorn
