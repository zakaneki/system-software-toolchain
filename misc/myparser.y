%{
  #include <cstdio>
  #include <iostream>
  #include "string.h"
  using namespace std;
  // Declare stuff from Flex that Bison needs to know about:
  extern int yylex();
  extern int yyparse();
  extern FILE *yyin;
  extern int line_num;
 
  void yyerror(const char *s);

  typedef struct arg {
  const char *type;  // Type of the argument (INT, HEX, STRING, etc.)
  union {
    int ival;
    int hexval;
    char *sval;
    char *rval;
    char *immval;
  } value;  // Value of the argument
  struct arg *next;  // Pointer to the next argument in the list
} arg_t;

typedef struct instr {
  char *name;  // Name of the instruction
  arg_t *args;  // Linked list of arguments
  struct instr *next;
  int lineNumber;
} instr_t;

typedef struct directive_t {
  char *name;  // Name of the instruction
  arg_t *args;  // Linked list of arguments
  struct directive_t *next;
  int lineNumber;
} directive_t;

typedef struct label_t {
  char *name;  // Name of the instruction
  struct label_t *next;
  int lineNumber;
} label_t;

instr_t *instr_head = NULL;  // Head of the list of instructions
instr_t *instr_tail = NULL;  // Tail of the list of instructions
directive_t* directive_head = NULL;
directive_t* directive_tail = NULL;
label_t* label_head = NULL;
label_t* label_tail = NULL;
arg_t *arg_head = NULL;  // Head of the list of arguments
arg_t *arg_tail = NULL;  // Tail of the list of arguments
%}

%union {
  int ival;
  int hexval;
  char *sval;
  char *dval;
  char *instrval;
  char *lval;
  char *rval;
  char *immval;
}

%token END ENDl
%token ENDD
%token EQU

// Define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the %union:
%token <ival> INT
%token <hexval> HEX
%token <sval> STRING
%token <dval> DIR
%token <instrval> INSTR
%token <lval> LBL
%token <rval> REG
%token <immval> IMM

%%
snazzle:

  snazzle EQU STRING EQUL
  {
    //cout << $3 <<endl;
    //cout << ".equ" <<endl;
    directive_t* new_directive = (directive_t*)malloc(sizeof(directive_t));
    new_directive->name = ".equ";
    new_directive->args = arg_head;  // Assuming arg_head points to the list of arguments
    new_directive->next = NULL;
    arg_head = NULL;
    new_directive->lineNumber = line_num++;

    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "STRING";
    new_arg->value.sval = $3;  // TODO: Set the value of the argument
    new_arg->next = new_directive->args;
    new_directive->args = new_arg;
    

    if (directive_head == NULL) {
      directive_head = new_directive;
      directive_tail = new_directive;
    } else {
      directive_tail->next = new_directive;
      directive_tail = new_directive;
    }

    

  }
  | EQU STRING EQUL
  {
    //cout << $2 <<endl;
    //cout << ".equ" <<endl;
    directive_t* new_directive = (directive_t*)malloc(sizeof(directive_t));
    new_directive->name = ".equ";
    new_directive->args = arg_head;  // Assuming arg_head points to the list of arguments
    new_directive->next = NULL;
    arg_head = NULL;
    new_directive->lineNumber = line_num++;
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "STRING";
    new_arg->value.sval = $2;  // TODO: Set the value of the argument
    new_arg->next = new_directive->args;
    new_directive->args = new_arg;
  

    if (directive_head == NULL) {
      directive_head = new_directive;
      directive_tail = new_directive;
    } else {
      directive_tail->next = new_directive;
      directive_tail = new_directive;
    }

    
  }
  |  snazzle LBL { //cout << "labela1 " << $2 << line_num << endl;
    label_t* new_label = (label_t*)malloc(sizeof(label_t));
    new_label->name = strdup($2);
    new_label->next = NULL;
    new_label->lineNumber = line_num++;

    if (label_head == NULL) {
      label_head = new_label;
      label_tail = new_label;
    } else {
      label_tail->next = new_label;
      label_tail = new_label;
    }
  }
  | LBL { //cout << "labela2 " << $1 << line_num << endl;
    label_t* new_label = (label_t*)malloc(sizeof(label_t));
    new_label->name = strdup($1);
    new_label->next = NULL;
    new_label->lineNumber = line_num++;

    if (label_head == NULL) {
      label_head = new_label;
      label_tail = new_label;
    } else {
      label_tail->next = new_label;
      label_tail = new_label;
    }
  }
  | snazzle DIR LISTA {//cout << "direktiva1 " << $2 << line_num << endl;
    directive_t* new_directive = (directive_t*)malloc(sizeof(directive_t));
    new_directive->name = strdup($2);
    new_directive->args = arg_head;  // Assuming arg_head points to the list of arguments
    new_directive->next = NULL;
    arg_head = NULL;
    new_directive->lineNumber = line_num++;

    if (directive_head == NULL) {
      directive_head = new_directive;
      directive_tail = new_directive;
    } else {
      directive_tail->next = new_directive;
      directive_tail = new_directive;
    }
  }
  | snazzle INSTR LISTA { //cout << "instrukcija1 " << $2 << line_num << endl;
    instr_t *new_instr = (instr_t*)malloc(sizeof(instr_t));
    new_instr->name = strdup($2);
    new_instr->args = arg_head;
    arg_head = NULL;
    new_instr->next = NULL;
    new_instr->lineNumber = line_num++;

    if (instr_head == NULL) {
      instr_head = new_instr;
      instr_tail = new_instr;
    } else {
      instr_tail->next = new_instr;
      instr_tail = new_instr;
    }
  }
  | snazzle INSTR { //cout << "instrukcija2 " << $2 << line_num << endl;
    instr_t *new_instr = (instr_t*)malloc(sizeof(instr_t));
    new_instr->name = strdup($2);
    new_instr->args = arg_head;
    arg_head = NULL;
    new_instr->next = NULL;
    new_instr->lineNumber = line_num++;

    if (instr_head == NULL) {
      instr_head = new_instr;
      instr_tail = new_instr;
    } else {
      instr_tail->next = new_instr;
      instr_tail = new_instr;
    }}
  
  | snazzle ENDD { //cout << "kraj1" << endl;
  }
  | DIR LISTA {//cout << "direktiva2 " << $1 << line_num << endl;
    directive_t* new_directive = (directive_t*)malloc(sizeof(directive_t));
    new_directive->name = strdup($1);
    new_directive->args = arg_head;  // Assuming arg_head points to the list of arguments
    new_directive->next = NULL;
    arg_head = NULL;
    new_directive->lineNumber = line_num++;

    if (directive_head == NULL) {
      directive_head = new_directive;
      directive_tail = new_directive;
    } else {
      directive_tail->next = new_directive;
      directive_tail = new_directive;
    }
  }
  | INSTR LISTA { //cout << "instrukcija3 " << $1 << line_num << endl;
    instr_t *new_instr = (instr_t*)malloc(sizeof(instr_t));
    new_instr->name = strdup($1);
    new_instr->args = arg_head;
    arg_head = NULL;
    new_instr->next = NULL;
    new_instr->lineNumber = line_num++;

    if (instr_head == NULL) {
      instr_head = new_instr;
      instr_tail = new_instr;
    } else {
      instr_tail->next = new_instr;
      instr_tail = new_instr;
    }
  }

  | INSTR { //cout << "instrukcija4 " << $1 << line_num << endl;
    instr_t *new_instr = (instr_t*)malloc(sizeof(instr_t));
    new_instr->name = strdup($1);
    new_instr->args = arg_head;
    arg_head = NULL;
    new_instr->next = NULL;
    new_instr->lineNumber = line_num++;

    if (instr_head == NULL) {
      instr_head = new_instr;
      instr_tail = new_instr;
    } else {
      instr_tail->next = new_instr;
      instr_tail = new_instr;
    }
  }
  
  | ENDD { //cout << "kraj2" << endl;
  }
  ;
LISTA:
  LISTA '+' INT {//cout << "operand int1 " << $3 << endl;
      arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
      new_arg->type = "INT";
      new_arg->value.ival = $3;  // TODO: Set the value of the argument
      new_arg->next = NULL;

      if (arg_head == NULL) {
        // This is the first argument, so it's both the head and tail of the list
        arg_head = new_arg;
        arg_tail = new_arg;
      } else {
        // This is not the first argument, so add it to the end of the list
        arg_tail->next = new_arg;
        arg_tail = new_arg;
      }
    }
  | LISTA INT {//cout << "operand int1 " << $2 << endl;
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "INT";
    new_arg->value.ival = $2;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  |LISTA '+' HEX {//cout << "operand hex1 " << $3 << endl; 
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "HEX";
    new_arg->value.hexval = $3;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  | 
   LISTA HEX {//cout << "operand hex1 " << $2 << endl; 
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "HEX";
    new_arg->value.hexval = $2;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  | LISTA '+' STRING {//cout << "operand ostalo1 " << $3 << endl; 
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "STRING";
    new_arg->value.sval = $3;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  |
  LISTA STRING {//cout << "operand ostalo1 " << $2 << endl; 
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "STRING";
    new_arg->value.sval = $2;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  | LISTA REG {//cout << "operand registar1 " << $2 << endl; 
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "REG";
    new_arg->value.rval = $2;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  | LISTA IMM {//cout << "operand neposredno1 " << $2 << endl; 
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "IMM";
    new_arg->value.immval = $2;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  | INT {//cout << "operand int2 " << $1 << endl; 
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "INT";
    new_arg->value.ival = $1;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  | HEX {//cout << "operand hex2 " << $1 << endl;
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "HEX";
    new_arg->value.hexval = $1;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  } 
  | STRING {//cout << "operand ostalo2 " << $1 << endl; 
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "STRING";
    new_arg->value.sval = $1;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  | REG {//cout << "operand registar2 " << $1 << endl; 
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "REG";
    new_arg->value.rval = $1;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  | IMM {//cout << "operand neposredno2 " << $1 << endl;
    arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
    new_arg->type = "IMM";
    new_arg->value.immval = $1;  // TODO: Set the value of the argument
    new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  }
  ;


EQUL:
  EQUL '+' HEX 
{
  arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "OP";
  new_arg->value.sval = "+";  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }

  
  //cout << $3 << " + " << endl;
  new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "HEX";
  new_arg->value.hexval = $3;  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
}
| EQUL '+' INT 
{
  arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "OP";
  new_arg->value.sval = "+";  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }

  
  //cout << $3 << " + " << endl;
  new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "INT";
  new_arg->value.ival = $3;  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
}| EQUL '+' STRING 
{
  arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "OP";
  new_arg->value.sval = "+";  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }

  
  //cout << $3 << " + " << endl;
  new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "STRING";
  new_arg->value.sval = $3;  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
}
| EQUL '-' HEX 
{
  arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "OP";
  new_arg->value.sval = "-";  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }

  
  //cout << $3 << " - " << endl;
  new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "HEX";
  new_arg->value.hexval = $3;  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
}
| EQUL '-' INT 
{
  arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "OP";
  new_arg->value.sval = "-";  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
  
  //cout << $3 << " - " << endl;
  new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "INT";
  new_arg->value.ival = $3;  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
}
| EQUL '-' STRING 
{
  arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "OP";
  new_arg->value.sval = "-";  // TODO: Set the value of the argument
  new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }

  
  //cout << $3 << " - " << endl;
  new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "STRING";
  new_arg->value.sval = $3;  // TODO: Set the value of the argument
 new_arg->next = NULL;

    if (arg_head == NULL) {
      // This is the first argument, so it's both the head and tail of the list
      arg_head = new_arg;
      arg_tail = new_arg;
    } else {
      // This is not the first argument, so add it to the end of the list
      arg_tail->next = new_arg;
      arg_tail = new_arg;
    }
}
|  HEX
{
  //cout << $1 << endl;
  arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "HEX";
  new_arg->value.hexval = $1;  // TODO: Set the value of the argument
  new_arg->next = NULL;

  if (arg_head == NULL) {
    // This is the first argument, so it's both the head and tail of the list
    arg_head = new_arg;
    arg_tail = new_arg;
  } else {
    // This is not the first argument, so add it to the end of the list
    arg_tail->next = new_arg;
    arg_tail = new_arg;
  }
}
| INT
{
   //cout << $1 << endl;
  arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "INT";
  new_arg->value.ival = $1;  // TODO: Set the value of the argument
  new_arg->next = NULL;

  if (arg_head == NULL) {
    // This is the first argument, so it's both the head and tail of the list
    arg_head = new_arg;
    arg_tail = new_arg;
  } else {
    // This is not the first argument, so add it to the end of the list
    arg_tail->next = new_arg;
    arg_tail = new_arg;
  }
}
| STRING
{
   //cout << $1 << endl;
  arg_t *new_arg = (arg_t*)malloc(sizeof(arg_t));
  new_arg->type = "STRING";
  new_arg->value.sval = $1;  // TODO: Set the value of the argument
  new_arg->next = NULL;

  if (arg_head == NULL) {
    // This is the first argument, so it's both the head and tail of the list
    arg_head = new_arg;
    arg_tail = new_arg;
  } else {
    // This is not the first argument, so add it to the end of the list
    arg_tail->next = new_arg;
    arg_tail = new_arg;
  }
}
;
%%



void yyerror(const char *s) {
  cout << "EEK, parse error on line " << line_num << "!  Message: " << s << endl;
  // might as well halt now:
  exit(-1);
}
