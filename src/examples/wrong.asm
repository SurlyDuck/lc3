;file to test assembler error handling
.ORIG x3000

AND R0 R0 #0
LEA R0 MYSTRING
PUTS
HALT

MYSTRING: .STRINGZ "wassup\ntest"

.END
