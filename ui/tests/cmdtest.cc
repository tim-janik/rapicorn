/* This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 */
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
using namespace Rapicorn;

static String
cmdjoin (const String &name, const StringSeq &args)
{
  return name + ";" + string_join (";", args);
}

static void
test_command_scanning (void)
{
  String name;
  StringSeq args;
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
REGISTER_UITHREAD_TEST ("Commands/Scanning", test_command_scanning);

static void
test_command_unquoting (void)
{
  String ret;
  ret = command_string_unquote ("\"x\"");
  assert (ret == "x");
  ret = command_string_unquote (" \"foo\" ");
  assert (ret == "foo");
  ret = command_string_unquote (" \"foo\\\"boo\" ");
  assert (ret == "foo\"boo");
  ret = command_string_unquote (" 'foo' ");
  assert (ret == "foo");
  ret = command_string_unquote (" 'foo\\'boo' ");
  assert (ret == "foo'boo");
  ret = command_string_unquote ("''");
  assert (ret == "");
}
REGISTER_UITHREAD_TEST ("Commands/Unquoting", test_command_unquoting);
