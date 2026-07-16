;file to test assembler error handling
.ORIG x3000

START:
	AND R0 R0 #0
	ADD R0 R0 #-10
COUNT:
	ADD R0 R0 #1
	BRN COUNT
BR START

.END
