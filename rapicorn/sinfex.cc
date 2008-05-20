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

String
string_cunquote (const String &input) // FIXME
{
  uint i = 0;
  if (i < input.size() && (input[i] == '"' || input[i] == '\''))
    {
      const char qchar = input[i];
      i++;
      String out;
      bool be = false;
      while (i < input.size() && (input[i] != qchar || be))
        {
          if (!be && input[i] == '\\')
            be = true;
          else
            {
              if (be)
                switch (input[i])
                  {
                  case 'n':     out += '\n';            break;
                  case 'r':     out += '\r';            break;
                  case 't':     out += '\t';            break;
                  case 'b':     out += '\b';            break;
                  case 'f':     out += '\f';            break;
                  case 'v':     out += '\v';            break;
                  default:      out += input[i];        break;
                  }
              else
                out += input[i];
              be = false;
            }
          i++;
        }
      if (i < input.size() && input[i] == qchar)
        {
          i++;
          if (i < input.size())
            return input; // extraneous characters after string quotes
          return out;
        }
      else
        return input; // unclosed string quotes
    }
  else if (i == input.size())
    return input; // empty string arg: ""
  else
    return input; // missing string quotes
}

/* --- Sinfex standard functions --- */
class StandardScope : public Sinfex::Scope {
  Scope &m_chain_scope;
  typedef Sinfex::Value Value;
  double
  vdouble (const Value &v)
  {
    return v.isreal() ? v.real() : v.asbool();
  }
  enum SeqType { SEQMIN = 1, SEQMAX, SEQSUM, SEQAVG, };
  Value
  seqfunc (const SeqType        seqtype,
           const vector<Value> &args)
  {
    double accu = args.size() ? vdouble (args[0]) : 0;
    for (uint i = 1; i < args.size(); i++)
      {
        double v = vdouble (args[i]);
        switch (seqtype)
          {
          case SEQMIN:  accu = MIN (accu, v);   break;
          case SEQMAX:  accu = MAX (accu, v);   break;
          case SEQAVG:
          case SEQSUM:  accu += v;              break;
          }
      }
    if (seqtype == SEQAVG && args.size())
      accu /= args.size();
    return Value (accu);
  }
public:
  StandardScope (Scope &chain) :
    m_chain_scope (chain)
  {}
  virtual Value
  resolve_variable (const String        &entity,
                    const String        &name)
  {
    return m_chain_scope.resolve_variable (entity, name);
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
        else
          {
            if (name == "min")
              return seqfunc (SEQMIN, args);
            else if (name == "max")
              return seqfunc (SEQMAX, args);
            else if (name == "sum")
              return seqfunc (SEQSUM, args);
            else if (name == "avg")
              return seqfunc (SEQAVG, args);
          }
      }
    return m_chain_scope.call_function (entity, name, args);
  }
};

/* --- Sinfex Class --- */
Sinfex::Sinfex () :
  m_start (NULL)
{}

Sinfex::~Sinfex ()
{
  if (m_start)
    {
      delete[] m_start;
      m_start = NULL;
    }
}

Sinfex::Value::Value (const String &s) :
  m_string (string_cunquote (s)), m_real (0), m_strflag (1)
{}

String
Sinfex::Value::realstring () const
{
  return string_from_double (m_real);
}

/* --- SinfexExpression Class --- */
class SinfexExpression : public virtual Sinfex {
  inline int
  cmp_values (const Value &a, const Value &b)
  {
    if (RAPICORN_UNLIKELY (a.isstring()))
      {
        if (RAPICORN_UNLIKELY (!b.isstring()))
          return 1;
        return strcmp (a.string().c_str(), b.string().c_str());
      }
    else
      {
        if (RAPICORN_UNLIKELY (b.isstring()))
          return -1;
        return a.real() - b.real() < 0 ? -1 : a.real() - b.real() > 0 ? +1 : 0;
      }
  }
  Value eval_op (Scope &scope, uint opx);
  RAPICORN_PRIVATE_CLASS_COPY (SinfexExpression);
public:
  SinfexExpression (const SinfexExpressionStack &estk)
  {
    const uint *prog = estk.startmem();
    if (prog)
      {
        m_start = new uint[prog[0] / sizeof (uint)];
        memcpy (m_start, prog, prog[0]);
      }
  }
  virtual Value
  eval (Scope &scope)
  {
    if (!m_start || !m_start[1])
      return Value (0);
    else
      {
        StandardScope stdscope (scope);
        return eval_op (stdscope, m_start[1]);
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
  mark.up = m_start + opx;
  assert (mark.up + 1 <= m_start + m_start[0]);
  switch (*mark.up++)
    {
      uint ui;
    case SINFEX_0:      return Value (0);
    case SINFEX_REAL:   return Value (*mark.dp);
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
        if (a.isreal())
          return Value (-a.real());
        error ("incompatible value type for unary sign");
      }
    case SINFEX_POS:
      {
        const Value &a = eval_op (scope, *mark.up++);
        if (a.isreal())
          return a;
        error ("incompatible value type for unary sign");
      }
    case SINFEX_ADD:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        if (a.isreal() && b.isreal())
          return Value (a.real() + b.real());
        else if (a.isstring() && b.isstring())
          return Value (a.string() + b.string());
        else
          error ("incompatible value types in '+'\n");
      }
    case SINFEX_SUB:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        if (a.isreal() && b.isreal())
          return Value (a.real() - b.real());
        else
          error ("incompatible value types in '-'\n");
      }
    case SINFEX_MUL:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        if (a.isreal() && b.isreal())
          return Value (a.real() * b.real());
        else
          error ("incompatible value types in '*'\n");
      }
    case SINFEX_DIV:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        if (a.isreal() && b.isreal())
          return Value (b.real() != 0.0 ? a.real() / b.real() : nanl (0));
        else
          error ("incompatible value types in '/'\n");
      }
    case SINFEX_POW:
      {
        const Value &a = eval_op (scope, *mark.up++), &b = eval_op (scope, *mark.up++);
        if (a.isreal() && b.isreal())
          return Value (pow (a.real(), b.real()));
        else
          error ("incompatible value types in '*'\n");
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
        arg.up = m_start + *mark.up++;
        ui = *mark.up++;
        vector<Value> funcargs;
        while (arg.up > m_start && *arg.up++ == SINFEX_ARG)
          {
            uint valindex = *arg.up++;
            arg.up = m_start + *arg.up;
            funcargs.push_back (eval_op (scope, valindex));
          }
        String funcname = String (mark.cp, ui);
        Value result = scope.call_function ("", funcname, funcargs);
        return result;
      }
    case SINFEX_ARG: ;
    }
  error ("Expression with invalid op code: (%p:%u:%u)", m_start, mark.up - 1 - m_start, *(mark.up-1));
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

Sinfex*
Sinfex::parse_string (const String &expression)
{
  SinfexParser yyself;
  yyself.yyparse_string (expression);
  return new SinfexExpression (yyself.estk);
}

} // Rapicorn

namespace { // Anon
using namespace Rapicorn;

#define YYESTK  YYSELF.estk

/* SinfexParser accessor for sinfex.l and sinfex.y */
#define YYSELF  (*(SinfexParser*) flex_yyget_extra (yyscanner))
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
