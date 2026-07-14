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
#include "os.h"

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

enum {
	FL_POS = 1 << 0, /* FLAG POSITIVE */
	FL_ZRO = 1 << 1, /* FLAG ZERO     */
	FL_NEG = 1 << 2  /* FLAG NEGATIVE */
};

typedef enum{
	RUNNING = 0,
	PAUSED,
	HALTED
}status;

uint16_t memory[MEM_ADDRESSES_NUM];
uint16_t reg[REG_COUNT];
struct termios oldTerminalMode;
status machineStatus = RUNNING;
size_t pc;

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

bool LoadProgram(const char *path){
	FILE *image = fopen(path, "rb");
	if(image == NULL){
		fprintf(stderr, "Couldn't open file: %s\n", strerror(errno));
		return false;
	}
	
	return true;
}

int main(int argc, char **argv){
	if(argc < 2){
		fprintf(stderr, "Usage: lc3 image.obj \n");
		return 1;
	}

	LoadOS();
	if(!LoadProgram(argv[1])){
		fprintf(stderr, "Couldn't load program \"%s\" \n", argv[1]);
		return 1;
	}
	return 0;

	signal(SIGINT, HandleTerminalInterrupt);
	SetNewTerminalMode();

	while(machineStatus == RUNNING){
		break;	
	}
	
	SetOldterminalMode();

	return 0;
}
















