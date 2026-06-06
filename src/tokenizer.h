#ifndef _TOKENIZER_
#define _TOKENIZER_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum{
	KIND_LABEL = 0,
	KIND_OPCODE,
	KIND_OPERAND,
	KIND_PSEUDO_OP,
	KIND_TRAP,
	KIND_COMMENT,
	KIND_INVALID
}token_kind;

typedef struct{
	size_t line;
	char *text;
	token_kind kind;
}token;

extern token** GetTokens(char *raw, size_t rawSize);

#endif
