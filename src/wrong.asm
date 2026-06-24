;file to test assembler error handling

.ORIG x3000

ADD: AND R0 R0 R1
ADD R0 R0 #-2
XIAO: .STRINGZ "My string"
XIAO2: .FILL x10
XIAO3: .BLKW #10

.END
