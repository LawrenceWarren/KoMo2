        B main

value   DEFW 0 ; Store the read number here
enterI  DEFB "Please enter your integer: " , 0
newline DEFB "\n" , 0
message DEFB " Enter a series of intigers\n" , 0

        ALIGN
main    ; Enter your code to read in the number here...
        mov R10, #10
        mov R1, #0
        adr R0, enterI
        swi 3
start   swi 1
        cmp R0, #10
        beq continue
        cmp R0, #48
        blt error
        cmp R0, #57
        bgt error
        sub R0, R0, #48
        swi 4
        mla R1, R1, R10, R0

        b start
error   adr R0, message
        swi 3
        b main
        
        ; Prints out the number stored in value
continue  
        str R1, value
        adr R0, newline
        swi 3
        ldr R0, value        
        swi 4
        swi 2