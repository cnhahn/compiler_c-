/* $Id: lexer.l,v 1.7 2016-10-27 19:45:59-07 - - $ */

%{

#include "lyutils.h"
#include "auxlib.h"

#define YY_USER_ACTION  { lexer::advance(); }

int yylval_token (int symbol) {
   yylval = new astree (symbol, lexer::lloc, yytext);
   fprintf(tokfile, "%4zd %3zd.%.3zd %3d %-13s (%s)\n",
    lexer::lloc.filenr, lexer::lloc.linenr, lexer::lloc.offset,
    symbol, parser::get_tname(symbol), yytext);
   return symbol;
}

void includetok(){
    lexer::include();
    fprintf(tokfile, "%s\n", yytext);
}

%}

%option 8bit
%option debug
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option warn
/*%option verbose*/

LETTER          [A-Za-z_]
DIGIT           [0-9]
IDENT           ({LETTER}({LETTER}|{DIGIT})*)
CHARCON         ('([^\\'\n]|\\[\\'"0nt])')
STRINGCON       (\"([^\\"\n]|\\[\\'"0nt])*\")
NUMBER          {DIGIT}+

%%

"#".*           { includetok(); }
[ \t]+          { }
\n              { lexer::newline(); }

"if"            { return yylval_token (TOK_IF); }
"else"          { return yylval_token (TOK_ELSE); }
"while"         { return yylval_token (TOK_WHILE); }
"return"        { return yylval_token (TOK_RETURN); }
"int"           { return yylval_token (TOK_INT); }
"string"        { return yylval_token (TOK_STRING); }
"struct"        { return yylval_token (TOK_STRUCT); }
"void"          { return yylval_token (TOK_VOID); }
"new"           { return yylval_token (TOK_NEW); }
"null"          { return yylval_token (TOK_NULL); }

{CHARCON}       { return yylval_token (TOK_CHARCON); }
{STRINGCON}     { return yylval_token (TOK_STRINGCON); }
{NUMBER}        { return yylval_token (TOK_INTCON); }
{IDENT}         { return yylval_token (TOK_IDENT); }

"[]"            { return yylval_token (TOK_NEWARRAY); }
"=="            { return yylval_token (TOK_EQ); }
"!="            { return yylval_token (TOK_NE); }
"<="            { return yylval_token (TOK_LE); }
">="            { return yylval_token (TOK_GE); }
"<"             { return yylval_token (TOK_LT); }
">"             { return yylval_token (TOK_GT); }

"="             { return yylval_token ('='); }
"+"             { return yylval_token ('+'); }
"-"             { return yylval_token ('-'); }
"*"             { return yylval_token ('*'); }
"/"             { return yylval_token ('/'); }
"^"             { return yylval_token ('^'); }
";"             { return yylval_token (';'); }
"%"             { return yylval_token ('%'); }
"!"             { return yylval_token ('!'); }
","             { return yylval_token (','); }
"."             { return yylval_token ('.'); }
"("             { return yylval_token ('('); }
")"             { return yylval_token (')'); }
"}"             { return yylval_token ('}'); }
"{"             { return yylval_token ('{'); }
"["             { return yylval_token ('['); }
"]"             { return yylval_token (']'); }

.               { lexer::badchar (*yytext); }

%%
