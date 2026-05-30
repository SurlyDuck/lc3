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
	REG_PC,
	REG_COUNT   
};


uint16_t memory[MEM_ADDRESSES_NUM];

int main(int argc, char **argv){
	
	return 0;
}
