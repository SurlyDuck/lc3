;file to test assembler error handling

.ORIG x0000                        
LEA R0, HELLO_STR
TEST: ADD R0 R1 R2
TEST2:
	ADD R1 R2 R2
PUTs
HALT                               
JMP TEST2
.STRINGZ "TEST"
PLACE .BLKW 2
PLACE2 .FILL xFFFF           
HELLO_STR: .STRINGZ "Hi"
TEST_STR: 
	.STRINGZ "aaa"
.END          
