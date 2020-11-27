/* JDG Trying to scratch out all "global" definitions                         */
/* Last modified 17/1/2003                                                    */

#define MAX_NO_OF_TRAPS 32 /* In any category */

/* Display columns */
#define ADDRESS_ENTRY 0
#define MIN_HEX_ENTRY 1
#define MAX_HEX_ENTRY 32
#define ASCII_ENTRY 33
#define DIS_ENTRY 34
#define TOTAL_ENTRY 35 /* Maximum number of columns in memory window */

#define CPSR 16 /* defining CPSR and SPSR register numbers */
#define SPSR 17

/* Return error states */
#define eval_okay 0
#define eval_no_operand 1
#define eval_no_operator 2
#define eval_not_closebr 3
#define eval_not_openbr 4
#define eval_no_identifier 5
#define eval_mathstack_limit 6
