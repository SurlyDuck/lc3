.ORIG x3000

START:
	LEA R0 START_LETTER
	LDR R0 R0 x0
	
	; Load ending letter into R1 and ADD 1
	; So the program knows to stop after printing it
	LEA R1 ENDING_LETTER
	LDR R1 R1 x0
	ADD R1 R1 x1
	
	; Convert the ending letter incremented to negative
	; (2's complement)
	NOT R1 R1
	ADD R1 R1 x1 

PRINT_LOOP:
	OUT
	
	; Increment to the next letter and
	; store it into r3 temporally
	ADD R3 R0 x1
	
	LEA R0 NEW_LINE
	PUTS
	
	; Restore incremented letter into R0
	; and clear R3 for use in the next loop
	AND R0 R0 x0
	ADD R0 R3 R0
	AND R3 R3 x0
	 
	; Check if it reached the end of the loop
	ADD R2 R0 R1
	BRZ END
	BR PRINT_LOOP
	
END:
	HALT

NEW_LINE: .STRINGZ "\n"
START_LETTER:  .FILL  x41
ENDING_LETTER: .FILL  x5A

.END
