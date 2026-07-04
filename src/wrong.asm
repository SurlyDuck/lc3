;file to test assembler error handling
.ORIG x3000

AND R0 R0 #0
LD  R0 STR_ADR
TRAP x22

STR_ADR: .FILL STRING
STRING: .STRINGZ "Hello World!"

.END
