%{
// Dummy parser for scanner project.

#include <cassert>

#include "lyutils.h"
#include "astree.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%destructor { destroy ($$); } <*>
%printer { astree::dump (yyoutput, $$); } <>


%token TOK_VOID TOK_CHAR TOK_INT TOK_STRING TOK_NEWSTRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_NULL TOK_NEW TOK_ARRAY TOK_DECLID TOK_VARDECL
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE TOK_INDEX 
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON TOK_PROTOTYPE

%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_INITDECL TOK_RETURNVOID
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD
%token TOK_ORD TOK_CHR TOK_ROOT TOK_FUNCTION TOK_PARAMLIST

%right  TOK_IF TOK_ELSE
%right  '='
%left   TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left   '+' '-'
%left   '*' '/' '%'
%right  TOK_POS TOK_NEG '!' TOK_NEW 
%left   '[' '.' TOK_CALL
%nonassoc '(' ')'

%start start

%%

                                                                     
start         : program            { yyparse_astree = $1; }               
              ;  

program       : program structdef  { $$ = $1->adopt ($2); }               
              | program function   { $$ = $1->adopt ($2); }               
              | program statement  { $$ = $1->adopt ($2); }               
              | program error '}'  { $$ = $1; }                           
              | program error ';'  { $$ = $1; }                           
              |                    { $$ = new_parseroot (); }             
              ;                                                           

structdef     : structfront '}'   { destroy($2);
                                    $$ = $1; }
              ;

structfront   : TOK_STRUCT  TOK_IDENT '{'
                                  { destroy($3); 
                                    $2->symbol = TOK_TYPEID;
                                    $$ = $1->adopt($2); }
              | structfront fieldecl ';' 
                                  { destroy($3);
                                    $$ = $1->adopt($2); }
              ;

fieldecl      : basetype TOK_IDENT           
                                   { $2->symbol = TOK_FIELD;
                                     $$ = $1->adopt($2); }
              | basetype TOK_NEWARRAY TOK_IDENT
                                   { $3->symbol = TOK_FIELD;
                                     $$ = $1->adopt($3, $2);}
              ;

basetype      : TOK_VOID           { $$ = $1; } 
              | TOK_STRING         { $$ = $1; }
              | TOK_INT            { $$ = $1; }
              | TOK_IDENT          { $1->symbol = TOK_TYPEID;
                                     $$ = $1; }
              ;

function      : identdecl '(' ')' block
                                   { destroy($3);
                                     $$ = new_function($1, $2, $4); }
              | identdecl funkparam ')' block
                                   { destroy($3);
                                     $$ = new_function($1, $2, $4); }
              ;

funkparam     : '(' identdecl     { $1->symbol = TOK_PARAMLIST;
                                    $$ = $1->adopt($2); }
              | funkparam ',' identdecl
                                  { destroy($2);
                                    $$ = $1->adopt($3); }
              ;

identdecl     : basetype TOK_IDENT           
                                  { $2->symbol = TOK_DECLID;
                                    $$ = $2->adopt($1); }
              | basetype TOK_NEWARRAY TOK_IDENT
                                  { $3->symbol = TOK_DECLID;
                                    $$ = $3->adopt($1, $2); } 

block         : ';'               { $$ = $1 ; }
              |  blockfront '}'   { destroy($2); $$ = $1; }
              ;

blockfront    : '{'              { $1->symbol =TOK_BLOCK;
                                   $$ = $1; }
              | blockfront statement
                                 { $$ = $1->adopt($2); }
              ;
           

statement     : block             { $$ = $1; }
              | vardecl           { $$ = $1; }
              | while             { $$ = $1; }
              | ifelse            { $$ = $1; }
              | return            { $$ = $1; }
              | expr ';'          { destroy($2); $$ = $1; }
              ;

vardecl       : identdecl '=' expr ';' { destroy($4);
                                         $2->symbol = TOK_VARDECL; 
                                         $$ = $2->adopt($1, $3); }
              ;

while         : TOK_WHILE '(' expr ')' statement
                                       { destroy($2, $4);
                                         $$ = $1->adopt($3,$5); }
              ;

ifelse        : TOK_IF '(' expr ')' statement %prec TOK_IF
                                       { destroy($2, $4); 
                                         $$ = $1->adopt($3, $5); }
              | TOK_IF '(' expr ')' statement TOK_ELSE statement
                                       { destroy($2, $4, $6);
                                         $1->symbol = TOK_IFELSE; 
                                         $$ = $1->adopt($3, $5, $7); }
              ;

return        : TOK_RETURN ';'           { destroy($2); 
                                           $1->symbol = TOK_RETURNVOID;
                                           $$ = $1; }
              | TOK_RETURN expr ';'      { destroy($3);  
                                           $$ = $1->adopt($2); }
              ;

expr          : expr '=' expr         { $$ = $2->adopt ($1, $3); }
              | expr '+' expr         { $$ = $2->adopt ($1, $3); }
              | expr '-' expr         { $$ = $2->adopt ($1, $3); }
              | expr '*' expr         { $$ = $2->adopt ($1, $3); }
              | expr '/' expr         { $$ = $2->adopt ($1, $3); }
              | expr TOK_EQ expr      { $$ = $2->adopt ($1, $3); }
              | expr TOK_NE expr      { $$ = $2->adopt ($1, $3); }
              | expr TOK_LT expr      { $$ = $2->adopt ($1, $3); }
              | expr TOK_LE expr      { $$ = $2->adopt ($1, $3); }
              | expr TOK_GT expr      { $$ = $2->adopt ($1, $3); }
              | expr TOK_GE expr      { $$ = $2->adopt ($1, $3); }
              | '+' expr %prec TOK_POS    
                             { $$ = $1->adopt_sym ($2, TOK_POS); }
              | '-' expr %prec TOK_NEG    
                             { $$ = $1->adopt_sym ($2, TOK_NEG); }
              | '(' expr ')'         { destroy($1, $3); $$ = $2; }

              | allocator             { $$ = $1; } 
              | call                  { $$ = $1; }
              | variable              { $$ = $1; }
              | constant              { $$ = $1; } 
              | '!' expr              { $$ = $1->adopt($2);}
              ;

allocator     : TOK_NEW TOK_IDENT '(' ')'
                                      { destroy($3, $4); 
                                        $2->symbol = TOK_TYPEID;
                                        $$ = $1->adopt($2); }
              | TOK_NEW TOK_STRING '(' expr ')' 
                                      { destroy($2, $3, $5); 
                                        $1->symbol = TOK_NEWSTRING;
                                        $$ = $1->adopt($4); }
              | TOK_NEW basetype '[' expr ']'
                                      { destroy($3, $5); 
                                        $1->symbol = TOK_NEWARRAY;
                                        $$ = $1->adopt($2, $4); }
              ;

call          : TOK_IDENT '(' ')'      { destroy($3); 
                                        $2->symbol = TOK_CALL;
                                        $$ = $2->adopt($1); }
              | callfront  ')'         { destroy($2); $$ = $1; }
              ;

callfront     : TOK_IDENT '(' expr     { $2->symbol = TOK_CALL;
                                        $$ = $2->adopt($1, $3); }
              | callfront ',' expr     { destroy($2);
                                        $$ = $1->adopt($3); }
              ;

variable      : TOK_IDENT              { $$ = $1; }
              | expr '[' expr ']'      { destroy($4);
               $2->symbol = TOK_INDEX;
                                  $$ = $2->adopt($1, $3); }
              | expr '.' TOK_IDENT    { $3->symbol = TOK_FIELD;
                                  $$ = $2->adopt($1, $3); }
              ;


constant      : TOK_INTCON            { $$ = $1; }    
              | TOK_CHARCON           { $$ = $1; }            
              | TOK_STRINGCON         { $$ = $1; } 
              | TOK_NULL              { $$ = $1; } 
              ;
%%


const char *parser::get_tname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}

bool is_defined_token (int symbol) {
   return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

/*
static void* yycalloc (size_t size) {
   void* result = calloc (1, size);
   assert (result != nullptr);
   return result;
}
*/
