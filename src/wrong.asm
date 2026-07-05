;file to test assembler error handling
.ORIG x3000

AND R0 R0 #0
LD  R0 HI_ADR
TRAP x22

HI_ADR: .FILL HI
HI: .STRINGZ "a\ntest"

.END
