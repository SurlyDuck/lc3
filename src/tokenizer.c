#include "tokenizer.h"

typedef enum{
	SEARCHING = 0,
	IGNORING, 
	READING
}token_status;

char *tokenStrings[] =  {
	"",		
	"ADD",	/* OP_ADD, */
	"AND",	/* OP_AND, */
	"BR",		/* OP_BR, */
	"BRN",
	"BRZ",
	"BRP",
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
	"",     
	"R0",	/* REG0,*/
	"R1",	/* REG1,*/
	"R2",	/* REG2,*/
	"R3",	/* REG3,*/
	"R4",	/* REG4,*/
	"R5",	/* REG5,*/
	"R6",	/* REG6,*/
	"R7",	/* REG7,*/
	"",      /* END REGISTERS */
	"",
	"GETC",	/* GETC,*/
	"OUT",	/* OUT,*/
	"PUTS",	/* PUTS,*/
	"IN",		/* IN,*/
	"PUTSP",	/* PUTSP,*/
	"HALT",	/* HALT,*/
	"",		/* END_TRAP_ROUTINES, */
	"", 
	".ORIG",           /* ORIG,*/
	".END",            /* END,*/
	".BLKW",           /* BLKW,*/
	".FILL",           /* FILL,*/
	".STRINGZ",        /* STRINGZ,*/
	"",                /* END PSEUDO OPCODES */
	"LABEL",
	"#0",      /* IMMEDIATE */
	"string", 
	"invalid"
};

static tokens tokensArray = {0};

static void Uppercase(char *text, size_t size){
	for(size_t i = 0; i < size; ++i){
		if(text[i] > 'Z' && !isdigit(text[i])) text[i] -= 'a' - 'A';
	}
}

static int IsStrImmediate(const char *txt, int invalidReturn){
	int i = 0;
	int invalid = (invalidReturn) ? -1 : 0;
	int bin = ((txt[0] == 'b' || txt[0] == 'B') && (txt[1] != '\0'));
	int hex = ((txt[0] == 'x' || txt[0] == 'X') && (txt[1] != '\0'));
	int dec = ((txt[0] == '#' && txt[1] != '\0') || (txt[0] >= '0' && txt[0] <= '9') || (txt[0] == '-' && txt[1] != '\0'));
	
	if(!bin && !hex && !dec) return 0;

	while(1){
		i++;
		if(txt[i] == '\0') break;
		if(txt[i] == '-' && i > 1) return invalid;
		if(txt[i] == '-') continue;
		if(dec && !(txt[i] >= '0' && txt[i] <= '9')) return invalid; 
		if(bin && !(txt[i] >= '0' && txt[i] <= '1')) return invalid; 
		if(hex && !isxdigit(txt[i])) return invalid; 
	}
	return 1;
}

tokens* InitTokenizer(char *raw, size_t rawSize){
	long long cursor = -1;
	token_status currentStatus = SEARCHING;
	token currentToken = {0};
	size_t currentLine = 0;
	int currentTokenCursor = 0;
	int isString = 0;
	char currentTokenText[TOKEN_MAX_SIZE] = {0};
	while((rawSize - cursor) > 0){
		if(currentTokenCursor >= TOKEN_MAX_SIZE){
			fprintf(stderr, "<Error> at line %lu, token '%s' is too big\n", currentLine,currentTokenText);
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
			if(b == '"') isString = 1; 
			else currentTokenText[currentTokenCursor] = b; 
			currentToken = (token) {currentLine, currentTokenText, currentTokenCursor, KIND_INVALID};
			currentStatus = READING;
			continue;
		}
		else if(currentStatus != READING) continue;	
		
		if(currentStatus == READING && ((b != ' ' && b != '\t' && b != '"' && b != '\n') || (isString && b != '"'))){
			currentTokenCursor++;
			currentToken.text[currentTokenCursor] = b;
		}else if(currentStatus == READING && ((b == ' ' || b == ',' || b == '\t' || b == '\n') || (isString && b == '"'))){
			/* get token kind, add to tokens array and keep searching */
			if(isString){
				/* need to remove double quotes from the left */
				for(int i = 0; i < currentTokenCursor; ++i){
					currentTokenText[i] = currentTokenText[i+1];
				}
				currentTokenText[currentTokenCursor] = '\0';
				currentToken.kind = KIND_STRING;
			}else if(IsStrImmediate(currentTokenText,0)){
				currentToken.kind = KIND_IMMEDIATE;
			}else if (IsStrImmediate(currentTokenText,1) == -1){
				currentToken.kind = KIND_INVALID;
			}else{
				Uppercase(currentTokenText, currentTokenCursor + 1);
				for(int id = 0; id < TOKENS_COUNT; ++id){
					if(strcmp(currentTokenText, tokenStrings[id]) == 0){
						if(id > START_OPCODES && id < END_OPCODES){
							currentToken.kind = KIND_OPCODE;
							break;
						}else if(id > START_REGISTERS && id < END_REGISTERS){
							currentToken.kind = KIND_REGISTER;
							break;
						}else if(id > START_TRAP_ROUTINES && id < END_TRAP_ROUTINES){
							currentToken.kind = KIND_TRAP;
							break;
						}else if(id > START_PSEUDO_OPS && id < END_PSEUDO_OPS){
							currentToken.kind = KIND_PSEUDO_OP;
							break;
						}else{
							continue;
						}
					}
				}
				if(currentToken.kind == KIND_INVALID) {
					currentToken.kind = KIND_LABEL;
					for(int i = 0; i < currentTokenCursor + 1; ++i){
						/* colon is ignored on labels */
						if(currentTokenText[i] == ':'){currentTokenText[i] = '\0'; break; }
					}
				}
			}

			currentToken.text = (char*) malloc(sizeof(char) * currentTokenCursor + 1);
			for(int i = 0; i <= currentTokenCursor; ++i){
				currentToken.text[i] = currentTokenText[i];	
			}
			currentToken.text[currentTokenCursor + 1] = '\0';
			currentToken.textSize = currentTokenCursor + 1;
			if(tokensArray.size == 0){
				tokensArray = (tokens) {
				.items = (token*) malloc(sizeof(token) * 1),
				.size   = 1,
				.capacity = sizeof(tokens),
				};
			}else{
				tokensArray.size += 1;
				if(tokensArray.capacity < sizeof(token) * tokensArray.size){
					tokensArray.capacity *= 2;
					tokensArray.items = (token*) realloc(tokensArray.items, tokensArray.capacity); 				
				}
			}

			tokensArray.items[tokensArray.size - 1] = currentToken;

			currentTokenCursor = 0;
			memset(&currentTokenText,'\0', 256);
			isString = 0;
			currentStatus = SEARCHING;
		}	
	}	
	return &tokensArray;
}

void ExitTokenizer(void){
	for(size_t i = 0; i < tokensArray.size; ++i){
		free(tokensArray.items[i].text);
	}
	free(tokensArray.items);

}

