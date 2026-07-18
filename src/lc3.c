#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <assert.h>
#include <ncurses.h>
#include "os.h"

#define IMM5 5
#define PCOFFSET9 9 
#define MEM_ADDRESSES_NUM (1<<16)

enum {
	REG0 = 0,
	REG1, 	   		
	REG2,   
	REG3,   
	REG4,   
	REG5,   
	REG6,   
	REG7,   
	REG_COND,   
	REG_PC, /*program counter */
	REG_COUNT   
};

enum {
	OP_BR = 0, /* branch */
	OP_ADD,    /* add  */
	OP_LD,     /* load */
	OP_ST,     /* store */
	OP_JSR,    /* jump register */
	OP_AND,    /* bitwise and */
	OP_LDR,    /* load register */
	OP_STR,    /* store register */
	OP_RTI,    /* unused */
	OP_NOT,    /* bitwise not */
	OP_LDI,    /* load indirect */
	OP_STI,    /* store indirect */
	OP_JMP,    /* jump */
	OP_RES,    /* reserved (unused) */
	OP_LEA,    /* load effective address */
	OP_TRAP    /* execute trap */
};

enum{
	FL_POS = 1 << 0, /* FLAG POSITIVE */
	FL_ZRO = 1 << 1, /* FLAG ZERO     */
	FL_NEG = 1 << 2  /* FLAG NEGATIVE */
};

typedef enum{
	RUNNING = 0,
	PAUSED,
	HALTED
}status;

typedef enum{
	NORMAL = 0,
	DEBUGGER,
	CLI
}mode;

uint16_t memory[MEM_ADDRESSES_NUM];
uint16_t reg[REG_COUNT];
uint16_t pc;
struct termios oldTerminalMode;
status machineStatus = RUNNING;
mode currentMode = NORMAL;

void SetOldterminalMode(){
	tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminalMode); 
}

void HandleTerminalInterrupt(){
	printf("terminal interrupted\n");
	SetOldterminalMode();
	exit(0);
}

void SetNewTerminalMode(){
	/* stop input buffering and echo mode */
	tcgetattr(STDIN_FILENO, &oldTerminalMode);
	struct termios newTerminalMode = oldTerminalMode;
	newTerminalMode.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalMode); 
}

struct pollfd fds[1];
bool IsKeyPressed(){
	int timeout = 500;
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	int ret = poll(fds,1,timeout);

	if(ret > 0) return true;

	return false;
}

void LoadOS(){
	uint16_t memPtr = 0;
	for(unsigned int i = 1; i < __bin_os_obj_len-2; i+=2){
		uint16_t msb   = (uint16_t)(__bin_os_obj[i-1]);
		uint16_t lsb   = (uint16_t)(__bin_os_obj[i]);
		memory[memPtr] = (uint16_t)((lsb << 8) | msb);
		memPtr++;
	}
}

uint16_t ToLittleEndian(uint16_t val){
	return (val << 8 | val >> 8);
}

bool LoadProgram(const char *path){
	FILE *image = fopen(path, "rb");
	if(image == NULL){
		fprintf(stderr, "Couldn't open file: %s\n", strerror(errno));
		return false;
	}
	
	size_t res = fread(&pc, sizeof(uint16_t), 1, image);
	assert(res > 0);
	pc = ToLittleEndian(pc);

	size_t maxRead = MEM_ADDRESSES_NUM - pc;
	uint16_t *ptr = memory + pc;
	res = fread(ptr, sizeof(uint16_t), maxRead, image);

	while(res--> 0){
		*ptr = ToLittleEndian(*ptr);
		ptr++;
	}
	
	fclose(image);

	return true;
}

uint16_t SEXT(uint16_t num, int MODE){
	if(num >> (MODE - 1) & 1){
		return num | (0xFFFF << MODE);
	}

	return num;
}

void setcc(uint8_t DR){
	if(reg[DR] >> 15 & 1){
		reg[REG_COND] = FL_NEG;
	}else if(reg[DR] == 0){
		reg[REG_COND] = FL_ZRO;
	}else{
		reg[REG_COND] = FL_POS;
	}
}

void ADD_AND(uint16_t instr){
	uint8_t DR  = instr >> 9 & 0x3;
	uint8_t SR1 = instr >> 6 & 0x3;
	
	if(instr >> 5 & 1){
		uint16_t imm5 = SEXT(instr & 0x1F, IMM5);
		if(instr >> 12 == OP_ADD)reg[DR] = reg[SR1] + imm5;
		else reg[DR] = reg[SR1] & imm5;
	}else{
		uint8_t SR2 = instr & 0x3;
		if(instr >> 12 == OP_ADD) reg[DR] = reg[SR1] + reg[SR2];
		else reg[DR] = reg[SR1] * reg[SR2];
	}
	
	setcc(DR);
}

void BR(uint16_t instr){
	if(((instr >> 9) & 0x7) & reg[REG_COND]){
		uint16_t pcoffset9 = instr & 0x1FF;
		pc = pc + SEXT(pcoffset9, PCOFFSET9);
	}
}

void JUMP(uint16_t instr){
	uint8_t BaseR = instr >> 6 & 0x7;
	pc = BaseR;
}

/* ----------- Curses ----------- */
int terminalColumns;
int terminalRows;
void InitCurses(){
	initscr();
	noecho();
	raw();

	getmaxyx(stdscr,terminalRows, terminalColumns);
	assert(terminalRows > 20 && terminalColumns > 80 && "Terminal too small");

	refresh();
}

WINDOW *CreateNewWindow(int rows, int cols, int y, int x){
	WINDOW *win = newwin(rows,cols,y,x);
	box(win, 0,0);
	wrefresh(win);

	return win;
}

void DrawMainWindow(){

}

int main(int argc, char **argv){
	if(argc < 2){
help:
		fprintf(stderr, "Usage: ./lc3 -OPTIONS[dh] image.obj \n");
		fprintf(stderr, "d --> debugger mode \n");
		fprintf(stderr, "h --> prints this help message\n");
		return 1;
	}
	
	char opt;
	int cargc = 1;
	while((opt = getopt(argc, argv, "dh")) != -1){
		switch(opt){
			case 'd': currentMode = DEBUGGER; cargc++; break;
			case 'h': goto help;
			default: break;
		}
	}
	
	if(cargc > argc-1) goto help;
	const char *programPath = argv[cargc];

	LoadOS();
	if(!LoadProgram(programPath)){
		fprintf(stderr, "Couldn't load program \"%s\" \n", argv[1]);
		return 1;
	}
	
	WINDOW *mainWindow;
	WINDOW *registerWindow;
	WINDOW *infoWindow;
	WINDOW *outputWindow;
	WINDOW *inputWindow;
	if(currentMode == NORMAL){
		signal(SIGINT, HandleTerminalInterrupt);
		SetNewTerminalMode();
	}else if(currentMode == DEBUGGER){
		InitCurses();
		mainWindow = CreateNewWindow(terminalRows/1.3,terminalColumns/2,0,0);
		registerWindow = CreateNewWindow(terminalRows/3,terminalColumns/2,0,terminalColumns/2);
		outputWindow = CreateNewWindow(terminalRows - terminalRows/1.3,terminalColumns/2,terminalRows/1.3,0);
		inputWindow = CreateNewWindow(terminalRows - terminalRows/1.3,terminalColumns/2,terminalRows/1.3,terminalColumns/2);
		infoWindow = CreateNewWindow((terminalRows/1.3)-terminalRows/3,terminalColumns/2,terminalRows/3,terminalColumns/2);
		getch();
		endwin();
	}else{
		/* TODO: CLI */
	}
	
	while(machineStatus == RUNNING){
		uint16_t opcode = memory[pc] >> 12;
		switch(opcode){
			case OP_ADD: ADD_AND(memory[pc++]);  break;
			case OP_AND: ADD_AND(memory[pc++]);  break;
			case OP_BR:  BR(memory[pc++]);       break;
			case OP_JMP: JUMP(memory[pc++]);     break;
			case OP_JSR: break;
			case OP_LD: break;
			case OP_LDI: break;
			case OP_LEA: break;
			case OP_NOT: break;
			case OP_RTI: break;
			case OP_STI: break;
			case OP_STR: break;
			case OP_TRAP: break;
			default: pc++; break;
		}
	}
	
	SetOldterminalMode();

	return 0;
}
















