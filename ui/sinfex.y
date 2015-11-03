// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
//                                                      -*-mode:c++;-*-

%locations
%pure-parser
%define api.prefix {bison_yy}
%lex-param {void *yyscanner}
%parse-param {void *yyscanner}

/* === PRE-UNION DECLARATIONS === */
%{
/* Sinfex &YYSELF is provided by sinfex.cc */
%}

%union { // defines YYSTYPE
  double        df;
  long long int li;
  String       *cs;
  uint          pi; // SinfexProgram index
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
%token <cs> STRING IDENT
%type  <pi> num single fargs args expr input start
%destructor { if ($$) delete $$; $$ = NULL; } STRING IDENT
/* precedence increases per line */
%left OR
%left AND
%left NOT
%left EQ NE
%left '<' '>' LE GE
%left '-' '+'
%left '*' '/'
%left SIGN	// unary minus precedence
%right POW

/* === RULES === */
%%
start   : input                 { YYESTK.set_start ($1); $$ = 0; }
;
input   : /* empty */		{ $$ = 0; }
	| expr  		{ $$ = $1; }
;
args	: expr			{ $$ = YYESTK.push_arg ($1, 0); }
	| expr ',' args		{ $$ = YYESTK.push_arg ($1, $3); }
;
fargs	: /* empty */		{ $$ = 0; }
	| args			{ $$ = $1; }
;
expr	: single                { $$ = $1; }
	| IDENT '(' fargs ')'	{ $$ = YYESTK.push_func (*$1, $3); delete $1; $1 = NULL; }
	| expr OR  expr		{ $$ = YYESTK.push_or ($1, $3); }
	| expr AND expr		{ $$ = YYESTK.push_and ($1, $3); }
	| NOT expr		{ $$ = YYESTK.push_not ($2); }
	| expr NE  expr		{ $$ = YYESTK.push_ne ($1, $3); }
	| expr EQ  expr		{ $$ = YYESTK.push_eq ($1, $3); }
	| expr LE  expr		{ $$ = YYESTK.push_le ($1, $3); }
	| expr GE  expr		{ $$ = YYESTK.push_ge ($1, $3); }
	| expr '<' expr		{ $$ = YYESTK.push_lt ($1, $3); }
	| expr '>' expr		{ $$ = YYESTK.push_gt ($1, $3); }
	| expr '+' expr		{ $$ = YYESTK.push_add ($1, $3); }
	| expr '-' expr		{ $$ = YYESTK.push_sub ($1, $3); }
	| expr '*' expr		{ $$ = YYESTK.push_mul ($1, $3); }
	| expr '/' expr		{ $$ = YYESTK.push_div ($1, $3); }
	| '-' expr %prec SIGN	{ $$ = YYESTK.push_neg ($2); }
	| '+' expr %prec SIGN	{ $$ = YYESTK.push_pos ($2); }
	| expr POW expr		{ $$ = YYESTK.push_pow ($1, $3); }
	| '(' expr ')'		{ $$ = $2; }
;
single	: num		        { $$ = $1; }
	| STRING	        { $$ = YYESTK.push_string (*$1); delete $1; $1 = NULL; }
	| IDENT '.' IDENT	{ $$ = YYESTK.push_entity_variable (*$1, *$3);
				  delete $1; delete $3; $1 = $3 = NULL; }
	| IDENT                 { $$ = YYESTK.push_variable (*$1);
				  delete $1; $1 = NULL; }
;
num	: INTEGER               { $$ = YYESTK.push_double ($1); }
	| FLOAT                 { $$ = YYESTK.push_double ($1); }
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
