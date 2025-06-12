#include <list>
using namespace std;
typedef struct {
  char* type;
  union {
    int ival;
    int hexval;
    char *sval;
  } value;
} EquArg;
  
typedef struct {
  char* symbol;
  list<EquArg> la;
  bool resolved;
} TUSEntry;

typedef struct {
  int offset;
  char* type;
  char* symbol;
  int addend;

} RelocationTableEntry;

typedef struct {

  char* type;
  union {
    char* sval;
    int ival;
  } value;
  list<int> usedAt;
} LiteralPoolEntry;

typedef struct {
    int number;
    int value;
    int size;
    char* type;
    bool isGlobal;
    int sectionNumber;
    char* name;
    bool defined;
    bool relocatable;
    unsigned long startAddress;
    list<pair<int,int>> flink;
    list<LiteralPoolEntry> lpool;
    list<RelocationTableEntry> relTable;
    list<char> binary;
} SymbolTableEntry;