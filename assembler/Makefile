CC = g++
FLAGS = -std=c++11 -Wall -g

assembler.exe: assembler.cpp
	${CC} ${FLAGS} $^ -o $@

asm: assembler.cpp
	${CC} ${FLAGS} -S $^ -o assembler.asm

clean:
	rm *.exe *.asm
