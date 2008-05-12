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
%locations
%pure-parser
%name-prefix="bison_yy"
%parse-param {void *yyscanner}

/* === PRE-UNION DECLARATIONS === */
%{
/* Sinfex &YYSELF is provided by sinfex.cc */
%}

%union { // defines YYSTYPE
  double        df;
  long long int li;
  Exon         *xx;
  ExonArgs     *xa;
  ExonString   *xs;
}

/* === POST-UNION DECLARATIONS === */
%{
static void   bison_yyerror  (YYLTYPE    *yylloc_param,
                              void       *yyscanner,
                              const char *error_message);
static int    bison_yylex    (YYSTYPE    *yylval_param,	/* impl in sinfex.cc */
                              YYLTYPE    *yylloc_param,
                              void       *yyscanner);   /* YYLEX_PARAM */
#define YYLEX_PARAM yyscanner // pass yyscanner as last arg to yylex()
%}

/* === TOKENS & OPERATORS === */
%token	    POW
%token <li> INTEGER
%token <df> FLOAT
%token <xs> STRING IDENT
%type  <xx> start input expr num single
%type  <xa> args fargs
%destructor { if ($$) delete $$; $$ = NULL; } STRING IDENT
%destructor { if ($$) delete $$; $$ = NULL; } start input expr num single
%destructor { if ($$) delete $$; $$ = NULL; } args fargs
/* precedence increases per line */
%left OR
%left AND
%left NOT
%left EQ NE
%left '<' '>' LE GE
%left '-' '+'
%left P0n
%left '*' '/'
%left SIGN	// unary minus precedence
%right POW

/* === RULES === */
%%
start   : input                 { YYSELF.exon = $1; $$ = NULL; }
;
input   : /* empty */		{ $$ = NULL; }
	| expr  		{ $$ = $1; }
;
args	: expr			{ $$ = new ExonArgs(); $$->add (*$1); }
	| args ',' expr		{ $$ = $1; $$->add (*$3); }
;
fargs	: /*< empty */		{ $$ = new ExonArgs(); }
	| args			{ $$ = $1; }
;
expr	: single                { $$ = $1; }
	| IDENT '(' fargs ')'	{ $$ = new ExonFun (*$1, dynamic_cast<ExonArgs&> (*$3)); }
	| expr OR  expr		{ $$ = new ExonOr (*$1, *$3); }
	| expr AND expr		{ $$ = new ExonAnd (*$1, *$3); }
	| NOT expr		{ $$ = new ExonNot (*$2); }
	| expr NE  expr		{ $$ = new ExonCmp (*$1, 99, *$3); }
	| expr EQ  expr		{ $$ = new ExonCmp (*$1,  0, *$3); }
	| expr LE  expr		{ $$ = new ExonCmp (*$1, -2, *$3); }
	| expr GE  expr		{ $$ = new ExonCmp (*$1, +2, *$3); }
	| expr '<' expr		{ $$ = new ExonCmp (*$1, -1, *$3); }
	| expr '>' expr		{ $$ = new ExonCmp (*$1, +1, *$3); }
	| expr '+' expr		{ $$ = new ExonAdd (*$1, *$3); }
	| expr '-' expr		{ $$ = new ExonSub (*$1, *$3); }
	| expr '*' expr		{ $$ = new ExonMul (*$1, *$3); }
	| expr '/' expr		{ $$ = new ExonDiv (*$1, *$3); }
	| '-' expr %prec SIGN	{ $$ = new ExonSign ('-', *$2); }
	| '+' expr %prec SIGN	{ $$ = new ExonSign ('+', *$2); }
	| expr POW expr		{ $$ = new ExonPow (*$1, *$3); }
	| '(' expr ')'		{ $$ = $2; }
;
single	: num		        { $$ = $1; }
	| STRING	        { $$ = $1; }
	| IDENT '.' IDENT	{ $$ = new ExonVar (*$1, *$3); }
;
num	: INTEGER               { $$ = new ExonReal ($1); }
	| FLOAT                 { $$ = new ExonReal ($1); }
;
%%
/* === FUNCTIONS === */

static void
bison_yyerror (YYLTYPE    *yylloc_param,
               void       *yyscanner,
               const char *error_message)
{
  YYSELF.parse_error (error_message,
                      yylloc_param->first_line, yylloc_param->first_column,
                      yylloc_param->last_line, yylloc_param->last_column);
}
