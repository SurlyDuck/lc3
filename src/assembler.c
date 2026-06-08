#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"

char *fileRaw;
size_t rawSize = 0;
tokens *allTokens = {0};

int OpenFile(char *filePath);

int main(int argc, char **argv){
	
	if(argc < 2){
		fprintf(stderr, "\nUSAGE: assembler program.asm\n\n");	
		return 1;
	}
	
	if(!OpenFile(argv[1])){
		fprintf(stderr, "\nError: Couldn't open file.\n");	
		return 1;
	}
	
	if((allTokens = GetTokens(fileRaw, rawSize)) == NULL){
		fprintf(stderr, "\nError: Couldn't tokenize file.\n");
		return 1;
	}
	
	free(fileRaw);
	free(allTokens->items);

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
		return 0;
	}
	
	ReadFile(file);

	fclose(file);

	return 1;
}

