;file to test assembler error handling
.ORIG x3000

AND R0 R0 #0
AND R1 R1 #0
STI R0 ADR1
STI R1 ADR2

ADR1: .FILL OP1 
ADR2: .FILL OP2 
OP1:  ADD R0 R0 #1
OP2:  ADD R1 R2 #1

.END
