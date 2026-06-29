;file to test assembler error handling
.ORIG x3000

BR #1
TEST1 ADD R0 R0 #1
TEST2 ADD R0 R0 #1
TEST3 ADD R0 R0 #1
TEST4 ADD R0 R0 #1
TEST5 ADD R0 R0 #1

.END
