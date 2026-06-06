#include "tokenizer.h"
#include <assert.h>

typedef enum{
	SEARCHING = 0,
	IGNORING, 
	READING
}token_status;

token** GetTokens(char *raw, size_t rawSize){
	
	long long cursor = -1;
	token_status currentStatus = SEARCHING;
	token currentToken = {0};
	size_t currentLine = 0;
	int currentTokenCursor    = 0;
	char currentTokenText[256] = {0};
	while((rawSize - cursor) > 0){
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
			/*TODO: token is finished, indentifiy it here and add to array */
			currentStatus = SEARCHING;
			printf("Current Token: %s\n",currentToken.text);
			currentTokenCursor = 0;
			memset(&currentTokenText,'\0', 256);
		}	

	}	


	return NULL;
}
