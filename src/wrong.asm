;file to test assembler error handling

.ORIG x3000

AND R0 R0 R1
ADD R0 R0 #1
XIAO: .STRINGZ "My string"

.END
