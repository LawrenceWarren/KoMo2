        B readB

enterB  DEFB "Base:   ",0
enterP  DEFB "Power:  ",0
newline DEFB "\n",0
result  DEFB "Result: ",0
error   DEFB "\nPlease only input numeric characters.\n\n",0

        ALIGN
; Reads a base
readB   MOV R10, #10
        MOV R1, #0 ; Base stored in R1
        ADR R0, enterB
        SWI 3

; Checks that the base read is valid
checkB  SWI 1
        CMP R0, #10 ; If enter pressed, proceed
        BEQ between
        CMP R0, #48 ; If not an integer, error
        BLT errorB
        CMP R0, #57 ; if not an integer, error
        BGT errorB
        SUB R0, R0, #48 ; Else keep reading int
        SWI 4
        MLA R1, R1, R10, R0
        B checkB

; Handles the base error state
errorB  ADR R0, error
        SWI 3
        B readB

; Prints a new line between reading base and power
between ADR R0, newline
        SWI 3

; Reads a power
readP   MOV R10, #10
        MOV R2, #0 ; Power stored in R2
        ADR R0, enterP
        SWI 3

; Checks that the power read is valid
checkP  SWI 1
        CMP R0, #10 ; If enter pressed, proceed
        BEQ doMath
        CMP R0, #48 ; If not an integer, error
        BLT errorP
        CMP R0, #57 ; more error
        BGT errorP
        SUB R0, R0, #48 ; Else keep reading int
        SWI 4
        MLA R2, R2, R10, R0
        B checkP

; Handles the power error state
errorP  ADR R0, error
        SWI 3
        B readP

; Begins calculating base-to-power
doMath  ADR R0, newline
        SWI 3
        ADR R0, result
        SWI 3
        MOV R3, #1 ; Result stored in R3

; Loops through calculating power
loop    CMP R2, #1
        BLT exit  ; Exit if finished raising to power
        SUB R2, R2, #1 ; Reduce the power
        MUL R3, R3, R1 ; Perform raising to power
        B loop         

; Print and exit
exit    MOV R0, R3     
        SWI 4
        SWI 2
