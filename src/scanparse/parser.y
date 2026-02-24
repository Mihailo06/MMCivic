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
void AddLocToNode(node_st *node, void *begin_loc, void *end_loc);
node_st *reverse_vardecs(node_st *vardecs);

char *current_id = NULL;
node_st *current_forloop_inc_expr = NULL;
node_st *current_exprs = NULL;
bool is_single_expr = true;
%}

%union {
 char               *id;
 int                 cint;
 float               cflt;
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

%left OR
%left AND
%left EQ NE
%left LT LE GT GE
%left PLUS MINUS
%left STAR SLASH PERCENT
%right NOT

%nonassoc IFF
%nonassoc ELSE

%type <node> intval floatval boolval constant exprs expr exprlogicor exprlogicand expreq exprrel expradd exprmul exprunary exprprime
%type <node> block ids
%type <node> stmts stmt varlet program declarations
%type <node> assign procedurecall procedurecalltail conditional conditionalelse whileloop dowhileloop forloop forloopincs return returnprime
%type <node> declaration globaldec globaldef globaldefprime globaldeflet globaldefletarr headerparams headerparamstail
%type <node> vardecs vardec vardeclet vardecletexprs arrayexpr arrayexprs arrayexprprime
%type <node> fundef fundefs fundec funheader funbody
%type <node> parameter parameterarray
%type <cbasictype> basictype
%type <crettype> rettype

%start program

%%

program: declaration declarations
         {
           parseresult = ASTprogram(ASTdeclarations($1, $2));
         }
         ;

declarations: declaration declarations
              {
                $$ = ASTdeclarations($1, $2);
              }
            | 
              {
                $$ = NULL;
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
        ;

globaldec: EXTERN basictype SQUARE_BRACKET_L ID ids SQUARE_BRACKET_R ID SEMICOLON
           {
             $$ = ASTglobaldec(ASTids($5, ASTid(NULL, $4)), ASTid(NULL, $7), $2);
           }
         | EXTERN basictype ID SEMICOLON
           {
             $$ = ASTglobaldec(NULL, ASTid(NULL, $3), $2);
           }
           ;

globaldef: basictype globaldefprime
           {
             $$ = ASTglobaldef($2, current_exprs, ASTid(NULL, current_id), $1, false, is_single_expr); // developer note: if there's somehow any unambigous parts (which there shouln't be), this might break...
           }
         | EXPORT basictype globaldefprime
           {
             $$ = ASTglobaldef($3, current_exprs, ASTid(NULL, current_id), $2, true, is_single_expr);
           }
           ;

globaldefprime: SQUARE_BRACKET_L expr exprs SQUARE_BRACKET_R ID globaldefletarr
                {
                  current_id = $5;
                  current_exprs = ASTexprs($2, $3);
                  $$ = $6;
                }
              | ID globaldeflet
                {
                  current_exprs = NULL;
                  current_id = $1;
                  $$ = $2;
                }
                ;

globaldefletarr: LET arrayexpr SEMICOLON
                 {
                   is_single_expr = false;
                   $$ = $2;
                 }
               | LET expr SEMICOLON
                 {
                   is_single_expr = true;
                   $$ = ASTarrexprs(NULL, NULL, ASTexprs($2, NULL));
                 }
               | SEMICOLON
                 {
                   is_single_expr = true;
                   $$ = NULL;
                 }
                 ;

arrayexpr: SQUARE_BRACKET_L arrayexprprime SQUARE_BRACKET_R
           {
             $$ = $2;
           }
//         | expr        // Developer note: This part was commented out and changed, due to an impossible reduce/reduce conflict caused by the grammar. The case "arrexpr -> expr" was moved a rule up
//           {           // This doesn't allow for the recursion in arrexpr -> [ arrexprs ] | [ exprs ] | expr
//                       // For example arrexpr -> [ expr ] could reduce arrexpr -> [exprs] -> [expr] or arrexpr -> [ arrexprs ] -> [expr]
//                       // This comment is here (for now at least, until we test our implementation on the tests) in case for some weird reason we need to change this back, but we would have an extra reduce/reduce conflict in the end.
//             $$ = ASTarrexprs(ASTexprs($1, NULL), NULL);
//           }
           ;

arrayexprprime: expr exprs
                {
                  $$ = ASTarrexprs(NULL, NULL, ASTexprs($1, $2));
                }
              | arrayexpr arrayexprs
                {
                  $$ = ASTarrexprs($2, $1, NULL);
                }
              | 
                {
                  $$ = NULL;
                }
              ;

arrayexprs: COMMA arrayexpr arrayexprs
            {
              $$ = ASTarrexprs($3, $2, NULL);
            }
          | 
            {
              $$ = NULL;
            }
            ;

globaldeflet: LET expr SEMICOLON // not sure if this is a valid rule
              {
                $$ = ASTarrexprs(NULL, NULL, ASTexprs($2, NULL));
              }
            | SEMICOLON
              {
                $$ = NULL;
              }
              ;

funheader: rettype ID BRACKET_L headerparams BRACKET_R
           {
             $$ = ASTvoidfunheader($4, ASTid(NULL, $2), $1);
           }
         | basictype ID BRACKET_L headerparams BRACKET_R
           {
             $$ = ASTbasicfunheader($4, ASTid(NULL, $2), $1);
           }
           ;

ids: ids COMMA ID
     {
       $$ = ASTids($1, ASTid(NULL, $3));
     }
   |
     {
       $$ = NULL;
     }
     ;
       
headerparams: headerparamstail parameter
              {
                $$ = ASTheaderparams($2, $1);
              }
            |
              {
                $$ = NULL;
              }
              ;

headerparamstail: headerparams COMMA
                  {
                    $$ = $1;
                  }
                | 
                  {
                    $$ = NULL;
                  }
                  ;

parameter: basictype parameterarray
           {
             $$ = ASTparameter($2, ASTid(NULL, current_id), $1);
           }
           ;

parameterarray: ID
               {
                 current_id = $1;
                 $$ = NULL;
               }
             | SQUARE_BRACKET_L ID ids SQUARE_BRACKET_R ID
               {
                 current_id = $5;
                 $$ = ASTids($3, ASTid(NULL, $2));
               }
               ;

funbody: vardecs fundefs stmts
         {
           $$ = ASTfunbody(reverse_vardecs($1), $2, $3); // Developer note: $2 used to be ASTfundefs(NULL, $2, true); and the ASTfundefs in fundefsrule was set to local = false, which is probably an error.
         }
         ;

fundefs: fundef fundefs
              {
                $$ = ASTfundefs($1, $2, true);
              }
            |
              {
                $$ = NULL;
              }
              ;

vardecs: vardecs vardec 
         {
           $$ = ASTvardecs($2, $1);
         }
       |
         {
           $$ = NULL;
         }
         ;

vardec: basictype SQUARE_BRACKET_L expr exprs SQUARE_BRACKET_R ID vardecletexprs SEMICOLON
        {
          $$ = ASTvardec(ASTexprs($3, $4), $7, ASTid(NULL, $6), $1, is_single_expr);
        }
      | basictype ID vardeclet SEMICOLON
        {
          $$ = ASTvardec(NULL, $3, ASTid(NULL, $2), $1, true);
        }
        ;

vardeclet: LET expr
           {
             $$ = ASTarrexprs(NULL, NULL, ASTexprs($2, NULL));
           }
         | 
           {
             $$ = NULL;
           }
           ;

vardecletexprs: LET arrayexpr
                 {
                  is_single_expr = false;
                   $$ = $2;
                 }
               | LET expr
                 {
                  is_single_expr = true;
                   $$ = ASTarrexprs(NULL, NULL, ASTexprs($2, NULL));
                 }
              | 
                {
                  is_single_expr = true;
                  $$ = NULL;
                }
                ;

stmts: stmt stmts
        {
          $$ = ASTstmts($1, $2);
        }
      |
        {
          $$ = NULL;
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
    | conditionalelse
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

assign: varlet LET expr SEMICOLON
        {
          $$ = ASTassign($1, $3);
        }
        ;

procedurecall: ID BRACKET_L procedurecalltail
               {
                 $$ = ASTprocedurecall($3, ASTid(NULL, $1));
                 AddLocToNode($$, &@1, &@1);
               }
               ;

procedurecalltail: expr exprs BRACKET_R SEMICOLON
                   {
                     $$ = ASTexprs($1, $2);
                   }
                 | BRACKET_R SEMICOLON
                   {
                     $$ = NULL;
                   }
                   ;

conditional: IF BRACKET_L expr BRACKET_R block %prec IFF
             {
               $$ = ASTconditional($3, $5, NULL);
             }
             ;

conditionalelse: IF BRACKET_L expr BRACKET_R block ELSE block
             {
               $$ = ASTconditional($3, $5, $7);
             }
             ;

whileloop: WHILE BRACKET_L expr BRACKET_R block
           {
             $$ = ASTwhileloop($3, $5);
           }
           ;

dowhileloop: DO block WHILE BRACKET_L expr BRACKET_R SEMICOLON
             {
               $$ = ASTdowhileloop($2, $5);
             }
             ;

forloop: FOR BRACKET_L basictype ID LET expr COMMA forloopincs BRACKET_R block
         {
           $$ = ASTforloop($6, $8, current_forloop_inc_expr, $10, ASTid(NULL, $4), $3);
         }
         ;

forloopincs: expr
             {
               $$ = $1;
             }
           | expr COMMA expr
             {
              current_forloop_inc_expr = $3;
               $$ = $1;
             }
             ;

return: RETURN returnprime
        {
          $$ = ASTreturn($2);
        }
        ;

returnprime: expr SEMICOLON
             {
               $$ = $1;
             }
           | SEMICOLON
             {
               $$ = NULL;
             }
             ;

varlet: ID
        {
          $$ = ASTvarlet(NULL, ASTid(NULL, $1));
          AddLocToNode($$, &@1, &@1);
        }
      | ID SQUARE_BRACKET_L expr exprs SQUARE_BRACKET_R
        {
          $$ = ASTvarlet(ASTexprs($3, $4), ASTid(NULL, $1));
          AddLocToNode($$, &@1, &@1);
        }
        ;

exprs: exprs COMMA expr
       {
         $$ = ASTexprs($3, $1);
       }
     |
       {
         $$ = NULL;
       }
       ;

expr: exprlogicor
      {
        $$ = $1;
      }
      ;

exprlogicor: exprlogicor OR exprlogicand
             {
               $$ = ASTbinop( $1, $3, BO_or);
               AddLocToNode($$, &@1, &@3);
             }
           | exprlogicand
             {
               $$ = $1;
             }
             ;

exprlogicand: exprlogicand AND expreq
              {
                $$ = ASTbinop( $1, $3, BO_and);
                AddLocToNode($$, &@1, &@3);
              }
            | expreq 
              {
                $$ = $1;
              }
              ;

expreq: expreq EQ exprrel
        {
          $$ = ASTbinop( $1, $3, BO_eq);
          AddLocToNode($$, &@1, &@3);
        }
      | expreq NE exprrel
        {
          $$ = ASTbinop( $1, $3, BO_ne);
          AddLocToNode($$, &@1, &@3);
        }
      | exprrel
        {
          $$ = $1;
        }
        ;

exprrel: exprrel LE expradd 
         {
           $$ = ASTbinop( $1, $3, BO_le);
           AddLocToNode($$, &@1, &@3);
         }
       | exprrel LT expradd
         {
           $$ = ASTbinop( $1, $3, BO_lt);
           AddLocToNode($$, &@1, &@3);
         }
       | exprrel GE expradd
         {
           $$ = ASTbinop( $1, $3, BO_ge);
           AddLocToNode($$, &@1, &@3);
         }
       | exprrel GT expradd
         {
           $$ = ASTbinop( $1, $3, BO_gt);
           AddLocToNode($$, &@1, &@3);
         }
       | expradd
         {
           $$ = $1;
         }
         ;

expradd: expradd PLUS exprmul
         {
           $$ = ASTbinop( $1, $3, BO_add);
           AddLocToNode($$, &@1, &@3);
         }
       | expradd MINUS exprmul
         {
           $$ = ASTbinop( $1, $3, BO_sub);
           AddLocToNode($$, &@1, &@3);
         }
       | exprmul
         {
           $$ = $1;
         }
         ;

exprmul: exprmul STAR exprunary
         {
           $$ = ASTbinop( $1, $3, BO_mul);
           AddLocToNode($$, &@1, &@3);
         }
       | exprmul SLASH exprunary
         {
           $$ = ASTbinop( $1, $3, BO_div);
           AddLocToNode($$, &@1, &@3);
         }
       | exprmul PERCENT exprunary
         {
           $$ = ASTbinop( $1, $3, BO_mod);
           AddLocToNode($$, &@1, &@3);
         }
       | exprunary
         {
           $$ = $1;
         }
         ;

exprunary: MINUS exprunary
           {
             $$ = ASTmonop($2, MO_neg);
             AddLocToNode($$, &@2, &@2);
           }
         | NOT exprunary
           {
             $$ = ASTmonop($2, MO_not);
             AddLocToNode($$, &@2, &@2);
           }
         | BRACKET_L basictype BRACKET_R exprunary
           {
             $$ = ASTcast($4, $2);
           }
         | exprprime
           {
             $$ = $1;
           }
           ;

exprprime: BRACKET_L expr BRACKET_R
           {
             $$ = $2;
           }
         | ID BRACKET_L BRACKET_R
           {
             $$ = ASTprocedurecall(NULL, ASTid(NULL, $1));
             AddLocToNode($$, &@1, &@1);
           }
         | ID BRACKET_L expr exprs BRACKET_R
           {
             $$ = ASTprocedurecall(ASTexprs($3, $4), ASTid(NULL, $1));
             AddLocToNode($$, &@1, &@1);
           }
         | ID SQUARE_BRACKET_L expr exprs SQUARE_BRACKET_R
           {
             $$ = ASTarrexpr(ASTexprs($3, $4), ASTid(NULL, $1));
           }
         | ID
           {
             $$ = ASTvar(NULL, ASTid(NULL, $1));
           }
         | constant
           {
             $$ = $1;
           }
           ;

block: CURLY_BRACKET_L stmts CURLY_BRACKET_R
       {
         $$ = ASTblock($2);
       }
     | stmt
       {
         $$ = ASTblock(ASTstmts($1, NULL));
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

node_st *reverse_vardecs(node_st *list)
{
    node_st *prev = NULL;
    node_st *curr = list;
    node_st *next;

    while (curr != NULL)
    {
        next = VARDECS_NEXT(curr);
        VARDECS_NEXT(curr) = prev;
        prev = curr;
        curr = next;
    }

    return prev;
}
