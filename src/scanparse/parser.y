%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "palm/memory.h"
#include "palm/ctinfo.h"
#include "palm/dbug.h"
#include "palm/str.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "global/globals.h"

static node_st *parseresult = NULL;
extern int yylex();
int yyerror(char *errname);
extern FILE *yyin;
void debug();
void AddLocToNode(node_st *node, void *begin_loc, void *end_loc);


%}

%union {
 char               *id;
 int                 cint;
 float               cflt;
 enum BinOpType     cbinop;
 enum MonOpType     cmonop;
 enum BasicType     cbasictype;
 enum RetType       crettype;
 node_st             *node;
}

%locations

%token BRACKET_L BRACKET_R COMMA SEMICOLON CURLY_BRACKET_L CURLY_BRACKET_R SQUARE_BRACKET_L SQUARE_BRACKET_R
%token MINUS PLUS STAR SLASH PERCENT LE LT GE GT EQ NE OR AND NOT
%token BOOL_TYPE INT_TYPE FLOAT_TYPE
%token TRUEVAL FALSEVAL LET
%token EXTERN EXPORT VOID_TYPE
%token IF ELSE WHILE DO FOR RETURN

%token <cint> NUM
%token <cflt> FLOAT
%token <id> ID

%type <node> intval floatval boolval constant exprs expr block ids
%type <node> stmts stmt varlet program declarations
%type <node> assign procedurecall conditional whileloop dowhileloop forloop return
%type <node> declaration globaldec globaldef headerparams
%type <node> vardecs vardec 
%type <node> fundef fundefs fundec funheader funbody
%type <node> parameter
%type <cbinop> binop
%type <cmonop> monop
%type <cbasictype> basictype
%type <crettype> rettype

%start program

%%

program: declarations 
         {
           parseresult = ASTprogram($1);
         }
         ;

declarations: declaration declarations
              {
                $$ = ASTdeclarations($1, $2);
              }
            | declaration
              {
                $$ = ASTdeclarations($1, NULL);
              }
              ;

declaration: fundec
             {
               $$ = $1;
             }
           | funheader CURLY_BRACKET_L funbody CURLY_BRACKET_R
             {
               $$ = ASTfundef($1, $3, false, false);
             }
           | EXPORT funheader CURLY_BRACKET_L funbody CURLY_BRACKET_R
             {
               $$ = ASTfundef($2, $4, false, true);
             }
           | globaldec
             {
               $$ = $1;
             }
           | globaldef
             {
               $$ = $1;
             }
             ;

fundec: EXTERN funheader SEMICOLON
        {
          $$ = ASTfundec($2);
        }
        ;

fundef: funheader CURLY_BRACKET_L funbody CURLY_BRACKET_R
        {
          $$ = ASTfundef($1, $3, true, false);
        }
      | EXPORT funheader CURLY_BRACKET_L funbody CURLY_BRACKET_R
        {
          $$ = ASTfundef($2, $4, true, true);
        }
        ;

globaldec: EXTERN basictype ID SEMICOLON
           {
             $$ = ASTglobaldec(NULL, $2, $3, NULL);
           }
         | EXTERN basictype SQUARE_BRACKET_L ID SQUARE_BRACKET_R ID SEMICOLON
           {
             $$ = ASTglobaldec(NULL, $2, $6, $4);
           }
         | EXTERN basictype SQUARE_BRACKET_L ids SQUARE_BRACKET_R ID SEMICOLON
           {
             $$ = ASTglobaldec($4,$2,$6, NULL);
           }  
           ;

globaldef: basictype ID SEMICOLON
           {
             $$ = ASTglobaldef(NULL, NULL, $1, $2, false);
           }
         | EXPORT basictype ID SEMICOLON
           {
             $$ = ASTglobaldef(NULL, NULL, $2, $3, true);
           }
         | basictype ID LET expr SEMICOLON
           {
             $$ = ASTglobaldef(ASTexprs($4, NULL), NULL, $1, $2, false);
           }
         | EXPORT basictype ID LET expr SEMICOLON
           {
             $$ = ASTglobaldef(ASTexprs($5, NULL), NULL, $2, $3, true);
           }
         | EXPORT basictype SQUARE_BRACKET_L expr SQUARE_BRACKET_R ID LET SQUARE_BRACKET_L exprs SQUARE_BRACKET_R SEMICOLON
           {
             $$ = ASTglobaldef($9, $4, $2, $6, true);
           }
         | basictype SQUARE_BRACKET_L expr SQUARE_BRACKET_R ID LET SQUARE_BRACKET_L exprs SQUARE_BRACKET_R SEMICOLON
           {
             $$ = ASTglobaldef($8, $3, $1, $5, false);
           }
         | EXPORT basictype SQUARE_BRACKET_L expr SQUARE_BRACKET_R ID SEMICOLON
           {
             $$ = ASTglobaldef(NULL, $4, $2, $6, true);
           }
         | basictype SQUARE_BRACKET_L expr SQUARE_BRACKET_R ID SEMICOLON
           {
             $$ = ASTglobaldef(NULL, $3, $1, $5, false);
           }
         | EXPORT basictype ID LET exprs SEMICOLON
           {
             $$ = ASTglobaldef(ASTexprs($5, NULL), NULL, $2, $3, true);
           }
         | basictype ID LET exprs SEMICOLON
           {
             $$ = ASTglobaldef(ASTexprs($4, NULL), NULL, $1, $2, false);
           } 
           ;

funheader: rettype ID BRACKET_L BRACKET_R
           {
             $$ = ASTvoidfunheader(NULL, $2, $1);
           }
         | rettype ID BRACKET_L headerparams BRACKET_R
           {
             $$ = ASTvoidfunheader($4, $2, $1);
           }
         | basictype ID BRACKET_L BRACKET_R
           {
             $$ = ASTbasicfunheader(NULL, $2, $1);
           }
         | basictype ID BRACKET_L headerparams BRACKET_R
           {
             $$ = ASTbasicfunheader($4, $2, $1);
           }
           ;

ids: ID COMMA ids
     {
       $$ = ASTids($3, $1);
     }
   | ID
     {
       $$ = ASTids(NULL, $1);
     }
       
headerparams: parameter COMMA headerparams
              {
                $$ = ASTheaderparams($1, $3);
              }
            | parameter
              {
                $$ = ASTheaderparams($1, NULL);
              }
              ;

parameter: basictype ID
           {
             $$ = ASTparameter(NULL, $1, $2);
           }
         | basictype SQUARE_BRACKET_L ids SQUARE_BRACKET_R ID
           {
             $$ = ASTparameter($3, $1, $5);
           }
           ;

funbody: vardecs
         {
           $$ = ASTfunbody($1, NULL, NULL);
         }
       | fundef fundefs
         {
           $$ = ASTfundefs($1, $2, true);
         }
       | fundef
         {
           $$ = ASTfundef($1, NULL, true, false);
         }
       | stmts
         {
           $$ = ASTfunbody(NULL, NULL, $1);
         }
       | vardecs fundef fundefs
         {
           $$ = ASTfunbody($1, ASTfundefs($2, $3, true), NULL);
         }
       | vardecs fundef
         {
           $$ = ASTfunbody($1, ASTfundefs($2, NULL, true), NULL);
         }
       | vardecs stmts
         {
           $$ = ASTfunbody($1, NULL, $2);
         }
       | fundef fundefs stmts
         {
           $$ = ASTfunbody(NULL, ASTfundefs($1, $2, true), $3);
         }
       | fundef stmts
         {
           $$ = ASTfunbody(NULL, ASTfundefs($1, NULL, true), $2);
         }
       | vardecs fundef fundefs stmts
         {
           $$ = ASTfunbody($1, ASTfundefs($2, $3, true), $4);
         }
       | vardecs fundef stmts
         {
           $$ = ASTfunbody($1, ASTfundefs($2, NULL, true), $2);
         }
       | 
         {
           $$ = ASTfunbody(NULL, NULL, NULL);
         }
         ;

fundefs: fundef fundefs
              {
                $$ = ASTfundefs($1, $2, false);
              }
            | fundef
              {
                $$ = ASTfundefs($1, NULL, false);
              }
              ;

vardecs: vardec vardecs
         {
           $$ = ASTvardecs($1, $2);
         }
       | vardec
         {
           $$ = ASTvardecs($1, NULL);
         }
         ;


vardec: basictype ID SEMICOLON
        {
           $$ = ASTvardec(NULL, NULL, $1, $2);
        }
      | basictype ID LET expr SEMICOLON
        {
           $$ = ASTvardec(NULL, $4, $1, $2);
        }
      | basictype SQUARE_BRACKET_L expr SQUARE_BRACKET_R ID LET exprs SEMICOLON
        {
          debug();
           $$ = ASTvardec($3, $7, $1, $5);
        }
      | basictype SQUARE_BRACKET_L expr SQUARE_BRACKET_R ID LET SQUARE_BRACKET_L exprs SQUARE_BRACKET_R SEMICOLON
        {
           $$ = ASTvardec($3, $8, $1, $5);
        }
      | basictype SQUARE_BRACKET_L exprs SQUARE_BRACKET_R ID SEMICOLON
        {
           $$ = ASTvardec($3, NULL, $1, $5);
        }
        ;

stmts: stmt stmts
        {
          $$ = ASTstmts($1, $2);
        }
      | stmt
        {
          $$ = ASTstmts($1, NULL);
        }
        ;

stmt: assign
      {
        $$ = $1;
      }
    | procedurecall
      {
        $$ = $1;
      }
    | conditional
      {
        $$ = $1;
      }
    | whileloop
      {
        $$ = $1;
      }
    | dowhileloop
      {
        $$ = $1;
      }
    | forloop
      {
        $$ = $1;
      }
    | return
      {
        $$ = $1;
      }
      ;

assign: varlet LET exprs SEMICOLON
        {
          debug();
          debug();
          debug();
          $$ = ASTassign($1, $3);

        }
      | varlet LET SQUARE_BRACKET_L expr SQUARE_BRACKET_R SEMICOLON
        {
          debug();
          $$ = ASTassign($1,ASTexprs($4, NULL));
        }
        ;

procedurecall: ID BRACKET_L expr BRACKET_R SEMICOLON
         {
           $$ = ASTprocedurecall($3, NULL, $1);
         }
       | ID BRACKET_L expr exprs BRACKET_R SEMICOLON
         {
           $$ = ASTprocedurecall($3, $4, $1);
         }
       | ID BRACKET_L BRACKET_R SEMICOLON
         {
           $$ = ASTprocedurecall(NULL, NULL, $1);
         }
         ;

conditional: IF BRACKET_L expr BRACKET_R block
             {
               $$ = ASTconditional($3, $5, NULL);
             }
           | IF BRACKET_L expr BRACKET_R block ELSE block
             {
               $$ = ASTconditional($3, $5, $7);
             }
             ;

whileloop: WHILE BRACKET_L expr BRACKET_R block
           {
             $$ = ASTwhileloop($3, $5);
           }
           ;

dowhileloop: DO block WHILE BRACKET_L expr BRACKET_R
             {
               $$ = ASTdowhileloop($2, $5);
             }
             ;

forloop: FOR BRACKET_L INT_TYPE ID LET expr COMMA expr BRACKET_R block
         {
           $$ = ASTforloop($6, $8, NULL, $10, $4);
         }
       | FOR BRACKET_L INT_TYPE ID LET expr COMMA expr COMMA expr BRACKET_R block
         {
           $$ = ASTforloop($6, $8, $10, $12, $4);
         }
         ;

return: RETURN SEMICOLON
        {
          $$ = ASTreturn(NULL);
        }
      | RETURN expr SEMICOLON
        {
          $$ = ASTreturn($2);
        }
        ;

varlet: ID
        {
          $$ = ASTvarlet(NULL, $1);
          AddLocToNode($$, &@1, &@1);
        }
      | ID SQUARE_BRACKET_L exprs SQUARE_BRACKET_R
        {
          $$ = ASTvarlet($3, $1);
          AddLocToNode($$, &@1, &@1);
        }
        ;

exprs: expr COMMA exprs
       {
         $$ = ASTexprs($1, $3);
       }
     | expr 
       {
         $$ = ASTexprs($1, NULL);
       }
       ;

expr: BRACKET_L expr BRACKET_R
      {
        $$ = $2;
      }
    | expr[left] binop[type] expr[right]
      {
        $$ = ASTbinop( $left, $right, $type);
        AddLocToNode($$, &@left, &@right);
      }
    | monop[type] expr[exp]
      {
        $$ = ASTmonop($exp, $type);
        AddLocToNode($$, &@exp, &@exp);
      }
    | BRACKET_L basictype BRACKET_R expr
      {
        $$ = ASTcast($4, $2);
      }
    | ID BRACKET_L BRACKET_R
      {
        $$ = ASTprocedurecall(NULL, NULL, $1);
      }
    | ID BRACKET_L expr BRACKET_R
      {
        $$ = ASTprocedurecall($3, NULL, $1);
      }
    | ID BRACKET_L expr COMMA exprs BRACKET_R
      {
        $$ = ASTprocedurecall($3, $5, $1);
      }
    | ID
      {
        $$ = ASTvar(NULL, $1);
      }
    | ID SQUARE_BRACKET_L exprs SQUARE_BRACKET_R
      {
        $$ = ASTarrexpr($3, $1); ///// error lies here!!!!!!! do not forget!!!!
      }
    | constant
      {
        $$ = $1;
      }
      ;

block: CURLY_BRACKET_L CURLY_BRACKET_R
       {
         $$ = ASTblock(NULL);
       }
     | CURLY_BRACKET_L stmts CURLY_BRACKET_R
       {
         $$ = ASTblock($2);
       }
     | stmt
       {
         $$ = ASTblock($1);
       }
       ;

constant: floatval
          {
            $$ = $1;
          }
        | intval
          {
            $$ = $1;
          }
        | boolval
          {
            $$ = $1;
          }
          ;

floatval: FLOAT
           {
             $$ = ASTfloat($1);
           }
           ;

intval: NUM
        {
          $$ = ASTnum($1);
        }
        ;

boolval: TRUEVAL
         {
           $$ = ASTbool(true);
         }
       | FALSEVAL
         {
           $$ = ASTbool(false);
         }
         ;

binop: PLUS      { $$ = BO_add; }
     | MINUS     { $$ = BO_sub; }
     | STAR      { $$ = BO_mul; }
     | SLASH     { $$ = BO_div; }
     | PERCENT   { $$ = BO_mod; }
     | LE        { $$ = BO_le; }
     | LT        { $$ = BO_lt; }
     | GE        { $$ = BO_ge; }
     | GT        { $$ = BO_gt; }
     | EQ        { $$ = BO_eq; }
     | NE        { $$ = BO_ne; }
     | OR        { $$ = BO_or; }
     | AND       { $$ = BO_and; }
     ;

monop: MINUS     { $$ = MO_neg; }
     | NOT       { $$ = MO_not; }
     ;

basictype: BOOL_TYPE  { $$ = BT_bool; }
         | INT_TYPE   { $$ = BT_int;  }
         | FLOAT_TYPE { $$ = BT_float;}
         ;

rettype: VOID_TYPE    { $$ = RT_void; }
       ;
%%

void AddLocToNode(node_st *node, void *begin_loc, void *end_loc)
{
    // Needed because YYLTYPE unpacks later than top-level decl.
    YYLTYPE *loc_b = (YYLTYPE*)begin_loc;
    YYLTYPE *loc_e = (YYLTYPE*)end_loc;
    NODE_BLINE(node) = loc_b->first_line;
    NODE_BCOL(node) = loc_b->first_column;
    NODE_ELINE(node) = loc_e->last_line;
    NODE_ECOL(node) = loc_e->last_column;
}

int yyerror(char *error)
{
  CTI(CTI_ERROR, true, "line %d, col %d\nError parsing source code: %s\n",
            global.line, global.col, error);
  CTIabortOnError();
  return 0;
}

node_st *SPdoScanParse(node_st *root)
{
    DBUG_ASSERT(root == NULL, "Started parsing with existing syntax tree.");
    yyin = fopen(global.input_file, "r");
    if (yyin == NULL) {
        CTI(CTI_ERROR, true, "Cannot open file '%s'.", global.input_file);
        CTIabortOnError();
    }
    yyparse();
    return parseresult;
}

void debug()
{
  printf("\n \n node \n \n \n");
}
