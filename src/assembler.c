#include <stdio.h>
#include <stdlib.h>

char *fileRaw;

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
		
	free(fileRaw);

	return 0;
}

void ReadFile(FILE *file){
	fseek(file,0,SEEK_END);
	long size = ftell(file);
	fseek(file,0,SEEK_SET);
	fileRaw = (char*) malloc(sizeof(char) * size);
	fread(fileRaw, sizeof(char), size, file);
		
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

