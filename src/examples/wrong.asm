;file to test assembler error handling
.ORIG x3000

AND R4 R4 x0
AND R0 R0 x0
LD R0 LETTER

inputloop:        ; Else start reading input.
  add r4, r4, #1  ; Go to next index in array.
  add r0, r0, #-1 ; Decrement counter by 1.
  BRp inputloop   ; Loop if not zero.

LEA R0 TEXT
PUTS
HALT

TEXT: .STRINGZ "Over?"
LETTER: .FILL x6B

.END
