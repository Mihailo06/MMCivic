%{

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "scanparse/lexer_util.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "parser.h"
#include "palm/str.h"
#include "global/globals.h"
#include "palm/ctinfo.h"

// Use for error messages, see parser.y
extern int yyerror(char *errname);

#define FILTER(token) \
  global.col += yyleng;  \
  return( token);

#define LINEBUF_SIZE 256
#define YY_NO_UNPUT
#define YY_NO_INPUT

static inline void token_action();

int yycolumn = 1;
#define YY_USER_ACTION token_action();


%}

%p 6000 /* set positions to 6000 */
%e 2000 /* set nodes to 1100 */
%option noyywrap
%option nounput
%option yylineno


%%
("//".*"\n")               { global.line += 1;}

"{"                        { FILTER( CURLY_BRACKET_L);}
"}"                        { FILTER( CURLY_BRACKET_R);}
"("                        { FILTER( BRACKET_L); }
")"                        { FILTER( BRACKET_R); }
"["                        { FILTER( SQUARE_BRACKET_L); }
"]"                        { FILTER( SQUARE_BRACKET_R); }
","                        { FILTER( COMMA); }
";"                        { FILTER( SEMICOLON); }

"-"                        { FILTER( MINUS); }
"+"                        { FILTER( PLUS);  }
"*"                        { FILTER( STAR);  }
"/"                        { FILTER( SLASH);  }
"%"                        { FILTER( PERCENT);  }

"<="                       { FILTER( LE);    }
"<"                        { FILTER( LT);    }
">="                       { FILTER( GE);    }
">"                        { FILTER( GT);    }
"=="                       { FILTER( EQ);    }
"!="                       { FILTER( NE);    }

"&&"                       { FILTER( AND);    }
"||"                       { FILTER( OR);    }

"!"                        { FILTER( NOT);      }

"="                        { FILTER( LET);    }

"true"                     { FILTER( TRUEVAL); }
"false"                    { FILTER( FALSEVAL); }

"extern"                   { FILTER( EXTERN); }
"export"                   { FILTER( EXPORT); }
"void"                     { FILTER( VOID_TYPE); }

"bool"                     { FILTER( BOOL_TYPE); }
"int"                      { FILTER( INT_TYPE); }
"float"                    { FILTER( FLOAT_TYPE); }

"if"                       { FILTER( IF); }
"else"                     { FILTER( ELSE); }
"while"                    { FILTER( WHILE); }
"do"                       { FILTER( DO); }
"for"                      { FILTER( FOR); }
"return"                   { FILTER( RETURN); }

[A-Za-z][A-Za-z0-9_]*      { yylval.id = STRcpy(yytext);
                             FILTER( ID);
                           }

[0-9]+([eE][0-9]+)         { yylval.cint = lexerDigitExponent(yytext, yyleng);
                             FILTER( NUM);
                           }

[0-9]+\.[0-9]+([eE][0-9]+) { yylval.cflt = lexerFloatExponent(yytext, yyleng);
                             FILTER( NUM);
                           }

[0-9]+\.[0-9]+            { yylval.cflt = strtof(yytext, NULL); 
                             FILTER( NUM);
                           }

[0-9]+                     { yylval.cint = atoi(yytext); 
                             FILTER( NUM);
                           }

\n.*                       {
                             yycolumn = 1;
                             global.line += 1;
                             global.col = 0;
                             yyless(1);
                           }

[ \t]                      { global.col += yyleng;
                           }
%%

static inline void token_action() {
    yylloc.first_line = yylloc.last_line = yylineno;
    yylloc.first_column = yycolumn;
    yylloc.last_column = yycolumn + yyleng - 1;
    yycolumn += yyleng;
}
