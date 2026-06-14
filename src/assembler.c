#define ERROR_MESSAGE(msg) fprintf(stderr, "<Error> %s\n", msg)	
#define ERROR_MESSAGE_LONG(msg,line,val) fprintf(stderr, "<Error> %s at line %lu: <%s>\n",msg,line,val)	

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "tokenizer.h"

typedef struct {
	char *symbol;
	uint16_t address;
}symbol;

typedef struct{
	symbol *items;
	size_t size;
	size_t capacity;
}s_table;

tokens *allTokens = NULL;
s_table symbolTable = {0};
size_t rawSize = 0;
char *fileRaw;
uint16_t memory[1<<16];

uint8_t tokenCode[] =  {
	0x00,		
	0x01,		/* OP_ADD, */
	0x05,		/* OP_AND, */
	0x00,		/* OP_BR, */
	0x00,
	0x00,
	0x00,
	0x0C,		/* OP_JMP, */
	0x04,		/* OP_JSR, */
	0x04,		/* OP_JSRR, */
	0x02,		/* OP_LD, */
	0x0A,		/* OP_LDI, */
	0x06,		/* OP_LDR, */
	0x0E,		/* OP_LEA, */
	0x09,		/* OP_NOT, */
	0x0C,		/* OP_RET, */
	0x08,		/* OP_RTI, */
	0x03,		/* OP_ST, */
	0x0B,		/* OP_STI, */
	0x07,		/* OP_STR, */
	0x0F,	/* OP_TRAP, */
	0x00,		/* END_OPCODES, */
	0x00,     
	0x00,	/* REG0,*/
	0x01,	/* REG1,*/
	0x02,	/* REG2,*/
	0x03,	/* REG3,*/
	0x04,	/* REG4,*/
	0x05,	/* REG5,*/
	0x06,	/* REG6,*/
	0x07,	/* REG7,*/
	0x00, /* END REGISTERS */
	0x00,
	0x20,		/* GETC,*/
	0x21,		/* OUT,*/
	0x22,		/* PUTS,*/
	0x23,		/* IN,*/
	0x24,		/* PUTSP,*/
	0x25,		/* HALT,*/
	0x00,		/* END_TRAP_ROUTINES, */
	0x00, 
	0xFF,        /* ORIG,*/
	0xFF,        /* END,*/
	0xFF,        /* BLKW,*/
	0xFF,        /* FILL,*/
	0xFF,        /* STRINGZ,*/
	0x00,        /* END PSEUDO OPCODES */
	0x00,
	0x00,      	 /* IMMEDIATE */
	0x00, 
	0x00
};

int OpenFile(char *filePath);
bool ParseTokens(void);
bool FillSymbolTable(uint16_t entryPoint);
size_t StrToInt(char *str, int base);
bool GetMachineCode(token t, uint16_t *pc, uint16_t *bufrCode, size_t line);

int main(int argc, char **argv){
	if(argc < 2){
		ERROR_MESSAGE("\nUsage: assembler program.asm");
		return 1;
	}
	
	if(!OpenFile(argv[1])){
		ERROR_MESSAGE("Couldn't tokenize file.");
		return 1;
	}
	
	if((allTokens = InitTokenizer(fileRaw, rawSize)) == NULL){
		ERROR_MESSAGE("Error: Couldn't tokenize file.");
		return 1;
	}
	
	for(size_t i = 0; i < allTokens->size; ++i){
		printf("Current Token at line %lu: %s",allTokens->items[i].line + 1, allTokens->items[i].text);
		printf(" - Kind: ");
		switch (allTokens->items[i].kind){
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
	}

	if(allTokens->size == 0){
		ERROR_MESSAGE("No tokens to parse.");
		return 1;
	}

	if(!ParseTokens()){
		ERROR_MESSAGE("Couldn't parse.");
		return 1;
	}

	ExitTokenizer();
	free(fileRaw);
	if(symbolTable.size > 0) free(symbolTable.items); 

	return 0;
}

#define STR_TO_INT(str, size, res, base) do { \
	for(uint8_t numPOS = (size - 1); numPOS > 0; --numPOS){ \
 		if(str[numPOS] == '#' || str[numPOS] == 'x' || str[numPOS] == 'b') {  \
			continue; \
		} \
		if(str[numPOS] == '-') { \
			*res *= -1; \
			continue; \
		} \
		char digit = str[numPOS]; \
		if(digit >= 'A' && digit <= 'F') digit = (digit - 'A') + 10;\
		else digit = digit - '0';\
		*res += pow(base,(size-1)-numPOS) * digit; \
	} \
} while(0) 

#define STR_TO_INT_NOPREFIX(str, size, res, base) do { \
	for(uint8_t numPOS = size; numPOS > 0; --numPOS){ \
		if(str[numPOS] == '-') { \
			*res *= -1; \
			continue; \
		} \
		char digit = str[numPOS-1] - '0'; \
		*res += pow(base,size-numPOS) * digit; \
	} \
} while(0) 

bool ParseTokens(){
	if(strcmp(allTokens->items[0].text,tokenStrings[ORIG])){
		ERROR_MESSAGE("Entry point <.ORIG xxxx> not found.");
		return false;}
	if(strcmp(allTokens->items[allTokens->size-1].text,tokenStrings[END])){
		ERROR_MESSAGE("End program <.END> not found.");
		return false;}
	if(allTokens->items[1].kind != KIND_IMMEDIATE){
		ERROR_MESSAGE_LONG("Invalid entrypoint address", allTokens->items[1].line+1, allTokens->items[1].text);
		return false;}	

	size_t entryPoint = 0;
	uint8_t tokenSize = allTokens->items[1].textSize;
	switch(allTokens->items[1].text[0]){
		case '#': STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 10); break;
		case 'x': STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 16); break;
		case 'b': STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 2); break;
		default: ERROR_MESSAGE_LONG("Unknown address format <# - Dec | x - Hex | b - Bin>",allTokens->items[1].line+1, allTokens->items[1].text); return false;}
	//printf("%lu\n",entryPoint);
	if(entryPoint >= (1<<16)){
		ERROR_MESSAGE_LONG("Entrypoint address too high",allTokens->items[1].line+1, allTokens->items[1].text);
		return false;}
	
	uint16_t programCounter = (uint16_t) entryPoint;
	memory[0] = programCounter;
		
	if(!FillSymbolTable(programCounter)) {
		ERROR_MESSAGE("Failure during creation of symbol table.");
		return false;
	}
	
	size_t currentLine = allTokens->items[0].line;
	for(size_t i = 2; i < allTokens->size-1; ++i){
		if(allTokens->items[i].line != currentLine){
			programCounter++;
			currentLine = allTokens->items[i].line;
			uint16_t lineCode = 0x00000000;
			if(!GetMachineCode(allTokens->items[i],&programCounter,&lineCode,currentLine)){
				ERROR_MESSAGE("Failure during generation of machine code"); 
				return false;
			}
		}
	}

	return true;
}

bool GetMachineCode(token t, uint16_t *pc, uint16_t *bufrCode, size_t line){
	

	return true;
}

#define SYMBOL_APPEND(symbolName, addr) \
{\
	if(symbolTable.size == 0){\
		symbolTable.items = (symbol*) malloc(sizeof(symbol)); \
		symbolTable.capacity = sizeof(symbol);\
	}else if(symbolTable.capacity < (symbolTable.size+1) * sizeof(symbol)){\
		symbolTable.capacity *= 2; \
		symbolTable.items = (symbol*) realloc(symbolTable.items, symbolTable.capacity);\
	}\
	symbolTable.size++;\
	symbolTable.items[symbolTable.size-1].symbol  = symbolName;\
	symbolTable.items[symbolTable.size-1].address = addr;\
}\

bool FillSymbolTable(uint16_t entryPoint){
	uint16_t count = entryPoint;
	size_t currentLine = allTokens->items[0].line;
	bool isLabeling = false;
	for (size_t i = 0; i < allTokens->size -1; ++i){
		if(allTokens->items[i].kind == KIND_STRING) continue;
		bool isEnding = (allTokens->size < i + 1);
		size_t tokenLine = allTokens->items[i].line;
		token_kind tokenKind = allTokens->items[i].kind;
		char *tokenSymbol = allTokens->items[i].text;
		
		if(strcmp(tokenSymbol,tokenStrings[STRINGZ]) == 0){
			if(isEnding){
				ERROR_MESSAGE_LONG("String undefined",allTokens->items[i].line + 1,allTokens->items[i].text);
				return false;
			}else if(allTokens->items[i+1].kind != KIND_STRING){
				ERROR_MESSAGE_LONG("String undefined",allTokens->items[i+1].line + 1,allTokens->items[i + 1].text);
				return false;
			}
			if(isLabeling) {count--; isLabeling = false;}
			else{ currentLine = tokenLine;}
			count += allTokens->items[i+1].textSize-1;
		}else if(strcmp(tokenSymbol,tokenStrings[BLKW]) == 0){
			if(isEnding){
				ERROR_MESSAGE_LONG("Value for .BLKW undefined",allTokens->items[i].line + 1,allTokens->items[i].text);
				return false;
			}else if(allTokens->items[i+1].kind != KIND_IMMEDIATE){
				ERROR_MESSAGE_LONG("Value for .BLKW undefined",allTokens->items[i+1].line + 1,allTokens->items[i+1].text);
				return false;
			}
			if(isLabeling) {count--; isLabeling = false;}
			else{ currentLine = tokenLine;}
			int words = 0;	
			STR_TO_INT_NOPREFIX(allTokens->items[i + 1].text, allTokens->items[i+1].textSize, &words, 10); 
			count += words;
		}else if(strcmp(tokenSymbol,tokenStrings[FILL]) == 0){
			if(isEnding){
				ERROR_MESSAGE_LONG("Value for .FILL undefined",allTokens->items[i].line + 1,allTokens->items[i].text);
				return false;
			}else if(allTokens->items[i+1].kind != KIND_IMMEDIATE){
				ERROR_MESSAGE_LONG("Value for .FILL undefined",allTokens->items[i + 1].line + 1,allTokens->items[i + 1].text);
				return false;
			}
			if(isLabeling) {count--; isLabeling = false;}
			else{ currentLine = tokenLine;}
			count++;
		}else if(tokenLine != currentLine && !isLabeling){
			count++;
			currentLine = tokenLine;
			if(tokenKind == KIND_LABEL){
				isLabeling = true;
				SYMBOL_APPEND(tokenSymbol, count);
			}
		}else if(isLabeling){
			currentLine = tokenLine;
			isLabeling = false;
		}
	}
	
	for (size_t t = 0; t < allTokens->size -1; ++t){
		if(allTokens->items[t].kind == KIND_LABEL){
			bool found = false;
			for(size_t s = 0; s < symbolTable.size; ++s){
				if(strcmp(symbolTable.items[s].symbol, allTokens->items[t].text) == 0){found = true; break;}
			}
			if(!found){
				ERROR_MESSAGE_LONG("Label not found",allTokens->items[t].line + 1,allTokens->items[t].text);
				return false;
			}
		}
	}

	return true;
}

void ReadFile(FILE *file){
	fseek(file,0,SEEK_END);
	size_t size = ftell(file);
	fseek(file,0,SEEK_SET);
	fileRaw = (char*) malloc(sizeof(char) * size);
	fread(fileRaw, sizeof(char), size, file);
	rawSize = size;	
}

int OpenFile(char *filePath){
	FILE *file = NULL;
	if(!(file = fopen(filePath, "r"))){
		ERROR_MESSAGE("Coundn't open file.");
		return 0;
	}
	
	ReadFile(file);

	fclose(file);

	return 1;
}

