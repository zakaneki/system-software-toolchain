#include "../myparser.tab.c"
#include "../inc/assembler.hpp"
#include <algorithm>
#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>

void resolveFlink(list<SymbolTableEntry>& symbolTable, list<SymbolTableEntry>& sectionTable){
  for(auto& symbol: symbolTable){
    for(auto& f: symbol.flink){
      for(auto& section: sectionTable){
        if(f.first == section.number){
          if(!(symbol.value != 0 && symbol.sectionNumber == 0 && !symbol.relocatable)){
            cout << "Konacna vrednost simbola " << symbol.name << " nije poznata u trenutku asembliranja." << endl;
            throw new exception;
          } else if(abs(symbol.value) >> 12) {
            cout << "Konacnu vrednost simbola " << symbol.name << " nije moguce zapisati na sirini od 12 bita kao oznacenu vrednost." << endl;
            throw new exception;
          } else {
            int cnt = 0;
            for(auto& b: section.binary){
              if(cnt == f.second - 4){
                b = symbol.value & 0xFF;
              } else if(cnt == f.second - 3){
                b |= symbol.value >> 8;
              }
              cnt++;
            }
          }
        }
      }
    }
  }
}

bool indeksKlasifikacije(map<int, pair<int, int>>& mapa){
  int c = 0;
  for(auto& [section, pair] : mapa){
    if(pair.second < -1 || pair.second > 1){
      return false;
    }
    if(pair.second != 0) c++;
  }
  return c <= 1;
}

bool noUnresolvedArgs(char* symbol, list<TUSEntry>& TUS){
  for(auto& t: TUS){
    if(!strcmp(t.symbol, symbol)){
      for(auto& a: t.la){
        if(!strcmp(a.type, "STRING")){
          for(auto& t1: TUS){
            if(!strcmp(t1.symbol, a.value.sval)){
              if(!t1.resolved) return false;
            }
          }
        }
      }
    }
  }
  return true;
}

void resolveEqu(list<SymbolTableEntry>& symbolTable, list<SymbolTableEntry>& sectionTable, list<TUSEntry>& TUS){
  map<int, pair<int, int>> mapa;
  
  bool loop = true;
  while(loop){
    loop = false;
    for(auto& t: TUS){
      mapa.clear();
      int v = 0;
      if(noUnresolvedArgs(t.symbol, TUS) && !t.resolved){
        loop = true;
        char* op = "+";
        for(auto& l: t.la){
          if(!strcmp(l.type, "INT")){
            if(!strcmp(op, "+")){
              v += l.value.ival;
            } else {
              v -= l.value.ival;
            }
          }
          if(!strcmp(l.type, "HEX")) {
            if(!strcmp(op, "+")){
              v += l.value.ival;
            } else {
              v -= l.value.ival;
            }
          }
          if(!strcmp(l.type, "STRING")){
            int sn;
            int sv;
            for(auto& s: symbolTable){
              if(!strcmp(l.value.sval, s.name)){
                sn = s.sectionNumber;
                sv = s.value;
              } 
            }
            if(!strcmp(op, "+")){
              if(sn == 0 && sv != 0) mapa[sn] = {mapa[sn].first + sv, mapa[sn].second};
              else mapa[sn] = {mapa[sn].first + sv, mapa[sn].second + 1};
              
            } else {
              if(sn == 0 && sv != 0) mapa[sn] = {mapa[sn].first - sv, mapa[sn].second};
              else if(sn == 0 && sv == 0) mapa[sn] = {mapa[sn].first - sv, mapa[sn].second + 1};
              else mapa[sn] = {mapa[sn].first - sv, mapa[sn].second - 1};
            }
            
          }
          if(!strcmp(l.type, "OP")){
            op = l.value.sval;
          }
        }
        if(indeksKlasifikacije(mapa)){
          int s = -1;
          for(auto& [section, pair]: mapa){
            v += pair.first;
            if(pair.second != 0) s = section;
          }
          if(s == -1){
            for(auto& sy: symbolTable){
              if(!strcmp(sy.name, t.symbol)){
                sy.value = v;
              }
            }
            for(auto& section: sectionTable){
              for(auto it = section.relTable.begin(); it != section.relTable.end();){
                if(!strcmp(it->symbol, t.symbol)){
                  int val = v + it->addend;
                  int cnt = 0;
                  for(auto& b: section.binary){
                    if(cnt == it->offset){
                      b = val & 0xFF;
                    } else if(cnt == it->offset + 1){
                      b = (val >> 8) & 0xFF;
                    } else if(cnt == it->offset + 2){
                      b = (val >> 16) & 0xFF;
                    } else if(cnt == it->offset + 3){
                      b = (val >> 24) & 0xFF;
                    }
                    cnt++;
                  }
                  it = section.relTable.erase(it);
                } else {
                  ++it;
                }
              }
            }
            for(auto& sy: symbolTable){
              if(!strcmp(sy.name, t.symbol)){
                sy.relocatable = false;                
              }
            }
          } else if(s == 0){
            char* newSymbol;
            for(auto& x: t.la){
              if(!strcmp(x.type, "STRING")){
                for(auto& sy: symbolTable){
                  if(!strcmp(sy.name, x.value.sval) && sy.sectionNumber == 0 && sy.value == 0){
                    newSymbol = sy.name;
                  }
                }
              }
            }
            for(auto& sy: symbolTable){
              if(!strcmp(sy.name, t.symbol)){
                sy.relocatable = true;                
              }
            }
            for(auto& section: sectionTable){
              for(auto& r: section.relTable){
                if(!strcmp(r.symbol, t.symbol)){
                  r.symbol = newSymbol;
                  r.addend += v;
                }
              }
            }
          } else {
            int s;
            for(auto& [section, pair]: mapa){
              if(pair.second != 0) s = section;
            }
            char* newSection;
            for(auto& ss: sectionTable){
              if(ss.number == s) {
                newSection = ss.name;
              }
            }
            for(auto& section: sectionTable){
              for(auto& r: section.relTable){
                if(!strcmp(r.symbol, t.symbol)){
                  r.symbol = newSection;
                  r.addend += v;
                }
              }
            }
            for(auto& sy: symbolTable){
              if(!strcmp(sy.name, t.symbol)){
                sy.relocatable = true;                
              }
            }
          }
          t.resolved = true;
        } else {
          cout << "Indeks klasifikacije za simbol " << t.symbol << " je nekorektan." << endl;
          throw new exception;
        }
      }
    }
  }
   for(auto& t: TUS){
    if(!t.resolved){
      cout << "Simbol " << t.symbol << " nije moguce razresiti." << endl;
      throw new exception;
    }
   }

}

void proveri(list<SymbolTableEntry> symbolTable){
  for(auto& symbol: symbolTable){
    if(!symbol.defined){
      cout << "Simbol " << symbol.name << " nije definisan niti uvezen." << endl;
      throw new exception;
    }
  }
}

void ispis(ofstream& outText, list<SymbolTableEntry> sectionTable, list<SymbolTableEntry> symbolTable){
    int extraSpace = 3;
    int maxSectionNumberLength = 6;
    int maxSectionSizeLength = 4;
    int maxSectionTypeLength = 4;
    int maxSectionNameLength = 4;

    for (const auto& section : sectionTable) {
      if (strlen(section.name) > maxSectionNameLength) {
          maxSectionNameLength = strlen(section.name);
      }
    }
    outText << "Section Table:\n";
    outText << std::left << std::setw(maxSectionNumberLength + extraSpace) << "Number" << std::setw(maxSectionSizeLength + extraSpace) << "Size" << std::setw(maxSectionTypeLength + extraSpace) << "Type" << "Name\n";
    for (const auto& section : sectionTable) {
        outText << std::left << std::setw(maxSectionNumberLength + extraSpace) << section.number << std::setw(maxSectionSizeLength + extraSpace) << section.size << std::setw(maxSectionTypeLength + extraSpace) << "SCTN" << section.name << "\n";
    }

    int maxSymbolNumberLength = 6;
    int maxSymbolValueLength = 5;
    int maxSymbolTypeLength = 5; 
    int maxSymbolIsGlobalLength = 8; 
    int maxSymbolSectionNumberLength = 13;
    int maxSymbolNameLength = 4; 

    for (const auto& symbol : symbolTable) {
        if (strlen(symbol.name) > maxSymbolNameLength) {
            maxSymbolNameLength = strlen(symbol.name);
        }
    }

    outText << "\nSymbol Table:\n";
    outText << std::left << std::setw(maxSymbolNumberLength + extraSpace) << "Number" << std::setw(maxSymbolValueLength + extraSpace) << "Value" << std::setw(maxSymbolTypeLength + extraSpace) << "Type" << std::setw(maxSymbolIsGlobalLength + extraSpace) << "IsGlobal" << std::setw(maxSymbolSectionNumberLength + extraSpace) << "SectionNumber" << "Name\n";
    for (const auto& symbol : symbolTable) {
        outText << std::left << std::setw(maxSymbolNumberLength + extraSpace) << symbol.number << std::setw(maxSymbolValueLength + extraSpace) << symbol.value << std::setw(maxSymbolTypeLength + extraSpace) << symbol.type << std::setw(maxSymbolIsGlobalLength + extraSpace) << (symbol.isGlobal ? "Yes" : "No") << std::setw(maxSymbolSectionNumberLength + extraSpace) << symbol.sectionNumber << symbol.name << "\n";
    }

    for (const auto& section : sectionTable) {
        int maxRelocationOffsetLength = 6;
        int maxRelocationTypeLength = 4;
        int maxRelocationSymbol= 6;
        int maxRelocationAddend = 6;

        for (const auto& symbol : section.relTable) {
          if (to_string(symbol.offset).length() > maxRelocationOffsetLength) {
              maxRelocationOffsetLength = to_string(symbol.offset).length();
          }
          if (to_string(symbol.addend).length() > maxRelocationAddend) {
              maxRelocationAddend = to_string(symbol.addend).length();
          }
          if (strlen(symbol.type) > maxRelocationTypeLength) {
              maxRelocationTypeLength = strlen(symbol.type);
          }
          if (strlen(symbol.symbol) > maxRelocationSymbol) {
              maxRelocationSymbol = strlen(symbol.symbol);
          }
        }

        outText << "\nRelocation Table for section " << section.number << ":\n";
        if (!section.relTable.empty()) {
            outText << std::hex << std::setfill(' ') << std::left << std::setw(maxRelocationOffsetLength) <<  "Offset" << " " << std::setw(maxRelocationTypeLength) << "Type" << " " << std::setw(maxRelocationSymbol) << "Symbol" << " " << "Addend\n";
            for (const auto& relocation : section.relTable) {
                outText << "0x" << std::hex << std::setfill(' ') << std::left << std::setw(maxRelocationOffsetLength)  <<  relocation.offset << " " << std::setw(maxRelocationTypeLength) << relocation.type << " " << std::setw(maxRelocationSymbol) << relocation.symbol << " " << "0x"<< relocation.addend << "\n";
            }
        } else {
            outText << "No entries in the relocation table.\n";
        }
        outText << "\nBinary content for section " << section.number << ":\n";
        if(!section.binary.empty()){
          int count = 0;
          int address = 0;
          for (const auto& byte : section.binary) {
              if (count % 8 == 0) {
                  outText << std::right << std::hex << std::setw(4) << std::setfill('0') << address << ": ";
              }
              outText << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(byte)) << " ";
              if (++count % 8 == 0) {
                  outText << "\n";
                  address += 8;
              }
          }
        } else {
          outText << "This section is empty.";
        }
        
        outText << "\n";
    }
}

void ispisB(ofstream& outputfile, list<SymbolTableEntry> sectionTable, list<SymbolTableEntry> symbolTable){
  unsigned section_tables_count = sectionTable.size();
  outputfile.write((char*)&section_tables_count, sizeof(section_tables_count));

  for (auto x: sectionTable){
    outputfile.write((char*)&x.number, sizeof(x.number));
    outputfile.write((char*)&x.size, sizeof(x.size));
    unsigned szname = strlen(x.name);
    outputfile.write((char*)&szname, sizeof(szname));
    outputfile.write(x.name, strlen(x.name));
    for (char c: x.binary){
      outputfile.write((char*)&c, sizeof(c));
    }
  }

  unsigned symbol_table_count = symbolTable.size();
  outputfile.write((char*)&symbol_table_count, sizeof(symbol_table_count));

  for (auto x: symbolTable){
    outputfile.write((char*)&x.number, sizeof(x.number));
    outputfile.write((char*)&x.value, sizeof(x.value));
    unsigned sztype = strlen(x.type);
    outputfile.write((char*)&sztype, sizeof(sztype));
    outputfile.write(x.type, sztype);
    outputfile.write((char*)&x.isGlobal, sizeof(x.isGlobal));
    outputfile.write((char*)&x.sectionNumber, sizeof(x.sectionNumber));
    unsigned szname = strlen(x.name);
    outputfile.write((char*)&szname, sizeof(szname));
    outputfile.write(x.name, szname);
    //outputfile.write((char*)&x.defined, sizeof(x.defined));
  }

  for(auto sec: sectionTable){
    if(!sec.relTable.empty()){
      unsigned rel_tab_size = sec.relTable.size();
      outputfile.write((char*)&rel_tab_size, sizeof(rel_tab_size));
      outputfile.write((char*)&sec.number, sizeof(sec.number));

      for (auto x: sec.relTable){
        outputfile.write((char*)&x.offset, sizeof(x.offset));

        unsigned sztip = strlen(x.type);
        outputfile.write((char*)&sztip, sizeof(sztip));
        outputfile.write(x.type, sztip);

        unsigned szsimbol = strlen(x.symbol);
        outputfile.write((char*)&szsimbol, sizeof(szsimbol));
        outputfile.write(x.symbol, szsimbol);

        outputfile.write((char*)&x.addend, sizeof(x.addend));
      }
    }
  }
  
}

int obradiGPR(char* reg){
  char* regt = reg + 1;
  if(!strcmp(regt, "r0")) return 0;
  else if(!strcmp(regt, "r1")) return 1;
  else if(!strcmp(regt, "r2")) return 2;
  else if(!strcmp(regt, "r3")) return 3;
  else if(!strcmp(regt, "r4")) return 4;
  else if(!strcmp(regt, "r5")) return 5;
  else if(!strcmp(regt, "r6")) return 6;
  else if(!strcmp(regt, "r7")) return 7;
  else if(!strcmp(regt, "r8")) return 8;
  else if(!strcmp(regt, "r9")) return 9;
  else if(!strcmp(regt, "r10")) return 10;
  else if(!strcmp(regt, "r11")) return 11;
  else if(!strcmp(regt, "r12")) return 12;
  else if(!strcmp(regt, "r13")) return 13;
  else if(!strcmp(regt, "r14")) return 14;
  else if(!strcmp(regt, "r15")) return 15;
  else if(!strcmp(regt, "sp")) return 14;
  else if(!strcmp(regt, "pc")) return 15;
  else {
    cout << "Nije dobar GPR registar." << endl;
    throw new exception;
  } 
}

int obradiCSR(char* reg){
  char* regt = reg + 1;
  if(!strcmp(regt, "status")) return 0;
  else if(!strcmp(regt, "handler")) return 1;
  else if(!strcmp(regt, "cause")) return 2;
   else {
    cout << "Nije dobar CSR registar." << endl;
    throw new exception;
  } 
}

bool symbolTableContains(list<SymbolTableEntry> symbolTable, string name){
  for(const auto& entry: symbolTable){
    string str(entry.name);
    if(name == entry.name) return true;
  }
  return false;
}

void onePass(instr_t* instr_head, directive_t* directive_head, label_t* label_head, FILE* myfile, ofstream& outputfile, ofstream& outText){
  int symbolCounter = 0;
  int sectionCounter = 0;
  instr_t* itmp = instr_head;
  directive_t* dtmp = directive_head;
  label_t* ltmp = label_head;
  list<SymbolTableEntry> symbolTable;
  list<SymbolTableEntry> sectionTable;
  list<TUSEntry> TUS;
  SymbolTableEntry und;
  und.number = sectionCounter;
  und.value = 0;
  und.size = 0;
  und.type = "SCTN";
  und.isGlobal = false;
  und.sectionNumber = 0;
  und.name = "UND";
  sectionTable.push_back(und);
  int locationCounter = 0;
  while(itmp || dtmp || ltmp){
    unsigned int a,b,c;
    if(itmp) a = itmp->lineNumber;
    else a = -1;
    if(ltmp) b = ltmp->lineNumber;
    else b = -1;
    if(dtmp) c = dtmp->lineNumber;
    else c = -1;
    int minimum = min(min(a, b), c);
    if(ltmp && ltmp->lineNumber == minimum){
      //cout << ltmp->name << endl;
      string str(ltmp->name);
      string substr = str.substr(0, str.size() - 1);
      if(symbolTableContains(symbolTable, substr)){
        for (auto& entry : symbolTable) {
          string str(entry.name);
          if (str == substr) {
            if(entry.defined  && !entry.isGlobal){ // alias
              cout << "Visestruka definicija simbola " << entry.name << "." << endl;
              throw new exception;
            }
            entry.value = locationCounter;
            entry.sectionNumber = sectionCounter;
            entry.defined = true;
            if(entry.isGlobal){
              for(auto& sec : sectionTable){
                for(auto& rl : sec.relTable){
                  if(!strcmp(rl.symbol, str.c_str())){
                    //rl.addend = 0;
                  }
                }
              }
            } else {
              for(auto& sec : sectionTable){
                for(auto& rl : sec.relTable){
                  if(!strcmp(rl.symbol, str.c_str())){
                    rl.addend += entry.value;
                    char* ss;
                    for(auto& tt: sectionTable){
                      if(tt.sectionNumber == entry.sectionNumber) ss = (char*) tt.name;
                    }
                    rl.symbol = ss;
                  }
                }
              }
            }
            break;
          }
        }
      }
      else{
        SymbolTableEntry e;
        e.number = symbolCounter++;
        e.value = locationCounter;
        e.size = 0;
        e.type = "NOTYP";
        e.isGlobal = false;
        e.sectionNumber = sectionCounter;
        char* csubstr = new char[substr.size() + 1];
        strcpy(csubstr, substr.c_str());
        e.name = csubstr;
        e.defined = true;
        symbolTable.push_back(e);
      }
      
      ltmp = ltmp->next;
    }
    else if (itmp && itmp->lineNumber == minimum){
      //cout << itmp->name << " ";
      arg_t* argtmp = itmp->args;
      // while(argtmp){
      //   if(!strcmp(argtmp->type, "INT"))  cout << argtmp->value.ival << " ";
      //   if(!strcmp(argtmp->type, "HEX"))  cout << argtmp->value.hexval << " ";
      //   if(!strcmp(argtmp->type, "REG"))  cout << argtmp->value.rval << " ";
      //   if(!strcmp(argtmp->type, "IMM"))  cout << argtmp->value.immval << " ";
      //   if(!strcmp(argtmp->type, "STRING"))  cout << argtmp->value.sval << " ";
      //   argtmp = argtmp->next;
      // }
      // cout << endl;
      locationCounter += 4;
      if(!strcmp(itmp->name, "halt")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            for(int i = 0; i < sizeof(int); i++){
              s.binary.push_back(0);
            }
          }
        }
         
      } else if(!strcmp(itmp->name, "int")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            s.binary.push_back(0);
            s.binary.push_back(0);
            s.binary.push_back(0x10);
          }
        }
      } else if(!strcmp(itmp->name, "iret")){
        locationCounter += 4;
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0x04);
            s.binary.push_back(0);
            s.binary.push_back(0x0E);
            s.binary.push_back(0x96);

            s.binary.push_back(0x08);
            s.binary.push_back(0);
            s.binary.push_back(0xFE);
            s.binary.push_back(0x93);
          }
        }
      } else if(!strcmp(itmp->name, "call")){
        argtmp = itmp->args;
        while(argtmp){
          if(!strcmp(argtmp->type, "INT")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.ival == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.ival;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if(!strcmp(argtmp->type, "HEX")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.hexval == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.hexval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if (!strcmp(argtmp->type, "STRING")){
            if(symbolTableContains(symbolTable, argtmp->value.sval)){
              // for(auto& entry:symbolTable){
              //   if(!strcmp(entry.name, argtmp->value.sval) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
              // }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = argtmp->value.sval;
              e.defined = false;
              //e.flink.push_back({sectionCounter, locationCounter});
              symbolTable.push_back(e);
            }
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "STRING") && !strcmp(argtmp->value.sval, ep.value.sval)){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }  
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "STRING";
                  e.value.sval = argtmp->value.sval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          }

          argtmp = argtmp->next;
        }
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            s.binary.push_back(0);
            s.binary.push_back(0xF0);
            s.binary.push_back(0x21);
          }
        }
      } else if(!strcmp(itmp->name, "ret")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0x04);
            s.binary.push_back(0);
            s.binary.push_back(0xFE);
            s.binary.push_back(0x93);
          }
        }
      } else if(!strcmp(itmp->name, "jmp")){
        argtmp = itmp->args;
        while(argtmp){
          if(!strcmp(argtmp->type, "INT")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.ival == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.ival;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if(!strcmp(argtmp->type, "HEX")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.hexval == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.hexval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if (!strcmp(argtmp->type, "STRING")){
            if(symbolTableContains(symbolTable, argtmp->value.sval)){
              // for(auto& entry:symbolTable){
              //   if(!strcmp(entry.name, argtmp->value.sval) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
              // }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = argtmp->value.sval;
              e.defined = false;
              //e.flink.push_back({sectionCounter, locationCounter});
              symbolTable.push_back(e);
            }
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "STRING") && !strcmp(argtmp->value.sval, ep.value.sval)){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }  
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "STRING";
                  e.value.sval = argtmp->value.sval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          }

          argtmp = argtmp->next;
        }
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            s.binary.push_back(0);
            s.binary.push_back(0xF0);
            s.binary.push_back(0x38);
          }
        }

      } else if(!strcmp(itmp->name, "beq")){
        argtmp = itmp->args;
        while(argtmp){
          if(!strcmp(argtmp->type, "INT")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.ival == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.ival;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if(!strcmp(argtmp->type, "HEX")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.hexval == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.hexval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if (!strcmp(argtmp->type, "STRING")){
            if(symbolTableContains(symbolTable, argtmp->value.sval)){
              // for(auto& entry:symbolTable){
              //   if(!strcmp(entry.name, argtmp->value.sval) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
              // }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = argtmp->value.sval;
              e.defined = false;
              //e.flink.push_back({sectionCounter, locationCounter});
              symbolTable.push_back(e);
            }
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "STRING") && !strcmp(argtmp->value.sval, ep.value.sval)){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }  
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "STRING";
                  e.value.sval = argtmp->value.sval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          }

          argtmp = argtmp->next;
        }

        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r2 << 4);
            s.binary.push_back((0xF << 4) | r1); 
            s.binary.push_back(0x39);
          }
        }

      } else if(!strcmp(itmp->name, "bne")){
        argtmp = itmp->args;
        while(argtmp){
          if(!strcmp(argtmp->type, "INT")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.ival == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.ival;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if(!strcmp(argtmp->type, "HEX")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.hexval == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.hexval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if (!strcmp(argtmp->type, "STRING")){
            if(symbolTableContains(symbolTable, argtmp->value.sval)){
              // for(auto& entry:symbolTable){
              //   if(!strcmp(entry.name, argtmp->value.sval) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
              // }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = argtmp->value.sval;
              e.defined = false;
             // e.flink.push_back({sectionCounter, locationCounter});
              symbolTable.push_back(e);
            }
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "STRING") && !strcmp(argtmp->value.sval, ep.value.sval)){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }  
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "STRING";
                  e.value.sval = argtmp->value.sval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          }

          argtmp = argtmp->next;
        }
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r2 << 4);
            s.binary.push_back((0xF << 4) | r1); 
            s.binary.push_back(0x3A);
          }
        }

      } else if(!strcmp(itmp->name, "bgt")){
        argtmp = itmp->args;
        while(argtmp){
          if(!strcmp(argtmp->type, "INT")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.ival == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.ival;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if(!strcmp(argtmp->type, "HEX")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.hexval == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.hexval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          } else if (!strcmp(argtmp->type, "STRING")){
            if(symbolTableContains(symbolTable, argtmp->value.sval)){
              // for(auto& entry:symbolTable){
              //   if(!strcmp(entry.name, argtmp->value.sval) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
              // }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = argtmp->value.sval;
              e.defined = false;
             // e.flink.push_back({sectionCounter, locationCounter});
              symbolTable.push_back(e);
            }
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "STRING") && !strcmp(argtmp->value.sval, ep.value.sval)){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }  
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "STRING";
                  e.value.sval = argtmp->value.sval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
          }

          argtmp = argtmp->next;
        }
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r2 << 4);
            s.binary.push_back((0xF << 4) | r1); 
            s.binary.push_back(0x3B);
          }
        }

      } else if(!strcmp(itmp->name, "push")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0xFC);
            int r1 = obradiGPR(itmp->args->value.rval);
            s.binary.push_back((r1 << 4) | 0xF);
            s.binary.push_back(0xE0); 
            s.binary.push_back(0x81);
          }
        }
      } else if(!strcmp(itmp->name, "pop")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(4);
            int r1 = obradiGPR(itmp->args->value.rval);
            s.binary.push_back(0);
            s.binary.push_back((r1 << 4) | 0xE); 
            s.binary.push_back(0x93);
          }
        }

      } else if(!strcmp(itmp->name, "xchg")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r2 << 4);
            s.binary.push_back(r1); 
            s.binary.push_back(0x40);
          }
        }

      } else if(!strcmp(itmp->name, "add")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r1 << 4);
            s.binary.push_back((r2 << 4) | r2); 
            s.binary.push_back(0x50);
          }
        }

      } else if(!strcmp(itmp->name, "sub")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r1 << 4);
            s.binary.push_back((r2 << 4) | r2); 
            s.binary.push_back(0x51);
          }
        }

      } else if(!strcmp(itmp->name, "mul")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r1 << 4);
            s.binary.push_back((r2 << 4) | r2); 
            s.binary.push_back(0x52);
          }
        }

      } else if(!strcmp(itmp->name, "div")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r1 << 4);
            s.binary.push_back((r2 << 4) | r2); 
            s.binary.push_back(0x53);
          }
        }

      } else if(!strcmp(itmp->name, "not")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            s.binary.push_back(0);
            s.binary.push_back((r1 << 4) | r1); 
            s.binary.push_back(0x60);
          }
        }

      } else if(!strcmp(itmp->name, "and")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r1 << 4);
            s.binary.push_back((r2 << 4) | r2); 
            s.binary.push_back(0x61);
          }
        }

      } else if(!strcmp(itmp->name, "or")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r1 << 4);
            s.binary.push_back((r2 << 4) | r2); 
            s.binary.push_back(0x62);
          }
        }

      } else if(!strcmp(itmp->name, "xor")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r1 << 4);
            s.binary.push_back((r2 << 4) | r2); 
            s.binary.push_back(0x63);
          }
        }

      } else if(!strcmp(itmp->name, "shl")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r1 << 4);
            s.binary.push_back((r2 << 4) | r2); 
            s.binary.push_back(0x70);
          }
        }

      } else if(!strcmp(itmp->name, "shr")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(r1 << 4);
            s.binary.push_back((r2 << 4) | r2); 
            s.binary.push_back(0x71);
          }
        }

      } else if(!strcmp(itmp->name, "ld")){
        argtmp = itmp->args;
        int c = 0;
        while(argtmp){
          c++;
          argtmp = argtmp->next;
        }
        argtmp = itmp->args;
        if(c == 2){
          if(!strcmp(argtmp->type, "STRING")){
            if(symbolTableContains(symbolTable, argtmp->value.sval)){
              // for(auto& entry:symbolTable){
              //   if(!strcmp(entry.name, argtmp->value.sval) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
              // }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = argtmp->value.sval;
              e.defined = false;
             // e.flink.push_back({sectionCounter, locationCounter});
              symbolTable.push_back(e);
            }
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "STRING") && !strcmp(argtmp->value.sval, ep.value.sval)){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }  
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "STRING";
                  e.value.sval = argtmp->value.sval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
            argtmp = argtmp->next;
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(0);
                int r1 = obradiGPR(argtmp->value.rval);
                s.binary.push_back(0);
                s.binary.push_back((r1 << 4) | 0xF); 
                s.binary.push_back(0x92);

                s.binary.push_back(0);
                s.binary.push_back(0);
                s.binary.push_back((r1 << 4) | r1); 
                s.binary.push_back(0x92);
              }
            }
            locationCounter += 4;
          } else if(!strcmp(argtmp->type, "IMM")){
            try {
              long t;
              if((argtmp->value.immval + 1)[1] == 'x'){
                t = stol(argtmp->value.immval + 1, nullptr, 16);
              } else {
                t = stol(argtmp->value.immval + 1);
              }
              for(auto& entry: sectionTable){
                if(entry.sectionNumber == sectionCounter){
                  bool t1 = false;
                  for(auto& ep : entry.lpool){
                    if(!strcmp(ep.type, "INT") && t == ep.value.ival){
                      ep.usedAt.push_back(locationCounter);
                      t1 = true;
                      break;
                    }
                  }
                  if(!t1){
                    LiteralPoolEntry e;
                    e.type = "INT";
                    e.value.ival = t;
                    e.usedAt.push_back(locationCounter);
                    entry.lpool.push_back(e);
                  }
                }
              }
              argtmp = argtmp->next;
              for(auto& s: sectionTable){
                if(s.sectionNumber == sectionCounter){
                  s.binary.push_back(0);
                  int r1 = obradiGPR(argtmp->value.rval);
                  s.binary.push_back(0);
                  s.binary.push_back((r1 << 4) | 0xF); 
                  s.binary.push_back(0x92);
                }
              }
            } catch (invalid_argument const &e) {
              char* t = argtmp->value.immval + 1;
              if(symbolTableContains(symbolTable, t)){
                // for(auto& entry:symbolTable){
                //   if(!strcmp(entry.name, t) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
                // }
              } else {
                SymbolTableEntry e;
                e.number = symbolCounter++;
                e.value = 0;
                e.size = 0;
                e.type = "NOTYP";
                e.isGlobal = false;
                e.sectionNumber = 0;
                e.name = t;
                e.defined = false;
              //  e.flink.push_back({sectionCounter, locationCounter});
                symbolTable.push_back(e);
              }
              for(auto& entry: sectionTable){
                if(entry.sectionNumber == sectionCounter){
                  bool t1 = false;
                  for(auto& ep : entry.lpool){
                    if(!strcmp(ep.type, "STRING") && !strcmp(t, ep.value.sval)){
                      ep.usedAt.push_back(locationCounter);
                      t1 = true;
                      break;
                    }
                  }  
                  if(!t1){
                    LiteralPoolEntry e;
                    e.type = "STRING";
                    e.value.sval = t;
                    e.usedAt.push_back(locationCounter);
                    entry.lpool.push_back(e);
                  }
                }
              }
              argtmp = argtmp->next;
              for(auto& s: sectionTable){
                if(s.sectionNumber == sectionCounter){
                  s.binary.push_back(0);
                  int r1 = obradiGPR(argtmp->value.rval);
                  s.binary.push_back(0);
                  s.binary.push_back((r1 << 4) | 0xF); 
                  s.binary.push_back(0x92);
                }
              }
            }
            
          } else if(!strcmp(argtmp->type, "INT")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.ival == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.ival;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
            argtmp = argtmp->next;
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(0);
                int r1 = obradiGPR(argtmp->value.rval);
                s.binary.push_back(0);
                s.binary.push_back((r1 << 4) | 0xF); 
                s.binary.push_back(0x92);

                s.binary.push_back(0);
                s.binary.push_back(0);
                s.binary.push_back((r1 << 4) | r1); 
                s.binary.push_back(0x92);
              }
            }
            locationCounter += 4;
          } else if(!strcmp(argtmp->type, "HEX")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.hexval == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.hexval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
            argtmp = argtmp->next;
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(0);
                int r1 = obradiGPR(argtmp->value.rval);
                s.binary.push_back(0);
                s.binary.push_back((r1 << 4) | 0xF); 
                s.binary.push_back(0x92);

                s.binary.push_back(0);
                s.binary.push_back(0);
                s.binary.push_back((r1 << 4) | r1); 
                s.binary.push_back(0x92);
              }
            }
            locationCounter += 4;
          } else if(!strcmp(argtmp->type, "REG")){
            if(argtmp->value.rval[0] == '['){
              // [%<reg>]
              for(auto& s: sectionTable){
                if(s.sectionNumber == sectionCounter){
                  s.binary.push_back(0);
                  int r1 = obradiGPR(argtmp->value.rval + 1);
                  argtmp = argtmp->next;
                  int r2 = obradiGPR(argtmp->value.rval);
                  s.binary.push_back(0);
                  s.binary.push_back((r2 << 4) | r1); 
                  s.binary.push_back(0x92);
                }
              }
            } else {
              // reg
              for(auto& s: sectionTable){
                if(s.sectionNumber == sectionCounter){
                  s.binary.push_back(0);
                  int r1 = obradiGPR(argtmp->value.rval + 1);
                  argtmp = argtmp->next;
                  int r2 = obradiGPR(argtmp->value.rval);
                  s.binary.push_back(0);
                  s.binary.push_back((r2 << 4) | r1); 
                  s.binary.push_back(0x91);
                }
              }
            }
          }
        } else if(c == 3){
          // [%<reg>]
          int r1 = obradiGPR(argtmp->value.rval + 1);
          argtmp = argtmp->next;
          if(!strcmp(argtmp->type, "INT")){
            // for(auto& entry: sectionTable){
            //   if(entry.sectionNumber == sectionCounter){
            //     bool t = false;
            //     for(auto& ep : entry.lpool){
            //       if(!strcmp(ep.type, "INT") && argtmp->value.ival == ep.value.ival){
            //         ep.usedAt.push_back(locationCounter);
            //         t = true;
            //         break;
            //       }
            //     }
            //     if(!t){
            //       LiteralPoolEntry e;
            //       e.type = "INT";
            //       e.value.ival = argtmp->value.ival;
            //       e.usedAt.push_back(locationCounter);
            //       entry.lpool.push_back(e);
            //     }
            //   }
            // }
            
            int val = argtmp->value.ival;
            if(abs(val) >> 12){
              cout << "Literal nije moguce zapisati na sirini od 12 bita kao oznacenu vrednost." << endl;
              throw new exception();
            } 
            argtmp = argtmp->next;
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(val & 0xFF);
                int r2 = obradiGPR(argtmp->value.rval);
                s.binary.push_back(val >> 8);
                s.binary.push_back((r2 << 4) | r1); 
                s.binary.push_back(0x92);
              }
            }
          } else if(!strcmp(argtmp->type, "HEX")){
            // for(auto& entry: sectionTable){
            //   if(entry.sectionNumber == sectionCounter){
            //     bool t = false;
            //     for(auto& ep : entry.lpool){
            //       if(!strcmp(ep.type, "INT") && argtmp->value.hexval == ep.value.ival){
            //         ep.usedAt.push_back(locationCounter);
            //         t = true;
            //         break;
            //       }
            //     }
            //     if(!t){
            //       LiteralPoolEntry e;
            //       e.type = "INT";
            //       e.value.ival = argtmp->value.hexval;
            //       e.usedAt.push_back(locationCounter);
            //       entry.lpool.push_back(e);
            //     }
            //   }
            // }
            int val = argtmp->value.hexval;
            if(abs(val) >> 12){
              cout << "Literal nije moguce zapisati na sirini od 12 bita kao oznacenu vrednost." << endl;
              throw new exception();
            } 
            argtmp = argtmp->next;
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(val & 0xFF);
                int r2 = obradiGPR(argtmp->value.rval);
                s.binary.push_back(val >> 8);
                s.binary.push_back((r2 << 4) | r1); 
                s.binary.push_back(0x92);
              }
            }
          } else if (!strcmp(argtmp->type, "STRING")){
          //   if(symbolTableContains(symbolTable, argtmp->value.sval)){
          //     for(auto& entry:symbolTable){
          //       if(!strcmp(entry.name, argtmp->value.sval) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
          //     }
          //   } else {
          //     SymbolTableEntry e;
          //     e.number = symbolCounter++;
          //     e.value = locationCounter;
          //     e.size = 0;
          //     e.type = "NOTYP";
          //     e.isGlobal = false;
          //     e.sectionNumber = sectionCounter;
          //     e.name = argtmp->value.sval;
          //     e.defined = false;
          //     e.flink.push_back({sectionCounter, locationCounter});
          //     symbolTable.push_back(e);
          //   }
          //   for(auto& entry: sectionTable){
          //     if(entry.sectionNumber == sectionCounter){
          //       bool t = false;
          //       for(auto& ep : entry.lpool){
          //         if(!strcmp(ep.type, "STRING") && !strcmp(argtmp->value.sval, ep.value.sval)){
          //           ep.usedAt.push_back(locationCounter);
          //           t = true;
          //           break;
          //         }
          //       }  
          //       if(!t){
          //         LiteralPoolEntry e;
          //         e.type = "STRING";
          //         e.value.sval = argtmp->value.sval;
          //         e.usedAt.push_back(locationCounter);
          //         entry.lpool.push_back(e);
          //       }
          //     }
          //   }
          //   argtmp = argtmp->next;
          //   // reg
          // }
            char* symbol = argtmp->value.sval;
            if(symbolTableContains(symbolTable, symbol)){
              for(auto& s: symbolTable){
                s.flink.push_back({sectionCounter, locationCounter});
              }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = symbol;
              e.flink.push_back({sectionCounter, locationCounter});
              e.defined = false;
              symbolTable.push_back(e);
            }
            argtmp = argtmp->next;
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(0);
                int r2 = obradiGPR(argtmp->value.rval);
                s.binary.push_back(0);
                s.binary.push_back((r2 << 4) | r1); 
                s.binary.push_back(0x92);
              }
            }
          }
        }
      } else if(!strcmp(itmp->name, "st")){
        argtmp = itmp->args;
        int c = 0;
        while(argtmp){
          c++;
          argtmp = argtmp->next;
        }
        argtmp = itmp->args;
        int r1 = obradiGPR(argtmp->value.rval);
        argtmp = argtmp->next;
         if(c == 2){
          if(!strcmp(argtmp->type, "STRING")){
            if(symbolTableContains(symbolTable, argtmp->value.sval)){
              // for(auto& entry:symbolTable){
              //   if(!strcmp(entry.name, argtmp->value.sval) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
              // }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = argtmp->value.sval;
              e.defined = false;
             // e.flink.push_back({sectionCounter, locationCounter});
              symbolTable.push_back(e);
            }
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "STRING") && !strcmp(argtmp->value.sval, ep.value.sval)){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }  
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "STRING";
                  e.value.sval = argtmp->value.sval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(0);
                s.binary.push_back(r1 << 4);
                s.binary.push_back(0xF << 4); 
                s.binary.push_back(0x82);
              }
            }
          } else if(!strcmp(argtmp->type, "IMM")){
          //   try {
          //     long t;
          //     if((argtmp->value.immval + 1)[1] == 'x'){
          //       t = stol(argtmp->value.immval + 1, nullptr, 16);
          //     } else {
          //       t = stol(argtmp->value.immval + 1);
          //     }
          //     for(auto& entry: sectionTable){
          //       if(entry.sectionNumber == sectionCounter){
          //         bool t1 = false;
          //         for(auto& ep : entry.lpool){
          //           if(!strcmp(ep.type, "INT") && t == ep.value.ival){
          //             ep.usedAt.push_back(locationCounter);
          //             t1 = true;
          //             break;
          //           }
          //         }
          //         if(!t1){
          //           LiteralPoolEntry e;
          //           e.type = "INT";
          //           e.value.ival = t;
          //           e.usedAt.push_back(locationCounter);
          //           entry.lpool.push_back(e);
          //         }
          //       }
          //     }
          //   } catch (invalid_argument const &e) {
          //     char* t = argtmp->value.immval + 1;
          //     if(symbolTableContains(symbolTable, t)){
          //       for(auto& entry:symbolTable){
          //         if(!strcmp(entry.name, t) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
          //       }
          //     } else {
          //       SymbolTableEntry e;
          //       e.number = symbolCounter++;
          //       e.value = locationCounter;
          //       e.size = 0;
          //       e.type = "NOTYP";
          //       e.isGlobal = false;
          //       e.sectionNumber = sectionCounter;
          //       e.name = t;
          //       e.defined = false;
          //       e.flink.push_back({sectionCounter, locationCounter});
          //       symbolTable.push_back(e);
          //     }
          //     for(auto& entry: sectionTable){
          //       if(entry.sectionNumber == sectionCounter){
          //         bool t1 = false;
          //         for(auto& ep : entry.lpool){
          //           if(!strcmp(ep.type, "STRING") && !strcmp(t, ep.value.sval)){
          //             ep.usedAt.push_back(locationCounter);
          //             t1 = true;
          //             break;
          //           }
          //         }  
          //         if(!t1){
          //           LiteralPoolEntry e;
          //           e.type = "STRING";
          //           e.value.sval = t;
          //           e.usedAt.push_back(locationCounter);
          //           entry.lpool.push_back(e);
          //         }
          //       }
          //     }
          //   }
            cout << "Nemoguca naredba st sa neposrednim adresiranjem." << endl;
            throw new exception();
          } else if(!strcmp(argtmp->type, "INT")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.ival == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.ival;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(0);
                s.binary.push_back(r1 << 4);
                s.binary.push_back(0xF << 4); 
                s.binary.push_back(0x82);
              }
            }
          } else if(!strcmp(argtmp->type, "HEX")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.hexval == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.hexval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(0);
                s.binary.push_back(r1 << 4);
                s.binary.push_back(0xF << 4); 
                s.binary.push_back(0x82);
              }
            }
          } else if(!strcmp(argtmp->type, "REG")){
            if(argtmp->value.rval[0] == '['){
              // [%<reg>]
              int r2 = obradiGPR(argtmp->value.rval + 1);
              for(auto& s: sectionTable){
                if(s.sectionNumber == sectionCounter){
                  s.binary.push_back(0);
                  s.binary.push_back(r1 << 4);
                  s.binary.push_back(r2 << 4); 
                  s.binary.push_back(0x80);
                }
              }
            } else {
              // reg
              int r2 = obradiGPR(argtmp->value.rval);
              for(auto& s: sectionTable){
                if(s.sectionNumber == sectionCounter){
                  s.binary.push_back(0);
                  s.binary.push_back(0);
                  s.binary.push_back((r2 << 4) | r1); 
                  s.binary.push_back(0x91);
                }
              }
            }
          }
        } else if(c == 3){
          // [%<reg>]
          int r2 = obradiGPR(argtmp->value.rval + 1);
          argtmp = argtmp->next;
          if(!strcmp(argtmp->type, "INT")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.ival == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.ival;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
            int val = argtmp->value.ival;
            if(abs(val) >> 12){
              cout << "Literal nije moguce zapisati na sirini od 12 bita kao oznacenu vrednost." << endl;
              throw new exception();
            } 
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(val & 0xFF);
                s.binary.push_back((r1 << 4) | (val >> 8));
                s.binary.push_back(r2 << 4); 
                s.binary.push_back(0x82);
              }
            }
          } else if(!strcmp(argtmp->type, "HEX")){
            for(auto& entry: sectionTable){
              if(entry.sectionNumber == sectionCounter){
                bool t = false;
                for(auto& ep : entry.lpool){
                  if(!strcmp(ep.type, "INT") && argtmp->value.hexval == ep.value.ival){
                    ep.usedAt.push_back(locationCounter);
                    t = true;
                    break;
                  }
                }
                if(!t){
                  LiteralPoolEntry e;
                  e.type = "INT";
                  e.value.ival = argtmp->value.hexval;
                  e.usedAt.push_back(locationCounter);
                  entry.lpool.push_back(e);
                }
              }
            }
            int val = argtmp->value.hexval;
            if(abs(val) >> 12){
              cout << "Literal nije moguce zapisati na sirini od 12 bita kao oznacenu vrednost." << endl;
              throw new exception();
            } 
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(val & 0xFF);
                s.binary.push_back((r1 << 4) | (val >> 8));
                s.binary.push_back(r2 << 4); 
                s.binary.push_back(0x82);
              }
            }
          } else if (!strcmp(argtmp->type, "STRING")){
            // if(symbolTableContains(symbolTable, argtmp->value.sval)){
            //   for(auto& entry:symbolTable){
            //     if(!strcmp(entry.name, argtmp->value.sval) && !entry.defined) entry.flink.push_back({sectionCounter, locationCounter});
            //   }
            // } else {
            //   SymbolTableEntry e;
            //   e.number = symbolCounter++;
            //   e.value = locationCounter;
            //   e.size = 0;
            //   e.type = "NOTYP";
            //   e.isGlobal = false;
            //   e.sectionNumber = sectionCounter;
            //   e.name = argtmp->value.sval;
            //   e.defined = false;
            //   e.flink.push_back({sectionCounter, locationCounter});
            //   symbolTable.push_back(e);
            // }
            // for(auto& entry: sectionTable){
            //   if(entry.sectionNumber == sectionCounter){
            //     bool t = false;
            //     for(auto& ep : entry.lpool){
            //       if(!strcmp(ep.type, "STRING") && !strcmp(argtmp->value.sval, ep.value.sval)){
            //         ep.usedAt.push_back(locationCounter);
            //         t = true;
            //         break;
            //       }
            //     }  
            //     if(!t){
            //       LiteralPoolEntry e;
            //       e.type = "STRING";
            //       e.value.sval = argtmp->value.sval;
            //       e.usedAt.push_back(locationCounter);
            //       entry.lpool.push_back(e);
            //     }
            //   }
            // }
            char* symbol = argtmp->value.sval;
            if(symbolTableContains(symbolTable, symbol)){
              for(auto& s: symbolTable){
                s.flink.push_back({sectionCounter, locationCounter});
              }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = symbol;
              e.flink.push_back({sectionCounter, locationCounter});
              e.defined = false;
              symbolTable.push_back(e);
            }
            argtmp = argtmp->next;
            for(auto& s: sectionTable){
              if(s.sectionNumber == sectionCounter){
                s.binary.push_back(0);
                s.binary.push_back(r1 << 4);
                s.binary.push_back(r2 << 4); 
                s.binary.push_back(0x82);
              }
            }
          }
        }
        

      } else if(!strcmp(itmp->name, "csrrd")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiCSR(itmp->args->value.rval);
            int r2 = obradiGPR(itmp->args->next->value.rval);
            s.binary.push_back(0);
            s.binary.push_back((r2 << 4) | r1); 
            s.binary.push_back(0x90);
          }
        }
        

      } else if(!strcmp(itmp->name, "csrwr")){
        for(auto& s: sectionTable){
          if(s.sectionNumber == sectionCounter){
            s.binary.push_back(0);
            int r1 = obradiGPR(itmp->args->value.rval);
            int r2 = obradiCSR(itmp->args->next->value.rval);
            s.binary.push_back(0);
            s.binary.push_back((r2 << 4) | r1); 
            s.binary.push_back(0x94);
          }
        }
      }
      
      
      itmp = itmp->next;
    
    }
    else if(dtmp && dtmp->lineNumber == minimum){
      //cout << dtmp->name << " ";
      arg_t* argtmp = dtmp->args;
      // while(argtmp){
      //   if(!strcmp(argtmp->type, "INT"))  cout << argtmp->value.ival << " ";
      //   if(!strcmp(argtmp->type, "HEX"))  cout << argtmp->value.hexval << " ";
      //   if(!strcmp(argtmp->type, "REG"))  cout << argtmp->value.rval << " ";
      //   if(!strcmp(argtmp->type, "IMM"))  cout << argtmp->value.immval << " ";
      //   if(!strcmp(argtmp->type, "STRING"))  cout << argtmp->value.sval << " ";
      //   argtmp = argtmp->next;
      // }
      // cout << endl;
      if(strcmp(dtmp->name, ".global") == 0){
        arg_t* argtmp = dtmp->args;
        while(argtmp){
          char* symbol = argtmp->value.sval;
          if(symbolTableContains(symbolTable, symbol)){
            for (auto& entry : symbolTable) {
              if (strcmp(entry.name, symbol) == 0) { 
                if(entry.isGlobal){
                    cout << "Simbol " << entry.name << " se vec uvozi." << endl;
                    throw new exception;
                }
                entry.isGlobal = true;
                char* es;
                for(auto& t: sectionTable){
                  if(t.sectionNumber == entry.sectionNumber) es = (char*) t.name;
                }
                for(auto& sec: sectionTable){
                  for(auto& re : sec.relTable){
                    if(!strcmp(re.symbol, es) && re.addend == entry.value){
                      re.symbol = (char*) entry.name;
                      re.addend = 0;
                    }
                  }
                }
                break;
              }
            }
          } else {
            SymbolTableEntry e;
            e.number = symbolCounter++;
            e.value = 0;
            e.size = 0;
            e.type = "NOTYP";
            e.isGlobal = true;
            e.sectionNumber = 0;
            e.name = symbol;
            // alias
            e.defined = true;
            symbolTable.push_back(e);
          }
          argtmp = argtmp->next;
        }
      } else if(strcmp(dtmp->name, ".extern") == 0){
        arg_t* argtmp = dtmp->args;
        while(argtmp){
          char* symbol = argtmp->value.sval;
          if(symbolTableContains(symbolTable, symbol)){
            for (auto& entry : symbolTable) {
              if (strcmp(entry.name, symbol) == 0) { 
                  if(entry.defined  && !entry.isGlobal){ // alias
                    cout << "Visestruka definicija simbola " << entry.name << "." << endl;
                    throw new exception;
                  }
                  if(entry.isGlobal){
                    cout << "Simbol " << entry.name << " se vec izvozi." << endl;
                    throw new exception;
                  }
                  entry.isGlobal = true;
                  entry.defined = true;
                  break;
              }
            }
          } else {
            SymbolTableEntry e;
            e.number = symbolCounter++;
            e.value = 0;
            e.size = 0;
            e.type = "NOTYP";
            e.isGlobal = true;
            e.sectionNumber = 0;
            e.name = symbol;
            e.defined = true;
            symbolTable.push_back(e);
          }
          argtmp = argtmp->next;
        }
      } else if(strcmp(dtmp->name, ".section") == 0){
        for(auto& sec: sectionTable){
          if(sec.sectionNumber == sectionCounter){
            int count = 0;
            for(auto& en: sec.lpool){
              if(!strcmp(en.type,"STRING")){
                RelocationTableEntry e;
                bool t = false;
                int sec1 = 0;
                for(auto& s: symbolTable){
                  if(!strcmp(en.value.sval, s.name) && s.isGlobal) {
                    t = true;
                  }
                }
                for(auto& s: symbolTable){
                  if(!strcmp(en.value.sval, s.name)) {
                    sec1 = s.sectionNumber;
                  }
                }
                char* nm;
                for(auto& tmp: sectionTable){
                  if(tmp.sectionNumber == sec1) nm = (char*)tmp.name;
                }
                int off = 0;
                for(auto& s: symbolTable){
                  if(!strcmp(en.value.sval, s.name) && !s.isGlobal) {
                    off = s.value;
                  }
                }
                e.symbol = (t || sec1 == 0) ? en.value.sval : nm;
                e.addend = off;
                e.type = "32";
                e.offset = locationCounter + count;

                sec.relTable.push_back(e);
              }

              count += 4;
            }
            for(auto& lp: sec.lpool){
              
              if(!strcmp(lp.type, "INT")){
                int x = lp.value.ival;
                for(auto& i: lp.usedAt){
                  int tmp = 0;
                  int disp = locationCounter - i;
                  for(auto& b: sec.binary){
                    if(tmp + 4 == i){
                      b = disp & 0xFF;
                    }
                    if(tmp + 3 == i){
                      b = b | ((disp >> 8) & 0xF);
                    }
                    tmp++;
                  }
                }
                for (int i = 0; i < sizeof(int); ++i) {      
                  sec.binary.push_back(x & 0xFF);
                  x >>= 8;
                }
              } else {
                for(auto& i: lp.usedAt){
                  int tmp = 0;
                  int disp = locationCounter - i;
                  for(auto& b: sec.binary){
                    if(tmp + 4 == i){
                      b = disp & 0xFF;
                    }
                    if(tmp + 3 == i){
                      b = b | ((disp >> 8) & 0xF);
                    }
                    tmp++;
                  }
                }
                for (int i = 0; i < sizeof(int); ++i) {      
                  sec.binary.push_back(0);
                }
              }
              locationCounter += 4;
            }
            sec.size = locationCounter;
          }
        }
        locationCounter = 0;
        SymbolTableEntry e;
        e.number = ++sectionCounter;
        e.value = 0;
        e.size = 0;
        e.type = "SCTN";
        e.isGlobal = true;
        e.sectionNumber = sectionCounter;
        e.name = dtmp->args->value.sval;
        e.defined = true;
        sectionTable.push_back(e);
      } else if(strcmp(dtmp->name, ".word") == 0){
        arg_t* argtmp = dtmp->args;
        while(argtmp){
          locationCounter += 4;
          if(!strcmp(argtmp->type, "INT")){
            for(auto& sec: sectionTable){
              if(sec.sectionNumber == sectionCounter){
                int x = argtmp->value.ival;
                for (int i = 0; i < sizeof(int); ++i) {      
                  sec.binary.push_back(x & 0xFF);
                  x >>= 8;
                }
              }
            }
          }
          if(!strcmp(argtmp->type, "HEX")){
            for(auto& sec: sectionTable){
              if(sec.sectionNumber == sectionCounter){
                int x = argtmp->value.hexval;
                for (int i = 0; i < sizeof(int); ++i) {
                  sec.binary.push_back(x & 0xFF);
                  x >>= 8;
                }
              }
            }
          }
          if(!strcmp(argtmp->type, "STRING")){
            char* symbol = argtmp->value.sval;
            if(symbolTableContains(symbolTable, symbol)){
              // for(auto& entry: symbolTable){
              //   if(!strcmp(symbol, entry.name)){
              //     if(entry.defined){
              //     } else {
              //       entry.flink.push_back({sectionCounter, locationCounter});
              //     }
              //   } 
              // }
            } else {
              SymbolTableEntry e;
              e.number = symbolCounter++;
              e.value = 0;
              e.size = 0;
              e.type = "NOTYP";
              e.isGlobal = false;
              e.sectionNumber = 0;
              e.name = symbol;
              e.defined = false;
             // e.flink.push_back({sectionCounter, locationCounter});
              symbolTable.push_back(e);
            }
            for(auto& en: symbolTable){
              if(!strcmp(en.name, symbol)){
                char* sect;
                for(auto& s: sectionTable){
                  if(s.sectionNumber == en.sectionNumber) sect = (char*)s.name;
                }
                RelocationTableEntry e;
                e.type = "32";
                e.offset = locationCounter;
                e.addend = (en.isGlobal || en.sectionNumber == 0) ? 0 : en.value;
                e.symbol = (en.isGlobal || en.sectionNumber == 0) ? symbol : sect;
                for(auto& sec : sectionTable){
                  if(sec.sectionNumber == sectionCounter){
                    sec.relTable.push_back(e);
                    for(int i = 0; i < sizeof(int); i++){
                      sec.binary.push_back(0);
                    }
                  }
                }
              }
            }
          }
          argtmp = argtmp->next;
        }
      } else if(strcmp(dtmp->name, ".skip") == 0){
        if(strcmp(dtmp->args->type, "HEX") == 0){
          for(auto& sec : sectionTable){
            if(sec.sectionNumber == sectionCounter){
              for(int i = 0; i < dtmp->args->value.hexval; i++){
                sec.binary.push_back(0);
              }
            }
          }
          locationCounter += dtmp->args->value.hexval;
        } else {
          for(auto& sec : sectionTable){
            if(sec.sectionNumber == sectionCounter){
              for(int i = 0; i < dtmp->args->value.ival; i++){
                sec.binary.push_back(0);
              }
            }
          }
          locationCounter += dtmp->args->value.ival;
        }
      } else if(strcmp(dtmp->name, ".ascii") == 0){
        char* s = dtmp->args->value.sval;
        for(auto& sec : sectionTable){
          if(sec.sectionNumber == sectionCounter){
            while(s[0] != '\"' && s[0] != 0){
              sec.binary.push_back(s[0]);
              locationCounter++;
              s++;
            }
          }
        }
      } else if(strcmp(dtmp->name, ".equ") == 0){
        char* s = dtmp->args->value.sval;
        if(!symbolTableContains(symbolTable, s)){
          SymbolTableEntry e;
          e.number = symbolCounter++;
          e.value = 0;
          e.size = 0;
          e.type = "NOTYP";
          e.isGlobal = false;
          e.sectionNumber = 0;
          e.name = dtmp->args->value.sval;
          e.defined = true;
          symbolTable.push_back(e);
        } else {
          for(auto& symbol: symbolTable){
            if (strcmp(symbol.name, s) == 0) { 
              if(symbol.defined && !symbol.isGlobal){ // alias
                cout << "Visestruka definicija simbola " << symbol.name << "." << endl;
                throw new exception;
              }
              symbol.defined = true;
            }
          }     
        } 
        TUSEntry t;
        t.symbol = s;
        for(auto& argt = dtmp->args->next; argt; argt = argt->next){
          EquArg e;
          e.type = (char*) argt->type;
          if(!strcmp(argt->type, "INT")) e.value.ival = argt->value.ival;
          if(!strcmp(argt->type, "HEX")) e.value.hexval = argt->value.hexval;
          if(!strcmp(argt->type, "STRING") || !strcmp(argt->type, "OP") ) e.value.sval = argt->value.sval;
          t.la.push_back(e);
        }
        t.resolved = false;
        TUS.push_back(t);
        for(auto& argt = dtmp->args->next; argt; argt = argt->next){
          if(!strcmp(argt->type, "STRING") && !symbolTableContains(symbolTable, argt->value.sval)){
            SymbolTableEntry e;
            e.number = symbolCounter++;
            e.value = 0;
            e.size = 0;
            e.type = "NOTYP";
            e.isGlobal = false;
            e.sectionNumber = 0;
            e.name = argt->value.sval;
            e.defined = false;
            symbolTable.push_back(e);
          }
        }
      }

      dtmp = dtmp->next;
    }
    
  }
  for(auto& sec: sectionTable){
    if(sec.sectionNumber == sectionCounter){
      int count = 0;
      for(auto& en: sec.lpool){
        if(!strcmp(en.type,"STRING")){
          RelocationTableEntry e;
          bool t = false;
          int sec1 = 0;
          for(auto& s: symbolTable){
            if(!strcmp(en.value.sval, s.name) && s.isGlobal) {
              t = true;
            }
          }
          for(auto& s: symbolTable){
            if(!strcmp(en.value.sval, s.name)) {
              sec1 = s.sectionNumber;
            }
          }
          char* nm;
          for(auto& tmp: sectionTable){
            if(tmp.sectionNumber == sec1) nm = (char*)tmp.name;
          }
          int off = 0;
          for(auto& s: symbolTable){
            if(!strcmp(en.value.sval, s.name) && !s.isGlobal) {
              off = s.value;
            }
          }
          e.symbol = (t || sec1 == 0) ? en.value.sval : nm;
          e.addend = off;
          e.type = "32";
          e.offset = locationCounter + count;

          sec.relTable.push_back(e);
        }

        count += 4;
      }
      for(auto& lp: sec.lpool){
        
        if(!strcmp(lp.type, "INT")){
          int x = lp.value.ival;
          for(auto& i: lp.usedAt){
            int tmp = 0;
            int disp = locationCounter - i;
            for(auto& b: sec.binary){
              if(tmp + 4 == i){
                b = disp & 0xFF;
              }
              if(tmp + 3 == i){
                b = b | ((disp >> 8) & 0xF);
              }
              tmp++;
            }
          }
          for (int i = 0; i < sizeof(int); ++i) {      
            sec.binary.push_back(x & 0xFF);
            x >>= 8;
          }
        } else {

          for(auto& i: lp.usedAt){
            int tmp = 0;
            int disp = locationCounter - i;
            for(auto& b: sec.binary){
              if(tmp + 4 == i){
                b = disp & 0xFF;
              }
              if(tmp + 3 == i){
                b = b | ((disp >> 8) & 0xF);
              }
              tmp++;
            }
          }
          
          for (int i = 0; i < sizeof(int); ++i) {      
            sec.binary.push_back(0);
          }
        }
        locationCounter += 4;
      }
      sec.size = locationCounter;
    }
  }
  resolveEqu(symbolTable, sectionTable, TUS);
  proveri(symbolTable);
  resolveFlink(symbolTable, sectionTable);
  ispis(outText, sectionTable, symbolTable);
  ispisB(outputfile, sectionTable, symbolTable);

  return;
}


int main(int argc, char** argv) {
  if (argc != 2 && argc != 4) {
    cout << "Usage: " << argv[0] << " <inputfile> or " << argv[0] << " -o <outputfile> <inputfile>" << endl;
    return -1;
  }

  // Open a file handle to a particular file:
  FILE *myfile = fopen(argv[argc - 1], "r");
  // Make sure it is valid:
  if (!myfile) {
    cout << "I can't open " << argv[argc - 1] << "!" << endl;
    return -1;
  }
  // Set Flex to read from it instead of defaulting to STDIN:
  yyin = myfile;

  // If an output file is specified, redirect output to this file
  ofstream outputfile;
  ofstream outText;
  if (argc == 4) {
    outputfile.open(argv[2], ios::binary);
    string s(argv[2]);
    s = s.substr(0, s.length() - 2);
    s += ".txt";
    outText.open(s);
    if (!outputfile) {
      cout << "I can't open " << argv[2] << "!" << endl;
      return -1;
    }
  }
  else{
    outputfile.open("izlaz.o", ios::binary);
    outText.open("izlaz.txt");
    if (!outputfile) {
      cout << "I can't open " << argv[2] << "!" << endl;
      return -1;
    }
  }
  
  // Parse through the input: 
  yyparse();

  onePass(instr_head, directive_head, label_head, myfile, outputfile, outText);
  

  fclose(myfile);
  outputfile.close();
  outText.close();
}

