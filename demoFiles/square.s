        B readB

base    DEFW 0
power   DEFW 0
enterB  DEFB "Base: ",0
enterP  DEFB "Power: ",0
newline DEFB "\n",0
error   DEFB "\nPlease enter an integer:\n",0

        ALIGN
readB   MOV R10, #10
        MOV R1, #0
        ADR R0, enterB
        SWI 3

checkB  SWI 1
        CMP R0, #10 ; If enter pressed, proceed
        BEQ readP
        CMP R0, #48 ; If not an integer, error
        
        errorB
        CMP R0, #57 ; more error
        BGT errorB
        SUB R0, R0, #48 ; Else keep reading int
        SWI 4
        MLA R1, R1, R10, R0
        B checkB

errorB  ADR R0, error
        SWI 3
        B readB

readP   MOV R10, #10
        MOV R1, #0
        ADR R0, enterP
        SWI 3
        
checkP  ADR R0, newline
        SWI 3
        SWI 1
        CMP R0, #10 ; If enter pressed, proceed
        BEQ doMath
        CMP R0, #48 ; If not an integer, error
        BLT errorP
        CMP R0, #57 ; more error
        BGT errorP
        SUB R0, R0, #48 ; Else keep reading int
        SWI 4
        MLA R1, R1, R10, R0
        B checkP

errorP  ADR R0, error
        SWI 3
        B readP

doMath  SWI 2