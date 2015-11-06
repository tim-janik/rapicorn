/* This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
 *                                                	-*-mode:c++;-*-
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
HEXNUM                  0[xX][0-9a-fA-F]+
FULLFLOAT               ([1-9][0-9]*|0)(\.[0-9]*)?([eE][+-]?[0-9]+)?
FRACTFLOAT              \.[0-9]+([eE][+-]?[0-9]+)?
FLOAT                   ({FULLFLOAT}|{FRACTFLOAT})
IDENTIFIER              [a-z_A-Z][0-9a-z_A-Z]*

/* === RULES === */
%%
{WHITESPACE}+           { /* return ANYSPACE; */ }
\n                      { /* return '\n'; */ }

{INTEGER}               { if (yyleng <= 17) { yylval->li = string_to_uint (yytext, 0); return INTEGER; }
			  else { yylval->df = string_to_double (yytext); return FLOAT; } }

{HEXNUM}                { yylval->li = string_to_uint (yytext, 0); return INTEGER; }

{FLOAT}                 { yylval->df = string_to_double (yytext, NULL); return FLOAT; }

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
