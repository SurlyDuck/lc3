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
	CLI = 0,
	DEBUGGER,
	EMBEDDED
}mode;

uint16_t memory[MEM_ADDRESSES_NUM];
uint16_t reg[REG_COUNT];
struct termios oldTerminalMode;
status machineStatus = RUNNING;
mode currentMode = CLI;

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
	
	size_t res = fread(&reg[REG_PC], sizeof(uint16_t), 1, image);
	assert(res > 0);
	reg[REG_PC] = ToLittleEndian(reg[REG_PC]);

	size_t maxRead = MEM_ADDRESSES_NUM - reg[REG_PC];
	uint16_t *ptr = memory + reg[REG_PC];
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
		reg[REG_PC] = reg[REG_PC] + SEXT(pcoffset9, PCOFFSET9);
	}
}

void JUMP(uint16_t instr){
	uint8_t BaseR = instr >> 6 & 0x7;
	reg[REG_PC] = BaseR;
}

/* ----------- Curses ----------- */
int terminalColumns;
int terminalRows;
WINDOW *mainWindow;
WINDOW *registerWindow;
WINDOW *infoWindow;
WINDOW *outputWindow;
WINDOW *inputWindow;
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

#define INSTRUCTION_TEXT_LEN 16
void disassemble(char dest[], uint16_t instruction){

}

void DrawMainWindow(){
	int cols = 0;
	int rows = 0;
	getmaxyx(mainWindow, rows ,cols);
	werase(mainWindow);
	box(mainWindow,0,0);
	
	if(machineStatus == RUNNING){
		mvwprintw(mainWindow, rows-rows, cols/2-3, "RUNNING");
	}else if(machineStatus == HALTED){
		mvwprintw(mainWindow, rows-rows, cols/2-3, "HALTED");
	}else mvwprintw(mainWindow, rows-rows, cols/2-3, "PAUSED");
	
	mvwprintw(mainWindow, 1, 1, "HEX");
	mvwprintw(mainWindow, 1, 10, "ADR");
	for(int i = 0; i < rows-3; ++i){
		mvwprintw(mainWindow, i+2, 1, "0x%04X", memory[reg[REG_PC] + i]);
		mvwprintw(mainWindow, i+2, 10, "0x%04X", reg[REG_PC] + i);
		char instr[INSTRUCTION_TEXT_LEN] = {0};
		disassemble(instr, memory[reg[REG_PC] + i]);
	}

	wrefresh(mainWindow);
}

void DrawRegisterWindow(){
	int cols = 0;
	getmaxyx(registerWindow, cols,cols);
	mvwprintw(registerWindow, 0, cols/2-4, "Registers");

	int x = 1, y = 1;
	wmove(registerWindow, y,x);
	for (int i = 0; i < REG_COUNT; ++i){
		char *val = "REG%d: 0x%04X ";
		char *cond  = "COND: 0x%04X ";
		char *pc = "PC  : 0x%04X ";
		if(i == 8) wprintw(registerWindow, cond,reg[i]);
		else if(i == 9) wprintw(registerWindow, pc,reg[i]);
		else wprintw(registerWindow, val,i,reg[i]);
		getyx(registerWindow,y,x);
		if((int)(x + strlen(val))  > cols){
			x = 1;
			y = y+1;
			wmove(registerWindow, y,x);
		}

	}

	wrefresh(registerWindow);
}

void CreateAllWindows(){
	mainWindow = CreateNewWindow(terminalRows/1.3,terminalColumns/2,0,0);
	registerWindow = CreateNewWindow(terminalRows/3,terminalColumns/2,0,terminalColumns/2);
	outputWindow = CreateNewWindow(terminalRows - terminalRows/1.3,terminalColumns/2,terminalRows/1.3,0);
	inputWindow = CreateNewWindow(terminalRows - terminalRows/1.3,terminalColumns/2,terminalRows/1.3,terminalColumns/2);
	infoWindow = CreateNewWindow((terminalRows/1.3)-terminalRows/3,terminalColumns/2,terminalRows/3,terminalColumns/2);

	DrawRegisterWindow();
	DrawMainWindow();
}

int main(int argc, char **argv){
	if(argc < 2){
help:
		fprintf(stderr, "Usage: ./lc3 OPTIONS[-dh] image.obj \n");
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
	
	if(currentMode == CLI){
		signal(SIGINT, HandleTerminalInterrupt);
		SetNewTerminalMode();
	}else if(currentMode == DEBUGGER){
		machineStatus = PAUSED;
		InitCurses();
		CreateAllWindows();
		getch();
		endwin();
	}else{
		/* TODO: EMBEDDED */
	}
	
	while(machineStatus == RUNNING){
		uint16_t opcode = memory[reg[REG_PC]] >> 12;
		switch(opcode){
			case OP_ADD: ADD_AND(memory[reg[REG_PC]]++);  break;
			case OP_AND: ADD_AND(memory[reg[REG_PC]]++);  break;
			case OP_BR:  BR(memory[reg[REG_PC]]++);       break;
			case OP_JMP: JUMP(memory[reg[REG_PC]]++);     break;
			case OP_JSR: break;
			case OP_LD: break;
			case OP_LDI: break;
			case OP_LEA: break;
			case OP_NOT: break;
			case OP_RTI: break;
			case OP_STI: break;
			case OP_STR: break;
			case OP_TRAP: break;
			default: reg[REG_PC]++; break;
		}
	}
	
	if(currentMode == CLI) SetOldterminalMode();

	return 0;
}
















