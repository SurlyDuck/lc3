#include "tokenizer.h"


token** GetTokens(char *raw, size_t rawSize){
	
	size_t cursor = -1;
	token_kind currentKind = -1;
	while((rawSize - cursor) > 0){
		++cursor;
		char b = raw[cursor];
		if(b == ';') currentKind = KIND_COMMENT;

		if((currentKind == KIND_COMMENT && b != '\n') || b == ',') {
			raw[cursor] = ' ';
			continue;
		}
		else currentKind = -1;

		printf("%c",raw[cursor]);
	}	


	return NULL;
}
