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
#include "sinfeximpl.hh"

namespace Rapicorn {

struct Sinfex { // YYSELF - main lexer & parser state keeping
  String input;
  uint   input_offset;
  SinfexExpressionStack estk;
  explicit      Sinfex  () : input_offset (0) {}
  int           yyread          (int         max_size,
                                 char       *buffer);
  void          parse_error     (const char *error_message,
                                 int         first_line,
                                 int         first_column, // unmaintained
                                 int         last_line,
                                 int         last_column)  // unmaintained
  {
    printerr ("<string>:%u: Sinfex: parse error\n", first_line);
  }
  void          yyparse_string  (const String &string);
};

int
Sinfex::yyread (int   max_size,
                char *buffer)
{
  if (input_offset < input.size())
    {
      int l = MIN (input.size() - input_offset, (uint) max_size);
      memcpy (buffer, input.c_str() + input_offset, l);
      input_offset += l;
      return l;
    }
  return 0;
}

void
sinfex_parse_eval_string (const String &string)
{
  Sinfex yyself;

  yyself.yyparse_string (string);

  SinfexExpression expr (yyself.estk);
  Value v = expr.eval (NULL);
  String vstr = v.tostring();
  printf ("= %s\n", vstr.c_str());
}

} // Rapicorn

namespace { // Anon
using namespace Rapicorn;

#define YYESTK  YYSELF.estk

/* Sinfex accessor for sinfex.l and sinfex.y */
#define YYSELF  (*(Sinfex*) flex_yyget_extra (yyscanner))
extern void* flex_yyget_extra (void *yyscanner); // flex forward declaration, needed by bison

/* bison generated parser */
#include "sinfex.ygen"
#undef  yylval
#undef  yylloc

/* flex generated lexer */
#include "sinfex.lgen"
#undef  yylval
#undef  yylloc
#undef  YYSELF

/* glue scanning layers of flex and bison together */
static int
bison_yylex (YYSTYPE *yylval_param, // %union from sinfex.y
             YYLTYPE *yylloc_param, // struct { int first_line, first_column, last_line, last_column; }
             void    *yyscanner)
{
  /* this function implements bison's scanning stub with flex
   * and completes the rudimentary yylloc handling of flex.
   */
  yylloc_param->first_line = yylloc_param->last_line;
  yylloc_param->first_column = yylloc_param->last_column;
  int result = flex_yylex (yylval_param, yylloc_param, yyscanner);
  yylloc_param->last_line = flex_yyget_lineno (yyscanner);
  yylloc_param->last_column = flex_yyget_column (yyscanner);
  return result;
  /* silence the compiler about some unused functions */
  void *dummy[] RAPICORN_UNUSED = {
    (void*) &yyunput, (void*) &yy_top_state,
    (void*) &yy_push_state, (void*) &yy_pop_state,
  };
}

} // Anon

/* encapsulate bison parsing and flex setup & teardown */
void
Rapicorn::Sinfex::yyparse_string (const String &string)
{
  input = string;
  input_offset = 0;
  void *yyscanner = NULL;
  flex_yylex_init (&yyscanner);
  flex_yyset_extra (this, yyscanner);
  yyparse (yyscanner);
  flex_yylex_destroy (yyscanner);
}

/* lots of stale definitions and declarations from sinfex.ygen and sinfex.lgen
 * are polluting the namespaces at this point, so no additional code should go
 * below here.
 */
