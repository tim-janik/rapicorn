/* Rapicorn
 * Copyright (C) 2008-2010 Tim Janik
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
#ifndef __RAPICORN_ROPE_HH__
#define __RAPICORN_ROPE_HH__

#include <ui/primitives.hh>
#include <rcore/plicutils.hh>

namespace Rapicorn {

typedef Plic::FieldBuffer       FieldBuffer;
typedef Plic::FieldBufferReader FieldBufferReader;

uint64          rope_thread_start (const String         &application_name,
                                   const vector<String> &cmdline_args,
                                   int                   cpu = -1);
FieldBuffer*    rope_thread_call  (FieldBuffer *call);

} // Rapicorn

#endif /* __RAPICORN_ROPE_HH__ */
