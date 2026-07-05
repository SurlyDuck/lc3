;file to test assembler error handling
.ORIG x3000

BR WELCOME 

WELCOME:
     LEA R0 WELCOME_MESSAGE
     PUTs    
 
WELCOME_MESSAGE .STRINGZ "Welcome to LC3 Rogue.\nUse WSAD to move.\nPress any key..\n"


.END
