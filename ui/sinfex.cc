// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include "sinfeximpl.hh"
#include <stdio.h>      // needed by sinfex.lgen or sinfex.ygen

namespace Rapicorn {

/* --- Sinfex standard functions --- */
class StandardScope : public Sinfex::Scope {
  Scope &chain_scope_;
  typedef Sinfex::Value Value;
  double
  vdouble (const Value &v)
  {
    return v.real();
  }
  Value
  printfunc (uint                 stdnum,
             const vector<Value> &args)
  {
    String s;
    for (uint i = 0; i < args.size(); i++)
      s += args[i].string();
    if (stdnum == 1)
      printout ("%s", s.c_str());
    else if (stdnum == 2)
      printerr ("%s", s.c_str());
    return Value ("");
  }
  enum SeqType { SEQCOUNT, SEQMIN, SEQMAX, SEQSUM, SEQAVG, };
  Value
  seqfunc (const SeqType        seqtype,
           const vector<Value> &args)
  {
    double accu = args.size() ? vdouble (args[0]) : 0;
    if (seqtype == SEQCOUNT)
      return Value (args.size());
    for (uint i = 1; i < args.size(); i++)
      {
        double v = vdouble (args[i]);
        switch (seqtype)
          {
          case SEQMIN:  accu = MIN (accu, v);   break;
          case SEQMAX:  accu = MAX (accu, v);   break;
          case SEQAVG:
          case SEQSUM:  accu += v;              break;
          default: ;
          }
      }
    if (seqtype == SEQAVG && args.size())
      accu /= args.size();
    return Value (accu);
  }
public:
  StandardScope (Scope &chain) :
    chain_scope_ (chain)
  {}
  virtual Value
  resolve_variable (const String        &entity,
                    const String        &name)
  {
    return chain_scope_.resolve_variable (entity, name);
  }
#define MATH1FUNC(func,args)    Value (({ double v = func (vdouble (args[0])); v; }))
#define RETURN_IF_MATH1FUNC(func,args,nm)     ({ if (nm == #func) return MATH1FUNC (func, args); })
#define MATH2FUNC(func,args)    Value (({ double v = func (vdouble (args[0]), vdouble (args[1])); v; }))
#define RETURN_IF_MATH2FUNC(func,args,nm)     ({ if (nm == #func) return MATH2FUNC (func, args); })
  virtual Value
  call_function (const String        &entity,
                 const String        &name,
                 const vector<Value> &args)
  {
    if (entity == "")
      {
        if (args.size() == 0)
          {
            if (name == "rand")
              return Value (drand48());
          }
        else if (args.size() == 1)
          {
            if (name == "bool")
              return Value (args[0].asbool());
            else if (name == "strbool")
              return Value (string_to_bool (args[0].string()));
            else if (name == "real")
              return Value (args[0].real());
            RETURN_IF_MATH1FUNC (ceil,  args, name);
            RETURN_IF_MATH1FUNC (floor, args, name);
            RETURN_IF_MATH1FUNC (round, args, name);
            RETURN_IF_MATH1FUNC (log10, args, name);
            RETURN_IF_MATH1FUNC (log2, args, name);
            RETURN_IF_MATH1FUNC (log, args, name);
            RETURN_IF_MATH1FUNC (exp, args, name);
          }
        else if (args.size() == 2)
          {
            RETURN_IF_MATH2FUNC (hypot, args, name);
          }
        if (name == "count")
          return seqfunc (SEQCOUNT, args);
        else if (name == "min")
          return seqfunc (SEQMIN, args);
        else if (name == "max")
          return seqfunc (SEQMAX, args);
        else if (name == "sum")
          return seqfunc (SEQSUM, args);
        else if (name == "avg")
          return seqfunc (SEQAVG, args);
        else if (name == "printout")
          return printfunc (1, args);
        else if (name == "printerr")
          return printfunc (2, args);
      }
    return chain_scope_.call_function (entity, name, args);
  }
};

/* --- Sinfex Class --- */
Sinfex::Sinfex () :
  start_ (NULL)
{}

Sinfex::~Sinfex ()
{
  if (start_)
    {
      delete[] start_;
      start_ = NULL;
    }
}

Sinfex::Value::Value (const String &s) :
  string_ (string_from_cquote (s)), real_ (0), strflag_ (1)
{}

String
Sinfex::Value::real2string () const
{
  return string_from_double (real_);
}

double
Sinfex::Value::string2real () const
{
  const char *str = string_.c_str();
  while (*str && (str[0] == ' ' || str[0] == '\t'))
    str++;
  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    return string_to_uint (str);
  else
    return string_to_double (str);
}

/* --- SinfexExpression Class --- */
class SinfexExpression : public virtual Sinfex {
  inline int
  cmp_values (const Value &a, const Value &b)
  {
    /* like ECMAScript, perform string comparison only if both types are strings */
    if (RAPICORN_UNLIKELY (a.isstring()) && RAPICORN_UNLIKELY (b.isstring()))
      return strcmp (a.string().c_str(), b.string().c_str());
    else
      {
        /* numrical comparison */
        const double ar = a.real(), br = b.real();
        return ar - br < 0 ? -1 : ar - br > 0 ? +1 : 0;
      }
  }
  Value eval_op (Scope &scope, uint opx);
public:
  SinfexExpression (const SinfexExpressionStack &estk)
  {
    const uint *prog = estk.startmem();
    if (prog)
      {
        start_ = new uint[prog[0] / sizeof (uint)];
        memcpy (start_, prog, prog[0]);
      }
  }
  virtual Value
  eval (Scope &scope)
  {
    if (!start_ || !start_[1])
      return Value (0);
    else
      {
        StandardScope stdscope (scope);
        return eval_op (stdscope, start_[1]);
      }
  }
};

Sinfex::Value
SinfexExpression::eval_op (Scope &scope,
                           uint   opx)
{
  union Mark {
    const uint   *up;
    const int    *ip;
    const char   *cp;
    const double *dp;
  };
  Mark mark;
  mark.up = start_ + opx;
  assert (mark.up + 1 <= start_ + start_[0]);
  switch (*mark.up++)
    {
      uint ui;
    case SINFEX_0:      return Value (0);
    case SINFEX_REAL:
      if (ptrdiff_t (mark.up) & 0x7)
        mark.up++;
      return Value (*mark.dp);
    case SINFEX_STRING:
      ui = *mark.up++;
      return Value (String (mark.cp, ui));
    case SINFEX_VARIABLE:
      {
        ui = *mark.up++;
        String varname = String (mark.cp, ui);
        Value result = scope.resolve_variable ("", varname);
        return result;
      }
    case SINFEX_ENTITY_VARIABLE:
      {
        String v1, v2;
        ui = *mark.up++;
        String entity = String (mark.cp, ui);
        mark.up += (ui + 3) / 4;
        ui = *mark.up++;
        String varname = String (mark.cp, ui);
        v2 = String (mark.cp, ui);
        Value result = scope.resolve_variable (entity, varname);
        return result;
      }
    case SINFEX_OR:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (a.asbool() ? a : b);
      }
    case SINFEX_AND:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (a.asbool() ? b : a);
      }
    case SINFEX_NOT:
      {
        const Value &a = eval_op (scope, *mark.up++);
        return Value (!a.asbool());
      }
    case SINFEX_NEG:
      {
        const Value &a = eval_op (scope, *mark.up++);
        return Value (-a.real());
      }
    case SINFEX_POS:
      {
        const Value &a = eval_op (scope, *mark.up++);
        return Value (a.real());
      }
    case SINFEX_ADD:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        /* like ECMAScript, perform string concatenation if either type is string */
        if (a.isstring() || b.isstring())
          return Value (a.string() + b.string());
        else
          return Value (a.real() + b.real());
      }
    case SINFEX_SUB:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (a.real() - b.real());
      }
    case SINFEX_MUL:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (a.real() * b.real());
      }
    case SINFEX_DIV:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        double ar = a.real(), br = b.real();
        /* like ECMAScript, produce +-Infinity for division by zero */
        return Value (br != 0 ? ar / br : copysign (ar == 0 ? nanl ("0xbad0") : INFINITY, ar * br));
      }
    case SINFEX_POW:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (pow (a.real(), b.real()));
      }
    case SINFEX_EQ:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (cmp_values (a, b) == 0);
      }
    case SINFEX_NE:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (cmp_values (a, b) != 0);
      }
    case SINFEX_LT:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (cmp_values (a, b) < 0);
      }
    case SINFEX_GT:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (cmp_values (a, b) > 0);
      }
    case SINFEX_LE:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (cmp_values (a, b) <= 0);
      }
    case SINFEX_GE:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        return Value (cmp_values (a, b) >= 0);
      }
    case SINFEX_FUNCTION:
      {
        Mark arg;
        arg.up = start_ + *mark.up++;
        ui = *mark.up++;
        vector<Value> funcargs;
        while (arg.up > start_ && *arg.up++ == SINFEX_ARG)
          {
            uint valindex = *arg.up++;
            arg.up = start_ + *arg.up;
            funcargs.push_back (eval_op (scope, valindex));
          }
        String funcname = String (mark.cp, ui);
        Value result = scope.call_function ("", funcname, funcargs);
        return result;
      }
    case SINFEX_ARG: ;
    }
  fatal ("Expression with invalid op code: (%p:%u:%u)", start_, mark.up - 1 - start_, *(mark.up-1));
}

/* --- Parser Class --- */
struct SinfexParser { // YYSELF - main lexer & parser state keeping
  String input;
  uint   input_offset;
  SinfexExpressionStack estk;
  explicit      SinfexParser    () : input_offset (0) {}
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
SinfexParser::yyread (int   max_size,
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

SinfexP
Sinfex::parse_string (const String &expression)
{
  SinfexParser yyself;
  yyself.yyparse_string (expression);
  return SinfexP (new SinfexExpression (yyself.estk));
}

} // Rapicorn

namespace { // Anon
using namespace Rapicorn;

#define YYESTK  YYSELF.estk

/* SinfexParser accessor for sinfex.l and sinfex.y */
#define YYSELF  (*(SinfexParser*) flex_yyget_extra (yyscanner))
extern void* flex_yyget_extra (void *yyscanner); // flex forward declaration, needed by bison

// *** Adjust GCC diagnostics for ygen/lgen code that we cannot fix
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"

/* bison generated parser */
#include "sinfex.ygen"
#undef  yylval
#undef  yylloc
#undef  YY_NULL // work around bison & flex disagreeing on nullptr vs 0

/* flex generated lexer */
#include "sinfex.lgen"
#undef  yylval
#undef  yylloc
#undef  YYSELF

// *** Restore GCC diagnostics after ygen/lgen code
#pragma GCC diagnostic pop

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
Rapicorn::SinfexParser::yyparse_string (const String &string)
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
