#include "tokenizer.h"

typedef enum{
	SEARCHING = 0,
	IGNORING, 
	READING
}token_status;

char *tokenStrings[] =  {
	"",		
	"ADD",	/* OP_ADD, */
	"AND",	/* OP_AND, */
	"BR",		/* OP_BR, */
	"BRN",
	"BRZ",
	"BRP",
	"JMP",	/* OP_JMP, */
	"JSR",	/* OP_JSR, */
	"JSRR",	/* OP_JSRR, */
	"LD",		/* OP_LD, */
	"LDI",	/* OP_LDI, */
	"LDR",	/* OP_LDR, */
	"LEA",	/* OP_LEA, */
	"NOT",	/* OP_NOT, */
	"RET",	/* OP_RET, */
	"RTI",	/* OP_RTI, */
	"ST",		/* OP_ST, */
	"STI",	/* OP_STI, */
	"SRT",	/* OP_STR, */
	"TRAP",	/* OP_TRAP, */
	"",		/* END_OPCODES, */
	"",     
	"R0",	/* REG0,*/
	"R1",	/* REG1,*/
	"R2",	/* REG2,*/
	"R3",	/* REG3,*/
	"R4",	/* REG4,*/
	"R5",	/* REG5,*/
	"R6",	/* REG6,*/
	"R7",	/* REG7,*/
	"",      /* END REGISTERS */
	"",
	"GETC",	/* GETC,*/
	"OUT",	/* OUT,*/
	"PUTS",	/* PUTS,*/
	"IN",		/* IN,*/
	"PUTSP",	/* PUTSP,*/
	"HALT",	/* HALT,*/
	"",		/* END_TRAP_ROUTINES, */
	"", 
	".ORIG",           /* ORIG,*/
	".END",            /* END,*/
	".BLKW",           /* BLKW,*/
	".FILL",           /* FILL,*/
	".STRINGZ",        /* STRINGZ,*/
	"",                /* END PSEUDO OPCODES */
	"LABEL",
	"#0",      /* IMMEDIATE */
	"string", 
	"invalid"
};

static tokens tokensArray = {0};

static void Uppercase(char *text, size_t size){
	for(size_t i = 0; i < size; ++i){
		if(text[i] > 'Z' && !isdigit(text[i])) text[i] -= 'a' - 'A';
	}
}

tokens* InitTokenizer(char *raw, size_t rawSize){
	long long cursor = -1;
	token_status currentStatus = SEARCHING;
	token currentToken = {0};
	size_t currentLine = 0;
	int currentTokenCursor = 0;
	int isString = 0;
	char currentTokenText[TOKEN_MAX_SIZE] = {0};
	while((rawSize - cursor) > 0){
		if(currentTokenCursor >= TOKEN_MAX_SIZE){
			fprintf(stderr, "[ERROR]: at line %lu, token '%s' is too big", currentLine,currentTokenText);
			return NULL;
		}
	
		++cursor;
		char b = raw[cursor];
		if(b == '\n') ++currentLine;
		if(b == ';') currentStatus = IGNORING;

		if((currentStatus == IGNORING && b != '\n') || b == ',') {
			raw[cursor] = ' ';
			continue;
		}
		else if(currentStatus == IGNORING) currentStatus = SEARCHING;

		if(currentStatus == SEARCHING && (b != ' ' && b != '\t' && b != '\n')){
			if(b == '"') isString = 1; 
			else currentTokenText[currentTokenCursor] = b; 
			currentToken = (token) {currentLine, currentTokenText, KIND_INVALID};
			currentStatus = READING;
			continue;
		}
		else if(currentStatus != READING) continue;	
		
		if(currentStatus == READING && ((b != ' ' && b != '\t' && b != '"' && b != '\n') || (isString && b != '"'))){
			currentTokenCursor++;
			currentToken.text[currentTokenCursor] = b;
		}else if(currentStatus == READING && ((b == ' ' || b == ',' || b == '\t' || b == '\n') || (isString && b == '"'))){
			/* get token kind, add to tokens array and keep searching */
			if(isString){
				/* need to remove double quotes from the left */
				for(int i = 0; i < currentTokenCursor; ++i){
					currentTokenText[i] = currentTokenText[i+1];
				}
				currentTokenText[currentTokenCursor] = '\0';
				currentToken.kind = KIND_STRING;
			}else if(currentTokenText[0] == '#' || currentTokenText[0] == 'x' || currentTokenText[0] == 'X'){
				currentToken.kind = KIND_IMMEDIATE;
				for(int i = 1; i < currentTokenCursor+1; ++i){
					if(currentTokenText[i] == '-' || currentTokenText[i] == '+') continue;
					if(!isalnum(currentTokenText[i]) || currentTokenText[i] == '_'){
						currentToken.kind = KIND_INVALID;
						break;
					}
				}
			}else if (isdigit(currentTokenText[0]) && currentTokenText[0] >= '0' && currentTokenText[0] <= '9'){
				currentToken.kind = KIND_IMMEDIATE;
			}else{
				Uppercase(currentTokenText, currentTokenCursor + 1);
				for(int id = 0; id < TOKENS_COUNT; ++id){
					if(strcmp(currentTokenText, tokenStrings[id]) == 0){
						if(id > START_OPCODES && id < END_OPCODES){
							currentToken.kind = KIND_OPCODE;
							break;
						}else if(id > START_REGISTERS && id < END_REGISTERS){
							currentToken.kind = KIND_REGISTER;
							break;
						}else if(id > START_TRAP_ROUTINES && id < END_TRAP_ROUTINES){
							currentToken.kind = KIND_TRAP;
							break;
						}else if(id > START_PSEUDO_OPS && id < END_PSEUDO_OPS){
							currentToken.kind = KIND_PSEUDO_OP;
							break;
						}else{
							continue;
						}
					}
				}
				if(currentToken.kind == KIND_INVALID) currentToken.kind = KIND_LABEL;

			}
			currentToken.text = (char*) malloc(sizeof(char) * currentTokenCursor + 1);
			for(int i = 0; i <= currentTokenCursor; ++i){
				currentToken.text[i] = currentTokenText[i];	
			}
			currentToken.text[currentTokenCursor + 1] = '\0';
			/*
			printf("Current Token at line %lu: %s",currentToken.line + 1, currentToken.text);
			printf(" - Kind: ");
			switch (currentToken.kind){
				case KIND_INVALID: printf("INVALID\n"); break;
				case KIND_STRING: printf("STRING\n"); break;
				case KIND_IMMEDIATE: printf("IMMEDIATE\n"); break;
				case KIND_OPCODE: printf("OPCODE\n"); break;
				case KIND_REGISTER: printf("REGISTER\n"); break;
				case KIND_TRAP: printf("TRAP\n"); break;
				case KIND_PSEUDO_OP: printf("PSEUDO-OP\n"); break;
				case KIND_LABEL: printf("LABEL\n"); break;
				break;
				default: break;
			}
			*/
			if(tokensArray.size == 0){
				tokensArray = (tokens) {
				.items = (token*) malloc(sizeof(token) * 1),
				.size   = 1,
				.capacity = sizeof(tokens),
				};
			}else{
				tokensArray.size += 1;
				if(tokensArray.capacity < sizeof(token) * tokensArray.size){
					tokensArray.capacity *= 2;
					tokensArray.items = (token*) realloc(tokensArray.items, tokensArray.capacity); 				
				}
			}

			tokensArray.items[tokensArray.size - 1] = currentToken;

			currentTokenCursor = 0;
			memset(&currentTokenText,'\0', 256);
			isString = 0;
			currentStatus = SEARCHING;
		}	
	}	
	return &tokensArray;
}


void ExitTokenizer(void){
	for(size_t i = 0; i < tokensArray.size; ++i){
		free(tokensArray.items[i].text);
	}
	free(tokensArray.items);

}

