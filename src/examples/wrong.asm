;file to test assembler error handling
.ORIG x3000


lea r6, pos97;Load address of pos97 into r6.
ldr r6, r6, #0  ; Load contents (pos97) into r6.

pos97:  .fill #97     ; Constant for converting to ASCII letters (a-z).

.END
