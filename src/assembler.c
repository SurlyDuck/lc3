#define ERROR_USAGE fprintf(stderr, "\nUSAGE: assembler program.asm\n\n")	
#define ERROR_FILE fprintf(stderr, "\nError: Couldn't open file.\n")	
#define ERROR_TOKENIZER fprintf(stderr, "\n<lexer>Error: Couldn't tokenize file.\n")
#define ERROR_NO_TOKENS fprintf(stderr, "\n<lexer>Error: No tokens to parse.\n") 
#define ERROR_PARSING fprintf(stderr, "\n<parser>Error: Couldn't parse.\n") 
#define ERROR_NO_ORIG fprintf(stderr, "\n<parser>Error: Entry point <.ORIG xxxx> not found.\n") 
#define ERROR_NO_END fprintf(stderr, "\n<parser>Error: End program <.END> not found.\n") 
#define ERROR_IMMEDIATE_ADDRESS(line, val) (fprintf(stderr, \
 "\n<parser>Error: Invalid Address at line %lu: <%s>\n",line,val)) \

#define ERROR_IMMEDIATE_ADDRESS_FORMAT(line, val) (fprintf(stderr, \
 "\n<parser>Error: Unknown address format <# - Dec | x - Hex | b - Bin> at line %lu: <%s>\n",line,val)) \

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "tokenizer.h"


tokens *allTokens = NULL;
size_t rawSize = 0;
char *fileRaw;
uint16_t memory[1<<16];

int OpenFile(char *filePath);
bool ParseTokens(void);
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

	return 0;
}

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
	switch(allTokens->items[1].text[0]){
		case '#': entryPoint = StrToInt(allTokens->items[1].text, 10); break;
		case 'x': entryPoint = StrToInt(allTokens->items[1].text, 16); break;
		case 'b': entryPoint = StrToInt(allTokens->items[1].text, 2); break;
		default: ERROR_IMMEDIATE_ADDRESS_FORMAT(allTokens->items[1].line+1, allTokens->items[1].text); return false;
	}
	printf("%lu", entryPoint);
	return true;
}

size_t StrToInt(char *str, int base){
	size_t num = 0;
	int size = 0;
	while(str[size] != '\0') size++;
	size--;
	for(int i = size; i >= 1; --i){
		if(str[i] == '#' || str[i] == 'x' || str[i] == 'b') continue;
		num += pow(base,size-i) * (size_t)(str[i] - '0');
	}

	return num;
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

