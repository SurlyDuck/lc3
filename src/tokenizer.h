#ifndef _TOKENIZER_
#define _TOKENIZER_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TOKEN_MAX_SIZE 256
#define MAX_LINES 1000000l

typedef enum{
	START_OPCODES = 0,
	OP_ADD,
	OP_AND,
	OP_BR,
	OP_JMP,
	OP_JSR,
	OP_JSRR,
	OP_LD,
	OP_LDI,
	OP_LDR,
	OP_LEA,
	OP_NOT,
	OP_RET,
	OP_RTI,
	OP_ST,
	OP_STI,
	OP_STR,
	OP_TRAP,
	END_OPCODES,
		
	START_REGISTERS,
	REG0,
	REG1,
	REG2,
	REG3,
	REG4,
	REG5,
	REG6,
	REG7,
	END_REGISTERS,

	START_TRAP_ROUTINES,
	GETC,
	OUT,
	PUTS,
	IN,
	PUTSP,
	HALT,
	END_TRAP_ROUTINES,
	
	START_PSEUDO_OPS,
	ORIG,
	END,
	BLKW,
	FILL,
	STRINGZ,
	END_PSEUDO_OPS,

	LABEL,
	IMMEDIATE,
	STRING,
	INVALID /* most of the times will just be a label also */
}token_id;

typedef enum{
	KIND_LABEL = 0,
	KIND_OPCODE,
	KIND_REGISTER,
	KIND_IMMEDIATE,
	KIND_PSEUDO_OP,
	KIND_TRAP,
	KIND_COMMENT,
	KIND_STRING,
	KIND_INVALID
}token_kind;

typedef struct{
	size_t line;
	char *text;
	token_kind kind;
}token;

extern char *tokenStrings[];

extern token** GetTokens(char *raw, size_t rawSize);

#endif
