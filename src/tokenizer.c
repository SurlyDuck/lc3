#include "tokenizer.h"

typedef enum{
	SEARCHING = 0,
	IGNORING, 
	READING
}token_status;

char *tokenStrings[] =  {
"",		/* START_OPCODES = 0, */
"ADD",	/* OP_ADD, */
"AND",	/* OP_AND, */
"BR",		/* OP_BR, */
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

};

token** GetTokens(char *raw, size_t rawSize){
	
	long long cursor = -1;
	token_status currentStatus = SEARCHING;
	token currentToken = {0};
	size_t currentLine = 0;
	int currentTokenCursor    = 0;
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
			currentTokenText[currentTokenCursor] = b;
			currentToken = (token) {currentLine, currentTokenText, KIND_INVALID};
			currentStatus = READING;
			continue;
		}
		else if(currentStatus != READING) continue;	
		
		if(currentStatus == READING && b != ' ' && b != '\t'){
			currentTokenCursor++;
			currentToken.text[currentTokenCursor] = b;
		}else if(currentStatus == READING && (b == ' ' || b == ',' || b == '\t')){
			/*TODO: token is finished, find its kind and add to an array */
			currentStatus = SEARCHING;
			currentToken.line = currentLine;
			printf("Current Token at line %lu: %s\n",currentToken.line, currentToken.text);
			currentTokenCursor = 0;
			memset(&currentTokenText,'\0', 256);
		}	

	}	


	return NULL;
}
