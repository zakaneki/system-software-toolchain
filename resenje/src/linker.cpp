#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <list>
#include "../inc/linker.hpp"
#include <string.h>
#include <algorithm>
#include <iomanip>

using namespace std;

void ispisRB(ofstream& outputfile, list<SymbolTableEntry> sectionTable, list<SymbolTableEntry> symbolTable){
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
  return;
}

void ispisR(ofstream& outText, list<SymbolTableEntry> sectionTable, list<SymbolTableEntry> symbolTable){
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

void mergeSections(list<list<SymbolTableEntry>>& sectionTables, list<list<SymbolTableEntry>>& symbolTables, list<SymbolTableEntry>& newSectionTable, list<SymbolTableEntry>& newSymbolTable){
  int sectionCounter = 0;
  int fileNumber = 0;
  for(auto& sectionTable: sectionTables){
    for(auto& section: sectionTable){
      bool t = false;
      int tc = 0;
      for(auto& tmp : newSectionTable){
        if(!strcmp(section.name, tmp.name)) t = true;
        tc++;
      }
      if(t){
        for(auto& ss: newSectionTable){
          if(!strcmp(section.name, ss.name)){
            for(auto& b: section.binary){
              ss.binary.push_back(b);
            }
            for(auto& s1: sectionTable){
              for(auto& r : s1.relTable){
                if(!strcmp(r.symbol, section.name)){
                  r.addend += ss.size;
                }
              }
            }
            for(auto& r: section.relTable){
              r.offset += ss.size;
              // if(!strcmp(r.symbol, ss.name)){
              //   r.addend += ss.size;
              // }
              ss.relTable.push_back(r);
            }
            int count = 0;
            for(auto& symbolTable: symbolTables){
              for(auto& symbol: symbolTable){
                if(count == fileNumber && symbol.sectionNumber == section.number){
                  symbol.sectionNumber = ss.number;
                  symbol.value += ss.size;
                }
              }
              count++;
            }

            ss.size += section.size;
          }
        }

      } else {
        int count = 0;
        for(auto& symbolTable: symbolTables){
          for(auto& symbol: symbolTable){
            if(count == fileNumber && symbol.sectionNumber == section.number){
              symbol.sectionNumber = sectionCounter;
            }
          }
          count++;
        }
        section.number = sectionCounter++;
        newSectionTable.push_back(section);
      }
    }
    fileNumber++;
  }
  
  int symbolCounter = 0; 
  for(auto& symbolTable: symbolTables){
    for(auto& symbol: symbolTable){
      bool t = false;
      for(auto& tmp : newSymbolTable){
        if(!strcmp(symbol.name, tmp.name)) t = true;
      }
      if(t){
        bool x = false;
        bool y = false;
        for(auto& tmp : newSymbolTable){
          if(!strcmp(symbol.name, tmp.name)){
            if(tmp.isGlobal && (tmp.sectionNumber != 0 || (tmp.sectionNumber == 0 && tmp.value != 0)) && symbol.sectionNumber == 0) y = true;
            if(symbol.isGlobal && tmp.isGlobal && (tmp.sectionNumber == 0 && tmp.value == 0) && (symbol.sectionNumber != 0 || (symbol.sectionNumber == 0 && symbol.value != 0))){
              tmp.sectionNumber = symbol.sectionNumber;
              tmp.value = symbol.value;
              x = true;
            }

            // if((symbol.isGlobal && !tmp.isGlobal) || !symbol.isGlobal && addressof(tmp) != addressof(symbol)){
            //   newSymbolTable.push_back(symbol);
            // } else if(symbol.sectionNumber != 0 && tmp.sectionNumber == 0){
            //   tmp.sectionNumber = symbol.sectionNumber;
            //   tmp.value = symbol.value;
            // }
          }
        }
        if(!x && !y){
          symbol.number = symbolCounter++;
          newSymbolTable.push_back(symbol);
        } 
      } else {
        symbol.number = symbolCounter++;
        newSymbolTable.push_back(symbol);
      }
    }
  }
  for(auto& section: newSectionTable){
    for(auto& r: section.relTable){
      for(auto& symbol: newSymbolTable){
        if(!strcmp(r.symbol, symbol.name)){
          if(symbol.isGlobal && symbol.sectionNumber != 0){
            r.addend = symbol.value;
            char* name;
            for(auto& s1: newSectionTable){
              if(s1.number == symbol.sectionNumber) name = s1.name;
            }
            r.symbol = name;
          }
        }
      }
    }
  }
  return;
}

void proveri(list<list<SymbolTableEntry>> symbolTables, int o){
  int f1 = 0;
  for(auto& symbolTable: symbolTables){
    for(auto& symbol: symbolTable){
      if(symbol.sectionNumber == 0 && symbol.value == 0){
        if(o == 1){
          bool t = false;
          int f2 = 0;
          for(auto& s1: symbolTables){

            for(auto& s2: s1){
              if(!strcmp(symbol.name, s2.name) && ((s2.sectionNumber != 0 && (s2.isGlobal || f1 == f2)) || (s2.sectionNumber == 0 && s2.value != 0 && (s2.isGlobal || f1 == f2)))) t = true;
            }
            f2++;
          }
          if(!t){
            cout << "Simbol " << symbol.name << " nije definisan." << endl;
            throw new exception;
          }
        }
      } else if(symbol.isGlobal){
        bool t = false;
        for(auto& s1: symbolTables){
          for(auto& s2: s1){
            if(!strcmp(symbol.name, s2.name) && (s2.sectionNumber != 0 || (s2.sectionNumber == 0 && s2.value != 0)) && s2.isGlobal && addressof(s2) != addressof(symbol)) {
              t = true;
            }
          }
        }
        if(t){
          cout << "Simbol " << symbol.name << " je visestruko definisan." << endl;
          throw new exception;
        }
      }
    }
  }
  f1++;
}

void ispisB(ofstream& outB, list<list<SymbolTableEntry>> sectionTables){
  for(auto& sectionTable: sectionTables){
    for(auto& section: sectionTable){
      unsigned long offset = 0;
      for(auto& c: section.binary){
        unsigned int address = section.startAddress + offset;
        outB.write((char*)&address, sizeof(address));
        outB.write((char*)&c, sizeof(c));
        offset++;
      }
    }
  }
}

void ispis(ofstream& outputfile, list<list<SymbolTableEntry>> sectionTables){
  for(auto& sectionTable : sectionTables){
    int count = 0;
    for (const auto& section : sectionTable) {
      if(!section.binary.empty()){
        count = 0;
        unsigned long address = section.startAddress;
        for (const auto& byte : section.binary) {
            if (count % 8 == 0) {
                outputfile << std::right << std::hex << std::setw(8) << std::setfill('0') << address << ": ";
            }
            outputfile << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(byte)) << " ";
            if (++count % 8 == 0) {
                outputfile << "\n";
                address += 8;
            }
        }
      }
      if (count % 8 != 0) {
      outputfile << "\n";
      }
    }
  }
}

void resolveRelocationTables(list<list<SymbolTableEntry>>& sectionTables, list<list<SymbolTableEntry>>& symbolTables, map<string, unsigned long>& sectionAddresses) {
    for (auto& sectionTable : sectionTables) {
        for (auto& section : sectionTable) {
            for (auto& relocation : section.relTable) {
              bool t = false;
              for(auto& tmp : sectionTable){
                if (strcmp(tmp.name, relocation.symbol) == 0){
                  t = true; 
                  unsigned value = tmp.startAddress + relocation.addend;
                  int i = 0;
                  for(auto& c: section.binary){
                    if(i == relocation.offset || i == (relocation.offset + 1) || i == (relocation.offset + 2) || i == (relocation.offset + 3)){
                      c = value & 0xFF;
                      value >>= 8;
                    }
                    i++;
                  }
                }
              }
              if(!t) {
                // Relocation symbol is a symbol
                int fileNumber = 0;
                for (auto& symbolTable : symbolTables) {
                  for(auto& symbol: symbolTable){
                    if (strcmp(symbol.name, relocation.symbol) == 0 && symbol.sectionNumber != 0){
                      unsigned long sectionAddress;
                      int secFileNumber = 0;
                      for(auto& tmp1: sectionTables){
                        if(fileNumber == secFileNumber){
                          for(auto& tmp2: tmp1){
                            if(tmp2.number == symbol.sectionNumber) sectionAddress = tmp2.startAddress + symbol.value;
                          }
                        }
                        secFileNumber++;
                      }
                      unsigned value = sectionAddress + relocation.addend;
                      int i = 0;
                      for(auto& c: section.binary){
                        if(i == relocation.offset || i == (relocation.offset + 1) || i == (relocation.offset + 2) || i == (relocation.offset + 3)){
                          c = value & 0xFF;
                          value >>= 8;
                        }
                        i++;
                      }
                    } else if( strcmp(symbol.name, relocation.symbol) == 0 && symbol.sectionNumber == 0 && symbol.value != 0){
                      unsigned value = symbol.value;
                      int i = 0;
                      for(auto& c: section.binary){
                        if(i == relocation.offset || i == (relocation.offset + 1) || i == (relocation.offset + 2) || i == (relocation.offset + 3)){
                          c = value & 0xFF;
                          value >>= 8;
                        }
                        i++;
                      }
                    }
                  }
                  fileNumber++;
                }
              }
            }
        }
    }
}
void assignAddresses(list<list<SymbolTableEntry>>& sectionTables, map<string, unsigned long>& sectionAddresses) {
  map<string, pair<unsigned int, unsigned int>> placedSections;
  map<string, unsigned int> totalSizes;
  for(auto& [section, startAddress] : sectionAddresses){
    unsigned int totalSize = 0;
    for(auto& psecTable: sectionTables){
      for(auto& psec: psecTable){
        if(!strcmp(section.c_str(), psec.name)){
          totalSize += psec.size;
        }
      }
    }
    totalSizes.insert({section, totalSize});
  }
  for(auto& [section, startAddress] : sectionAddresses){
    for(auto& [otherSection, otherStartAddress] : sectionAddresses){
      if(section != otherSection && ((startAddress >= otherStartAddress && startAddress <= otherStartAddress + totalSizes[otherSection])||(startAddress + totalSizes[section] >= otherStartAddress && startAddress + totalSizes[section] <= otherStartAddress + totalSizes[otherSection]))){
        cout << "Sekcije " << section << " i " << otherSection << " se preklapaju." << endl;
        throw new exception;
      }
    }
  }
  unsigned int lastAddress = 0;
  for(auto& [section, startAddress] : sectionAddresses){
    for(auto& psecTable: sectionTables){
      for(auto& psec: psecTable){
        if(!strcmp(section.c_str(), psec.name)){
          psec.startAddress = max(placedSections[section].first + placedSections[section].second, (unsigned)startAddress);
          placedSections[section] = {startAddress, placedSections[section].second + psec.size};
          lastAddress = max(lastAddress, placedSections[section].first + placedSections[section].second);
        }
      }
    }
  }
  for(auto& sectionTable: sectionTables){
    for(auto& sec: sectionTable){
      if(!sectionAddresses.count(sec.name) && sec.number != 0){
        if(!placedSections.count(sec.name)){
          sec.startAddress = lastAddress;
          placedSections[sec.name] = {sec.startAddress, sec.size};
          lastAddress = sec.startAddress + sec.size;
        } else {
          sec.startAddress = placedSections[sec.name].first + placedSections[sec.name].second;
          bool t = false;
          for(auto& [section, pair]: placedSections){
            if(strcmp(section.c_str(), sec.name) && ((sec.startAddress >= pair.first && sec.startAddress <= pair.first + pair.second) || (sec.startAddress + sec.size >= pair.first && sec.startAddress + sec.size <= pair.first + pair.second))){
              t = true;
              break;
            }
          }
          if(t){
            for(auto& [section, pair]: placedSections){
              if(pair.first >= sec.startAddress && strcmp(section.c_str(), sec.name)) {
                placedSections[section] = {placedSections[section].first + sec.size, placedSections[section].second};
                for(auto& sectionTable1: sectionTables){
                  for(auto& sec1: sectionTable1){
                    if(!strcmp(sec1.name, section.c_str())) sec1.startAddress += sec.size;
                  }
                }
              }
            }
            placedSections[sec.name] = {placedSections[sec.name].first, placedSections[sec.name].second + sec.size};
            lastAddress += sec.size;
          } else {
            placedSections[sec.name] = {placedSections[sec.name].first, placedSections[sec.name].second + sec.size};
            lastAddress = sec.startAddress + sec.size;
          }
        }
      }
    }
  }
  return;
}

void link(list<char*> inputFiles, ofstream& outputfile, ofstream& outB, map<string, unsigned long> sectionAddresses, int o){
  list<list<SymbolTableEntry>> symbolTables;
  list<list<SymbolTableEntry>> sectionTables;
  for(auto file: inputFiles){
    ifstream inf(file, ios::binary);
    list<SymbolTableEntry> symbolTable;
    list<SymbolTableEntry> sectionTable;
    unsigned sectionTableCount;
    inf.read((char*)&sectionTableCount, sizeof(sectionTableCount));
    for(int i = 0; i < sectionTableCount; i++){
      SymbolTableEntry x;
      inf.read((char*)&x.number, sizeof(x.number));
      inf.read((char*)&x.size, sizeof(x.size));
      unsigned szname;
      inf.read((char*)&szname, sizeof(szname));
      x.name = new char[szname + 1]; 
      inf.read(x.name, szname);
      x.name[szname] = '\0'; 
      for (int j = 0; j < x.size; j++){
        char c;
        inf.read((char*)&c, sizeof(c));
        x.binary.push_back(c);
      }
      sectionTable.push_back(x);
    }
    unsigned symbolTableCount;
    inf.read((char*)&symbolTableCount, sizeof(symbolTableCount));
    for(int i = 0; i < symbolTableCount; i++){
      SymbolTableEntry x;
      inf.read((char*)&x.number, sizeof(x.number));
      inf.read((char*)&x.value, sizeof(x.value));
      unsigned sztype;
      inf.read((char*)&sztype, sizeof(sztype));
      x.type = new char[sztype + 1];
      inf.read(x.type, sztype);
      x.type[sztype] = '\0';
      inf.read((char*)&x.isGlobal, sizeof(x.isGlobal));
      inf.read((char*)&x.sectionNumber, sizeof(x.sectionNumber));
      unsigned szname;
      inf.read((char*)&szname, sizeof(szname));
      x.name = new char[szname + 1];
      inf.read(x.name, szname);
      x.name[szname] = '\0';
      symbolTable.push_back(x);
    }
    unsigned rel_tab_size;
    unsigned secNumber;
    while(inf.read((char*)&rel_tab_size, sizeof(rel_tab_size)) && !inf.eof()){
      inf.read((char*)&secNumber, sizeof(secNumber));
      for(auto& sec: sectionTable){
        if(sec.number == secNumber){
          for(int i = 0; i < rel_tab_size; i++){
            RelocationTableEntry x;
            inf.read((char*)&x.offset, sizeof(x.offset));
            unsigned sztip;
            inf.read((char*)&sztip, sizeof(sztip));
            x.type = new char[sztip + 1];
            inf.read(x.type, sztip);
            x.type[sztip] = '\0';

            unsigned szsimbol;
            inf.read((char*)&szsimbol, sizeof(szsimbol));
            x.symbol = new char[szsimbol + 1];
            inf.read(x.symbol, szsimbol);
            x.symbol[szsimbol] = '\0';

            inf.read((char*)&x.addend, sizeof(x.addend));
            sec.relTable.push_back(x);
          }
        }
      }
    }
    symbolTables.push_back(symbolTable);
    sectionTables.push_back(sectionTable);
    inf.close();
  }
  proveri(symbolTables, o);
  if(o == 1){
    assignAddresses(sectionTables, sectionAddresses);
    resolveRelocationTables(sectionTables, symbolTables, sectionAddresses);
    ispis(outputfile, sectionTables);
    ispisB(outB, sectionTables);
  } else if (o == 2){
    list<SymbolTableEntry> newSectionTable;
    list<SymbolTableEntry> newSymbolTable;
    mergeSections(sectionTables, symbolTables, newSectionTable, newSymbolTable);
    ispisR(outputfile, newSectionTable, newSymbolTable);
    ispisRB(outB, newSectionTable, newSymbolTable);
  }
  
  return;
}

int main(int argc, char* argv[]) {
    ofstream outputfile;
    ofstream outB;
    map<string, unsigned long> sectionAddresses;
    list<char*> inputFiles;
    bool t = false;
    int o = 0;
    string outName;
    string outBName;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        
        if (arg == "-hex") {
          if(o == 2){
            cout << "Zabranjeno istovremeno navodjenje opcija -hex i -relocatable." << endl;
            throw new exception;
          }
          t = true;
          o = 1;
        } else if (arg == "-relocatable") {
          if(o == 1){
            cout << "Zabranjeno istovremeno navodjenje opcija -hex i -relocatable." << endl;
            throw new exception;
          }
          t = true;
          o = 2;
        } else if (arg == "-o" && i + 1 < argc) {
          outBName = argv[++i];
          string s(argv[i]);
          s = s.substr(0, s.length() - 4);
          s += ".txt";
          outName = s;
        } else if (arg.substr(0, 7) == "-place=") {
          string sectionAndAddress = arg.substr(7);
          size_t atPos = sectionAndAddress.find('@');
          if (atPos != string::npos) {
              string section = sectionAndAddress.substr(0, atPos);
              string addressStr = sectionAndAddress.substr(atPos + 1);
              unsigned long address = stoul(addressStr, nullptr, 0);
              sectionAddresses[section] = address;
          }
        } else {
          char* tmp=argv[i];
          inputFiles.push_back(tmp);
        }
    }
    outputfile.open(outName);
    outB.open(outBName, ios::binary);
    
    if (!outB) {
        cout << "I can't open " << outBName << "!" << endl;
        return -1;
    }
    if(!outputfile.is_open()) outputfile.open("linker.o", ios::binary);
    if(!t){
      cout << "Nije navedena nijedna od opcija -hex i -relocatable.";
      throw new exception;
    }

    link(inputFiles, outputfile, outB, sectionAddresses, o);

    outputfile.close();
    outB.close();

    return 0;
}