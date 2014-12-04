% RAPIDRUN(1) @RAPICORN_VERSION@ | Rapicorn Manual
%
% @FILE_REVISION@


# NAME
rapidrun - Utility to run and test Rapicorn dialogs


# SYNOPSIS
**rapidrun** [*OPTIONS*] [*uifile.xml*]


# DESCRIPTION

**Rapidrun** is a utility to display and operate Rapicorn UI dialogs.
This tool will read the GUI description file listed on the command line, look for a dialog window and show it.


# OPTIONS

**Rapidrun** follows the usual GNU command line syntax, with long options starting with two dashes ('-').

**\--parse-test**
:   Parse GuiFile.xml and exit.
**-x**
:   Enable auto-exit after first expose.
**\--list**
:   List parsed definitions.
**\--fatal-warnings**
:   Turn criticals/warnings into fatal conditions.
**\--snapshot** *pngname*
:   Dump a snapshot to *pngname*.
**\--test-dump**
:   Dump test stream after first expose.
**\--test-matched-node** *PATTERN*
:   Filter nodes in test dumps.
**-h**, **\--help**
:   Display this help and exit.
**-v**, **\--version**
:   Display version and exit.


# AUTHORS
Tim Janik.


# SEE ALSO

**rapidres**(1), [**Rapicorn Website**](http://rapicorn.org)
