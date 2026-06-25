#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include "tokenizer.h"

#define ERROR_MESSAGE(msg) fprintf(stderr, "<Error> %s\n", msg)	
#define ERROR_MESSAGE_LONG(msg,line,val) fprintf(stderr, "<Error> %s at line %lu: <%s>\n",msg,line,val)
#define WARNING_MESSAGE(msg) fprintf(stderr, "<Warning> %s\n", msg)	
#define WARNING_MESSAGE_LONG(msg,line,val) fprintf(stderr, "<Warning> %s at line %lu: <%s>\n",msg,line,val)
#define PRINT_USAGE fprintf(stdout,"\nUsage: ./assembler -f program.asm [OPTIONS: -hol]\n"); \
fprintf(stdout,"h - Print this message\no - Output file name\nl - Output file use little endianess\n")

#define ADD_AND_PARAMETERS   4
#define PSEUDO_OP_PARAMETERS 2
#define BR_PARAMETERS        2

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
char *inputFileName  = NULL;
char *outputFileName = NULL;
bool isLittleEndian  = false;
uint16_t memory[1<<16] = {0};

int OpenFile(char *filePath);
int GetRegCode(char *txt);
bool ParseTokens(void);
bool FillSymbolTable(uint16_t entryPoint);
bool SaveHex(uint16_t hexCursor);
bool ParseLineCode(size_t tokenID, uint16_t *pc, uint16_t *bufrCode, size_t line);
bool ParseSequence(size_t tokenID, int parametersReceived, int parametersNum,...);

int main(int argc, char **argv){
	if(argc < 2){
		PRINT_USAGE;
		return 1;
	}
	
	char opt = 0;
	while((opt = getopt(argc, argv,"hf:o:l")) != -1){
		switch(opt){
			case 'h': PRINT_USAGE; return 0;
			case 'f': inputFileName  = optarg; break;
			case 'o': outputFileName = optarg; break;
			case 'l': isLittleEndian = true; break;
			default: PRINT_USAGE;return 1;
		}
	}

	if(inputFileName == NULL){
		fprintf(stdout, "input file not given\n");
		PRINT_USAGE;
		return 1;
	}

	if(outputFileName == NULL){
		WARNING_MESSAGE("Output file name not given, choosing ´output.bin´");
		outputFileName = "output.obj";
	}

	if(!OpenFile(inputFileName)){
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

#define STR_TO_INT(str, size, res)\
do {\
	int base;\
	switch(str[0]){\
		case '#': base = 10; break;\
		case 'x': base = 16; break;\
		case 'X': base = 16; break;\
		case 'b': base = 2;  break;\
		case 'B': base = 2;  break;\
		default: base = 10; break;\
	}\
	\
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

typedef struct {
	uint16_t hexCode;
}code;

typedef struct {
	code *items;
	size_t size;
	size_t capacity;	
}code_lines;

code_lines hexArray = {0};

#define APPEND_CODE(val) do{\
   if(hexArray.size == 0){ \
		hexArray.items = (code*) malloc(sizeof(code)); \
	}else if(hexArray.capacity < sizeof(code) * (hexArray.size+1)){ \
		hexArray.items = (code*) realloc(hexArray.items, sizeof(code) * (hexArray.size+1)); \
	}\
	hexArray.size++;\
	hexArray.items[hexArray.size-1].hexCode = val; \
	hexArray.capacity = sizeof(code) * hexArray.size; \
   \
}while(0)

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
	switch(allTokens->items[1].text[0]){ /*TODO: remove this later */
		case '#': /*STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 10);*/ break;
		case 'x': /*STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 16);*/ break;
		case 'X': /*STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 16);*/ break;
		case 'b': /*STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 2); */break;
		case 'B': /*STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint, 2); */break;
		default: ERROR_MESSAGE_LONG("Unknown address format <# - Dec | x - Hex | b - Bin>",allTokens->items[1].line+1, allTokens->items[1].text); return false;}

	STR_TO_INT(allTokens->items[1].text, tokenSize, &entryPoint);
	if(entryPoint >= (1<<16)){
		ERROR_MESSAGE_LONG("Entrypoint address too high",allTokens->items[1].line+1, allTokens->items[1].text);
		return false;}
	
	uint16_t programCounter = (uint16_t) entryPoint;
	uint16_t binaryCursor   = 0;

	if(isLittleEndian) memory[binaryCursor] = programCounter;
	else memory[binaryCursor] = ((programCounter & 0xFF) << 8) | ((programCounter & 0xFF00) >> 8);
		
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
		token_kind k = allTokens->items[i].kind;
		if(allTokens->items[i].line != currentLine){
			if(k != KIND_OPCODE && k != KIND_PSEUDO_OP && k != KIND_TRAP){
				ERROR_MESSAGE_LONG("Invalid operator",allTokens->items[i].line+1,allTokens->items[i].text);
				return false;
			}

			currentLine = allTokens->items[i].line;
			uint16_t lineCode = 0x00000000;
			if(!ParseLineCode(i,&programCounter,&lineCode,currentLine)){
				ERROR_MESSAGE_LONG("Failure during generation of machine code",currentLine+1, allTokens->items[i].text); 
				return false;
			}
			for(size_t i = binaryCursor; i < hexArray.size; i++){
				binaryCursor++;
				uint16_t c = hexArray.items[i].hexCode;
				if(isLittleEndian) memory[binaryCursor] = c; 
				else memory[binaryCursor] = ((c & 0xFF) << 8) | ((c & 0xFF00) >> 8);
			}
	
		}
	}
	
	if(!SaveHex(binaryCursor)){
		ERROR_MESSAGE("Couldn't create hex file.");
	}

	return true;
}

bool ParseLineCode(size_t tokenID, uint16_t *pc, uint16_t *bufrCode, size_t line){
	int lineTokens = 1;
	uint16_t opcode = 0x0000;
	
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

	if(strcmp(allTokens->items[tokenID].text,tokenStrings[OP_ADD]) == 0 || strcmp(allTokens->items[tokenID].text,tokenStrings[OP_AND]) == 0){ /* ADD  & AND */
		if(strcmp(allTokens->items[tokenID].text,tokenStrings[OP_ADD]) == 0) opcode |= 0x1000;
		if(strcmp(allTokens->items[tokenID].text,tokenStrings[OP_AND]) == 0) opcode |= 0x5000;

		if(ParseSequence(tokenID,lineTokens,ADD_AND_PARAMETERS,KIND_OPCODE,KIND_REGISTER,KIND_REGISTER,KIND_REGISTER)){
			/* REGISTER ADD OR AND */
			opcode |=  GetRegCode(text[1]) << 9;
			opcode |=  GetRegCode(text[2]) << 6;
			opcode |=  GetRegCode(text[3]);
			//opcode &= 0x20;
			APPEND_CODE(opcode);
			*bufrCode = opcode;
			*pc = *pc + 1;
		}else if(ParseSequence(tokenID,lineTokens,ADD_AND_PARAMETERS,KIND_OPCODE,KIND_REGISTER,KIND_REGISTER,KIND_IMMEDIATE)){
			/* IMMEDIATE ADD OR AND */
			int res = 0;/*
			switch(allTokens->items[tokenID+3].text[0]){
				case '#': STR_TO_INT(text[3], textSize[3], &res, 10); break; 
				case 'x': STR_TO_INT(text[3], textSize[3], &res, 16); break; 
				case 'b': STR_TO_INT(text[3], textSize[3], &res, 2); break;
				default: WARNING_MESSAGE_LONG("Immediate value without prefix, assuming decimal",line,allTokens->items[tokenID+3].text); STR_TO_INT(text[3], textSize[3], &res, 10); break;
			}*/
			if(text[3][0] >= '0' && text[3][0] <= '9'){
			  	WARNING_MESSAGE_LONG("Immediate value without prefix, assuming decimal",line,allTokens->items[tokenID+3].text);
				STR_TO_INT_NOPREFIX(text[3], textSize[3], &res, 10);
			} else STR_TO_INT(text[3], textSize[3], &res);

			if(res > 0xF || res < -0x10){
				ERROR_MESSAGE_LONG("imm5 value out of range <-16..15>",line+1,text[3]);
				return false;
			}
			res = res & 0x1F;
			opcode |=  GetRegCode(text[1]) << 9;
			opcode |=  GetRegCode(text[2]) << 6;
			opcode |= res;
			opcode |= 0x20;
			APPEND_CODE(opcode);
			*bufrCode = opcode;
			*pc = *pc + 1;
			
		}else {
			ERROR_MESSAGE_LONG("Invalid parameters for opcode",line+1,text[0]);
			return false;
		}
		
	}else if(strcmp(text[0],tokenStrings[OP_BR]) == 0 || strcmp(text[0],tokenStrings[OP_BRn]) == 0 || strcmp(text[0],tokenStrings[OP_BRp]) == 0 ||  strcmp(text[0],tokenStrings[OP_BRz]) == 0){ /* BR BRn BRP opcode */
		if(!ParseSequence(tokenID,lineTokens,BR_PARAMETERS,KIND_OPCODE,KIND_LABEL)){
			ERROR_MESSAGE_LONG("Invalid parameters for BR parameter",line+1, text[0]);
			return false;
		}

		uint16_t labelAddr = 0;
		int16_t pcOffset9  = 0;
		for(size_t i = 0; i < symbolTable.size; ++i){
			if(strcmp(symbolTable.items[i].symbol, text[1]) == 0){
				labelAddr = symbolTable.items[i].address;
				break;
			}
		}
		pcOffset9 = labelAddr - *pc;
		if(pcOffset9 > 255 || pcOffset9 < -256){
			ERROR_MESSAGE_LONG("Label is out of reach. Address must fit in 9 bits",line+1, text[1]);
			return false;
		}

		if(pcOffset9 < 0) opcode |= (~(pcOffset9 & 0x01FF)) + 1;
		else opcode |= pcOffset9 & 0x01FF;

		opcode |= ((strcmp(text[0], tokenStrings[OP_BR]) == 0) * 0x7) << 11;
		opcode |= ((strcmp(text[0], tokenStrings[OP_BRn]) == 0) * 0x1) << 11;
		opcode |= ((strcmp(text[0], tokenStrings[OP_BRp]) == 0) * 0x1) << 9;
		opcode |= ((strcmp(text[0], tokenStrings[OP_BRz]) == 0) * 0x1) << 10;

		APPEND_CODE(opcode);

	}else if(strcmp(text[0],tokenStrings[STRINGZ]) == 0){ /* .STRINGZ pseudo-opcode */
		if(!ParseSequence(tokenID,lineTokens,PSEUDO_OP_PARAMETERS,KIND_PSEUDO_OP,KIND_STRING)){
			ERROR_MESSAGE_LONG("Invalid parameters for pseudo-opcode",line+1, text[0]);
			return false;
		}

		for(int i = 0; text[1][i] != '\0'; ++i) {
			APPEND_CODE(text[1][i]);
			*pc = *pc + 1;
		}
			*pc = *pc + 1;
		APPEND_CODE('\0');
	}else if(strcmp(text[0],tokenStrings[FILL]) == 0){ /* .FILL pseudo-opcode */
		if(!ParseSequence(tokenID,lineTokens,PSEUDO_OP_PARAMETERS,KIND_PSEUDO_OP,KIND_IMMEDIATE)){
			ERROR_MESSAGE_LONG("Invalid parameters for pseudo-opcode",line+1, text[0]);
			return false;
		}
		
		int res = 0;
		if(text[1][0] >= '0' && text[1][0] <= '9'){
			WARNING_MESSAGE_LONG("Immediate value without prefix, assuming decimal",line,allTokens->items[tokenID+1].text);
			STR_TO_INT_NOPREFIX(text[1], textSize[1], &res, 10);
		}else STR_TO_INT(text[1], textSize[1], &res);
		APPEND_CODE(res);
		*pc = *pc + 1;
	}else if(strcmp(text[0],tokenStrings[BLKW]) == 0){ /* .BLKW pseudo-opcode */
		if(!ParseSequence(tokenID,lineTokens,PSEUDO_OP_PARAMETERS,KIND_PSEUDO_OP,KIND_IMMEDIATE)){
			ERROR_MESSAGE_LONG("Invalid parameters for pseudo-opcode",line+1, text[0]);
			return false;
		}

		size_t res = 0;
		STR_TO_INT_NOPREFIX(text[1], textSize[1], &res, 10);
		
		for(size_t i = 0; i < res; ++i){
			APPEND_CODE(0x0000);
			*pc = *pc + 1;
		}

	}
	return true;
}

bool ParseSequence(size_t tokenID, int parametersReceived, int parametersNum, ...){
	va_list args;
	va_start(args, parametersNum);
	for(int i = 0; i < parametersNum; i++){
		if(allTokens->items[tokenID+i].kind != va_arg(args, token_kind)) return false;
	}
	
	va_end(args);

	if(parametersReceived != parametersNum) return false;
	return true;
}

int GetRegCode(char *txt){
	if(strcmp(txt, tokenStrings[REG0]) == 0) return 0x0;
	else if(strcmp(txt, tokenStrings[REG1]) == 0) return 0x1;
	else if(strcmp(txt, tokenStrings[REG2]) == 0) return 0x2;
	else if(strcmp(txt, tokenStrings[REG3]) == 0) return 0x3;
	else if(strcmp(txt, tokenStrings[REG4]) == 0) return 0x4;
	else if(strcmp(txt, tokenStrings[REG5]) == 0) return 0x5;
	else if(strcmp(txt, tokenStrings[REG6]) == 0) return 0x6;
	else  return 0x7;
}

bool SaveHex(uint16_t hexCursor){
	FILE *file = NULL;
	if((file = fopen(outputFileName,"wb")) == NULL){
		return false;
	}

	if(fwrite(memory, sizeof(uint16_t), hexCursor + 1, file) == 0){
		ERROR_MESSAGE("Couldn't write to file");
		fclose(file);
	}

	fclose(file);
	
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
			count += allTokens->items[i+1].textSize;
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
			currentLine = tokenLine;
			if(tokenKind == KIND_LABEL){
				isLabeling = true;
				SYMBOL_APPEND(tokenSymbol, count);
			}

			count++;
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
