;file to test assembler error handling

.ORIG x3000

AND R0 R0 #0
ADD R0 R0 #1
XIAO: .stringz "My string"

.END
