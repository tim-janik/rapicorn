// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
using namespace Rapicorn;

#include "../../configure.h"    // for HAVE_READLINE_AND_HISTORY
#ifdef HAVE_READLINE_AND_HISTORY
#include <readline/readline.h>  // for --shell
#include <readline/history.h>   // for --shell
#endif

#define STRLINE         STRLINE2 (__LINE__)
#define STRLINE2(txt)   RAPICORN_STRINGIFY_ARG (txt) // must be defined *after* STRLINE

namespace {
using namespace Rapicorn;

static void
test_basics ()
{
  SinfexP sinfex = Sinfex::parse_string ("21");
  assert (sinfex != NULL);
  assert (sinfex->eval (*(Sinfex::Scope*) NULL).real() == 21);
}
REGISTER_UITHREAD_TEST ("Sinfex/Basics", test_basics);

static void
eval_expect_tests ()
{
  const char *eetests[] = {
    /* basics */
    STRLINE, "", "0",
    // STRLINE, "foo", "foo",
    // STRLINE, "``", "`",
    STRLINE, "17", "17",
    STRLINE, "not 0", "1",
    STRLINE, "not 1", "0",
    STRLINE, "not not 'ABC'", "1",
    // STRLINE, "before`not 0`", "before1",
    // STRLINE, "`not 0`after", "1after",
    // STRLINE, "before`not 0`after", "before1after",
    STRLINE, "not 0", "1",
    STRLINE, "not 1", "0",
    STRLINE, "not (0)", "1",
    STRLINE, "not 'string'", "0",
    /* logic */
    STRLINE, "0 or 0", "0",
    STRLINE, "0 or 1", "1",
    STRLINE, "1 or 0", "1",
    STRLINE, "0 or 1 or 0", "1",
    STRLINE, "1 and 1", "1",
    STRLINE, "0 and 1", "0",
    STRLINE, "1 and 0", "0",
    STRLINE, "1 and 1 and 1", "1",
    STRLINE, "not 3 == 4 and 4 == 4 and not 5 == 4 and 'OK'", "OK",
    STRLINE, "3 != 4 and not 4 != 4 and 5 != 4 and 'OK'", "OK",
    STRLINE, "not 4 < 3 and not 4 < 4 and 4 < 5 and 'OK'", "OK",
    STRLINE, "not 3 > 4 and not 4 > 4 and 5 > 4 and 'OK'", "OK",
    STRLINE, "not 4 <= 3 and 4 <= 4 and 4 <= 5 and 'OK'", "OK",
    STRLINE, "not 3 >= 4 and 4 >= 4 and 5 >= 4 and 'OK'", "OK",
    STRLINE, "'a' == 'a' and not 'a' == 'b' and not 'b' == 'a' and 'OK'", "OK",
    STRLINE, "not 'a' != 'a' and 'a' != 'b' and 'b' != 'a' and 'OK'", "OK",
    STRLINE, "not 'a' < 'a' and 'a' < 'b' and not 'b' < 'a' and 'OK'", "OK",
    STRLINE, "'a' <= 'a' and 'a' <= 'b' and not 'b' <= 'a' and 'OK'", "OK",
    STRLINE, "not 'a' > 'a' and not 'a' > 'b' and 'b' > 'a' and 'OK'", "OK",
    STRLINE, "'a' >= 'a' and not 'a' >= 'b' and 'b' >= 'a' and 'OK'", "OK",
    /* boolean conversion */
    STRLINE, "bool (0)", "0",
    STRLINE, "bool (1)", "1",
    STRLINE, "bool (2)", "1",
    STRLINE, "bool (0)", "0",
    STRLINE, "bool (1)", "1",
    STRLINE, "bool (2)", "1",
    STRLINE, "bool ('')", "0",
    STRLINE, "bool ('t')", "1",
    STRLINE, "bool ('f')", "1",
    STRLINE, "bool ('0')", "1",
    STRLINE, "bool (0x0)", "0",
    STRLINE, "bool (0x1)", "1",
    STRLINE, "bool (0x5)", "1",
    STRLINE, "bool (0x9)", "1",
    STRLINE, "bool (0x0b)", "1",
    // STRLINE, "`bool (0b)`", "0", // parse error
    /* boolean strings */
    STRLINE, "strbool ('n')", "0",
    STRLINE, "strbool ('y')", "1",
    STRLINE, "strbool ('Off')", "0",
    STRLINE, "strbool ('On')", "1",
    STRLINE, "strbool ('false')", "0",
    STRLINE, "strbool ('true')", "1",
    /* strings as numbers */
    STRLINE, "(1 * real ('12') + 1 + real ('2') - 1) / real ('2')", "7",
    STRLINE, "real ('123') == 123 and real ('123') != 122 and 'OK'", "OK",
    STRLINE, "real ('0x2') > 1 and real ('1') < 0x2 and real ('0x1234') == 4660 and 'OK'", "OK",
    STRLINE, "real ('2') >= 2 and real ('3') <= 3 and 'OK'", "OK",
    STRLINE, "'123' == 123 and '123' != 122 and 'OK'", "OK",
    STRLINE, "'0x2' > 1 and '1' < 0x2 and '0x1234' == 4660 and 'OK'", "OK",
    STRLINE, "'2' >= 2 and '3' <= 3 and 'OK'", "OK",
    STRLINE, "'21' == '21' and '21' != '21 ' and 'OK'", "OK",
    /* variables */
    // STRLINE, "plaintext", "plaintext",
    STRLINE, "empty", "",
    STRLINE, "zero", "0",
    STRLINE, "one", "1",
    // STRLINE, "before^`one`", "before^1",
    // STRLINE, "`one`^after", "1^after",
    // STRLINE, "before^`one`^after", "before^1^after",
    STRLINE, "empty == ''", "1",
    STRLINE, "zero == 0", "1",
    STRLINE, "one == 1", "1",
    STRLINE, "zero == real ('0')", "1",
    STRLINE, "one == real ('1')", "1",
    STRLINE, "zero == '0'", "1",
    STRLINE, "one == '1'", "1",
    /* standard variables */
    STRLINE, "RAPICORN_ARCHITECTURE == ''", "0",
    STRLINE, "RAPICORN_VERSION == ''", "0",
    /* functions */
    STRLINE, "count()", "0",
    STRLINE, "count ()", "0",
    STRLINE, "count(0)", "1",
    STRLINE, "count (0)", "1",
    STRLINE, "count(0,1)", "2",
    STRLINE, "count (0,1)", "2",
    STRLINE, "count(0, 1)", "2",
    STRLINE, "count (0, 1)", "2",
    STRLINE, "count (1, 2, 3)", "3",
    STRLINE, "rand() > 0 and rand() != rand() and 'OK'", "OK",
  };
  Evaluator ev;
  Evaluator::VariableMap map;
  const char *variables[] = {
    "empty=", "zero=0", "one=1",
  };
  for (uint i = 0; i < ARRAY_SIZE (variables); i++)
    {
      String key, value;
      if (Evaluator::split_argument (variables[i], key, value))
        map[key] = value;
    }
  ev.push_map (map);
  for (uint i = 0; i < ARRAY_SIZE (eetests); i += 3)
    {
      const char *sline = eetests[i], *expr = eetests[i + 1], *expect = eetests[i + 2];
      String result = ev.parse_eval (expr);
      if (result != expect)
        fatal ("%s:%s: Evaluator mismatch: %s != %s", __FILE__, sline, result.c_str(), expect);
    }
  ev.pop_map (map);
}
REGISTER_UITHREAD_TEST ("Sinfex/Expressions", eval_expect_tests);

// === Shell Mode ===
static RAPICORN_UNUSED char*
freadline (const char *prompt,
           FILE       *stream)
{
  int sz = 2;
  char *malloc_string = (char*) malloc (sz);
  if (!malloc_string)
    return NULL;
  malloc_string[0] = 0;
  if (prompt)
    fputs (prompt, stderr);
  char *start = malloc_string;
 continue_readline:
  if (!fgets (start, sz - (start - malloc_string), stdin))
    {
      if (start != malloc_string)
        return malloc_string;
      free (malloc_string);
      return NULL; // end of input
    }
  if (strchr (start, '\n'))
    return malloc_string;
  sz *= 2;
  char *newstring = (char*) realloc (malloc_string, sz);
  if (!newstring)
    {
      free (malloc_string);
      return NULL; // OOM during single line input
    }
  malloc_string = newstring;
  start = malloc_string + strlen (malloc_string);
  goto continue_readline;
}

struct EvalScope : public Sinfex::Scope {
  virtual Sinfex::Value
  resolve_variable (const String        &entity,
                    const String        &name)
  {
    printout ("VAR: %s.%s\n", entity.c_str(), name.c_str());
    return Sinfex::Value (0);
  }
  virtual Sinfex::Value
  call_function (const String                &entity,
                 const String                &name,
                 const vector<Sinfex::Value> &args)
  {
    printout ("FUNC: %s (", name.c_str());
    for (uint i = 0; i < args.size(); i++)
      printout ("%s%s", i ? ", " : "", args[i].string().c_str());
    printout (");\n");
    return Sinfex::Value (0);
  }
};

static void
sinfex_shell (void)
{
  bool interactive_prompt = isatty (fileno (stdin));
  char *malloc_string;
#ifdef HAVE_READLINE_AND_HISTORY
  rl_instream = stdin;
  rl_readline_name = "Rapicorn"; // for inputrc conditionals
  rl_bind_key ('\t', rl_insert);
  using_history ();
  stifle_history (999);
#endif
  do
    {
#ifdef HAVE_READLINE_AND_HISTORY
      malloc_string = readline (interactive_prompt ? "sinfex> " : "");
      if (malloc_string && malloc_string[0] != 0 && !strchr (" \t\v", malloc_string[0]))
        add_history (malloc_string);
#else
      malloc_string = freadline (interactive_prompt ? "sinfex> " : "", stdin);
#endif
      if (malloc_string)
        {
          SinfexP sinfex = Sinfex::parse_string (malloc_string);
          free (malloc_string);
          EvalScope scope;
          Sinfex::Value v = sinfex->eval (scope);
          String s = v.string();
          if (v.isreal())
            s = string_format ("%.15g", v.real());
          printf ("= %s\n", s.c_str());
        }
    }
  while (malloc_string);
  if (interactive_prompt)
    fprintf (stderr, "\n"); // newline after last prompt
}

} // Anon

namespace ServerTests {

void sinfex_shell_wrapper (void);

void
sinfex_shell_wrapper (void)
{
  uithread_test_trigger (sinfex_shell);
}

} // ServerTests
