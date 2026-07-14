;file to test assembler error handling
.ORIG x3000

AND R0 R0 #0
ADD R0 R0 #0

TEST: .FILL x1


.END
