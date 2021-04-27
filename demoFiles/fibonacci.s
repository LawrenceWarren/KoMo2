        B   main

maxF    DEFW    15
enterI  DEFB "Please enter the number of fibonacci numbers to print: ",0
newline DEFB "\n",0
fibbo   DEFB    "Fibonacci number ",0
is      DEFB    " is ",0
end     DEFB    ".\n",0
message DEFB "Enter a series of digits followed by the enter key. \n",0

        ALIGN
main    
        MOV R10, #10
        MOV R6, #0
        ADRL R0, enterI
        SWI 3

start   SWI 1
        CMP R0, #10
        BEQ CONT
        CMP R0, #48
        BLT error
        CMP R0, #57
        BGT error
        SUB R0, R0, #48
        SWI 4
        MLA R6, R6, R10, R0      
        B start

error   ADRL R0, newline
        SWI 3
        ADRL R0, message
        SWI 3
        B main

CONT    ADRL R0, newline
        SWI 3
        ADRL R4, 1
        ADRL R1, 1
        ADRL R2, 0
        ADD R6, R6, #1
        ADRL R0, fibbo
        SWI 3
        MOV R0, R4
        SWI 4
        ADRL R0, is
        SWI 3
        MOV R0, R1
        SWI 4
        ADD R4, R4, #1
        ADRL R0, end
        SWI 3
        CMP R4, R6
        BEQ END

FIBBO   ADRL R0, fibbo
        SWI 3
        MOV R0, R4
        SWI 4
        ADRL R0, is
        SWI 3
        ADD R0, R1, R2
        SWI 4
        MOV R2, R1
        MOV R1, R0
        ADD R4, R4, #1
        CMP R4, R6
        ADRL R0, end
        SWI 3
        BNE FIBBO

END     SWI 2 