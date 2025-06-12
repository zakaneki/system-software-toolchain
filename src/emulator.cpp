#include <fstream>
#include "../inc/emulator.hpp"
#include <map>
#include <iostream>
#include <iomanip>
#include <thread>
#include <thread>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <shared_mutex>
#include <semaphore.h> 

using namespace std;

sem_t s1; 
sem_t s2; 

std::atomic<bool> stop(false);
std::shared_timed_mutex memory_mutex;
std::atomic<bool> termInInterrupt(false);
std::atomic<bool> timerInterrupt(false);
std::atomic<bool> termOutWrite(false);
std::atomic<bool> waitTermOut(true);


// status 0 
// handler 1 
// cause 2
void writeMemory(std::map<unsigned int, unsigned char>& memory, unsigned int value, unsigned int address);
unsigned int readMemory(std::map<unsigned int, unsigned char>& memory, unsigned int address);
void timer(map<unsigned int, unsigned char>& memory){
  while(!stop){
    int val = readMemory(memory, 0xFFFFFF10);
    switch(val){
      case 0x0:
        this_thread::sleep_for(chrono::milliseconds(500));
        break;
    case 0x1:
        this_thread::sleep_for(chrono::milliseconds(1000));
        break;
    case 0x2:
        this_thread::sleep_for(chrono::milliseconds(1500));
        break;
    case 0x3:
        this_thread::sleep_for(chrono::milliseconds(2000));
        break;
    case 0x4:
        this_thread::sleep_for(chrono::milliseconds(5000));
        break;
    case 0x5:
        this_thread::sleep_for(chrono::seconds(10));
        break;
    case 0x6:
        this_thread::sleep_for(chrono::seconds(30));
        break;
    case 0x7:
        this_thread::sleep_for(chrono::seconds(60));
        break;
    }
    timerInterrupt = true;
  }
}



void checkTermOutWrite(map<unsigned int, unsigned char>& memory) {
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &=(~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    while (!stop) {
        //if (termOutWrite) {
          sem_wait(&s1);
          if(!stop){
            unsigned char value = readMemory(memory, 0xFFFFFF00);
            value &= 0xFF;
            ssize_t written = write(STDOUT_FILENO, &value, 1);
            if (written > 0) { 
                // termOutWrite = false;
                // waitTermOut = false; 
                sem_post(&s2);
            }
          }
          
        //}
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return buf;
}

void checkForCharacters(map<unsigned int, unsigned char>& memory, unsigned int (&gprs)[16], unsigned int (&csrs)[3], bool& handlerInitialized) {
    while (!stop) {
        
        unsigned char c = getch();
        writeMemory(memory, c, 0xFFFFFF04);
        termInInterrupt = true;
        //while(waitTermOut);
    }
    return;
}

unsigned int signExtend12bit(unsigned int x) {
    if (x & (1 << 11)) {
        x |= 0xFFFFF000;
    }
    return x;
}

void writeMemory(map<unsigned int, unsigned char>& memory, unsigned int value, unsigned int address){
  memory_mutex.lock_shared();
  memory[address++] = value & 0xFF;
  value >>= 8;
  memory[address++] = value & 0xFF;
  value >>= 8;
  memory[address++] = value & 0xFF;
  value >>= 8;
  memory[address] = value & 0xFF;
  memory_mutex.unlock_shared();
}

unsigned int readMemory(map<unsigned int, unsigned char>& memory, unsigned int address){
  memory_mutex.lock();
  unsigned int value = memory[address++];
  value |= (memory[address++] << 8);
  value |= (memory[address++] << 16);
  value |= (memory[address] << 24);
  memory_mutex.unlock();
  return value;
}

bool processInstruction(map<unsigned int, unsigned char>& memory, unsigned int (&gprs)[16], unsigned int (&csrs)[3], bool& handlerInitialized ){
  unsigned char b1 = memory[gprs[15]++];
  unsigned char b2 = memory[gprs[15]++];
  unsigned char b3 = memory[gprs[15]++];
  unsigned char b4 = memory[gprs[15]++];
  switch(b4){
    case 0x00: 
      return false;
    case 0x10:
      writeMemory(memory, csrs[0], gprs[14] - 4);
      gprs[14] -= 4;
      writeMemory(memory, gprs[15], gprs[14] - 4);
      gprs[14] -= 4;
      csrs[2] = 4;
      csrs[0] |= 0x4;
      gprs[15] = csrs[1];
      break;
    case 0x20:
      writeMemory(memory, gprs[15], gprs[14] - 4);
      gprs[14] -= 4;
      gprs[15] = gprs[b3 >> 4] + gprs[b3 & 0xF] + signExtend12bit((b2 & 0xF) << 8 | b1);
      break;
    case 0x21:
      writeMemory(memory, gprs[15], gprs[14] - 4);
      gprs[14] -= 4;
      gprs[15] = readMemory(memory, gprs[b3 >> 4] + gprs[b3 & 0xF] + signExtend12bit((b2 & 0xF) << 8 | b1));
      break;
    case 0x30:
      gprs[15] = gprs[b3 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1);
      break;
    case 0x31:
      if(gprs[b3 & 0xF] == gprs[b2 >> 4]) gprs[15] = gprs[b3 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1);
      break;
    case 0x32:
      if(gprs[b3 & 0xF] != gprs[b2 >> 4]) gprs[15] = gprs[b3 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1);
      break;
    case 0x33:
      if(signed(gprs[b3 & 0xF]) > signed(gprs[b2 >> 4])) gprs[15] = gprs[b3 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1);
      break;
    case 0x38:
      gprs[15] = readMemory(memory, gprs[b3 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1));
      break;
    case 0x39:
      if(gprs[b3 & 0xF] == gprs[b2 >> 4]) gprs[15] = readMemory(memory, gprs[b3 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1));
      break;
    case 0x3A:
      if(gprs[b3 & 0xF] != gprs[b2 >> 4]) gprs[15] = readMemory(memory, gprs[b3 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1));
      break;
    case 0x3B:
      if(signed(gprs[b3 & 0xF]) > signed(gprs[b2 >> 4])) gprs[15] = readMemory(memory, gprs[b3 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1));
      break;
    case 0x40:{
      unsigned int temp = gprs[b3 & 0xF];
      gprs[b3 & 0xF] = gprs[b2 >> 4];
      gprs[b2 >> 4] = temp;}
      break;
    case 0x50:
      gprs[b3 >> 4] = gprs[b3 & 0xF] + gprs[b2 >> 4];
      break;
    case 0x51:
      gprs[b3 >> 4] = gprs[b3 & 0xF] - gprs[b2 >> 4];
      break;
    case 0x52:
      gprs[b3 >> 4] = gprs[b3 & 0xF] * gprs[b2 >> 4];
      break;
    case 0x53:
      gprs[b3 >> 4] = gprs[b3 & 0xF] / gprs[b2 >> 4];
      break;
    case 0x60:
      gprs[b3 >> 4] = ~gprs[b3 & 0xF];
      break;
    case 0x61:
      gprs[b3 >> 4] = gprs[b3 & 0xF] & gprs[b2 >> 4];
      break;
    case 0x62:
      gprs[b3 >> 4] = gprs[b3 & 0xF] | gprs[b2 >> 4];
      break;
    case 0x63:
      gprs[b3 >> 4] = gprs[b3 & 0xF] ^ gprs[b2 >> 4];
      break;
    case 0x70:
      gprs[b3 >> 4] = gprs[b3 & 0xF] << gprs[b2 >> 4];
      break;
    case 0x71:
      gprs[b3 >> 4] = gprs[b3 & 0xF] >> gprs[b2 >> 4];
      break;
    case 0x80:{
      writeMemory(memory, gprs[b2 >> 4], gprs[b3 >> 4] + gprs[b3 & 0xF] + signExtend12bit((b2 & 0xF) << 8 | b1));
      if(gprs[b3 >> 4] + gprs[b3 & 0xF] + signExtend12bit((b2 & 0xF) << 8 | b1) == 0xFFFFFF00){
        sem_post(&s1);
        sem_wait(&s2);
        // termOutWrite = true;
        // while(waitTermOut == true);
      } 
      break;
    }
      
    case 0x81:{
      gprs[b3 >> 4] = gprs[b3 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1);
      writeMemory(memory, gprs[b2 >> 4], gprs[b3 >> 4]);
      if(gprs[b3 >> 4] == 0xFFFFFF00){
        sem_post(&s1);
        sem_wait(&s2);
        // termOutWrite = true;  
        // while(waitTermOut == true);    
      } 
      break;
    }
      
    case 0x82:{
      writeMemory(memory, gprs[b2 >> 4], readMemory(memory, gprs[b3 >> 4] + gprs[b3 & 0xF] + signExtend12bit((b2 & 0xF) << 8 | b1)));
      if(readMemory(memory, gprs[b3 >> 4] + gprs[b3 & 0xF] + signExtend12bit((b2 & 0xF) << 8 | b1)) == 0xFFFFFF00){
        sem_post(&s1);
        sem_wait(&s2);
        // termOutWrite = true;
        // while(waitTermOut == true);  
      } 
      break;
    }
      
    case 0x90:
      gprs[b3 >> 4] = csrs[b3 & 0xF];
      break;
    case 0x91:
      gprs[b3 >> 4] = gprs[b3 & 0xF] + signExtend12bit((b2 & 0xF) << 8 | b1);
      break;
    case 0x92:
      gprs[b3 >> 4] = readMemory(memory, gprs[b3 & 0xF] + gprs[b2 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1));
      break;
    case 0x93:
      gprs[b3 >> 4] = readMemory(memory, gprs[b3 & 0xF]);
      gprs[b3 & 0xF] = gprs[b3 & 0xF] + signExtend12bit((b2 & 0xF) << 8 | b1);
      break;
    case 0x94:
      csrs[b3 >> 4] = gprs[b3 & 0xF];
      if((b3 >> 4) == 1) handlerInitialized = true;
      break;
    case 0x95:
      csrs[b3 >> 4] = csrs[b3 & 0xF] | signExtend12bit((b2 & 0xF) << 8 | b1);
      if((b3 >> 4) == 1) handlerInitialized = true;
      break;
    case 0x96:
      csrs[b3 >> 4] = readMemory(memory, gprs[b3 & 0xF] + gprs[b2 >> 4] + signExtend12bit((b2 & 0xF) << 8 | b1));
      if((b3 >> 4) == 1) handlerInitialized = true;
      break;
    case 0x97:
      csrs[b3 >> 4] = readMemory(memory, gprs[b3 & 0xF]);
      gprs[b3 & 0xF] = gprs[b3 & 0xF] + signExtend12bit((b2 & 0xF) << 8 | b1);
      if((b3 >> 4) == 1) handlerInitialized = true;
      break;
    default:{
      if(!handlerInitialized){
        cout << "Nepostojeci op_code" << endl;
        throw new exception;
      } else {
        writeMemory(memory, csrs[0], gprs[14] - 4);
        gprs[14] -= 4;
        writeMemory(memory, gprs[15], gprs[14] - 4);
        gprs[14] -= 4;
        csrs[2] = 1;
        csrs[0] |= 0x4;
        gprs[15] = csrs[1];
      }
    }
      
    

  }
  if(handlerInitialized){
      if(!(csrs[0] & 0x4)){
        if(!(csrs[0] & 0x2) && termInInterrupt){
          termInInterrupt = false;
          writeMemory(memory, csrs[0], gprs[14] - 4);
          gprs[14] -= 4;
          writeMemory(memory, gprs[15], gprs[14] - 4);
          gprs[14] -= 4;
          csrs[2] = 3;
          csrs[0] |= 0x4;
          gprs[15] = csrs[1];
        } 
        else if(!(csrs[0] & 0x1) && timerInterrupt){
          timerInterrupt = false;
          writeMemory(memory, csrs[0], gprs[14] - 4);
          gprs[14] -= 4;
          writeMemory(memory, gprs[15], gprs[14] - 4);
          gprs[14] -= 4;
          csrs[2] = 2;
          csrs[0] |= 0x4;
          gprs[15] = csrs[1];
        }
      }
      
    }
  return true;
}

void loadMemory(ifstream& in, map<unsigned int, unsigned char>& memory){
  unsigned int address;
  char value;
  while(in.read((char*)&address, sizeof(address))){
    if(in.read((char*)&value, sizeof(value))){
        memory.insert({address, value});
    }
  }
}


int main(int argc, char** argv){
  sem_init(&s1, 0, 0); 
  sem_init(&s1, 0, 0); 
  ifstream in(argv[1], ios::binary);
  map<unsigned int, unsigned char> memory;
  loadMemory(in, memory);
  unsigned int gprs[16] = {0};
  gprs[15] = 0x40000000;
  unsigned int csrs[3] = {0};
  bool handlerInitialized = false;
  std::thread tin(checkForCharacters, ref(memory), ref(gprs), ref(csrs), ref(handlerInitialized));
  std::thread tout(checkTermOutWrite, ref(memory));
  std::thread ttimer(timer, ref(memory));
  while(processInstruction(memory, gprs, csrs, handlerInitialized)){
    
  }
  stop = true;
  sem_post(&s1);
  tin.join();
  tout.join();
  ttimer.join();
  cout << endl;
  cout << "------------------------------------------------" << endl;
  cout << "Emulated processor executed halt instruction" << endl;
  cout << "Emulated processor state:" << endl;
  
  cout << "r0=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[0] << " " ;
  cout << "r1=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[1] << " " ;
  cout << "r2=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[2] << " " ;
  cout << "r3=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[3] << " " ;
  cout << endl;
  cout << "r4=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[4] << " " ;
  cout << "r5=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[5] << " " ;
  cout << "r6=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[6] << " " ;
  cout << "r7=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[7] << " " ;
  cout << endl;
  cout << "r8=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[8] << " " ;
  cout << "r9=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[9] << " " ;
  cout << "r10=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[10] << " " ;
  cout << "r11=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[11] << " " ;
  cout << endl;
  cout << "r12=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[12] << " " ;
  cout << "r13=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[13] << " " ;
  cout << "r14=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[14] << " " ;
  cout << "r15=0x" << std::setfill('0') << std::setw(8) << hex <<  gprs[15] << " " ;
  
  sem_destroy(&s1);
  sem_destroy(&s2);
  return 0;
}