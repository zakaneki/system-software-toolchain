myparser.tab.c myparser.tab.h: misc/myparser.y
	bison -d misc/myparser.y 

lex.yy.c: misc/mylexer.l myparser.tab.h
	flex misc/mylexer.l

asembler: lex.yy.c src/assembler.cpp 
	g++ lex.yy.c src/assembler.cpp -lfl -g -fdiagnostics-color=always -o asembler 

linker: asembler src/linker.cpp inc/linker.hpp
	g++ src/linker.cpp inc/linker.hpp -g -fdiagnostics-color=always -o linker 

emulator: linker inc/emulator.hpp src/emulator.cpp
	g++ src/emulator.cpp inc/emulator.hpp -g -fdiagnostics-color=always -o emulator  -lpthread