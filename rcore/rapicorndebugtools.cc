/* Rapicorn
 * Copyright (C) 2007 Tim Janik
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
#include "rapicorndebugtools.hh"
#include "rapicornthread.hh"
#include "regex.hh"
#include "markup.hh"
#include <stdio.h>
#include <syslog.h>
#include <errno.h>

#ifndef _ // FIXME
#define _(x)    (x)
#endif

namespace Rapicorn {

DebugChannel::DebugChannel()
{}

DebugChannel::~DebugChannel ()
{}

struct DebugChannelFileAsync : public virtual DebugChannel, public virtual Thread {
  FILE                    *fout;
  uint                     skip_count;
  Atomic::RingBuffer<char> aring;
  ~DebugChannelFileAsync()
  {
    if (fout)
      fclose (fout);
  }
  DebugChannelFileAsync (const String &filename) :
    Thread ("DebugChannelFileAsync::logger"),
    fout (NULL), skip_count (0), aring (65536)
  {
    fout = fopen (filename.c_str(), "w");
    if (fout)
      start();
  }
  virtual void
  printf_valist (const char *format,
                 va_list     args)
  {
    const int bsz = 4000;
    char buffer[bsz + 2];
    int l = vsnprintf (buffer, bsz, format, args);
    if (l > 0)
      {
        l = MIN (l, bsz);
        if (buffer[l - 1] != '\n')
          {
            buffer[l++] = '\n';
            buffer[l] = 0;
          }
        if (false) // skip trailing 0 in ring buffer
          buffer[l++] = 0;
        uint n = aring.write (l, buffer, false);
        if (!n)
          Atomic::uint_add (&skip_count, 1);
      }
  }
  virtual void
  run ()
  {
    do
      {
        char buffer[65536];
        uint n;
        do
          {
            n = aring.read (sizeof (buffer), buffer);
            if (n)
              {
                fwrite (buffer, n, 1, fout);
                fflush (fout);
              }
            else
              {
                uint j;
                do
                  j = Atomic::uint_get (&skip_count);
                while (!Atomic::uint_cas (&skip_count, j, 0));
                if (j)
                  fprintf (fout, "...[skipped %u messages]\n", j);
              }
          }
        while (n);
      }
    while (Thread::Self::sleep (15 * 1000));
  }
};

DebugChannel*
DebugChannel::new_from_file_async (const String &filename)
{
  DebugChannelFileAsync *dcfa = new DebugChannelFileAsync (filename);
  return dcfa;
}

TestStream::TestStream ()
{}

TestStream::~TestStream ()
{}

static String
value_escape (const String &text)
{
  /* escape XML attribute values for test dumps */
  const char *c = &*text.begin();
  size_t l = text.size();
  String str;
  for (size_t i = 0; i < l; i++)
    switch (c[i])
      {
      case '<':
        str += "&lt;";
        break;
      case '&':
        str += "&amp;";
        break;
      case '"':
        str += "&quot;";
        break;
      default:
        str.append (c + i, 1);
        break;
      }
  return str;
}

class TestStreamImpl : public TestStream {
  vector<String> node_stack;
  vector<String> node_matches;
  vector<String> node_unmatches;
  String         indent;
  String         dat;
  bool           node_open, nodematch;
  uint           ignore_count;
  bool // true == ignore node
  check_ignore_node (const String &name)
  {
    for (uint i = 0; i < node_unmatches.size(); i++)
      if (Regex::match_simple (node_unmatches[i], name,
                               Regex::EXTENDED | Regex::ANCHORED,
                               Regex::MATCH_NORMAL))
        return true;
    return false;
  }
  bool // true == show node
  check_match_node (const String &name)
  {
    for (uint i = 0; i < node_matches.size(); i++)
      if (Regex::match_simple (node_matches[i], name,
                               Regex::EXTENDED | Regex::ANCHORED,
                               Regex::MATCH_NORMAL))
        return true;
    return node_matches.size() < 1;
  }
  virtual void
  ddump (Kind          kind,
         const String &name,
         const String &val)
  {
    bool should_ignore;
    switch (kind)
      {
      case TEXT:
        return_if_fail (name == "");
        close_node();
        if (!ignore_count || nodematch)
          dat += value_escape (val) + "\n";
        break;
      case NODE:
        return_if_fail (val == "");
        close_node();
        node_stack.push_back (name);
        if (!ignore_count)
          should_ignore = check_ignore_node (name);
        if (ignore_count || should_ignore)
          ignore_count++;
        nodematch = check_match_node (name);
        if (!ignore_count || nodematch)
          dat += indent + "<" + name + "\n";
        node_open = true;
        push_indent();
        break;
      case POPNODE:
        close_node();
        pop_indent();
        if (!ignore_count || nodematch)
          dat += indent + "</" + node_stack.back() + ">\n";
        if (ignore_count)
          ignore_count--;
        node_stack.pop_back();
        nodematch = node_stack.size() < 1 || check_match_node (node_stack.back());
        break;
      case VALUE:
        if (!ignore_count || nodematch)
          {
            if (node_open)
              dat += indent + name + "=\"" + value_escape (val) + "\"\n";
            else
              dat += indent + "<ATTRIBUTE " + name + "=\"" + value_escape (val) + "\"/>\n";
          }
        break;
      case INTERN:
        close_node();
        if (!ignore_count || nodematch)
          dat += indent + "<INTERN " + name + "=\"" + value_escape (val) + "\"/>\n";
        break;
      case INDENT:
        if (!ignore_count || nodematch)
          indent += "  ";
        break;
      case POPINDENT:
        if (!ignore_count || nodematch)
          indent = indent.substr (0, indent.size() - 2);
        break;
      }
  }
  void
  close_node ()
  {
    if (!node_open)
      return;
    if (!ignore_count || nodematch)
      dat += indent.substr (0, indent.size() - 2) + ">\n";
    node_open = false;
  }
  virtual String
  string ()
  {
    return dat;
  }
  virtual void
  filter_matched_nodes (const String &matchpattern)
  {
    if (node_unmatches.size() < 1)
      ignore_count++; // ignore nodes by default now
    node_matches.push_back (matchpattern);
  }
  virtual void
  filter_unmatched_nodes (const String &matchpattern)
  {
    node_unmatches.push_back (matchpattern);
  }
public:
  TestStreamImpl() : node_open (false), nodematch (true), ignore_count (0)
  {}
};

TestStream*
TestStream::create_test_stream ()
{
  return new TestStreamImpl();
}

} // Rapicorn
