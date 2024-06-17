%{
  #include "myparser.tab.h"  // to get the token types from Bison
  int line_num = 1;
  #include <cstdio>
  #include <iostream>
  using namespace std;

%}
%option noyywrap
%%
\"[^\"]*\" {
  // Remove the double quotes from the beginning and end of yytext
  char* str = strdup(yytext + 1);
  str[strlen(str) - 1] = '\0';

  // Set the semantic value
  yylval.sval = str;

  return STRING;
}
.equ {
  return EQU;
}
"+" {
  return '+';
}

"-" {
  return '-';
}
\n             {  }
0x[a-fA-F0-9]{1,8}    { yylval.hexval = strtol(yytext, NULL, 16); return HEX; }
[0-9]+            { yylval.ival = atoi(yytext); return INT; }
.end {return ENDD;}
[a-zA-Z0-9_]+:  {
  yylval.sval = strdup(yytext);
  return LBL;
}
\.[a-zA-Z0-9_]+ {
  yylval.sval = strdup(yytext);
  return DIR;
}
\[%sp|\[%pc {
  yylval.sval = strdup(yytext);
  return REG;
}
%sp|%pc|%status|%handler|%cause {
  yylval.sval = strdup(yytext);
  return REG;
}
\[%r[0-9]+ {
  yylval.sval = strdup(yytext);
  return REG;
}
%r[0-9]+  {
  yylval.sval = strdup(yytext);
  return REG;
}
$[a-zA-Z0-9_]+  {
  yylval.sval = strdup(yytext);
  return IMM;
}
halt|int|iret|call|ret|jmp|beq|bne|bgt|push|pop|xchg|add|sub|mul|div|not|and|xor|or|shl|shr|ld|st|csrrd|csrwr {
  yylval.instrval = strdup(yytext);
  return INSTR;
}
[a-zA-Z0-9_]+      {
  // We have to strdup yytext because Flex will change it for the next token.
  // Note that this memory must be freed somewhere, so that's why we call
  // free() above in the Bison section.  (Aside: we use free() instead of
  // delete because strdup is a C function that uses malloc, not a C++
  // function that uses new.)
  yylval.sval = strdup(yytext);
  return STRING;
}
#[^\n]*         ;
[ \t,]          ;
.              ;
%%