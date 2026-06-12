#define ERROR_USAGE fprintf(stderr, "\nUSAGE: assembler program.asm\n\n")	
#define ERROR_FILE fprintf(stderr, "\nError: Couldn't open file.\n")	
#define ERROR_TOKENIZER fprintf(stderr, "\n<lexer>Error: Couldn't tokenize file.\n")
#define ERROR_NO_TOKENS fprintf(stderr, "\n<lexer>Error: No tokens to parse.\n") 
#define ERROR_PARSING fprintf(stderr, "\n<parser>Error: Couldn't parse.\n") 
#define ERROR_NO_ORIG fprintf(stderr, "\n<parser>Error: Entry point <.ORIG xxxx> not found.\n") 
#define ERROR_NO_END fprintf(stderr, "\n<parser>Error: End program <.END> not found.\n") 
#define ERROR_FILLING_SIMBOL_TABLE fprintf(stderr, "\n<parser>Error: Failure during creation of symbol table.\n") 
#define ERROR_IMMEDIATE_ADDRESS(line, val) (fprintf(stderr, \
 "\n<parser>Error: Invalid entrypoint address at line %lu: <%s>\n",line,val)) 
#define ERROR_IMMEDIATE_ADDRESS_FORMAT(line, val) (fprintf(stderr, \
 "\n<parser>Error: Unknown address format <# - Dec | x - Hex | b - Bin> at line %lu: <%s>\n",line,val)) 
#define ERROR_EXPLODED_MEMORY(line, val) (fprintf(stderr, \
 "\n<parser>Error: Entrypoint address too high at line %lu: <%s>\n",line,val)) 
#define ERROR_EXPLODED_MEMORY(line, val) (fprintf(stderr, \
 "\n<parser>Error: Entrypoint address too high at line %lu: <%s>\n",line,val)) 
#define ERROR_SYMBOL_NOT_FOUND(line, val) (fprintf(stderr, \
 "\n<parser>Error: Label not found at line %lu: <%s>\n",line,val)) 
#define ERROR_STRING_UNDEFINED(line, val) (fprintf(stderr, \
 "\n<parser>Error: String undefined at line %lu: <%s>\n",line,val)) 
#define ERROR_BLKW_UNDEFINED(line, val) (fprintf(stderr, \
 "\n<parser>Error: Value for .BLKW undefined  at line %lu: <%s>\n",line,val)) 

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

int OpenFile(char *filePath);
bool ParseTokens(void);
bool FillSymbolTable(uint16_t entryPoint);
size_t StrToInt(char *str, int base);

int main(int argc, char **argv){
	if(argc < 2){
		ERROR_USAGE;
		return 1;
	}
	
	if(!OpenFile(argv[1])){
		ERROR_FILE;
		return 1;
	}
	
	if((allTokens = InitTokenizer(fileRaw, rawSize)) == NULL){
		ERROR_TOKENIZER;
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
		ERROR_NO_TOKENS;
		return 1;
	}

	if(!ParseTokens()){
		ERROR_PARSING;
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
		ERROR_NO_ORIG;
		return false;}
	if(strcmp(allTokens->items[allTokens->size-1].text,tokenStrings[END])){
		ERROR_NO_END;
		return false;}
	if(allTokens->items[1].kind != KIND_IMMEDIATE){
		ERROR_IMMEDIATE_ADDRESS(allTokens->items[1].line+1, allTokens->items[1].text);
		return false;}	

	size_t entryPoint = 0;
	uint8_t tokenSize = allTokens->items[1].textSize;
	switch(allTokens->items[1].text[0]){
		case '#': STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 10); break;
		case 'x': STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 16); break;
		case 'b': STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 2); break;
		default: ERROR_IMMEDIATE_ADDRESS_FORMAT(allTokens->items[1].line+1, allTokens->items[1].text); return false;}
	//printf("%lu\n",entryPoint);
	if(entryPoint >= (1<<16)){
		ERROR_EXPLODED_MEMORY(allTokens->items[1].line+1, allTokens->items[1].text);
		return 1;}
	
	uint16_t programCounter = (uint16_t) entryPoint;
	memory[0] = programCounter;
		
	if(!FillSymbolTable(programCounter)) {
		ERROR_FILLING_SIMBOL_TABLE;
		return false;
	}

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
				ERROR_STRING_UNDEFINED(allTokens->items[i].line + 1,allTokens->items[i].text);
				return false;
			}else if(allTokens->items[i+1].kind != KIND_STRING){
				ERROR_STRING_UNDEFINED(allTokens->items[i + 1].line + 1,allTokens->items[i + 1].text);
				return false;
			}
			if(isLabeling) {count--; isLabeling = false;}
			else{ count++; currentLine = tokenLine;}
			count += allTokens->items[i+1].textSize-1;
		}else if(strcmp(tokenSymbol,tokenStrings[BLKW]) == 0){
			if(isEnding){
				ERROR_BLKW_UNDEFINED(allTokens->items[i].line + 1,allTokens->items[i].text);
				return false;
			}else if(allTokens->items[i+1].kind != KIND_IMMEDIATE){
				ERROR_BLKW_UNDEFINED(allTokens->items[i + 1].line + 1,allTokens->items[i + 1].text);
				return false;
			}
			if(isLabeling) {count--; isLabeling = false;}
			else{ count++; currentLine = tokenLine;}
			int words = 0;	
			STR_TO_INT_NOPREFIX(allTokens->items[i + 1].text, allTokens->items[i+1].textSize, &words, 10); 
			count += words;
			printf("%d",words);
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
				ERROR_SYMBOL_NOT_FOUND(allTokens->items[t].line + 1,allTokens->items[t].text);
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
		return 0;
	}
	
	ReadFile(file);

	fclose(file);

	return 1;
}

