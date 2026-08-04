;file to test assembler error handling
.ORIG x3000

AND R0 R0 #0
AND R1 R1 #0
LD  R1 ADR
LDR R0 R1 #0

LETTER:  .FILL x42
LETTER2: .FILL x42
LETTER3: .FILL x42
ADR:     .FILL LETTER2


.END
