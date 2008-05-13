/* Rapicorn                                                     -*-mode:c++;-*-
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
%option 8bit reentrant stack noyywrap yylineno
%option ansi-definitions ansi-prototypes nounistd
%option bison-bridge bison-locations
%option nodefault warn
%option prefix="flex_yy"

/* === DECLARATIONS === */
%{
/* Sinfex &YYSELF is provided by sinfex.cc */
#define YY_INPUT(buf,result,max_size)    { result = YYSELF.yyread (max_size, buf); }
%}

/* === PATTERNS === */
WHITESPACE              [ \t\v\f\r]
DQSTRING                \"([^\"\\]+|\\.|\\\n)*\"
SQSTRING                \'([^\'\\]+|\\.|\\\n)*\'
STRING                  ({DQSTRING}|{SQSTRING})
INTEGER                 [1-9][0-9]*
FULLFLOAT               ([1-9][0-9]*|0)(\.[0-9]*)?([eE][+-]?[0-9]+)?
FRACTFLOAT              \.[0-9]+([eE][+-]?[0-9]+)?
FLOAT                   ({FULLFLOAT}|{FRACTFLOAT})
IDENTIFIER              [a-z_A-Z][0-9a-z_A-Z]*

/* === RULES === */
%%
{WHITESPACE}+           { /* return ANYSPACE; */ }
\n                      { /* return '\n'; */ }

{INTEGER}               { if (yyleng <= 17) { yylval->li = strtoull (yytext, NULL, 0); return INTEGER; }
			  else { yylval->df = strtold (yytext, NULL); return FLOAT; } }

{FLOAT}                 { yylval->df = strtold (yytext, NULL); return FLOAT; }

{STRING}                { yylval->cs = new String (yytext); return STRING; }

or                      { return OR; }
and                     { return AND; }
not                     { return NOT; }

{IDENTIFIER}            { yylval->cs = new String (yytext); return IDENT; }

!=                      { return NE; }
==                      { return EQ; }
\<=                     { return LE; }
\>=                     { return GE; }
\*\*                    { return POW; }

.                       { return yytext[0]; }
%%
