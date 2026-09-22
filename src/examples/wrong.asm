;file to test assembler error handling
.ORIG x3000

lea r6 pos69
ldr r3 r6 #0

lea r6, neg69   ; Load address of neg69 into R6.
ldr r6, r6, #0  ; Load contents of neg69 into R6 (R6 now holds -69).
add r0, r3, r6  ; Add -69 to the value in R3, to check if it's 'E'.


neg69: .fill #-1
pos69: .fill #1


.END
