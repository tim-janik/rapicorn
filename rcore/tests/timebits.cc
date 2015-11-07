// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>

namespace {
using namespace Rapicorn;

static inline uint32
quick_rand32 (void)
{
  static uint32 accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu;
}

static void
quick_timer_test()
{
  // test ctor, timing, dtor
  {
    QuickTimer tr (0);
    TASSERT (1);
    while (!tr.expired())
      ;
    TASSERT (1);
    while (tr.expired())
      ;
    TASSERT (1);
    while (!tr.expired())
      ;
    TASSERT (1);
  }
  // test real timing
  {
    uint64 i, accu = 0;
    QuickTimer qtimer (20 * 1000); // µseconds
    const uint64 big = ~uint64 (0), benchstart = timestamp_realtime();
    for (i = 0; i < big; i++)
      {
        // iterate over some time consuming stuff
        accu += quick_rand32();
        // poll quick_timer to avoid stalling the user
        if (qtimer.expired())
          break;
      }
    const uint64 benchdone = timestamp_realtime();
    TASSERT (accu > 0);
    TASSERT (i != big);
    if (Test::verbose())
      printout ("QuickTimer: loop performed %u runs in %uµs and accumulated: accu=%u average=%u\n",
                size_t (i), size_t (benchdone - benchstart), size_t (accu), size_t (accu / i));
  }
}
REGISTER_TEST ("QuickTimer/Test 20usec expiration", quick_timer_test);

static const char ini_testfile[] = {
  "\n"  // empty line
  "# hash comment\n"
  "; semicolon comment\n"
  "\t# indented comment1\n"
  "  ; indented comment2\n"
  "~LINE-WITH-JUNK~\n"
  "[simple-section]\n"
  "key1=1\n"
  "  indented = value\n"
  "  empty =\r\n"
  "  colon : assignment\n"
  "  simple-key = simple-value # comment\n"
  "  string-key = string 'with # Hash'\n"
  "  name[de] = DE localized key\n"
  "  name[\"en_US.UTF-8\"] = US localized key\n"
  "[Section With Spaces And Comment] # comment\n"
  "  longvalue1 = value contains\\\n line continuation\n"
  "  longvalue2 = value contains\\\r\n line continuation\r\n"
  "  spacings = ' \tspacing@start' and \"spacing@end\f\t \"\n"
};

static void
test_ini_files()
{
  IniFile inifile (Blob::from (ini_testfile));
  StringVector rv = inifile.raw_values();
  TASSERT (rv.size() == 11);
  for (auto kv : rv)
    {
      ssize_t eq = kv.find ('=');
      assert (eq >= 0);
      printout ("  %s=%s\n", kv.substr (0, eq).c_str(), CQUOTE (IniFile::cook_string (kv.substr (eq + 1))));
    }
}
REGISTER_OUTPUT_TEST ("IniFiles/Parsing", test_ini_files);

} // Anon
