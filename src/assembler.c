#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
#include "tokenizer.h"

#define ERROR_MESSAGE(msg) fprintf(stderr, "<Error> %s\n", msg)	
#define ERROR_MESSAGE_LONG(msg,line,val) fprintf(stderr, "<Error> %s at line %lu: <%s>\n",msg,line,val)
#define WARNING_MESSAGE(msg) fprintf(stderr, "<Warning> %s\n", msg)	
#define WARNING_MESSAGE_LONG(msg,line,val) fprintf(stderr, "<Warning> %s at line %lu: <%s>\n",msg,line,val)

#define ADD_PARAMETERS 4

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
bool ParseLineCode(size_t tokenID, uint16_t *pc, uint16_t *bufrCode, size_t line);
bool ParseSequence(size_t tokenID, int parametersNum,...);

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
	uint16_t binaryCursor   = 0;
	memory[binaryCursor] = programCounter;
		
	if(!FillSymbolTable(programCounter)) {
		ERROR_MESSAGE("Failure during creation of symbol table.");
		return false;
	}
	
	size_t currentLine = allTokens->items[0].line;
	for(size_t i = 2; i < allTokens->size-1; ++i){
		if(allTokens->items[i].kind == KIND_LABEL) continue;
		if(allTokens->items[i].line == allTokens->items[0].line){
			WARNING_MESSAGE_LONG("Ignoring token at entrypoint line",allTokens->items[i].line+1,allTokens->items[i].text);	
			continue;
		}
		if(allTokens->items[i].line != currentLine){
			currentLine = allTokens->items[i].line;
			uint16_t lineCode = 0x00000000;
			if(!ParseLineCode(i,&programCounter,&lineCode,currentLine)){
				ERROR_MESSAGE_LONG("Failure during generation of machine code",currentLine+1, allTokens->items[i].text); 
				return false;
			}
			binaryCursor++;
			memory[binaryCursor] = lineCode;
		}
	}

	return true;
}

bool ParseLineCode(size_t tokenID, uint16_t *pc, uint16_t *bufrCode, size_t line){
	int lineTokens = 1;
	while(tokenID+lineTokens < allTokens->size-1){
		if(allTokens->items[tokenID+lineTokens].line != line) break;
		else lineTokens++;
	}
	
	char *text[lineTokens];
	uint8_t textSize[lineTokens];
	for(int i = 0; i < lineTokens; ++i){
		text[i] = allTokens->items[tokenID+i].text;
		textSize[i]  = allTokens->items[tokenID+i].textSize;
	}

	if(strcmp(allTokens->items[tokenID].text,tokenStrings[OP_ADD]) == 0){ /* ADD  & AND */
		if(lineTokens != ADD_PARAMETERS){
			ERROR_MESSAGE_LONG("Invalid number of parameters for opcode",line+1,allTokens->items[tokenID].text);
			return false;
		}
		
		if(ParseSequence(tokenID,ADD_PARAMETERS,KIND_OPCODE,KIND_REGISTER,KIND_REGISTER,KIND_REGISTER)){
			/* REGISTER ADD */
		}else if(ParseSequence(tokenID,ADD_PARAMETERS,KIND_OPCODE,KIND_REGISTER,KIND_REGISTER,KIND_IMMEDIATE)){
			/* IMMEDIATE ADD */
			int res = 0;
			switch(allTokens->items[tokenID+3].text[0]){
				case '#': STR_TO_INT(text[3], textSize[3], &res, 10); break; 
				case 'x': STR_TO_INT(text[3], textSize[3], &res, 16); break; 
				case 'b': STR_TO_INT(text[3], textSize[3], &res, 2); break;
				default: WARNING_MESSAGE_LONG("Immediate value without prefix, assuming decimal",line,allTokens->items[tokenID+3].text);
				
			}
		}else {
			ERROR_MESSAGE_LONG("Invalid parameters for opcode",line+1,"ADD");
			return false;
		}
		
	}
	
	return true;
}

bool ParseSequence(size_t tokenID, int parametersNum, ...){
	va_list args;
	va_start(args, parametersNum);
	for(int i = 0; i < parametersNum; i++){
		if(allTokens->items[tokenID+i].kind != va_arg(args, token_kind)) return false;
	}
	
	va_end(args);
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
