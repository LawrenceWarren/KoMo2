        B main

value   DEFW 0 ; Store the read number here
enterI  DEFB "Please enter your integer: ",0
newline DEFB "\n",0
message DEFB " Enter a series of intigers\n",0

        ALIGN
main    ; Enter your code to read in the number here...
        MOV R10, #10
        MOV R1, #0
        ADR R0, enterI
        SWI 3
start   SWI 1
        CMP R0, #10
        BEQ continue
        CMP R0, #48
        BLT error
        CMP R0, #57
        BGT error
        SUB R0, R0, #48
        SWI 4
        MLA R1, R1, R10, R0

        B start
error   ADR R0, message
        SWI 3
        B main
        
        ; Prints out the number stored in value
continue  
        STR R1, value
        ADR R0, newline
        SWI 3
        LDR R0, value        
        SWI 4
        SWI 2