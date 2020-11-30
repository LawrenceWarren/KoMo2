        B main

seed    DEFW    5
magic   DEFW    65539
mask    DEFW    0x7FFFFFFF
newline DEFB    "\n",0

main    ALIGN
        LDR R1, seed
        LDR R2, magic
        LDR R3, mask
        MUL R4, R2, R1
        AND R5, R4, R3
        MOV R0, R5
        SWI 4
        MOV R5, R0
        STR R5, seed
        ADR R0, newline
        SWI 3
        B main