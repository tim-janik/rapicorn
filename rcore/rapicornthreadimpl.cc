/* RapicornThreadImpl
 * Copyright (C) 2002-2006 Tim Janik
 * Copyright (C) 2002 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "rapicornconfig.h" // RAPICORN_HAVE_MUTEXATTR_SETTYPE
#if	(RAPICORN_HAVE_MUTEXATTR_SETTYPE > 0)
#define	_XOPEN_SOURCE   600	/* for full pthread facilities */
#endif	/* defining _XOPEN_SOURCE on random systems can have bad effects */
#include "utilities.hh"
#include "rapicornthread.hh"

namespace Rapicorn {

/* --- variables --- */
/* ::Rapicorn::ThreadTable must be a RapicornThreadTable, not a reference for the C API wrapper to work */
RapicornThreadTable ThreadTable = { NULL }; /* private, provided by rapicornthreadimpl.cc */


} // Rapicorn
