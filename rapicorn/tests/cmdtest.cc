/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#include <rapicorn-core/testutils.hh>
#include <rapicorn/rapicorn.hh>
using namespace Rapicorn;

static String
cmdjoin (const String       &name,
         const StringVector &args)
{
  return name + ";" + string_join (";", args);
}

static void
test_command_scanning (void)
{
  String name;
  StringVector args;
  bool success;
  success = command_scan (" cmd ", &name, &args);
  assert (success && cmdjoin (name, args) == "cmd;");
  success = command_scan ("x(1)", &name, &args);
  assert (success && cmdjoin (name, args) == "x;1");
  success = command_scan ("cmd(1,2)", &name, &args);
  assert (success && cmdjoin (name, args) == "cmd;1;2");
  success = command_scan ("cmd('foo\"bar','baz,baz')", &name, &args);
  assert (success && cmdjoin (name, args) == "cmd;'foo\"bar';'baz,baz'");
  success = command_scan ("cmd(\"foo'bar\",\"baz,baz\")", &name, &args);
  assert (success && cmdjoin (name, args) == "cmd;\"foo'bar\";\"baz,baz\"");
  success = command_scan ("cmd ((1,2,3), 'foo\\'bar')", &name, &args);
  assert (success && cmdjoin (name, args) == "cmd;(1,2,3); 'foo\\'bar'");
  success = command_scan ("cmd ((1,'))((',3), \"foo\\\"bar\")", &name, &args);
  assert (success && cmdjoin (name, args) == "cmd;(1,'))((',3); \"foo\\\"bar\"");
  // printerr ("\nergo (success=%d): %s\n", success, cmdjoin (name, args).c_str());
}

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  Test::add ("Commands/Scanning", test_command_scanning);

  return Test::run();
}
