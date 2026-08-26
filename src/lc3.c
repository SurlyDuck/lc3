#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <assert.h>
#include <ncurses.h>
#include "os.h"

#define IMM5               5
#define PCOFFSET6          6
#define PCOFFSET9          9 
#define PCOFFSET11        11
#define GETC            0x20
#define OUT             0x21
#define PUTS            0x22
#define IN              0x23
#define PUTSP           0x24
#define HALT            0x25
#define MEM_ADDRESSES_NUM (1<<16)

#define OS_KBSR 0xFE00     // keyboard status register
#define OS_KBDR 0xFE02     // keyboard data register
#define OS_DSR  0xFE04     // display status register
#define OS_DDR  0xFE06     // display data register
#define OS_MCR  0xFFFE     // machine control register

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

// Globals
static uint16_t memory[MEM_ADDRESSES_NUM];
static uint16_t reg[REG_COUNT];
static struct termios oldTerminalMode;
static status machineStatus = RUNNING;
static mode currentMode = CLI;

// Foward declarations in here
void AddCharacterToOutput(char value);

void SetOldterminalMode(){
	tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminalMode); 
}

void HandleTerminalInterrupt(){
	printf("terminal interrupted\n");
	SetOldterminalMode();
	exit(0);
}

void SetNewTerminalMode(){
	// disable key buffering and echoing
	tcgetattr(STDIN_FILENO, &oldTerminalMode);
	struct termios newTerminalMode = oldTerminalMode;
	newTerminalMode.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalMode); 
}

struct pollfd fds[1] = {
	{.fd = STDIN_FILENO, .events = POLLIN}
};
bool IsKeyPressed(){
	int timeout = 10; //TODO: poll blocks the thread and this is slowing down the machine
	int ret = poll(fds,1,timeout);

	if(ret > 0) return true;

	return false;
}

//------------------------------------------------------------------------------------
// Debugger and memory operations
//------------------------------------------------------------------------------------

// Command history buffer used by the input window
// This is different from input buffer, that loads user characters into 
// the keyboard data register (xFE02) when theres a reading operation
static char buff[120] = {0};
static char **buffHistory = NULL;
static size_t buffHistoryPtr = 0;

static uint16_t bnum = 0;
bool breakpoints[MEM_ADDRESSES_NUM] = {0};
bool onBreakpoint = false;

void AddBreakPoint(uint16_t adr){
	breakpoints[adr] = true;
	bnum++;
}

bool RemoveBreakPoint(){
	return true;
}

bool IsOnBreakPoint(uint16_t adr){
	return breakpoints[adr];
}

uint16_t *GetBreakPoints(){
	return NULL;
}

void LoadOS(){
	uint16_t memPtr = 0;
	for(unsigned int i = 3; i < __bin_os_obj_len-2; i+=2){
		uint16_t msb   = (uint16_t)(__bin_os_obj[i-1]);
		uint16_t lsb   = (uint16_t)(__bin_os_obj[i]);
		memory[memPtr] = (uint16_t)((msb << 8) | lsb);
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

#define MAX_INPUT_BUFFER_LENGTH 1024
static uint16_t inputBuffer[MAX_INPUT_BUFFER_LENGTH] = {0};
static uint16_t inputBufferPtr = 0;

void AddToInputBuffer(uint16_t value){
	if(inputBufferPtr+1 >= MAX_INPUT_BUFFER_LENGTH) return;
	inputBuffer[inputBufferPtr++] = value;
}

uint16_t GetInputBufferLength(){
	return inputBufferPtr;
}

uint16_t GetFromInputBuffer(){
	if(inputBufferPtr == 0) return 0;
	return inputBuffer[--inputBufferPtr];
}

void ClearInputBuffer(){
	for(size_t i = 0; i < buffHistoryPtr; ++i){
		// Free command strings
		if(buffHistory[i] != NULL) free(buffHistory[i]);
	}
	free(buffHistory);
	buffHistory = NULL;
	buffHistoryPtr = 0;
}

uint16_t GetFromMemory(uint16_t adr){
	assert(memory[OS_MCR] >> 15);
	if(!(memory[OS_DSR] >> 15)) memory[OS_DSR] = 0xFFFF;
	if(adr == OS_KBSR){
		if(currentMode != DEBUGGER){
			if(IsKeyPressed()) memory[OS_KBSR] = 0xFFFF;
			else memory[OS_KBSR] = 0x0000;
		}else{
			if(GetInputBufferLength() != 0) memory[OS_KBSR] = 0xFFFF;
			else memory[OS_KBSR] = 0x0000;
		}
	}else if(adr == OS_KBDR && (memory[OS_KBSR] >> 15)){
		if (currentMode != DEBUGGER)
			memory[OS_KBDR] = getchar();

		else 
			memory[OS_KBDR] = GetFromInputBuffer();
	}


	return memory[adr];
}

void UpdateToMemory(uint16_t adr, uint16_t value){
	if(adr == OS_DDR && memory[OS_DSR]){
		memory[OS_DSR] = 0x0000;
		if(currentMode != DEBUGGER) {
			putchar(value);
			fflush(stdout);
		}
		else AddCharacterToOutput(value);
	}else if(adr == OS_MCR && !(value >> 15)){
		machineStatus = HALTED;
	}

	memory[adr] = value;
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

//------------------------------------------------------------------------------------
// Instructions
//------------------------------------------------------------------------------------
void ADD_AND(uint16_t instr){
	uint8_t DR  = instr >> 9 & 0x7;
	uint8_t SR1 = instr >> 6 & 0x7;
	
	if(instr >> 5 & 1){
		uint16_t imm5 = SEXT(instr & 0x1F, IMM5);
		if(instr >> 12 == OP_ADD)reg[DR] = reg[SR1] + imm5;
		else reg[DR] = reg[SR1] & imm5;
	}else{
		uint8_t SR2 = instr & 0x3;
		if(instr >> 12 == OP_ADD) reg[DR] = reg[SR1] + reg[SR2];
		else reg[DR] = reg[SR1] & reg[SR2];
	}
	
	setcc(DR);
}

void LDR(uint16_t instr){
	uint8_t DR    = instr >> 9 & 0x7;
	uint8_t BASER = instr >> 6 & 0x7;
	uint16_t pcoffset6 = instr & 0x3F;
	reg[DR] = GetFromMemory(reg[BASER] + SEXT(pcoffset6, PCOFFSET6));
	
	setcc(DR);
}

void STR(uint16_t instr){
	uint8_t SR    = instr >> 9 & 0x3;
	uint8_t BASER = instr >> 6 & 0x3;
	uint16_t pcoffset6 = instr & 0x3F;
	
	UpdateToMemory(reg[BASER] + SEXT(pcoffset6, PCOFFSET6), reg[SR]);
}

void BR(uint16_t instr){
	if(((instr >> 9) & 0x7) & reg[REG_COND]){
		uint16_t pcoffset9 = instr & 0x1FF;
		reg[REG_PC] = reg[REG_PC] + SEXT(pcoffset9, PCOFFSET9);
	}
}

void JUMP(uint16_t instr){
	uint8_t BaseR = instr >> 6 & 0x7;
	reg[REG_PC] = reg[BaseR];
}

void JSR(uint16_t instr){
	reg[REG7] = reg[REG_PC];
	if(instr >> 11 & 1){
		uint16_t pcoffset11 = instr & 0x03FF;
		reg[REG_PC] = reg[REG_PC] + SEXT(pcoffset11, PCOFFSET11);
	}else{
		uint8_t baseR = instr >> 6 & 0x7;
		reg[REG_PC] = reg[baseR];
	}
}

void LD(uint16_t instr){
	uint8_t DR = instr >> 9 & 0x7;
	uint16_t pcoffset9 = instr & 0x01FF;
	reg[DR] = GetFromMemory(reg[REG_PC] + SEXT(pcoffset9, PCOFFSET9));
	setcc(DR);
}

void LDI(uint16_t instr){
	uint8_t DR = instr >> 9 & 0x7;
	uint16_t pcoffset9 = instr & 0x01FF;
	reg[DR] = GetFromMemory(GetFromMemory(reg[REG_PC] + SEXT(pcoffset9, PCOFFSET9)));
	setcc(DR);
}

void LEA(uint16_t instr){
	uint8_t DR = instr >> 9 & 0x7;
	uint16_t pcoffset9 = instr & 0x01FF;
	reg[DR] = reg[REG_PC] + SEXT(pcoffset9, PCOFFSET9);
	setcc(DR);
}

void NOT(uint16_t instr){
	uint8_t DR = instr >> 9 & 0x7;
	uint8_t SR = instr >> 6 & 0x7;

	reg[DR] = ~reg[SR];
	setcc(DR);
}

void RTI(){
	/* TODO */
	return;
}

void ST(uint16_t instr){
	uint8_t SR = instr >> 9 & 0x7;
	uint16_t pcoffset9 = instr & 0x01FF;

	UpdateToMemory(reg[REG_PC] + SEXT(pcoffset9, PCOFFSET9), reg[SR]);
}

void STI(uint16_t instr){
	uint8_t SR = instr >> 9 & 0x7;
	uint16_t pcoffset9 = instr & 0x01FF;
	
	UpdateToMemory(GetFromMemory(reg[REG_PC] + SEXT(pcoffset9, PCOFFSET9)), reg[SR]);
}

void TRAP(uint16_t instr){
	reg[REG7] = reg[REG_PC];
	uint16_t trapvect8 = instr & 0xFF;
	
	reg[REG_PC] = GetFromMemory(trapvect8);
}

//------------------------------------------------------------------------------------
// Disassembler
// TODO: too many repetitions in here
//------------------------------------------------------------------------------------
#define INSTRUCTION_TEXT_LEN 32
const char *GetRegisterText(uint16_t reg){
	switch(reg){
		case 0: return "R0 ";
		case 1: return "R1 ";
		case 2: return "R2 ";
		case 3: return "R3 ";
		case 4: return "R4 ";
		case 5: return "R5 ";
		case 6: return "R6 ";
		case 7: return "R7 ";
	}
	
	return "NR";
}

#define SPACE strcat(dest, " ")
void disassemble(char dest[], uint16_t instruction, uint16_t pc){
	uint16_t opcode = instruction >> 12;
	char buffer[256] = {0};
	switch(opcode){
		case OP_ADD: {
			strcat(dest, "ADD ");
			strcat(dest,GetRegisterText(instruction >> 9 & 0x7));
			strcat(dest,GetRegisterText(instruction >> 6 & 0x7));
			if(instruction >> 5 & 1){
				int16_t num = instruction & 0x1F;
				if(instruction & 0x10) num |= 0xFFE0;
				sprintf(buffer, "x%04X", (uint16_t) num);
				strcat(dest, buffer);
				sprintf(buffer, " (#%d)",num);
				strcat(dest, buffer);
			}else{
				strcat(dest,GetRegisterText(instruction & 0x7));
			}
			break;
		}case OP_AND: {
			strcat(dest, "AND ");
			strcat(dest,GetRegisterText(instruction >> 9 & 0x7));
			strcat(dest,GetRegisterText(instruction >> 6 & 0x7));
			if(instruction >> 5 & 1){
				int16_t num = instruction & 0x1F;
				if(instruction & 0x10) num |= 0xFFE0;
				sprintf(buffer, "x%04X", (uint16_t) num);
				strcat(dest, buffer);
				sprintf(buffer, " (#%d)",num);
				strcat(dest, buffer);
			}else{
				strcat(dest,GetRegisterText(instruction & 0x7));
			}
			break;
		}case OP_BR:{
			strcat(dest, "BR");
			if(instruction >> 11 & 1) strcat(dest, "n");
			if(instruction >> 10 & 1) strcat(dest, "z");
			if(instruction >> 9 & 1) strcat(dest, "p");
			
			int16_t adr = SEXT(instruction & 0x01FF, PCOFFSET9) + pc + 1;
				sprintf(buffer, "x%04X", adr);
			SPACE;
			strcat(dest, buffer);
			break;
		}case OP_JMP: {
			strcat(dest, "JMP ");
			strcat(dest, GetRegisterText(instruction >> 6 & 0x7));
			break;
		}case OP_JSR: {
			if(instruction >> 11 & 1){
				strcat(dest, "JSR ");
				int16_t adr = SEXT(instruction & 0x07FF, PCOFFSET11) + pc + 1;
				sprintf(buffer, "x%04X", adr);
				strcat(dest, buffer);
			}else{
				strcat(dest, "JSRR ");
				strcat(dest, GetRegisterText(instruction >> 6 & 0x7));
			}		
			break;
		}case OP_LD:{
			strcat(dest, "LD ");
			strcat(dest, GetRegisterText(instruction >> 9 & 0x7));
			int16_t adr = SEXT(instruction & 0x01FF, PCOFFSET9) + pc + 1;
			sprintf(buffer, "x%04X", adr);
			strcat(dest, buffer);
			break;
		}case OP_LDR:{
			strcat(dest, "LDR ");
			strcat(dest, GetRegisterText(instruction >> 9 & 0x7));
			strcat(dest, GetRegisterText(instruction >> 6 & 0x7));
			int16_t adr = SEXT(instruction & 0x3F, PCOFFSET6);
			sprintf(buffer, "x%04X", adr);
			strcat(dest, buffer);
			break;
		}case OP_LDI: {
			strcat(dest, "LDI ");
			strcat(dest, GetRegisterText(instruction >> 9 & 0x7));
			int16_t adr = SEXT(instruction & 0x01FF, PCOFFSET9) + pc + 1;
			sprintf(buffer, "x%04X", adr);
			strcat(dest, buffer);
			break;
		}case OP_LEA:{
			strcat(dest, "LEA ");
			strcat(dest, GetRegisterText(instruction >> 9 & 0x7));
			int16_t adr = SEXT(instruction & 0x01FF, PCOFFSET9) + pc + 1;
			sprintf(buffer, "x%04X", adr);
			strcat(dest, buffer);
			break;
		}case OP_NOT:{
			strcat(dest, "NOT ");
			strcat(dest, GetRegisterText(instruction >> 9 & 0x7));
			strcat(dest, GetRegisterText(instruction >> 6 & 0x7));
			break;
		}case OP_RTI:{
			strcat(dest, "RTI");
			break;
		}case OP_STI:{
			strcat(dest, "STI ");
			strcat(dest, GetRegisterText(instruction >> 9 & 0x7));
			uint16_t adr = memory[(uint16_t)(SEXT(instruction & 0x01FF, PCOFFSET9) + pc + 1)];
			sprintf(buffer, "x%04X", adr);
			strcat(dest, buffer);
			break;
		}
		case OP_STR:{
			strcat(dest, "STR ");
			strcat(dest, GetRegisterText(instruction >> 9 & 0x7));
			strcat(dest, GetRegisterText(instruction >> 6 & 0x7));
			int16_t adr = SEXT(instruction & 0x003F, PCOFFSET6);
			sprintf(buffer, "x%04X", adr);
			strcat(dest, buffer);
			break;
		}
		case OP_ST:{
			strcat(dest, "ST ");
			strcat(dest, GetRegisterText(instruction >> 9 & 0x7));
			strcat(dest, GetRegisterText(instruction >> 6 & 0x7));
			int16_t adr = SEXT(instruction & 0x003F, PCOFFSET6);
			sprintf(buffer, "x%04X", adr);
			strcat(dest, buffer);
			break;
		}
		case OP_TRAP:{
			if((instruction & 0x00FF) == GETC) strcat(dest,  "GETC");
			else if((instruction & 0x00FF) == OUT) strcat(dest,   "OUT");
			else if((instruction & 0x00FF) == PUTS) strcat(dest,  "PUTS");
			else if((instruction & 0x00FF) == IN) strcat(dest,    "IN");
			else if((instruction & 0x00FF) == PUTSP) strcat(dest, "PUTSP");
			else if((instruction & 0x00FF) == HALT) strcat(dest,  "HALT");
			else strcat(dest,  ".FILL");
			
			break;
		} 
		default: strcat(dest,".FILL"); break; 
	}
}

//------------------------------------------------------------------------------------
// Curses/debugger
//------------------------------------------------------------------------------------
int terminalColumns;
int terminalRows;
WINDOW *mainWindow;
WINDOW *registerWindow;
WINDOW *infoWindow;
WINDOW *outputWindow;
WINDOW *inputWindow;
void InitCurses(){
	initscr();
	noraw();
	echo();

	getmaxyx(stdscr,terminalRows, terminalColumns);
	assert(terminalRows > 20 && terminalColumns > 100 && "Terminal too small");
	refresh();
}

WINDOW *CreateNewWindow(int rows, int cols, int y, int x){
	WINDOW *win = newwin(rows,cols,y,x);
	box(win, 0,0);
	wrefresh(win);

	return win;
}

// TODO: add this to a command called `info` or `help` and increase the input window size.
void DrawInfoWindow(){
	int cols = 0;
	getmaxyx(infoWindow, cols, cols);
	mvwprintw(infoWindow, 0, cols /2 - 2, "INFO");
	mvwprintw(infoWindow, 1, 1, "`q` --> exit debugger");
	mvwprintw(infoWindow, 2, 1, "`n` --> next instruction");
	mvwprintw(infoWindow, 3, 1, "`r` --> run program until breakpoint");
	mvwprintw(infoWindow, 4, 1, "`b` x0000-xFFFF --> add breakpoint to address");
	mvwprintw(infoWindow, 5, 1, "`rb` x0000-xFFFF --> remove breakpoint");
	mvwprintw(infoWindow, 6, 1, "`lb` --> list breakpoints");
	mvwprintw(infoWindow, 7, 1, "`in char` --> add char to the input buffer (LIFO)");
	mvwprintw(infoWindow, 8, 1, "`c` --> clear command window");
	wrefresh(infoWindow);
}

// TODO: this is pretty bad
const char *DrawInputWindow(){
	size_t rows, cols;
	rows = cols = 0;
	getmaxyx(inputWindow, rows, cols);
	memset(buff, '\0', 120);
	
	werase(inputWindow);
	box(inputWindow, 0, 0);
	wmove(inputWindow, 1, 1);

	size_t  i = 0;
	uint8_t y = 0;
	if(buffHistory != NULL){
		if(buffHistoryPtr > rows-3){
			i = buffHistoryPtr - (rows - 3);
		}

		for(; i < buffHistoryPtr; ++i){
			mvwprintw(inputWindow, 1+y, 1, buffHistory[i]);
			y++;
		}
		waddch(inputWindow, '\n');
	}
	
	box(inputWindow, 0, 0);
	wmove(inputWindow, y+1, 1);
	waddstr(inputWindow, ":>");
	wgetstr(inputWindow, buff);
	if(strcmp(buff, "") == 0) return "";
	
	buffHistoryPtr++;

	buffHistory = realloc(buffHistory, sizeof(void*) * buffHistoryPtr);
	buffHistory[buffHistoryPtr-1] = (char *) malloc(sizeof(buff));
	strcpy(buffHistory[buffHistoryPtr-1], buff);

	return buff;
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
	
	mvwprintw(mainWindow, 1, 10, "HEX");
	mvwprintw(mainWindow, 1, 1, "ADR");
	mvwprintw(mainWindow, 1, 19, "INSTR");
	char instr[INSTRUCTION_TEXT_LEN] = {0};
	for(int i = 0; i < rows-3; ++i){
		if(!i) wattron(mainWindow, A_STANDOUT);
		mvwprintw(mainWindow, i+2, 1, "x%04X", reg[REG_PC] + i);
		mvwprintw(mainWindow, i+2, 10, "x%04X", memory[reg[REG_PC] + i]);
		disassemble(instr, memory[reg[REG_PC] + i], reg[REG_PC] + i);
		mvwprintw(mainWindow, i+2, 19, "%s",instr);
		memset(instr, '\0', INSTRUCTION_TEXT_LEN);
		if(!i) wattroff(mainWindow, A_STANDOUT);
	}

	wrefresh(mainWindow);
}

void DrawRegisterWindow(){
	int cols = 0;
	werase(registerWindow);	
	box(registerWindow, 0, 0);
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

#define OUTPUT_BUFFER_LENGTH 1024
char outputBuffer[OUTPUT_BUFFER_LENGTH] = {0};
int outputPtr = 0;
void AddCharacterToOutput(char c){
	outputBuffer[outputPtr++] = c;

	if(outputPtr >= OUTPUT_BUFFER_LENGTH)
		outputPtr = OUTPUT_BUFFER_LENGTH-1;
}

void DrawOutputWindow(){
	int rows, columns, y, x, windowSize,start;
	getmaxyx(outputWindow, rows, columns);
	windowSize = (rows-2) * (columns-2);
	if(outputPtr > windowSize) start = outputPtr - windowSize;
	else start = 0;
	
	werase(outputWindow);
	box(outputWindow, 0, 0);
	mvwprintw(outputWindow, 0, columns/2-3, "Output");
	wmove(outputWindow, 1, 1);
	for(int i = start; i < outputPtr; ++i){
		getyx(outputWindow, y, x);
		if(outputBuffer[i] == '\n') wmove(outputWindow, y+1, 1);
		else{
			waddch(outputWindow, outputBuffer[i]);
			if(x >= columns-1) wmove(outputWindow, y+1, 1);
		}
		
	}

	wrefresh(outputWindow);
}

void PrintHelpMessage(WINDOW *win){
	// TODO PrintDebuggerCommands()
	int cols, rows;
	rows = cols = 0;
	getmaxyx(win, rows, cols);

	const char *help[] = {
		"h --> print this help message",
		"q --> exit debugger",
		"n --> next instruction",
		"r --> run program until breakpoint",
		"b x0000-xFFFF --> add breakpoint to an address",
		"rb x0000-xFFFF --> remove breakpoint from an address",
		"lb x0000-xFFFF --> list breakpoints",
		"/char --> add 'char' to the inputer buffer (LIFO)",
		"c --> clear input window",
		NULL
	};
	
	wmove(win, 1, 1);
	int x,y;
	for(int i = 0; help[i] != NULL; ++i){
		getyx(win, y, x);
		if(x == 0) wmove(win, y, 1);
		if(y > rows-3){ 
			wprintw(win, "MORE...");
			wgetch(win);
			werase(win);
			box(win, 0, 0);
			wmove(win, 1, 1);
		}

		wprintw(win, "%s\n", help[i]);
		box(win, 0, 0);
		wrefresh(win);
	}
	getyx(win, y, x);
	wmove(win, y,1);
	wprintw(win, "CONTINUE...");
	wgetch(win);
}

void CreateAllWindows(){
	mainWindow = CreateNewWindow(terminalRows/1.3,terminalColumns/2,0,0);
	registerWindow = CreateNewWindow(terminalRows/3,terminalColumns/2,0,terminalColumns/2);
	inputWindow = CreateNewWindow(terminalRows - terminalRows/1.3,terminalColumns/2,terminalRows/1.3,0);
	outputWindow = CreateNewWindow(terminalRows - terminalRows/3-1,terminalColumns/2,terminalRows/3,terminalColumns/2);
	//infoWindow = CreateNewWindow((terminalRows/1.3)-terminalRows/3,terminalColumns/2,terminalRows/3,terminalColumns/2);

	DrawRegisterWindow();
	DrawMainWindow();
	DrawOutputWindow();
	//DrawInfoWindow()
}


//------------------------------------------------------------------------------------
// Machine loop 
//------------------------------------------------------------------------------------

void NextInstruction(){
	uint16_t opcode = memory[reg[REG_PC]] >> 12;
	switch(opcode){
		case OP_ADD:   ADD_AND(memory[reg[REG_PC]++]);  break;
		case OP_AND:   ADD_AND(memory[reg[REG_PC]++]);  break;
		case OP_LDR:   LDR(memory[reg[REG_PC]++]);      break;
		case OP_STR:   STR(memory[reg[REG_PC]++]);      break;
		case OP_BR:    BR(memory[reg[REG_PC]++]);       break;
		case OP_JMP:   JUMP(memory[reg[REG_PC]++]);     break;
		case OP_JSR:   JSR(memory[reg[REG_PC]++]);      break;
		case OP_LD:    LD(memory[reg[REG_PC]++]);       break;
		case OP_LDI:   LDI(memory[reg[REG_PC]++]);      break;
		case OP_LEA:   LEA(memory[reg[REG_PC]++]);      break;
		case OP_NOT:   NOT(memory[reg[REG_PC]++]);      break;
		case OP_RTI:   RTI();                           break;
		case OP_ST:    ST(memory[reg[REG_PC]++]);       break;
		case OP_STI:   STI(memory[reg[REG_PC]++]);      break;
		case OP_TRAP:  TRAP(memory[reg[REG_PC]++]);     break;
		default: reg[REG_PC]++; break;
	}
}

void RestartMachine(){
	/* TODO: Restart from where the program originally started */
	reg[REG_PC] = 0x3000;
	/* TODO: Reset all memory mapped registers */
	memory[OS_MCR] = 0xF000;
	memory[OS_DDR] = 0x0000;
	memory[OS_DSR] = 0x0000;
	reg[REG0] = 0;
	reg[REG1] = 0;
	reg[REG2] = 0;
	reg[REG3] = 0;
	reg[REG4] = 0;
	reg[REG5] = 0;
	reg[REG6] = 0;
	reg[REG7] = 0;
	machineStatus = RUNNING;

	if(currentMode != DEBUGGER) return;
	machineStatus = PAUSED;
	werase(inputWindow);
	memset(outputBuffer, '\0', OUTPUT_BUFFER_LENGTH);
	outputPtr = 0;

	memset(breakpoints, 0, sizeof(breakpoints)/sizeof(breakpoints[1]));

	DrawRegisterWindow();
	DrawMainWindow();
	DrawOutputWindow();
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
	setcc(0); 

	if(currentMode == CLI){
		signal(SIGINT, HandleTerminalInterrupt);
		SetNewTerminalMode();
	}else if(currentMode == DEBUGGER){
		machineStatus = PAUSED;
		InitCurses();
		CreateAllWindows();
	}else{
		/* TODO: EMBEDDED */
	}
	
	const char *lastInst = NULL;
	memory[OS_MCR] = 0xF000; // Starts the machine
	// Debugger mode
	while(currentMode == DEBUGGER){
		const char *input = NULL;
		if(machineStatus == PAUSED){
			input = DrawInputWindow();
			if(strcmp(input, "quit") == 0 || strcmp(input, "q") == 0) {endwin(); return 0;}
			if(strcmp(input, "") == 0 && lastInst != NULL) input = lastInst;
		}else if(machineStatus == HALTED){
			strcpy(buffHistory[buffHistoryPtr-1], "Machine halted. Restart? (y/n): ");
			input = DrawInputWindow();

			if(strcmp(input, "n") == 0 || strcmp(input, "N") == 0) {
				endwin(); 
				return 0;
			}else{
				ClearInputBuffer();
				RestartMachine();
				continue;
			}
		}else if(machineStatus == RUNNING){
			if(IsOnBreakPoint(reg[REG_PC]) && !onBreakpoint){
				char msg[256] = {0};	
				sprintf(msg, "Breakpoint reached at <%04X>", reg[REG_PC]);
				strcpy(buffHistory[buffHistoryPtr-1], msg);
				machineStatus = PAUSED;
				onBreakpoint = true;
			}else{
				NextInstruction();
				onBreakpoint = false;
			}
			DrawRegisterWindow();
			DrawMainWindow();
			DrawOutputWindow();
			usleep(500);
			continue;
		}

		if(input[0] == 'n') { // Next
			lastInst = "n";
			NextInstruction();
		}else if(input[0] == 'b'){ // Breakpoints
			char temp[50] = {0};
			bool foundDigit = false;
			int startNum = 1;
			for(int i = 1; input[i] != '\0'; ++i) 
				if(isxdigit(input[i]) && input[i] != ' ') {
					if(!foundDigit){ 
						startNum = i;
						foundDigit = true;
					}
				}else if(foundDigit){
					foundDigit = false;
					break;
				}
			if (!foundDigit){ // no hex number found
				strcat(buffHistory[buffHistoryPtr-1], " --> Invalid Address");
			}else{
				memcpy(temp, &input[startNum], 50);
				long long address = strtoll(temp, NULL, 16);
				if(address > 1<<16){
					strcat(buffHistory[buffHistoryPtr-1], " --> Invalid Address");
				}else{
					AddBreakPoint(address);
					char result[128] = {0};	
					sprintf(result, " --> Breakpoint added to %lld: ", address);
					strcat(buffHistory[buffHistoryPtr-1], result);
				}
			}

			lastInst = NULL;
		}else if(input[0] == '/'){  // Add char to input buffer
			for(int i = 1; input[i] != '\0'; ++i){
				if(input[i] == '\\'){
					// Add escape code
					switch(input[i+1]){
						case 'n': AddToInputBuffer('\n'); break;
						case 't': AddToInputBuffer('\t'); break;
						default: i++; continue;
					}
					i+= 2;
					continue;
				}
				AddToInputBuffer(input[i]);
				lastInst = "";
			}
			lastInst = NULL;
		}else if(input[0] == 'c'){ // Clear
			ClearInputBuffer();
			lastInst = "c";
		}else if(input[0] == 'r'){ // Run
			machineStatus = RUNNING;
			lastInst = "r";
		}else if(input[0] == 'h'){ // Help
			PrintHelpMessage(inputWindow);		
		}else{
			if(strcmp(input, "") != 0)
				strcat(buffHistory[buffHistoryPtr-1], " < Invalid command");
		}

		DrawRegisterWindow();
		DrawMainWindow();
		DrawOutputWindow();

	}
	
	// CLI mode
	while(machineStatus == RUNNING || machineStatus == HALTED){
		NextInstruction();
		
		if(machineStatus == HALTED) {
			printf("\nThe machine has been halted, restart it? (y/n): ");
			int ans = getchar();
			if(ans == 'n' || ans == 'N') {
				putchar('\n');
				break;
			}else {
				putchar('\n');
				RestartMachine();
				continue;
			}
		}
	}
	
	if(currentMode == CLI) SetOldterminalMode();

	return 0;
}



