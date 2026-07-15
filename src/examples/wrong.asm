;file to test assembler error handling
.ORIG x3000

ADD R0 R0 #0
ADD R0 R0 #1
ADD R0 R0 #2
ADD R0 R0 #-2

TEST: .FILL x1


.END
