#include <stdint.h>
#include <stdio.h>

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


uint16_t memory[MEM_ADDRESSES_NUM];
uint16_t reg[REG_COUNT];

int main(int argc, char **argv){
	
	if(argc < 2){
		fprintf(stderr, "Usage: saira image.hex \n");
		return 1;
	}
	
	//TODO: loading hex to memory


	return 0;
}
















