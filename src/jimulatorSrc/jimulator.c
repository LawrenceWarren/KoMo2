/******************************************************************************/
/*                Name:           jimulator.c                                 */
/*                Version:        1.5.1                                       */
/*                Date:           5/08/2008                                   */
/*                Emulation library for KMD                                   */
/*                                                                            */
/*============================================================================*/
#include <stdio.h>
#include <time.h>
#include <sys/poll.h>
#include <unistd.h>
#include <stdlib.h>

#include "definitions.h"
#include "interface.h"

#define NO_OF_BREAKPOINTS     32  /* Max 32 */
#define NO_OF_WATCHPOINTS      4  /* Max 32 */


/*  uses JDG's arm_v3
 
 notes:
 
 random changed to rand to allow linking
 N.B. check rand as may not produce 32-bit no.s
 
 
 needs:
 
 1. long multiply  - drafted BUT NOT TESTED
 3. more thumb testing
 
 lsl routine added but not fully tested (28/10/98)
 Long multiplication written (27/1/00)
 Architecture V5 added (2/2/00)
 
 To do:
 check long muls
 Flag checking (immediates ?!)
 Validation
 interrupt enable behaviour on exceptions (etc.)
 */


/* NB "int" is assumed to be at least 32-bit */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define RING_BUF_SIZE 64

typedef struct
{
	unsigned int iHead;
	unsigned int iTail;
	unsigned char buffer[RING_BUF_SIZE];
}
ring_buffer;


struct pollfd pollfd;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/* Local prototypes */

void step(void);
void comm(struct pollfd*);

int emulsetup (void);
void save_state(unsigned char new_status);
void initialise (unsigned int start_address, int initial_mode);
void execute (unsigned int op_code);

/* ARM execute */
void data_op (unsigned int op_code);
void clz (unsigned int op_code);
void transfer (unsigned int op_code);
void transfer_sbhw (unsigned int op_code);
void multiple (unsigned int op_code);
void branch (unsigned int op_code);
void coprocessor (unsigned int op_code);
void my_system (unsigned int op_code);
void undefined ();
void breakpoint ();

void mrs (unsigned int op_code);
void msr (unsigned int op_code);
void bx (unsigned int Rm, int link);
void my_multi (unsigned int op_code);
void swap (unsigned int op_code);
void normal_data_op (unsigned int op_code, int operation);
void ldm (int mode, int Rn, int reg_list, int write_back, int hat);
void stm (int mode, int Rn, int reg_list, int write_back, int hat);

int transfer_offset (int op2, int add, int imm, boolean sbhw);

int b_reg (int op2, int *cf);
int b_immediate (int op2, int *cf);

int bit_count (unsigned int source, int *first);

int check_cc (int condition);

int not (int x);
int and (int x, int y);
int or (int x, int y);
int xor (int x, int y);
int zf (int cpsr);
int cf (int cpsr);
int nf (int cpsr);
int vf (int cpsr);

void set_flags (int operation, int a, int b, int rd, int carry);
void set_NZ (unsigned int value);
void set_CF (unsigned int a, unsigned int rd, int carry);
void set_VF_ADD (int a, int b, int rd);
void set_VF_SUB (int a, int b, int rd);
int get_reg (int reg_no, int force_mode);
/* Returns PC+4 for ARM & PC+2 for Thumb */
int get_reg_monitor (int reg_no, int force_mode);
void put_reg (int reg_no, int value, int force_mode);
int instruction_length ();

unsigned int fetch ();
void inc_pc ();
void endian_swap (unsigned int start, unsigned int end);
int read_mem (unsigned int address, int size, boolean sign, boolean T, int source);
void write_mem (unsigned int address, int data, int size, boolean T, int source);


/* THUMB execute */
void data0 (unsigned int op_code);
void data1 (unsigned int op_code);
void data_transfer (unsigned int op_code);
void transfer0 (unsigned int op_code);
void transfer1 (unsigned int op_code);
void sp_pc (unsigned int op_code);
void lsm_b (unsigned int op_code);
void thumb_branch (unsigned int op_code);


int load_fpe ();
void fpe_install ();

int check_watchpoints(unsigned int address, int data, int size, int direction);

int get_number (char *ptr);
int lsl (int value, int distance, int *cf);
int lsr (unsigned int value, int distance, int *cf);
int asr (int value, int distance, int *cf);
int ror (unsigned int value, int distance, int *cf);

unsigned int getmem32 (int number);
void setmem32 (int number, unsigned int reg);
void execute_instruction (void);

int emul_getchar (unsigned char *to_get);
int emul_sendchar (unsigned char to_send);
int emul_sendbN (int value, int N);
int emul_getbN(int *val_ptr, int N);
int emul_getchararray (int char_number, unsigned char *data_ptr);
int emul_sendchararray (int char_number, unsigned char *data_ptr);

void boardreset (void);

void init_buffer(ring_buffer*);
int count_buffer(ring_buffer*);
int put_buffer(ring_buffer*, unsigned char);
int get_buffer(ring_buffer*, unsigned char*);

/*----------------------------------------------------------------------------*/

#define mem_size     0X100000                                      /* 4Mbytes */
#define RAMSIZE      0X100000                                      /* 4Mbytes */
// Why add "RAMSIZE", and then get it wrong?!?!  @@@
// Memory is modulo this to the monitor; excise and use the proper routines @@@


#define reserved_mem 0X002000                                     /* 32Kbytes */
#define user_stack   (mem_size - reserved_mem) << 2
#define start_string_addr 0X00007000                            /*ARM address */

#define max_instructions 10000000

//  const int   FALSE = 0;
//  const int   TRUE = -1;

const unsigned int nf_mask = 0X80000000;
const unsigned int zf_mask = 0X40000000;
const unsigned int cf_mask = 0X20000000;
const unsigned int vf_mask = 0X10000000;
const unsigned int if_mask = 0X00000080;
const unsigned int ff_mask = 0X00000040;
const unsigned int mode_mask = 0X0000001F;
const unsigned int tf_mask = 0X00000020;                         /* THUMB bit */

const unsigned int bit_31 = 0X80000000;
const unsigned int bit_0 = 0X00000001;

const unsigned int imm_mask = 0X02000000;            /* orginal word versions */
const unsigned int imm_hw_mask = 0X00400000;            /* half word versions */
const unsigned int data_op_mask = 0X01E00000;            /* ALU function code */
const unsigned int data_ext_mask = 0X01900000;    /* To sort out CMP from MRS */
const unsigned int arith_ext = 0X01000000;      /* Poss. arithmetic extension */
const unsigned int s_mask = 0X00100000;
const unsigned int rn_mask = 0X000F0000;
const unsigned int rd_mask = 0X0000F000;
const unsigned int rs_mask = 0X00000F00;
const unsigned int rm_mask = 0X0000000F;
const unsigned int op2_mask = 0X00000FFF;
const unsigned int hw_mask = 0X00000020;
const unsigned int sign_mask = 0X00000040;

const unsigned int mul_mask = 0X0FC000F0;
const unsigned int long_mul_mask = 0X0F8000F0;
const unsigned int mul_op = 0X00000090;
const unsigned int long_mul_op = 0X00800090;
const unsigned int mul_acc_bit = 0X00200000;
const unsigned int mul_sign_bit = 0X00400000;
const unsigned int mul_long_bit = 0X00800000;

const unsigned int sbhw_mask = 0X0E000FF0;

const unsigned int swp_mask = 0X0FB00FF0;
const unsigned int swp_op = 0X01000090;

const unsigned int pre_mask = 0X01000000;
const unsigned int up_mask = 0X00800000;
const unsigned int byte_mask = 0X00400000;
const unsigned int write_back_mask = 0X00200000;
const unsigned int load_mask = 0X00100000;
const unsigned int byte_sign = 0X00000080;
const unsigned int hw_sign = 0X00008000;

const unsigned int user_mask = 0X00400000;

const unsigned int link_mask = 0X01000000;
const unsigned int branch_field = 0X00FFFFFF;
const unsigned int branch_sign = 0X00800000;

const unsigned int undef_mask = 0X0E000010;
const unsigned int undef_code = 0X06000010;

const int mem_system = 0;                          /* sources for memory read */
const int mem_instruction = 1;
const int mem_data = 2;

const int flag_add = 1;
const int flag_sub = 2;

#define user_mode   0X00000010
#define fiq_mode    0X00000011
#define irq_mode    0X00000012
#define sup_mode    0X00000013
#define abt_mode    0X00000017
#define undef_mode  0X0000001B
#define system_mode 0X0000001F

#define reg_current 0  /* Values to force register accesses to specified bank */
#define reg_user    1                                        /* ... or system */
#define reg_svc     2
#define reg_fiq     3
#define reg_irq     4
#define reg_abt     5
#define reg_undef   6

/*----------------------------------------------------------------------------*/

#define REGSIZE 65536
#define uchar unsigned char


typedef struct
{
	int state;
	char cond;
	char size;
	int addra;
	int addrb;
	int dataa[2];
	int datab[2];
}
BreakElement;

/*----------------------------------------------------------------------------*/
/* Global data */

#define WOTLEN_FEATURES 1
#define WOTLEN_MEM_SEGS 1
#define WOTLEN (8 + 3 * WOTLEN_FEATURES + 8 * WOTLEN_MEM_SEGS)

uchar wotrustring[] = { WOTLEN - 1,          /* Length of rest of record HERE */
	(WOTLEN-3)&0xFF, ((WOTLEN-3)>>8)&0xFF,     /* Length of rest of message (H) */
	1, 0, 0,                                           /* Processor type (B, H) */
	WOTLEN_FEATURES,                                       /* Feature count (B) */
	0, 9, 0,                                               /* Feature ID (B, H) */
	WOTLEN_MEM_SEGS,                                /* Memory segment count (B) */
	0x00, 0x00, 0x00, 0x00,                       /* Memory segment address (W) */
	mem_size & 0xFF,         (mem_size >>  8) & 0xFF,    /* Memory segment      */
	(mem_size >> 16) & 0xFF, (mem_size >> 24) & 0xFF};           /*  length (W) */

BreakElement breakpoints[NO_OF_BREAKPOINTS];
BreakElement watchpoints[NO_OF_WATCHPOINTS];

unsigned int emul_bp_flag[2];
unsigned int emul_wp_flag[2];

uchar memory[RAMSIZE];       // @@@

uchar status, old_status;
unsigned int steps_togo;/*Number of left steps before halting (0 is infinite) */
unsigned int steps_reset;                 /* Number of steps since last reset */
char runflags;
uchar rtf;
boolean breakpoint_enable;                     /* Breakpoints will be checked */
boolean breakpoint_enabled;                /* Breakpoints will be checked now */
boolean run_through_BL;                          /* Treat BL as a single step */
boolean run_through_SWI;                        /* Treat SWI as a single step */

unsigned int tube_address;

int r[16];
int fiq_r[7];
int irq_r[2];
int sup_r[2];
int abt_r[2];
int undef_r[2];
unsigned int cpsr;
unsigned int spsr[32];          /* Lots of wasted space - safe for any "mode" */

boolean print_out;
int run_until_PC, run_until_SP, run_until_mode;     /* Used to determine when */
uchar run_until_status;      /*   to finish a `stepped' subroutine, SWI, etc. */

unsigned int exception_para[9];

int next_file_handle;
FILE *(file_handle[20]);

int count;

unsigned int last_addr;

int glob1, glob2;

int past_opc_addr[32];        /* History buffer of fetched op. code addresses */
int past_size;                                         /* Used size of buffer */
int past_opc_ptr;                                        /* Pointer into same */
int past_count;                       /* Count of hits in instruction history */

/* Thumb stuff */
int PC;
int BL_prefix, BL_address;
int next_char;
int ARM_flag;

struct pollfd *SWI_poll;    /* Pointer to allow SWIs to scan input - YUK! @@@ */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

//  int emul_bp_flag[2];
//  int emul_wp_flag[2];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* terminal data structures                                                   */

ring_buffer terminal0_Tx, terminal0_Rx;
ring_buffer terminal1_Tx, terminal1_Rx;
ring_buffer *terminal_table[16][2];			// @@@

/*----------------------------------------------------------------------------*/
/* Entry point                                                                */

int main()
{
	
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	
	{
		int i;
		
		for (i = 0; i < 16; i++)				// @@
		{
			terminal_table[i][0] = NULL;
			terminal_table[i][1] = NULL;
		}

		init_buffer(&terminal0_Tx);                            /* Initialise terminal */
		init_buffer(&terminal0_Rx);
		terminal_table[0][0] = &terminal0_Tx;
		terminal_table[0][1] = &terminal0_Rx;
		init_buffer(&terminal1_Tx);                            /* Initialise terminal */
		init_buffer(&terminal1_Rx);
		terminal_table[1][0] = &terminal1_Tx;
		terminal_table[1][1] = &terminal1_Rx;
	}
	
	pollfd.fd = 0;
	pollfd.events = POLLIN;
	SWI_poll = &pollfd;		// Grubby pass to "my_system" @@@
	
	emulsetup();
	
	/* These can cause compiler warnings; C does not understand shifts properly.  */
	/* They are safe.                                                             */
	//emul_bp_flag[0] = 0; emul_bp_flag[1] = (1 << NO_OF_BREAKPOINTS) - 1;
	//emul_wp_flag[0] = 0; emul_wp_flag[1] = (1 << NO_OF_WATCHPOINTS) - 1;
	emul_bp_flag[0] = 0;
	if(NO_OF_BREAKPOINTS == 0)
	{
		emul_bp_flag[1] = 0x00000000;    /* C work around */
	}
	else
	{
		emul_bp_flag[1] = ((1 << (NO_OF_BREAKPOINTS - 1)) << 1) - 1;
	}
	
	emul_wp_flag[0] = 0;
	if(NO_OF_WATCHPOINTS == 0)
	{
		emul_wp_flag[1] = 0x00000000;    /* C work around */
	}
	else
	{
		emul_wp_flag[1] = ((1 << (NO_OF_WATCHPOINTS - 1)) << 1) - 1;
	}
	
	while(TRUE)                                                     /* Main loop */
	{
		comm(&pollfd);                                 /* Check for monitor command */
		if((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_RUNNING)
		{
			step();                                      /* Step emulator as required */
		}
		else
		{
			poll(&pollfd, 1, -1);/* If not running, deschedule until command arrives */
		}
	}
}

/*----------------------------------------------------------------------------*/

void step(void)
{
	old_status = status;
	execute_instruction();
	
	if((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_RUNNING)
	{
		/* Still running - i.e. no breakpoint (etc.) found */

		if(status == CLIENT_STATE_RUNNING_SWI)      // OR _BL  @@@
		{                                /* Don't count the instructions from now */
			if((get_reg_monitor(15, reg_current) == run_until_PC)
			&& (get_reg_monitor(13, reg_current) == run_until_SP)
			&&((get_reg_monitor(16, reg_current) & 0x3F) == run_until_mode))
			{
				status = run_until_status;
			}
		}                 /* This can have changed status - hence no "else" below */
		
		if(status != CLIENT_STATE_RUNNING_SWI)      // OR _BL  @@@
		{
			/* Count steps unless inside routine */
			steps_reset++;
			if(steps_togo > 0)                                           /* Stepping */
			{
				steps_togo--;                  /* If -decremented- to reach zero, stop. */
				if(steps_togo == 0)
				{
					status = CLIENT_STATE_STOPPED;
				}
			}
		}                                              /* Running a whole routine */
	}
	
	if((status & CLIENT_STATE_CLASS_MASK) != CLIENT_STATE_CLASS_RUNNING)
	{
		breakpoint_enabled = FALSE;         /* No longer running - allow "continue" */
	}
	
	return;
}

/*----------------------------------------------------------------------------*/

void comm(struct pollfd *pPollfd)
{
	
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	
	void monitor_options_misc(uchar command)
	{
		uchar tempchar;
		int temp;
		switch (command & 0x3F)
		{
			case BR_NOP:
				break;
			case BR_PING:
				write(1, "OK00", 4);
				break;
			case BR_WOT_R_U:
				emul_sendchararray (wotrustring[0], &wotrustring[1]);
				/* wotrustring[0] holds length, followed by message */
				break;
				
			case BR_RESET:
				boardreset();
				break;
				
			case BR_RTF_GET:
				emul_sendchar(rtf);
				break;
				
			case BR_RTF_SET:
				emul_getchar (&rtf);
				break;
				
			case BR_WOT_U_DO:
				emul_sendchar (status);
				emul_sendbN (steps_togo, 4);
				emul_sendbN (steps_reset, 4);
				break;
				
			case BR_PAUSE:
			case BR_STOP:
				if ((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_RUNNING)
				{
					old_status = status;
					status = CLIENT_STATE_STOPPED;
				}
				break;
				
			case BR_CONTINUE:
				if (((status & CLIENT_STATE_CLASS_MASK) == CLIENT_STATE_CLASS_STOPPED)
					&& (status != CLIENT_STATE_BYPROG))    // Maybe others @@@
				/* Only act if already stopped */
					if ((old_status = CLIENT_STATE_STEPPING) || (steps_togo != 0))
						status = old_status;
				break;
				
			case BR_BP_GET:
				emul_sendbN (emul_bp_flag[0], 4);
				emul_sendbN (emul_bp_flag[1], 4);
				break;
				
			case BR_BP_SET:
			{
				int data[2];
				emul_getbN (&data[0], 4);
				emul_getbN (&data[1], 4);
				/* Note ordering to avoid temporary variable */
				emul_bp_flag[1] = (~emul_bp_flag[0] & emul_bp_flag[1])
				| (emul_bp_flag[0] & ((emul_bp_flag[1] & ~data[0]) | data[1]));
				emul_bp_flag[0] = emul_bp_flag[0] & (data[0] | ~data[1]);
			}
				break;
				
			case BR_BP_READ:
				emul_getchar(&tempchar);
				temp = tempchar;
				emul_sendchar(breakpoints[temp].cond);
				emul_sendchar(breakpoints[temp].size);
				emul_sendbN(breakpoints[temp].addra, 4);
				emul_sendbN(breakpoints[temp].addrb, 4);
				emul_sendbN(breakpoints[temp].dataa[0], 4);
				emul_sendbN(breakpoints[temp].dataa[1], 4);
				emul_sendbN(breakpoints[temp].datab[0], 4);
				emul_sendbN(breakpoints[temp].datab[1], 4);
				break;
				
   case BR_BP_WRITE:
				emul_getchar(&tempchar);
				temp = tempchar;
				emul_getchar(&breakpoints[temp].cond);
				emul_getchar(&breakpoints[temp].size);
				emul_getbN(&breakpoints[temp].addra, 4);
				emul_getbN(&breakpoints[temp].addrb, 4);
				emul_getbN(&breakpoints[temp].dataa[0], 4);
				emul_getbN(&breakpoints[temp].dataa[1], 4);
				emul_getbN(&breakpoints[temp].datab[0], 4);
				emul_getbN(&breakpoints[temp].datab[1], 4);
				/* add breakpoint */
				temp = (1 << temp) & ~emul_bp_flag[0];
				emul_bp_flag[0] |= temp;
				emul_bp_flag[1] |= temp;
				break;
				
			case BR_WP_GET:
				emul_sendbN(emul_wp_flag[0], 4);
				emul_sendbN(emul_wp_flag[1], 4);
				break;
				
			case BR_WP_SET:
			{
				int data[2];
				emul_getbN(&data[0], 4);
				emul_getbN(&data[1], 4);
				temp = data[1] & ~data[0];
				emul_wp_flag[0] &= ~temp;
				emul_wp_flag[1] |= temp;
				temp = data[0] & emul_wp_flag[0];
				emul_wp_flag[1] = (emul_wp_flag[1] & ~temp) | (data[1] & temp);
			}
				break;
				
			case BR_WP_READ:
				emul_getchar(&tempchar);
				temp = tempchar;
				emul_sendchar(watchpoints[temp].cond);
				emul_sendchar(watchpoints[temp].size);
				emul_sendbN(watchpoints[temp].addra, 4);
				emul_sendbN(watchpoints[temp].addrb, 4);
				emul_sendbN(watchpoints[temp].dataa[0], 4);
				emul_sendbN(watchpoints[temp].dataa[1], 4);
				emul_sendbN(watchpoints[temp].datab[0], 4);
				emul_sendbN(watchpoints[temp].datab[1], 4);
				break;
				
			case BR_WP_WRITE:
				emul_getchar(&tempchar);
				temp = tempchar;
				emul_getchar(&watchpoints[temp].cond);
				emul_getchar(&watchpoints[temp].size);
				emul_getbN(&watchpoints[temp].addra, 4);
				emul_getbN(&watchpoints[temp].addrb, 4);
				emul_getbN(&watchpoints[temp].dataa[0], 4);
				emul_getbN(&watchpoints[temp].dataa[1], 4);
				emul_getbN(&watchpoints[temp].datab[0], 4);
				emul_getbN(&watchpoints[temp].datab[1], 4);
				temp = 1 << temp & ~emul_wp_flag[0];
				emul_wp_flag[0] |= temp;
				emul_wp_flag[1] |= temp;
				break;
				
			case BR_FR_WRITE:
			{
				uchar device, length;
				ring_buffer *pBuff;
				
				emul_getchar(&device);
				pBuff = terminal_table[device][1];
				emul_getchar(&length);
				temp = tempchar;
				while (length-- > 0)
				{
					emul_getchar(&tempchar);                            /* Read character */
					if (pBuff != NULL) put_buffer(pBuff, tempchar); /*  and put in buffer */
				}
				emul_sendchar(0);
			}
				break;
				
			case BR_FR_READ:
			{
				unsigned char device, max_length;
				unsigned int  i, length, available;
				ring_buffer *pBuff;
				
				emul_getchar(&device);
				pBuff = terminal_table[device][0];
				emul_getchar(&max_length);
				available = count_buffer(&terminal0_Tx);  /* See how many chars we have */
				if (pBuff == NULL) length = 0;       /* Kill if no corresponding buffer */
				else length = MIN(available, max_length);  /* else clip to message max. */
				emul_sendchar(length);
				for (i = 0; i < length; i++)            /* Send zero or more characters */
				{
					unsigned char c;
					get_buffer(pBuff, &c);
					emul_sendchar(c);
				}
			}
				break;
				
			default: break;
		}
  return;
	}
	
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	
	void monitor_memory(uchar c)
	{
		int addr;
		char *pointer;
		int size;
		
		emul_getbN(&addr, 4);                             /* Start address really */
		if ((c & 0x30) == 0x10)
		{
			unsigned int temp;
			int reg_bank, reg_number;
			
			switch (addr & 0xE0)
			{
				case 0x00: reg_bank = reg_current; break;
				case 0x20: reg_bank = reg_user;    break;
				case 0x40: reg_bank = reg_svc;     break;
				case 0x60: reg_bank = reg_abt;     break;
				case 0x80: reg_bank = reg_undef;   break;
				case 0xA0: reg_bank = reg_irq;     break;
				case 0xC0: reg_bank = reg_fiq;     break;
				default:   reg_bank = reg_current; break;
			}
			reg_number = addr & 0x1F;
			
			emul_getbN(&size, 2);                             /* Length of transfer */
			
			while (size--)
				if ((c & 8) != 0)
					emul_sendbN(get_reg_monitor(reg_number++, reg_bank), 4);
				else
				{
					emul_getbN(&temp, 4);
					put_reg(reg_number++, temp, reg_bank);
				}
		}
		else
		{
			pointer = memory + (addr & (RAMSIZE - 1));   // @@@ @@@
			emul_getbN (&size, 2);
			size *= 1 << (c & 7);
			if (((uchar *) pointer + size) > ((uchar *) memory + RAMSIZE))
				pointer -= RAMSIZE;
			if (c & 8) emul_sendchararray(size, pointer);
			else        emul_getchararray(size, pointer);
		}
		return;
	}
	
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	
	void monitor_breakpoints(uchar c)
	{
		runflags = c & 0x3F;
		breakpoint_enable  = (runflags & 0x10) != 0;
		breakpoint_enabled = (runflags & 0x01) != 0;       /* Break straight away */
		run_through_BL     = (runflags & 0x02) != 0;
		run_through_SWI    = (runflags & 0x04) != 0;
		emul_getbN(&steps_togo, 4);
		if (steps_togo == 0) status = CLIENT_STATE_RUNNING;
		else                 status = CLIENT_STATE_STEPPING;
		return;
	}
	
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	
	uchar c;
	
	if(poll(pPollfd, 1, 0) > 0)
	{
		read(0, &c, 1);		// Look at error return - find EOF & exit @@@
		switch(c & 0xC0)
		{
			case 0x00:
				monitor_options_misc(c);
				break;
			case 0x40:
				monitor_memory(c);
				break;
			case 0x80:
				monitor_breakpoints(c);
				break;
			case 0xC0:
				break;
		}
	}
	return;
}

/******************************************************************************/

int emul_getchar(unsigned char *to_get)          /* Get 1 character from host */
{ return emul_getchararray(1, to_get); }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int emul_sendchar(unsigned char to_send)          /* Send 1 character to host */
{ return emul_sendchararray(1, &to_send); }

/******************************************************************************/

/******************************************************************************/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Sends N bytes from the supplied value to the host (??), LSB first.         */
/* Returns the number of bytes believed received successfully (i.e. N=>"Ok")  */

int emul_sendbN(int value, int N)
{
	char buffer[MAX_SERIAL_WORD];
	int i;
	
	if (N > MAX_SERIAL_WORD) N = MAX_SERIAL_WORD;       /* Clip, just in case ... */
	
	for (i = 0; i < N; i++)
	{
  buffer[i] = value & 0xFF;                               /* Byte into buffer */
  value = value >> 8;                                        /* Get next byte */
	}
	
	return emul_sendchararray(N, buffer);
}

/*                                                                            */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/******************************************************************************/



/******************************************************************************/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Gets N bytes from the host (??) into the indicated val_ptr, LSB first.     */
/* Returns the number of bytes received successfully (i.e. N=>"Ok")           */
/* If error suspected sets `board_version' to not present                     */

int emul_getbN(int *val_ptr, int N)
{
	char buffer[MAX_SERIAL_WORD];
	int i, No_received;
	
	if(N > MAX_SERIAL_WORD)
	{
		N = MAX_SERIAL_WORD;       /* Clip, just in case ... */
	}

	No_received = emul_getchararray (N, buffer);
	
	*val_ptr = 0;
	
	for (i = 0; i < No_received; i++)
	{
		*val_ptr = *val_ptr | ((buffer[i] & 0xFF) << (i * 8));  /* Assemble integer */
	}
	
	if(No_received != N)
	{
		board_version = -1;         /* Really do this here? @@@ */
	}
	
	return No_received;
}

/*                                                                            */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/******************************************************************************/


/******************************************************************************/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* reads a character array from buffer                                        */
/* sends char_number number of characters given by data_ptr.                  */
/* returns number of bytes received                                           */
/*                                                                            */
/*                                                                            */

int emul_getchararray(int char_number, unsigned char *data_ptr)
{
	int ret = char_number;
	int replycount = 0;
	struct pollfd pollfd;
	
	pollfd.fd = 0;
	pollfd.events = POLLIN;
	
	while (char_number)
	{
  if (!poll (&pollfd, 1, -1)) return ret - char_number;
  replycount = read (0, data_ptr, char_number);
  if (replycount < 0) replycount = 0;
  char_number -= replycount;
  data_ptr += replycount;
	}
	return ret;
}

/*                                                                            */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/******************************************************************************/


/******************************************************************************/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* writes an array of bytes in the buffer                                     */
/* sends char_number - number of bytes given by data_ptr                      */
/* data pointer points to the beginning of the sequence to be sent            */
/*                                                                            */
/*                                                                            */

int emul_sendchararray (int char_number, unsigned char *data_ptr)
{
	write(1, data_ptr, char_number);
	return char_number;                           /* send char array to the board */
}

/*                                                                            */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/******************************************************************************/

int emulsetup(void)
{
	int initial_mode;
	
	glob1 = 0;
	glob2 = 0;
	
	{
		int i;

		for (i = 0; i < 32; i++)
		{
			past_opc_addr[i] = 1;  /* Illegal op. code address */
		}
		past_opc_ptr = 0;
		past_count = 0;
		past_size = 4;
	}
	
	initial_mode = 0xC0 | sup_mode;
	print_out = FALSE;
	
	next_file_handle = 1;
	
	initialise(0, initial_mode);
	
	return 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void execute_instruction(void)
{
	unsigned int instr_addr, instr;
	int i;
	
	/* - - - - - - - - - - - - - - breakpoints - - - - - - - - - - - - - - - - - -*/
	
	/* Breakpoints - ignoring lots of things yet @@@              */
	/* Needs amalgamating with watchpoint checker */
	
	int check_breakpoint(unsigned int instr_addr, unsigned int instr)
	{
		boolean may_break;
			
		for (i = 0, may_break = FALSE; (i < NO_OF_BREAKPOINTS) && !may_break; i++)
		{
			/* Search breakpoints */
			may_break = ((emul_bp_flag[0] & emul_bp_flag[1] & (1<<i)) != 0);
			/* Breakpoint is active */
		  
			if(may_break)                                 /* Try address comparison */
			{
				switch (breakpoints[i].cond & 0x0C)
				{
					case 0x00:
						may_break = FALSE;
						break;
					case 0x04:
						may_break = FALSE;
						break;
					case 0x08:                 /* Case of between address A and address B */
						if ((instr_addr < breakpoints[i].addra) || (instr_addr > breakpoints[i].addrb))
						{
							may_break = FALSE;
						}
						break;
				  
					case 0x0C:                                            /* case of mask */
						if ((instr_addr & breakpoints[i].addrb) != breakpoints[i].addra)
						{
							may_break = FALSE;
						}
						break;
				}
			}
			
			if(may_break)                                     /* Try data comparison */
			{
				switch (breakpoints[i].cond & 0x03)
				{
					case 0x00:
						may_break = FALSE;
						break;
			  
					case 0x01:
						may_break = FALSE;
						break;
			  
					case 0x02:                         /* Case of between data A and data B */
						if((instr < breakpoints[i].dataa[0]) || (instr > breakpoints[i].datab[0]))
						{
							may_break = FALSE;
						}
						break;

					case 0x03:                                              /* Case of mask */
						if((instr & breakpoints[i].datab[0]) != breakpoints[i].dataa[0])
						{
							may_break = FALSE;
						}
						break;
				}
			}
		  
		  if (may_break)
		  {
			  // Expansion space for more comparisons @@@
			  //      fprintf(stderr, "BREAKING  %08X  %08X\n", instr_addr, instr);
		  }
		}                                                      /* End of for loop */
		return may_break;
	}
	
	/* - - - - - - - - - - - - - end breakpoint - - - - - - - - - - - - - - - - - */
	
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
	
	instr_addr = get_reg(15, reg_current) - instruction_length();
	last_addr  = get_reg(15, reg_current) - instruction_length();
	
	/* FETCH */
	instr = fetch();

	/* Check ID breakpoints enabled OR _BL  @@@ */
	if((breakpoint_enabled) && (status != CLIENT_STATE_RUNNING_SWI))
	{
		/* Don't look for breakpoints inside single stepped call ... correct ???? @@@ */
		if (check_breakpoint(instr_addr, instr))
		{
			status = CLIENT_STATE_BREAKPOINT;
			// YUK @@@
			return;                            /* and return the appropriate status */
		}
	}
	breakpoint_enabled = breakpoint_enable;    /* More likely after first fetch */
	
	/* BL instruction */
	if(((instr & 0x0F000000) == 0x0B000000) && run_through_BL)
	{
		save_state(CLIENT_STATE_RUNNING_BL);
	}
	else
	{
		if (((instr & 0x0F000000) == 0x0F000000) && run_through_SWI)
		{
			save_state(CLIENT_STATE_RUNNING_SWI);
		}
	}
	

	/* Execute */
	execute(instr);
	
	return;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Save state for leaving "procedure" {PC, SP, Mode, current state} */

void save_state(uchar new_status)
{
	run_until_PC     = get_reg(15, reg_current);/* Incremented once: correct here */
	run_until_SP     = get_reg(13, reg_current);
	run_until_mode   = get_reg(16, reg_current) & 0x3F;     /* Just the mode bits */
	run_until_status = status;
	
	status = new_status;
	
	return;
}

/*----------------------------------------------------------------------------*/

void boardreset(void)
{
	steps_reset = 0;
	initialise(0, sup_mode);
	return;
}

void initialise(unsigned int start_address, int initial_mode)
{
	cpsr = 0X000000C0 | initial_mode;                       /* Disable interrupts */
	r[15] = start_address;
	old_status = CLIENT_STATE_RESET;
	status = CLIENT_STATE_RESET;
	
	return;
}

/*----------------------------------------------------------------------------*/

void execute(unsigned int op_code)
{
	inc_pc();                                           /* Easier here than later */

	/* ARM or THUMB ? */
	if ((cpsr & tf_mask) != 0)                                           /* Thumb */
	{
		op_code = op_code & 0XFFFF;                              /* 16-bit op. code */
		switch (op_code & 0XE000)
		{
			case 0X0000: data0(op_code);         break;
			case 0X2000: data1(op_code);         break;
			case 0X4000: data_transfer(op_code); break;
			case 0X6000: transfer0(op_code);     break;
			case 0X8000: transfer1(op_code);     break;
			case 0XA000: sp_pc(op_code);         break;
			case 0XC000: lsm_b(op_code);         break;
			case 0XE000: thumb_branch(op_code);  break;
		}
	}
	else
	{
		 /* Check condition */
		if ((check_cc(op_code >> 28) == TRUE) || ((op_code & 0XFE000000) == 0XFA000000))      /* Nasty non-orthogonal BLX */
		{
			switch ((op_code >> 25) & 0X00000007)
			{
				case 0X0: data_op(op_code);     break;   /* includes load/store hw & sb */
				case 0X1: data_op(op_code);     break;       /* data processing & MSR # */
				case 0X2: transfer(op_code);    break;
				case 0X3: transfer(op_code);    break;
				case 0X4: multiple(op_code);    break;
				case 0X5: branch(op_code);      break;
				case 0X6: coprocessor(op_code); break;
				case 0X7: my_system(op_code);   break;
			}
		}
	}
	return;
}

/*----------------------------------------------------------------------------*/

int is_it_sbhw(unsigned int op_code)
{
	if (((op_code & 0X0E000090) == 0X00000090) && ((op_code & 0X00000060)
												   != 0X00000000)        /* No multiplies */
		&& ((op_code & 0X00100040) != 0X00000040))          /* No signed stores */
	{
  if (((op_code & 0X00400000) != 0) || ((op_code & 0X00000F00) == 0))
	  return TRUE;
  else
	  return FALSE;
	}
	else
  return FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void data_op(unsigned int op_code)
{
	int operation;
	
	if (((op_code & mul_mask) == mul_op) || ((op_code & long_mul_mask) == long_mul_op))
	{
		my_multi(op_code);
	}
	else if (is_it_sbhw (op_code) == TRUE)
	{
		transfer_sbhw(op_code);
	}
	else if ((op_code & swp_mask) == swp_op)
	{
		swap(op_code);
	}
	else
	{
		operation = (op_code & data_op_mask) >> 21;
		
		/* TST, TEQ, CMP, CMN - all lie in following range, but have S set */
		if((op_code & data_ext_mask) == arith_ext)          /* PSR transfers OR BX */
		{
			if ((op_code & 0X0FBF0FFF) == 0X010F0000)
			{
				mrs(op_code);            /* MRS */
			}
			else if (((op_code & 0X0DB6F000) == 0X0120F000) && ((op_code & 0X02000010) != 0X00000010))
			{
				msr(op_code);     /* MSR */
			}
			else if((op_code & 0X0FFFFFD0) == 0X012FFF10)                  /* BX/BLX */
			{
				bx (op_code & rm_mask, op_code & 0X00000020);
			}
			else if ((op_code & 0XFFF000F0) == 0XE1200070)
			{
				breakpoint();/* Breakpoint */
			}
			else if ((op_code & 0X0FFF0FF0) == 0X016F0F10)
			{
				clz(op_code);       /* CLZ */
			}
			else
			{
				undefined();
			}
		}
		else
		{
			normal_data_op(op_code, operation);     /* All data processing operations */
		}
	}
	
	return;
}

/*----------------------------------------------------------------------------*/

void transfer_sbhw (unsigned int op_code)
{
	unsigned int address;
	int size;
	int offset, rd;
	boolean sign;
	
	switch (op_code & 0X00000060)
	{
  case 0X00:
			fprintf(stderr, "Multiply shouldn't be here!\n");
			break;                        /* Error! */
  case 0X20: size = 2; sign = FALSE; break;                              /* H */
  case 0X40: size = 1; sign = TRUE;  break;                             /* SB */
  case 0X60: size = 2; sign = TRUE;  break;                             /* SH */
	}
	
	rd = ((op_code & rd_mask) >> 12);
	
	address = get_reg (((op_code & rn_mask) >> 16), reg_current);
	
	offset = transfer_offset (op_code & op2_mask, op_code & up_mask,
							  op_code & imm_hw_mask, TRUE);
	
	if ((op_code & pre_mask) != 0) address = address + offset;       /* pre-index */
	
	if ((op_code & load_mask) == 0)                                      /* store */
  write_mem(address, get_reg(rd, reg_current), size, FALSE, mem_data);
	else                                                                  /* load */
  put_reg(rd, read_mem(address, size, sign, FALSE, mem_data), reg_current);
	/* post index */
	
	if ((op_code & pre_mask) == 0)                   /* post index with writeback */
  put_reg((op_code & rn_mask) >> 16, address + offset, reg_current);
	else if ((op_code & write_back_mask) != 0)
  put_reg((op_code & rn_mask) >> 16, address, reg_current);
	
	return;
}

/*----------------------------------------------------------------------------*/

void mrs(unsigned int op_code)
{
	if ((op_code & 0X00400000) == 0)
  put_reg((op_code & rd_mask) >> 12, cpsr, reg_current);
	else
  put_reg((op_code & rd_mask) >> 12, spsr[cpsr & mode_mask], reg_current);
	
	return;
}

/*----------------------------------------------------------------------------*/

void msr(unsigned int op_code)
{
	int mask, source;
	
	switch (op_code & 0X00090000)
	{
  case 0X00000000: mask = 0X00000000; break;
  case 0X00010000: mask = 0X0FFFFFFF; break;
  case 0X00080000: mask = 0XF0000000; break;
  case 0X00090000: mask = 0XFFFFFFFF; break;
	}
	if ((cpsr & mode_mask) == 0X10) mask = mask & 0XF0000000;        /* User mode */
	
	if ((op_code & imm_mask) == 0)                 /* Test applies for both cases */
  source = get_reg (op_code & rm_mask, reg_current) & mask;
	else
	{
  unsigned int x, y, dummy;
		
  x = op_code & 0X0FF;                                     /* Immediate value */
  y = (op_code & 0XF00) >> 7;                            /* Number of rotates */
  source = ((x >> y) | lsl (x, 32 - y, &dummy)) & mask;
	}
	
	if ((op_code & 0X00400000) == 0)
  cpsr = (cpsr & ~mask) | source;
	else
  spsr[cpsr & mode_mask] = (spsr[cpsr & mode_mask] & ~mask) | source;
	
	return;
}

/*----------------------------------------------------------------------------*/

void bx (unsigned int Rm, int link)/* Link is performed if "link" is NON-ZERO */
{
	int PC, offset;
	int t_bit;
	
	PC = get_reg (15, reg_current);
	
	if ((cpsr & tf_mask) != 0)
	{
		PC = PC - 2;
		PC = PC | 1;
	}                                                    /* Remember Thumb mode */
	else
	{
		PC = PC - 4;
	}
	
	offset = get_reg(Rm, reg_current) & 0XFFFFFFFE;
	t_bit = get_reg(Rm, reg_current) & 0X00000001;
	
	if (t_bit == 1) cpsr = cpsr |  tf_mask;
	else            cpsr = cpsr & ~tf_mask;
	
	put_reg(15, offset, reg_current);                                /* Update PC */
	
	if(link != 0)
	{
		put_reg(14, PC, reg_current);                   /* Link if BLX */
	}
	return;
}

/*----------------------------------------------------------------------------*/

void my_multi(unsigned int op_code)
{
	int acc;
	
	if ((op_code & mul_long_bit) == 0)                                  /* Normal */
	{
  acc = get_reg(op_code & rm_mask, reg_current)
		* get_reg((op_code & rs_mask) >> 8, reg_current);
		
  if ((op_code & mul_acc_bit) != 0)
	  acc = acc + get_reg ((op_code & rd_mask) >> 12, reg_current);
		
  put_reg((op_code & rn_mask) >> 16, acc, reg_current);
		
  if ((op_code & s_mask) != 0) set_NZ (acc);                         /* Flags */
	}
	else                                                                  /* Long */
	{
  unsigned int Rm, Rs, th, tm, tl;
  int sign;
		
  Rm = get_reg (op_code & rm_mask, reg_current);
  Rs = get_reg ((op_code & rs_mask) >> 8, reg_current);
		
  sign = 0;
  if ((op_code & mul_sign_bit) != 0)                                /* Signed */
  {
	  if ((Rm & bit_31) != 0)
	  {
		  Rm = ~Rm + 1;
		  sign = 1;
	  }
	  if ((Rs & bit_31) != 0)
	  {
		  Rs = ~Rs + 1;
		  sign = sign ^ 1;
	  }
  }
  /* Everything now `positive' */
  tl = (Rm & 0X0000FFFF) * (Rs & 0X0000FFFF);
  th = ((Rm >> 16) & 0X0000FFFF) * ((Rs >> 16) & 0X0000FFFF);
  tm = ((Rm >> 16) & 0X0000FFFF) * (Rs & 0X0000FFFF);
  Rm = ((Rs >> 16) & 0X0000FFFF) * (Rm & 0X0000FFFF);  /* Rm no longer needed */
  tm = tm + Rm;
  if (tm < Rm) th = th + 0X00010000;                       /* Propagate carry */
  tl = tl + (tm << 16);
  if (tl < (tm << 16)) th = th + 1;
  th = th + ((tm >> 16) & 0X0000FFFF);
		
  if (sign != 0)                                     /* Change sign of result */
  {
	  th = ~th;
	  tl = ~tl + 1;
	  if (tl == 0) th = th + 1;
  }
		
  if ((op_code & mul_acc_bit) != 0)
  {
	  tm = tl + get_reg ((op_code & rd_mask) >> 12, reg_current);
	  if (tm < tl) th = th + 1;                              /* Propagate carry */
	  tl = tm;
	  th = th + get_reg ((op_code & rn_mask) >> 16, reg_current);
  }
		
  put_reg((op_code & rd_mask) >> 12, tl, reg_current);
  put_reg((op_code & rn_mask) >> 16, th, reg_current);
		
  if ((op_code & s_mask) != 0)
	  set_NZ (th | (((tl >> 16) | tl) & 0X0000FFFF));                  /* Flags */
	}
	
	return;
}

/*----------------------------------------------------------------------------*/

void swap(unsigned int op_code)
{
	unsigned int address, data, size;
	
	address = get_reg((op_code & rn_mask) >> 16, reg_current);
	
	if ((op_code & byte_mask) != 0) size = 1;
	else                            size = 4;
	
	data = read_mem(address, size, FALSE, FALSE, mem_data);
	write_mem(address, get_reg(op_code & rm_mask, reg_current),
			  size, FALSE, mem_data);
	put_reg((op_code & rd_mask) >> 12, data, reg_current);
	
	return;
}

/*----------------------------------------------------------------------------*/

void normal_data_op(unsigned int op_code, int operation)
{
	int rd, a, b, mode;
	int shift_carry;
	int CPSR_special;
	
	mode = cpsr & mode_mask;
	CPSR_special = FALSE;
	shift_carry = 0;
	a = get_reg((op_code & rn_mask) >> 16, reg_current);    /* force_user = FALSE */
	
	if ((op_code & imm_mask) == 0)
  b = b_reg (op_code & op2_mask, &shift_carry);
	else
  b = b_immediate (op_code & op2_mask, &shift_carry);
	
	switch (operation)                                               /* R15s @@?! */
	{
  case 0X0: rd = a & b; break;                                         /* AND */
  case 0X1: rd = a ^ b; break;                                         /* EOR */
  case 0X2: rd = a - b; break;                                         /* SUB */
  case 0X3: rd = b - a; break;                                         /* RSB */
  case 0X4: rd = a + b; break;                                         /* ADD */
  case 0X5: rd = a + b;
			if ((cpsr & cf_mask) != 0) rd = rd + 1; break;             /* ADC */
  case 0X6: rd = a - b - 1;
			if ((cpsr & cf_mask) != 0) rd = rd + 1; break;             /* SBC */
  case 0X7: rd = b - a - 1;
			if ((cpsr & cf_mask) != 0) rd = rd + 1; break;             /* RSC */
  case 0X8: rd = a & b; break;                                         /* TST */
  case 0X9: rd = a ^ b;                                                /* TEQ */
			if ((op_code & rd_mask) == 0XF000)                        /* TEQP */
			{
				CPSR_special = TRUE;
				if (mode != user_mode) cpsr = spsr[mode];
			}
			break;
  case 0XA: rd = a - b; break;                                         /* CMP */
  case 0XB: rd = a + b; break;                                         /* CMN */
  case 0XC: rd = a | b; break;                                         /* ORR */
  case 0XD: rd = b;     break;                                         /* MOV */
  case 0XE: rd = a & ~b;break;                                         /* BIC */
  case 0XF: rd = ~b;    break;                                         /* MVN */
	}
	
	if ((operation & 0XC) != 0X8)               /* Return result unless a compare */
  put_reg((op_code & rd_mask) >> 12, rd, reg_current);
	
	if (((op_code & s_mask) != 0) && (CPSR_special != TRUE))             /* S-bit */
	{                                                    /* Want to change CPSR */
  if (((op_code & rd_mask) >> 12) == 0XF)                     /* PC and S-bit */
  {
	  if (mode != user_mode) cpsr = spsr[mode];           /* restore saved CPSR */
	  else fprintf(stderr, "SPSR_user read attempted\n");
  }
  else                                               /* other dest. registers */
  {
	  switch (operation)
	  {                                                           /* LOGICALs */
		  case 0X0:                                                        /* AND */
		  case 0X1:                                                        /* EOR */
		  case 0X8:                                                        /* TST */
		  case 0X9:                                                        /* TEQ */
		  case 0XC:                                                        /* ORR */
		  case 0XD:                                                        /* MOV */
		  case 0XE:                                                        /* BIC */
		  case 0XF:                                                        /* MVN */
			  set_NZ (rd);
			  if (shift_carry == TRUE) cpsr = cpsr |  cf_mask;      /* CF := output */
			  else                     cpsr = cpsr & ~cf_mask;      /* from shifter */
			  break;
			  
		  case 0X2:                                                        /* SUB */
		  case 0XA:                                                        /* CMP */
			  set_flags (flag_sub, a, b, rd, 1);
			  break;
			  
		  case 0X6:                        /* SBC - Needs testing more !!!   @@@@ */
			  set_flags (flag_sub, a, b, rd, cpsr & cf_mask);
			  break;
			  
		  case 0X3:                                                        /* RSB */
			  set_flags (flag_sub, b, a, rd, 1);
			  break;
			  
		  case 0X7:                                                        /* RSC */
			  set_flags (flag_sub, b, a, rd, cpsr & cf_mask);
			  break;
			  
		  case 0X4:                                                        /* ADD */
		  case 0XB:                                                        /* CMN */
			  set_flags (flag_add, a, b, rd, 0);
			  break;
			  
		  case 0X5:                                                        /* ADC */
			  set_flags (flag_add, a, b, rd, cpsr & cf_mask);
			  break;
	  }
  }
	}
	
	return;
}

/*----------------------------------------------------------------------------*/
/* shift type: 00 = LSL, 01 = LSR, 10 = ASR, 11 = ROR                         */

int b_reg(int op2, int *cf)
{
	unsigned int shift_type, reg, distance, result;
	reg = get_reg (op2 & 0X00F, reg_current);                         /* Register */
	shift_type = (op2 & 0X060) >> 5;                             /* Type of shift */
	if ((op2 & 0X010) == 0)
	{                                                        /* Immediate value */
  distance = (op2 & 0XF80) >> 7;
  if (distance == 0)                                         /* Special cases */
  {
	  if (shift_type == 3)
	  {
		  shift_type = 4;                                                  /* RRX */
		  distance = 1;                                     /* Something non-zero */
	  }
	  else if (shift_type != 0) distance = 32;                  /* LSL excluded */
  }
	}
	else
  distance = (get_reg ((op2 & 0XF00) >> 8, reg_current) & 0XFF);
	/* Register value */
	
	*cf = ((cpsr & cf_mask) != 0);                              /* Previous carry */
	switch (shift_type)
	{
  case 0X0: result = lsl(reg, distance, cf); break;                    /* LSL */
  case 0X1: result = lsr(reg, distance, cf); break;                    /* LSR */
  case 0X2: result = asr(reg, distance, cf); break;                    /* ASR */
  case 0X3: result = ror(reg, distance, cf); break;                    /* ROR */
  case 0X4:                                                         /* RRX #1 */
			result = reg >> 1;
			if ((cpsr & cf_mask) == 0) result = result & ~bit_31;
			else                       result = result | bit_31;
			*cf = ((reg & bit_0) != 0);
			break;
	}
	
	if (*cf) *cf = TRUE; else *cf = FALSE;                 /* Change to "Boolean" */
	return result;
}

/*----------------------------------------------------------------------------*/

int b_immediate(int op2, int *cf)
{
	unsigned int x, y, dummy;
	
	x = op2 & 0X0FF;                                           /* Immediate value */
	y = (op2 & 0XF00) >> 7;                                  /* Number of rotates */
	if (y == 0) *cf = ((cpsr & cf_mask) != 0);                  /* Previous carry */
	else        *cf = (((x >> (y - 1)) & bit_0) != 0);
	if (*cf) *cf = TRUE; else *cf = FALSE;                 /* Change to "Boolean" */
	return ror(x, y, &dummy);                                /* Circular rotation */
}

/*----------------------------------------------------------------------------*/

void clz(unsigned int op_code)
{
	int i, j;
	
	j = get_reg(op_code & rm_mask, reg_current);
	
	if (j == 0) i = 32;
	else
	{
  i = 0;
  while ((j & 0X80000000) == 0) { i++; j = j << 1; }
	}
	
	put_reg((op_code & rd_mask) >> 12, i, reg_current);
	
	return;
}

/*----------------------------------------------------------------------------*/

void transfer(unsigned int op_code)
{
	unsigned int address;
	int offset, rd, size;
	boolean T;
	
	if ((op_code & undef_mask) == undef_code)
	{
		undefined();
	}
	else
	{
		if ((op_code & byte_mask) == 0) size = 4;
		else                            size = 1;
		
		//  if (((op_code & pre_mask) == 0) && ((op_code & write_back_mask) != 0))
		//    T = TRUE;
		//  else
		//    T = FALSE;
		
		T = (((op_code & pre_mask) == 0) && ((op_code & write_back_mask) != 0));
		
		rd = (op_code & rd_mask) >> 12;
		
		address = get_reg ((op_code & rn_mask) >> 16, reg_current);
		
		offset = transfer_offset(op_code & op2_mask, op_code & up_mask, op_code & imm_mask, FALSE);  /* bit(25) = 1 -> reg */
		
		if((op_code & pre_mask) != 0)
		{
			address = address + offset;     /* Pre-index */
		}
		
		
		if((op_code & load_mask) == 0)
		{
			write_mem(address, get_reg (rd, reg_current), size, T, mem_data);
		}
		else
		{
			put_reg(rd, read_mem (address, size, FALSE, T, mem_data), reg_current);
		}
		
		if((op_code & pre_mask) == 0)                                /* Post-index */
		{
			put_reg((op_code & rn_mask) >> 16, address + offset, reg_current);
			/* Post index write-back */
		}
		else if ((op_code & write_back_mask) != 0)
		{
			put_reg((op_code & rn_mask) >> 16, address, reg_current);
			/* Pre index write-back */
		}
	}
	return;
}

/*----------------------------------------------------------------------------*/

int transfer_offset(int op2, int add, int imm, boolean sbhw)
{                                   /* add and imm are zero/non-zero Booleans */
	int offset;
	int cf;                                                    /* Dummy parameter */
	
	if (!sbhw)                                               /* Addressing mode 2 */
	{
		if (imm != 0)
		{
			offset = b_reg(op2, &cf);               /* bit(25) = 1 -> reg */
		}
		else
		{
			offset = op2 & 0XFFF;
		}
	}
	else                                                     /* Addressing mode 3 */
	{
		if (imm != 0)
		{
			offset = ((op2 & 0XF00) >> 4) | (op2 & 0X00F);
		}
		else
		{
			offset = b_reg(op2 & 0xF, &cf);
		}
	}
	
	if(add == 0)
	{
		offset = -offset;
	}
	
	return offset;
}

/*----------------------------------------------------------------------------*/

void multiple(unsigned int op_code)
{
	
	if ((op_code & load_mask) == 0)
  stm((op_code & 0X01800000) >> 23, (op_code & rn_mask) >> 16,
	  op_code & 0X0000FFFF, op_code & write_back_mask,
	  op_code & user_mask);
	else
  ldm((op_code & 0X01800000) >> 23, (op_code & rn_mask) >> 16,
	  op_code & 0X0000FFFF, op_code & write_back_mask,
	  op_code & user_mask);
	
	return;
}

/*----------------------------------------------------------------------------*/

int bit_count(unsigned int source, int *first)
{
	int count, reg;
	
	count = 0;
	reg = 0;
	*first = -1;
	
	while (source != 0)
	{
  if ((source & bit_0) != 0)
  {
	  count = count + 1;
	  if (*first < 0) *first = reg;
  }
  source = source >> 1;
  reg = reg + 1;
	}
	
	return count;
}

/*----------------------------------------------------------------------------*/

void ldm(int mode, int Rn, int reg_list, int write_back, int hat)
{                           /* Last two parameters are zero/non-zero Booleans */
	
	int address, new_base, count, first_reg, reg, data;
	int force_user;
	int r15_inc;                                            /* internal `Boolean' */
	
	address = get_reg(Rn, reg_current);
	count = bit_count(reg_list, &first_reg);
	r15_inc = (reg_list & 0X00008000) != 0;        /* R15 in list */
	
	switch (mode)
	{
  case 0: new_base = address - 4 * count; address = new_base + 4; break;
  case 1: new_base = address + 4 * count;                         break;
  case 2: new_base = address - 4 * count; address = new_base;     break;
  case 3: new_base = address + 4 * count; address = address + 4;  break;
	}
	
	address = address & 0XFFFFFFFC;           /* Bottom 2 bits ignored in address */
	
	if (write_back != 0) put_reg(Rn, new_base, reg_current);
	
	if ((hat != 0) && !r15_inc) force_user = reg_user;
	else                        force_user = reg_current;
	/* Force user unless R15 in list */
	reg = 0;
	
	while (reg_list != 0)
	{
  if ((reg_list & bit_0) != 0)
  {
	  data = read_mem(address, 4, FALSE, FALSE, mem_data);    /* Keep for later */
	  put_reg(reg, data, force_user);
	  address = address + 4;
  }
  reg_list = reg_list >> 1;
  reg = reg + 1;
	}
	
	if (r15_inc)                                                   /* R15 in list */
	{
  if ((data & 1) != 0) cpsr = cpsr | tf_mask;/* data left over from last load */
  else                 cpsr = cpsr & ~tf_mask; /* used to set instruction set */
  if (hat != 0) cpsr = spsr[cpsr & mode_mask];        /* ... and if S bit set */
	}
	/*  @@@ untested */
	return;
}

/*----------------------------------------------------------------------------*/

void stm(int mode, int Rn, int reg_list, int write_back, int hat)
{                           /* Last two parameters are zero/non-zero Booleans */
	
	int address, new_base, count, first_reg, reg;
	int force_user;
	boolean special;
	
	address = get_reg(Rn, reg_current);
	count = bit_count(reg_list, &first_reg);
	
	switch (mode)
	{
  case 0: new_base = address - 4 * count; address = new_base + 4; break;
  case 1: new_base = address + 4 * count;                         break;
  case 2: new_base = address - 4 * count; address = new_base;     break;
  case 3: new_base = address + 4 * count; address = address + 4;  break;
	}
	
	address = address & 0XFFFFFFFC;           /* Bottom 2 bits ignored in address */
	
	special = FALSE;
	if (write_back != 0)
	{
  if (Rn == first_reg) special = TRUE;
  else put_reg(Rn, new_base, reg_current);
	}
	
	if (hat != 0) force_user = reg_user;
	else          force_user = reg_current;
	
	reg = 0;
	
	while (reg_list != 0)
	{
  if ((reg_list & bit_0) != 0)
  {
	  write_mem (address, get_reg (reg, force_user), 4, FALSE, mem_data);
	  address = address + 4;
  }
  reg_list = reg_list >> 1;
  reg = reg + 1;
	}
	
	if (special) put_reg(Rn, new_base, reg_current);
	
	return;
}

/*----------------------------------------------------------------------------*/

void branch(unsigned int op_code)
{
	int offset, PC;
	
	PC = get_reg(15, reg_current);           /* Get this now in case mode changes */
	
	if (((op_code & link_mask) != 0) || ((op_code & 0XF0000000) == 0XF0000000))
  put_reg(14, get_reg(15, reg_current) - 4, reg_current);
	
	offset = (op_code & branch_field) << 2;
	if ((op_code & branch_sign) != 0)
  offset = offset | (~(branch_field << 2) & 0XFFFFFFFC);       /* sign extend */
	
	if ((op_code & 0XF0000000) == 0XF0000000)                 /* Other BLX fix-up */
	{
  offset = offset | ((op_code >> 23) & 2);
  cpsr = cpsr | tf_mask;
	}
	
	put_reg(15, PC + offset, reg_current);
	
	return;
}

/*----------------------------------------------------------------------------*/

void coprocessor(unsigned int op_code)
{
	//    printf("Coprocessor %d data transfer\n", (op_code>>8) & 0XF);
	undefined();
	return;
}

/*----------------------------------------------------------------------------*/

void my_system (unsigned int op_code)
{
	
	int swi_char_out(char c)
	{
		int okay;
		
		okay = (0 == 0);
		while (!put_buffer(&terminal0_Tx, c))
		{
			if (status == CLIENT_STATE_RESET) { okay = (0 == 1); break; }
			else comm(SWI_poll);       /* If stalled, retain monitor communications */
		}
		return okay;
	}
	
	int temp;
	
	if (((op_code & 0X0F000000) == 0X0E000000)
		/* bodge to allow Thumb to use this code */
		|| ((op_code & 0X0F000000) == 0X0C000000)
		|| ((op_code & 0X0F000000) == 0X0D000000))
	{                                                       /* Coprocessor op.s */
  fprintf(stderr, "whoops -undefined \n");
		/*
		 if ((op_code & 0X00000010) == 0X00000000)
		 printf("Coprocessor %d data operation\n", (op_code>>8) & 0XF);
		 else
		 printf("Coprocessor %d register transfer\n", (op_code>>8) & 0XF);
		 */
  undefined();
	}
	else
	{
  if (print_out)
	  fprintf(stderr, "\n*** SWI CALL %06X ***\n\n", op_code & 0X00FFFFFF);
		
  switch (op_code & 0X00FFFFFF)
		{
			case 0:                              /* Output character R0 (to terminal) */
				put_reg(15, get_reg(15, reg_current) - 8, reg_current);
				/* Bodge PC so that stall looks `correct' */
				swi_char_out(get_reg(0, reg_current) & 0XFF);
				
				if (status != CLIENT_STATE_RESET)
					put_reg(15, get_reg(15, reg_current), reg_current);     /* Correct PC */
				break;
				
			case 1:                             /* Input character R0 (from terminal) */
			{
				char c;
				put_reg(15, get_reg(15, reg_current) - 8, reg_current);
				/* Bodge PC so that stall looks `correct' */
				while ((!get_buffer(&terminal0_Rx, &c)) && (status != CLIENT_STATE_RESET))
					comm(SWI_poll);
				
				if (status != CLIENT_STATE_RESET)
				{
					put_reg(0, c & 0XFF, reg_current);
					put_reg(15, get_reg(15, reg_current), reg_current);     /* Correct PC */
				}
			}
				break;
				
			case 2:                                                           /* Halt */
				status = CLIENT_STATE_BYPROG;
				break;
				
			case 3:                                 /* Print string @R0 (to terminal) */
			{
				unsigned int str_ptr;
				char c;
				
				put_reg(15, get_reg(15, reg_current) - 8, reg_current);
				/* Bodge PC so that stall looks `correct' */
				
				str_ptr = get_reg(0, reg_current);
				while (((c = read_mem(str_ptr, 1, FALSE, FALSE, mem_system)) != '\0')
					   && (status != CLIENT_STATE_RESET))
				{
					swi_char_out(c);                                  /* Returns if reset */
					str_ptr++;
				}
				
				if (status != CLIENT_STATE_RESET)
					put_reg(15, get_reg(15, reg_current), reg_current);     /* Correct PC */
			}
				break;
				
			case 4:                                               /* Decimal print R0 */
			{
				
				int swi_dec_print(unsigned int number)    /* Recursive zero suppression */
				{
					int okay;
					
					okay = TRUE;
					if (number > 0)              /* else nothing - suppress leading zeros */
					{
						okay = swi_dec_print(number / 10);                /* Recursive call */
						if (okay) swi_char_out((number % 10) | '0');    /* Returns if reset */
					}
					return okay;
				}
				
				unsigned int number;
				int okay;
				
				put_reg(15, get_reg(15, reg_current) - 8, reg_current);
				/* Bodge PC so that stall looks `correct' */
				
				number = get_reg(0, reg_current);
				if (number == 0) okay = swi_char_out('0');  /* Don't suppress last zero */
				else             okay = swi_dec_print(number);      /* Returns if reset */
				
				if (status != CLIENT_STATE_RESET)
					put_reg(15, get_reg(15, reg_current), reg_current);     /* Correct PC */
			}
				break;
				
				
			default:
				if (print_out)
					fprintf(stderr, "Untrapped SWI call %06X\n", op_code & 0X00FFFFFF);
				spsr[sup_mode] = cpsr;
				cpsr = (cpsr & ~mode_mask) | sup_mode;
				cpsr = cpsr & ~tf_mask;                           /* Always in ARM mode */
				put_reg (14, get_reg (15, reg_current) - 4, reg_current);
				put_reg (15, 8, reg_current);
				break;
		}
	}
}

/*----------------------------------------------------------------------------*/
/* This is the breakpoint *instruction*                                       */

void breakpoint()                              /* Looks like a prefetch abort */
{
	spsr[abt_mode] = cpsr;
	cpsr = (cpsr & ~mode_mask & ~tf_mask) | abt_mode;
	put_reg (14, get_reg (15, reg_current) - 4, reg_current);
	put_reg (15, 12, reg_current);
	return;
}

/*----------------------------------------------------------------------------*/

void undefined()
{
	spsr[undef_mode] = cpsr;
	cpsr = (cpsr & ~mode_mask & ~tf_mask) | undef_mode;
	put_reg (14, get_reg (15, reg_current) - 4, reg_current);
	put_reg (15, 4, reg_current);
	return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

void set_flags(int operation, int a, int b, int rd, int carry)
{
	set_NZ(rd);
	set_CF(a, rd, carry);
	switch (operation)
	{
  case 1: set_VF_ADD (a, b, rd); break;
  case 2: set_VF_SUB (a, b, rd); break;
  default: fprintf(stderr, "Flag setting error\n"); break;
	}
	return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void set_NZ(unsigned int value)
{
	if (value == 0)            cpsr = cpsr |  zf_mask;
	else                       cpsr = cpsr & ~zf_mask;
	if ((value & bit_31) != 0) cpsr = cpsr |  nf_mask;
	else                       cpsr = cpsr & ~nf_mask;
	return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void set_CF(unsigned int a, unsigned int rd, int carry)
{                                     /* Two ways result can equal an operand */
	if ((rd > a) || ((rd == a) && (carry == 0))) cpsr = cpsr & ~cf_mask;
	else                                         cpsr = cpsr | cf_mask;
	return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void set_VF_ADD(int a, int b, int rd)
{
	cpsr = cpsr & ~vf_mask;                                           /* Clear VF */
	if (((~(a ^ b) & (a ^ rd)) & bit_31) != 0) cpsr = cpsr | vf_mask;
	return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void set_VF_SUB(int a, int b, int rd)
{
	cpsr = cpsr & ~vf_mask;                                           /* Clear VF */
	if ((((a ^ b) & (a ^ rd)) & bit_31) != 0) cpsr = cpsr | vf_mask;
	return;
}

/*----------------------------------------------------------------------------*/

int check_cc(int condition)                   /*checks CC against flag status */
{
	int go;
	
	switch (condition & 0XF)
	{
  case 0X0: go = zf(cpsr);                                         break;
  case 0X1: go = not(zf(cpsr));                                    break;
  case 0X2: go = cf(cpsr);                                         break;
  case 0X3: go = not(cf(cpsr));                                    break;
  case 0X4: go = nf(cpsr);                                         break;
  case 0X5: go = not(nf(cpsr));                                    break;
  case 0X6: go = vf(cpsr);                                         break;
  case 0X7: go = not(vf(cpsr));                                    break;
  case 0X8: go = and(cf(cpsr), not(zf(cpsr)));                     break;
  case 0X9: go = or(not(cf(cpsr)), zf(cpsr));                      break;
  case 0XA: go = not(xor(nf(cpsr), vf(cpsr)));                     break;
  case 0XB: go = xor(nf(cpsr), vf(cpsr));                          break;
  case 0XC: go = and(not(zf(cpsr)), not(xor(nf(cpsr), vf(cpsr)))); break;
  case 0XD: go = or(zf(cpsr), xor(nf(cpsr), vf(cpsr)));            break;
  case 0XE: go = TRUE;                                             break;
  case 0XF: go = FALSE;                                            break;
	}
	return go;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

int not(int x) { if (x == TRUE) return FALSE; else return TRUE; }

int and(int x, int y)
{
	if ((x == TRUE) && (y == TRUE)) return TRUE; else return FALSE;
}

int or(int x, int y)
{
	if ((x == TRUE) || (y == TRUE)) return TRUE; else return FALSE;
}

int xor(int x, int y)
{
	if (((x == TRUE) && (y == FALSE)) || ((x == FALSE) && (y == TRUE)))
  return TRUE;
	else
  return FALSE;
}

/*----------------------------------------------------------------------------*/

int zf(int cpsr) { if ((zf_mask & cpsr) != 0) return TRUE; else return FALSE; }
int cf(int cpsr) { if ((cf_mask & cpsr) != 0) return TRUE; else return FALSE; }
int nf(int cpsr) { if ((nf_mask & cpsr) != 0) return TRUE; else return FALSE; }
int vf(int cpsr) { if ((vf_mask & cpsr) != 0) return TRUE; else return FALSE; }

/*----------------------------------------------------------------------------*/

int get_reg(int reg_no, int force_mode)
{
	int mode, value;
	
	switch (force_mode)
	{
  case reg_current: mode = cpsr & mode_mask; break;
  case reg_user:    mode = user_mode;        break;
  case reg_svc:     mode = sup_mode;         break;
  case reg_fiq:     mode = fiq_mode;         break;
  case reg_irq:     mode = irq_mode;         break;
  case reg_abt:     mode = abt_mode;         break;
  case reg_undef:   mode = undef_mode;       break;
	}
	
	if (reg_no == 16) value = cpsr;                  /* Trap for status registers */
	else if (reg_no == 17)
	{
  if ((mode == user_mode) || (mode == system_mode)) value = cpsr;
  else                                              value = spsr[mode];
	}
	else if (reg_no != 15)
	{
  switch (mode)
		{
			case user_mode:
			case system_mode:
				value = r[reg_no];
				break;
				
			case fiq_mode:
				if (reg_no < 8) value = r[reg_no];
				else            value = fiq_r[reg_no - 8];
				break;
				
			case irq_mode:
				if (reg_no < 13) value = r[reg_no];
				else             value = irq_r[reg_no - 13];
				break;
				
			case sup_mode:
				if (reg_no < 13) value = r[reg_no];
				else             value = sup_r[reg_no - 13];
				break;
				
			case abt_mode:
				if (reg_no < 13) value = r[reg_no];
				else             value = abt_r[reg_no - 13];
				break;
				
			case undef_mode:
				if (reg_no < 13) value = r[reg_no];
				else             value = undef_r[reg_no - 13];
				break;
		}
	}
	else                                                             /* PC access */
  value = r[15] + instruction_length();
	/* PC := PC+4 (or +2 - Thumb) at start of execution */
	
	return value;
}

/*----------------------------------------------------------------------------*/
/* Modified "get_reg" to give unadulterated copy of PC                        */

int get_reg_monitor(int reg_no, int force_mode)
{
	if (reg_no != 15) return get_reg (reg_no, force_mode);
	else              return r[15];                                  /* PC access */
}

/*----------------------------------------------------------------------------*/
/* Write to a specified processor register                                    */

void put_reg(int reg_no, int value, int force_mode)
{
	int mode;
	
	switch (force_mode)
	{
  case reg_current: mode = cpsr & mode_mask; break;
  case reg_user:    mode = user_mode;        break;
  case reg_svc:     mode = sup_mode;         break;
  case reg_fiq:     mode = fiq_mode;         break;
  case reg_irq:     mode = irq_mode;         break;
  case reg_abt:     mode = abt_mode;         break;
  case reg_undef:   mode = undef_mode;       break;
	}
	
	if (reg_no == 16) cpsr = value;                  /* Trap for status registers */
	else if (reg_no == 17)
	{
  if ((mode == user_mode) || (mode == system_mode)) cpsr = value;
  else spsr[mode] = value;
	}
	else if (reg_no != 15)
	{
  switch (mode)
		{
			case user_mode:
			case system_mode:
				r[reg_no] = value;
				break;
				
			case fiq_mode:
				if (reg_no < 8) r[reg_no] = value;
				else        fiq_r[reg_no - 8] = value;
				break;
				
			case irq_mode:
				if (reg_no < 13) r[reg_no] = value;
				else         irq_r[reg_no - 13] = value;
				break;
				
			case sup_mode:
				if (reg_no < 13) r[reg_no] = value;
				else         sup_r[reg_no - 13] = value;
				break;
				
			case abt_mode:
				if (reg_no < 13) r[reg_no] = value;
				else         abt_r[reg_no - 13] = value;
				break;
				
			case undef_mode:
				if (reg_no < 13) r[reg_no] = value;
				else       undef_r[reg_no - 13] = value;
				break;
		}
	}
	else
  r[15] = value & 0XFFFFFFFE;      /* Lose bottom bit, but NOT mode specific! */
	
	return;
}

/*----------------------------------------------------------------------------*/
/* Return the length, in bytes, of the currently expected instruction.        */
/* (4 for ARM, 2 for Thumb)                                                   */

int instruction_length()
{
	if ((cpsr & tf_mask) == 0) return 4;
	else                       return 2;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

unsigned int fetch()
{
	unsigned int op_code;
	int i;
	
	op_code = read_mem((get_reg(15, reg_current) - instruction_length ()), instruction_length(), FALSE, FALSE, mem_instruction);
	
	for (i = 0; i < 32; i++)
	{
		if(past_opc_addr[i] == get_reg(15, reg_current) - instruction_length())
		{
			past_count++;
			i = 32;                                        /* bodged escape from loop */
		}
	}
	past_opc_addr[past_opc_ptr++] = get_reg(15, reg_current) - instruction_length();
	past_opc_ptr = past_opc_ptr % past_size;
	
	return op_code;
}

/*----------------------------------------------------------------------------*/

void inc_pc()
{
	/*fprintf(stderr, "get PC: %08x\n", get_reg(15, reg_current) ); */
	
	put_reg(15, get_reg(15, reg_current), reg_current);
	/* get_reg returns PC+4 for ARM & PC+2 for THUMB */
	return;
}

/*----------------------------------------------------------------------------*/

void endian_swap (unsigned int start, unsigned int end)
{
	unsigned int i, j;
	
	for (i = start; i < end; i++)
	{
  j = getmem32(i);
  setmem32(i, ((j >> 24) & 0X000000FF) | ((j >>  8) & 0X0000FF00)
		   | ((j <<  8) & 0X00FF0000) | ((j << 24) & 0XFF000000));
	}
	return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* source indicates type of read {mem_system, mem_instruction, mem_data}      */

int read_mem(unsigned int address, int size, boolean sign, boolean T, int source)
{
	int data, alignment;
	
	if (address < mem_size)
	{
		alignment = address & 0X00000003;
		data = getmem32(address >> 2);
		
		switch(address & 0X00000003)                             /* Apply rotation */
		{
			case 0:
				break;                                                         /* RR  0 */
			case 1:
				data = (data << 24) | ((data >> 8) & 0X00FFFFFF);
				break;                                                         /* RR  8 */
			case 2:
				data = (data << 16) | ((data >> 16) & 0X0000FFFF);
				break;                                                         /* RR 16 */
			case 3:
				data = (data << 8) | ((data >> 24) & 0X000000FF);
				break;                                                         /* RR 24 */
		}
		
		switch (size)
		{
			case 0:
				data = 0;
				break;                                            /* A bit silly really */
				
			case 1:                                                    /* byte access */
				if ((sign) && ((data & 0X00000080) != 0))
					data = data | 0XFFFFFF00;
				else
					data = data & 0X000000FF;
				break;
				
			case 2:                                               /* half-word access */
				if ((sign) && ((data & 0X00008000) != 0))
					data = data | 0XFFFF0000;
				else
					data = data & 0X0000FFFF;
				break;
				
			case 4:
				break;                                                   /* word access */
				
			default:
				fprintf(stderr, "Illegally sized memory read\n");
		}
		
		if ((runflags & 0x20) && (source == mem_data)) /* check watchpoints enabled */
		{
			if (check_watchpoints(address, data, size, 1))      // @@@
			{
				status = CLIENT_STATE_WATCHPOINT;
			}
		}
	}
	else
	{
		data = 0X12345678;
		print_out = FALSE;
	}
	
	/*if (T == TRUE) printf("User space forced\n");                               */
	
	/*
	 switch(source)
	 {
	 case 0: break;
	 case 1: printf("Instruction address %08X data %08X\n", address, data); break;
	 case 2: printf("Data read   address %08X data %08X\n", address, data); break;
	 }
	 */
	
	return data;
}

/*----------------------------------------------------------------------------*/

void write_mem(unsigned int address, int data, int size, boolean T, int source)
{
	unsigned int mask;
	
	if ((address == tube_address) && (tube_address != 0))/* Deal with Tube output */
	{
  unsigned char c;
		
  c = data & 0XFF;
		
  if (!print_out)
  {
	  if ((c == 0X0A) || (c == 0X0D))     fprintf(stderr, "\n");
	  else if ((c < 0X20) || (c >= 0X7F)) fprintf(stderr, "%02X", c);
	  else                                fprintf(stderr, "%c", c);
  }
  else
  {
	  fprintf(stderr, "Tube output byte = %02X (", c);
	  if ((c < 0X20) || (c >= 0X7F)) fprintf(stderr, ".)\n");
	  else                           fprintf(stderr, "%c)\n", c);
  }
	}
	else
	{
  if ((address >> 2) < mem_size)
  {
	  switch (size)
	  {
		  case 0:
			  break;                                          /* A bit silly really */
			  
		  case 1:                                                  /* byte access */
			  mask = 0X000000FF << (8 * (address & 0X00000003));
			  data = data << (8 * (address & 0X00000003));
			  setmem32(address >> 2, (getmem32(address >> 2) & ~mask) | (data & mask));
			  break;
			  
		  case 2:                                            /* half-word acccess */
			  mask = 0X0000FFFF << (8 * (address & 0X00000002));
			  data = data << (8 * (address & 0X00000002));
			  setmem32 (address >> 2,
						(getmem32 (address >> 2) & ~mask) | (data & mask));
			  break;
			  
		  case 4:                                                  /* word access */
			  setmem32 (address >> 2, data);
			  break;
			  
		  default:
			  fprintf(stderr, "Illegally sized memory write\n");
	  }
  }
  else
  {
	  //fprintf(stderr, "Writing %08X  data = %08X\n", address, data);
	  print_out = FALSE;
  }
		
		
  if ((runflags & 0x20) && (source == mem_data)) /* check watchpoints enabled */
  {
	  if (check_watchpoints(address, data, size, 0))      // @@@
	  {
		  status = CLIENT_STATE_WATCHPOINT;
	  }
  }
		
		
		/*if (T == TRUE) printf("User space forced\n");   */
		
  /*
   switch(source)
   {
   case 0: break;
   case 1: printf("Shouldn't happen\n"); break;
   case 2: printf("Data write  address %08X data %08X\n", address, data); break;
   }
   */
	}
	
	return;
}


/*----------------------------------------------------------------------------*/

/*- - - - - - - - - - - - watchpoints - - - - - - - - - - - - - - - - - - - */
/*                      to be completed                                     */

/* Needs privilege information @@@*/

int check_watchpoints(unsigned int address, int data, int size, int direction)
{
	int i, may_break;
	
	//if (direction == 0)
	//  fprintf(stderr, "Data write, address %08X, data %08X, size %d\n", address, data, size);
	//else
	//  fprintf(stderr, "Data read,  address %08X, data %08X, size %d\n", address, data, size);
	
	for (i = 0, may_break = FALSE; (i < NO_OF_WATCHPOINTS) && !may_break; i++)
	{                                                     /* Search watchpoints */
  may_break = ((emul_wp_flag[0] & emul_wp_flag[1] & (1<<i)) != 0);
		/* Watchpoint is active */
		
  may_break &= ((watchpoints[i].size & size) != 0);       /* Size is allowed? */
		
  if (may_break)                                           /* Check direction */
	  if (direction == 0)                                          /* Write @@@ */
		  may_break = (watchpoints[i].cond & 0x10) != 0;
	  else                                                          /* Read @@@ */
		  may_break = (watchpoints[i].cond & 0x20) != 0;
		
  if (may_break)                                    /* Try address comparison */
	  switch (watchpoints[i].cond & 0x0C)
  {
	  case 0x00: may_break = FALSE; break;
	  case 0x04: may_break = FALSE; break;
	  case 0x08:                   /* Case of between address A and address B */
		  if ((address < watchpoints[i].addra) || (address > watchpoints[i].addrb))
			  may_break = FALSE;
		  break;
		  
	  case 0x0C:                                              /* Case of mask */
		  if ((address & watchpoints[i].addrb) != watchpoints[i].addra)
			  may_break = FALSE;
		  break;
  }
		
  if (may_break)                                       /* Try data comparison */
	  switch (watchpoints[i].cond & 0x03)
  {
	  case 0x00: may_break = FALSE; break;
	  case 0x01: may_break = FALSE; break;
	  case 0x02:                         /* Case of between data A and data B */
		  if ((data < watchpoints[i].dataa[0]) || (data > watchpoints[i].datab[0]))
			  may_break = FALSE;
		  break;
		  
	  case 0x03:                                              /* Case of mask */
		  if ((data & watchpoints[i].datab[0]) != watchpoints[i].dataa[0])
			  may_break = FALSE;
		  break;
  }
		// Expansion space for more comparisons @@@  e.g. privilege
		
	}                                                        /* End of for loop */
	
	//
	//if (may_break) fprintf(stderr, "Watchpoint!\n");
	//
	//for (i = 0; i < NO_OF_WATCHPOINTS; i++)
	//  {
	//  fprintf(stderr,"====== WATCHPOINT %d ====\n", i);
	//  fprintf(stderr,"address A: %08x\n",  watchpoints[i].addra);
	//  fprintf(stderr,"address B: %08x\n",  watchpoints[i].addrb);
	//  fprintf(stderr,"Data A: %08x%08x\n", watchpoints[i].dataa[1],
	//                                       watchpoints[i].dataa[0]);
	//  fprintf(stderr,"Data B: %08x%08x\n", watchpoints[i].datab[1],
	//                                       watchpoints[i].datab[0]);
	//  fprintf(stderr,"State: %08x\n",      watchpoints[i].state);
	//  fprintf(stderr,"Condition: %08X\n",  watchpoints[i].cond);
	//  fprintf(stderr,"Size: %d\n", watchpoints[i].size);
	//  }
	//
	//
	//     typedef struct {
	//     int state;
	//     char cond;
	//     char size;
	//     int addra;
	//     int addrb;
	//     int dataa[2];
	//     int datab[2];
	//     } BreakElement;
	
	return may_break;
}


/*- - - - - - - - - - - - end watchpoints - - - - - - - - - - - - - - - - - */


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/* This is filthy :-( */
int get_number(char *ptr)
{
	int a;
	
	a = 0;
	while (*ptr == ' ') ptr++;                        /* Strip any leading spaces */
	
	while (((*ptr >= '0') && (*ptr <= '9')) || ((*ptr >= 'A') && (*ptr <= 'F')))
	{
  if ((*ptr >= '0') && (*ptr <= '9')) a = 16 * a + *(ptr++) - '0';
  else                                a = 16 * a + *(ptr++) - 'A' + 10;
	}
	return a;
}

/*----------------------------------------------------------------------------*/
/* As the compiler can't manage it ...                                        */

int lsl(int value, int distance, int *cf)         /* cf is -internal- Boolean */
{
	int result;
	
	if (distance != 0)
	{
  if (distance < 32)
  {
	  result = value << distance;
	  *cf = (((value << (distance - 1)) & bit_31) != 0);
  }
  else
  {
	  result = 0X00000000;
	  if (distance == 32) *cf = ((value & bit_0) != 0);
	  else                *cf = (0 != 0);             /* internal "false" value */
  }
	}
	else
  result = value;
	
	return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int lsr(unsigned int value, int distance, int *cf)/* cf is -internal- Boolean */
{
	unsigned int result, mask;
	
	if (distance != 0)
	{
  if (distance < 32)
  {
	  if (distance != 0) mask = ~(0XFFFFFFFF << (32 - distance));
	  else               mask = 0XFFFFFFFF;
	  result = (value >> distance) & mask;             /* Enforce logical shift */
	  *cf = (((value >> (distance - 1)) & bit_0) != 0);
  }
  else
  {                             /* Make a special case because C is so crap */
	  result = 0X00000000;
	  if (distance == 32) *cf = ((value & bit_31) != 0);
	  else                *cf = (0 != 0);             /* internal "false" value */
  }
	}
	else
  result = value;
	
	return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int asr(int value, int distance, int *cf)         /* cf is -internal- Boolean */
{
	int result;
	
	if (distance != 0)
	{
  if (distance < 32)
  {
	  result = value >> distance;
	  if (((value & bit_31) != 0) && (distance != 0))
		  result = result | (0XFFFFFFFF << (32 - distance));
	  /* Sign extend - I don't trust the compiler */
	  *cf = (((value >> (distance - 1)) & bit_0) != 0);
  }
  else
  {                             /* Make a special case because C is so crap */
	  *cf = ((value & bit_31) != 0);
	  if ((value & bit_31) == 0) result = 0X00000000;
	  else                       result = 0XFFFFFFFF;
  }
	}
	else
  result = value;
	
	return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int ror(unsigned int value, int distance, int *cf)/* cf is -internal- Boolean */
{
	int result;
	
	if (distance != 0)
	{
  distance = distance & 0X1F;
  result = lsr (value, distance, cf) | lsl (value, 32 - distance, cf);
		/* cf acts as dummy here */
  *cf = (((value >> (distance - 1)) & bit_0) != 0);
	}
	else
  result = value;
	
	return result;
}

/*----------------------------------------------------------------------------*/

/*------------------------------ THUMB EXECUTE -------------------------------*/

/*----------------------------------------------------------------------------*/

void
data0 (unsigned int op_code)
{
	unsigned int op2, rn;
	unsigned int shift, result, cf;
	
	rn = get_reg (((op_code >> 3) & 7), reg_current);        /* Called "Rm" in shifts */
	shift = ((op_code >> 6) & 0X0000001F);        /* Extracted speculatively */
	
	if ((op_code & 0X1800) != 0X1800)        /* Shifts */
	{
		cf = ((cpsr & cf_mask) != 0);        /* default */
		switch (op_code & 0X1800)
		{
			case 0X0000:
				result = lsl (rn, shift, &cf);
				break;                /* LSL (1) */
			case 0X0800:                /* LSR (1) */
				if (shift == 0)
					shift = 32;
				result = lsr (rn, shift, &cf);
				break;
			case 0X1000:                /* ASR (1) */
				if (shift == 0)
					shift = 32;
				result = asr (rn, shift, &cf);
				break;
			default:
				fprintf(stderr, "This compiler is broken\n");
				break;
		}
		
		if (cf)
			cpsr = cpsr | cf_mask;
		else
			cpsr = cpsr & ~cf_mask;
		set_NZ (result);
		put_reg ((op_code & 7), result, reg_current);
	}
	else
	{
		if ((op_code & 0X0400) == 0)        /* ADD(3)/SUB(3) */
		{
			op2 = get_reg ((op_code >> 6) & 7, reg_current);
		}
		else                        /* ADD(1)/SUB(1) */
		{
			op2 = (op_code >> 6) & 7;
		};
		
		if ((op_code & 0X0200) == 0)
		{
			result = rn + op2;
			set_flags (flag_add, rn, op2, result, 0);
		}
		else
		{
			result = rn - op2;
			set_flags (flag_sub, rn, op2, result, 1);
		}
		
		put_reg (op_code & 7, result, reg_current);
	}
	
	return;
}

/*----------------------------------------------------------------------------*/

void
data1 (unsigned int op_code)
{
	int rd, imm;
	int result;
	
	rd = (op_code >> 8) & 7;
	imm = op_code & 0X00FF;
	
	switch (op_code & 0X1800)
	{
		case 0X0000:                /* MOV (1) */
			result = imm;
			set_NZ (result);
			put_reg (rd, result, reg_current);
			break;
			
		case 0X0800:                /* CMP (1) */
			result = (get_reg (rd, reg_current) - imm);
			set_flags (flag_sub, get_reg (rd, reg_current), imm, result, 1);
			break;
			
		case 0X1000:                /* ADD (2) */
			result = (get_reg (rd, reg_current) + imm);
			set_flags (flag_add, get_reg (rd, reg_current), imm, result, 0);
			put_reg (rd, result, reg_current);
			break;
			
		case 0X1800:                /* SUB (2) */
			result = (get_reg (rd, reg_current) - imm);
			set_flags (flag_sub, get_reg (rd, reg_current), imm, result, 1);
			put_reg (rd, result, reg_current);
			break;
	}
	
	return;
}

/*----------------------------------------------------------------------------*/

void data_transfer(unsigned int op_code)
{
	unsigned int rd, rm;
	int cf;
	
	unsigned int address;
	
	signed int result;
	
	if ((op_code & 0X1000) == 0)                                /* NOT load/store */
	{
  if ((op_code & 0X0800) == 0)                       /* NOT load literal pool */
  {
	  if ((op_code & 0X0400) == 0)                           /* Data processing */
	  {
		  rd = get_reg ((op_code & 7), reg_current);
		  rm = get_reg (((op_code >> 3) & 7), reg_current);
		  
		  switch (op_code & 0X03C0)                     /* data processing opcode */
		  {
			  case 0X0000:                                                   /* AND */
				  result = rd & rm;
				  put_reg (op_code & 7, result, reg_current);
				  set_NZ (result);
				  break;
				  
			  case 0X0040:                                                   /* EOR */
				  result = rd ^ rm;
				  put_reg (op_code & 7, result, reg_current);
				  set_NZ (result);
				  break;
				  
			  case 0X0080:                                               /* LSL (2) */
				  cf = ((cpsr & cf_mask) != 0);                            /* default */
				  result = lsl (rd, rm & 0X000000FF, &cf);
				  if (cf) cpsr = cpsr |  cf_mask;
				  else    cpsr = cpsr & ~cf_mask;
				  set_NZ (result);
				  put_reg (op_code & 7, result, reg_current);
				  break;
				  
			  case 0X00C0:                                               /* LSR (2) */
				  cf = ((cpsr & cf_mask) != 0);                            /* default */
				  result = lsr (rd, rm & 0X000000FF, &cf);
				  if (cf) cpsr = cpsr |  cf_mask;
				  else    cpsr = cpsr & ~cf_mask;
				  set_NZ (result);
				  put_reg (op_code & 7, result, reg_current);
				  break;
				  
			  case 0X0100:                                               /* ASR (2) */
				  cf = ((cpsr & cf_mask) != 0);                            /* default */
				  result = asr (rd, rm & 0X000000FF, &cf);
				  if (cf) cpsr = cpsr |  cf_mask;
				  else    cpsr = cpsr & ~cf_mask;
				  set_NZ (result);
				  put_reg (op_code & 7, result, reg_current);
				  break;
				  
			  case 0X0140:                                                   /* ADC */
				  result = rd + rm;
				  if ((cpsr & cf_mask) != 0) result = result + 1;           /* Add CF */
				  set_flags (flag_add, rd, rm, result, cpsr & cf_mask);
				  put_reg (op_code & 7, result, reg_current);
				  break;
				  
			  case 0X0180:                                                   /* SBC */
				  result = rd - rm - 1;
				  if ((cpsr & cf_mask) != 0) result = result + 1;
				  set_flags (flag_sub, rd, rm, result, cpsr & cf_mask);
				  put_reg (op_code & 7, result, reg_current);
				  break;
				  
			  case 0X01C0:                                                   /* ROR */
				  cf = ((cpsr & cf_mask) != 0);                            /* default */
				  result = ror (rd, rm & 0X000000FF, &cf);
				  if (cf) cpsr = cpsr |  cf_mask;
				  else    cpsr = cpsr & ~cf_mask;
				  set_NZ (result);
				  put_reg (op_code & 7, result, reg_current);
				  break;
				  
			  case 0X0200:                                                   /* TST */
				  set_NZ(rd & rm);
				  break;
				  
			  case 0X0240:                                                   /* NEG */
				  result = -rm;
				  put_reg (op_code & 7, result, reg_current);
				  set_flags (flag_sub, 0, rm, result, 1);
				  break;
				  
			  case 0X0280:                                               /* CMP (2) */
				  set_flags (flag_sub, rd, rm, rd - rm, 1);
				  break;
				  
			  case 0X02C0:                                                   /* CMN */
				  set_flags (flag_add, rd, rm, rd + rm, 0);
				  break;
				  
			  case 0X0300:                                                   /* ORR */
				  result = rd | rm;
				  set_NZ (result);
				  put_reg (op_code & 7, result, reg_current);
				  break;
				  
			  case 0X00340:                                                  /* MUL */
				  result = rm * rd;
				  set_NZ (result);
				  put_reg (op_code & 7, result, reg_current);
				  break;
				  
			  case 0X0380:                                                   /* BIC */
				  result = rd & ~rm;
				  set_NZ (result);
				  put_reg (op_code & 7, result, reg_current);
				  break;
				  
			  case 0X03C0:                                                   /* MVN */
				  result = ~rm;
				  set_NZ (result);
				  put_reg (op_code & 7, result, reg_current);
				  break;
		  }                                                    /* End of switch */
	  }
	  else                                           /* special data processing */
	  {                                                     /* NO FLAG UPDATE */
		  switch (op_code & 0X0300)
		  {
			  case 0X0000:                                /* ADD (4) high registers */
				  rd = ((op_code & 0X0080) >> 4) | (op_code & 7);
				  rm = get_reg (((op_code >> 3) & 15), reg_current);
				  put_reg (rd, get_reg (rd, reg_current) + rm, reg_current);
				  break;
				  
			  case 0X0100:                                /* CMP (3) high registers */
				  rd = get_reg((((op_code & 0X0080) >> 4) | (op_code & 7)),
							   reg_current);
				  rm = get_reg(((op_code >> 3) & 15), reg_current);
				  set_flags (flag_sub, rd, rm, rd - rm, 1);
				  break;
				  
			  case 0X0200:                                /* MOV (2) high registers */
				  rd = ((op_code & 0X0080) >> 4) | (op_code & 7);
				  rm = get_reg (((op_code >> 3) & 15), reg_current);
				  
				  if (rd == 15)
					  rm = rm & 0XFFFFFFFE;                          /* Tweak mov to PC */
				  put_reg (rd, rm, reg_current);
				  break;
				  
			  case 0X0300:                                             /* BX/BLX Rm */
				  bx ((op_code >> 3) & 0XF, op_code & 0X0080);
				  break;
		  }                                                    /* End of switch */
	  }
  }
  else                                              /* load from literal pool */
  {                                                               /* LDR PC */
	  rd = ((op_code >> 8) & 7);
	  address = (((op_code & 0X00FF) << 2)) +
	  (get_reg(15, reg_current) & 0XFFFFFFFC);
	  put_reg(rd, read_mem(address, 4, FALSE, FALSE, mem_data), reg_current);
  }
	}
	else
	{                           /* load/store word, halfword, byte, signed byte */
		int rm, rn;
		int data;
		
		rd = (op_code & 7);
		rn = get_reg (((op_code >> 3) & 7), reg_current);
		rm = get_reg (((op_code >> 6) & 7), reg_current);
		
		switch (op_code & 0X0E00)
		{
			case 0X0000:                                        /* STR (2) register */
				write_mem (rn + rm, get_reg (rd, reg_current), 4, FALSE, mem_data);
				break;
				
			case 0X0200:                                       /* STRH (2) register */
				write_mem (rn + rm, get_reg (rd, reg_current), 2, FALSE, mem_data);
				break;
				
			case 0X0400:                                       /* STRB (2) register */
				write_mem (rn + rm, get_reg (rd, reg_current), 1, FALSE, mem_data);
				break;
				
			case 0X0600:                                          /* LDRSB register */
				data = read_mem (rn + rm, 1, TRUE, FALSE, mem_data);     /* Sign ext. */
				put_reg (rd, data, reg_current);
				break;
				
			case 0X0800:                                        /* LDR (2) register */
				data = read_mem (rn + rm, 4, FALSE, FALSE, mem_data);
				put_reg (rd, data, reg_current);
				break;
				
			case 0X0A00:                                       /* LDRH (2) register */
				data = read_mem (rn + rm, 2, FALSE, FALSE, mem_data);    /* Zero ext. */
				put_reg (rd, data, reg_current);
				break;
				
			case 0X0C00:                                                /* LDRB (2) */
				data = read_mem (rn + rm, 1, FALSE, FALSE, mem_data);    /* Zero ext. */
				put_reg (rd, data, reg_current);
				break;
				
			case 0X0E00:                                               /* LDRSH (2) */
				data = read_mem (rn + rm, 2, TRUE, FALSE, mem_data);     /* Sign ext. */
				put_reg (rd, data, reg_current);
				break;
		}
	}
	return;
}

/*----------------------------------------------------------------------------*/

void transfer0(unsigned int op_code)
{
	int rd, rn;
	int location, data;
	
	rn = get_reg (((op_code >> 3) & 7), reg_current);
	
	if ((op_code & 0X0800) == 0)                                           /* STR */
	{
  rd = get_reg ((op_code & 7), reg_current);
  if ((op_code & 0X1000) == 0)                           /* STR (1) 5-bit imm */
  {
	  location = rn + ((op_code >> 4) & 0X07C);             /* shift twice = *4 */
	  write_mem (location, rd, 4, FALSE, mem_data);
  }
  else                                                            /* STRB (1) */
  {
	  location = rn + ((op_code >> 6) & 0X1F);
	  write_mem (location, rd, 1, FALSE, mem_data);
  }
	}
	else                                                               /* LDR (1) */
	{
  rd = op_code & 7;
  if ((op_code & 0X1000) == 0)
  {
	  location = (rn + ((op_code >> 4) & 0X07C));           /* shift twice = *4 */
	  data = read_mem (location, 4, FALSE, FALSE, mem_data);
  }
  else                                                            /* LDRB (1) */
  {
	  location = (rn + ((op_code >> 6) & 0X1F));
	  data = read_mem (location, 1, FALSE, FALSE, mem_data);   /* Zero extended */
  }
  put_reg (rd, data, reg_current);
	}
	
	return;
}

/*----------------------------------------------------------------------------*/

void transfer1 (unsigned int op_code)
{
	int rd, rn;
	int data, location;
	
	switch (op_code & 0X1800)
	{
  case 0X0000:                                                    /* STRH (1) */
			rn = get_reg ((op_code >> 3) & 7, reg_current);
			rd = op_code & 7;
			data = get_reg (rd, reg_current);
			location = rn + ((op_code >> 5) & 0X3E);                   /* x2 in shift */
			write_mem (location, data, 2, FALSE, mem_data);
			break;
			
  case 0X0800:                                                    /* LDRH (1) */
			rd = op_code & 7;
			rn = get_reg ((op_code >> 3) & 7, reg_current);
			location = rn + ((op_code >> 5) & 0X3E);                   /* x2 in shift */
			data = read_mem (location, 2, FALSE, FALSE, mem_data);   /* Zero extended */
			put_reg (rd, data, reg_current);
			break;
			
  case 0X1000:                                                 /* STR (3) -SP */
			data = get_reg (((op_code >> 8) & 7), reg_current);
			rn = get_reg (13, reg_current);                                     /* SP */
			location = rn + ((op_code & 0X00FF) * 4);
			write_mem (location, data, 4, FALSE, mem_data);
			break;
			
  case 0X1800:                                                 /* LDR (4) -SP */
			rd = (op_code >> 8) & 7;
			rn = get_reg (13, reg_current);                                     /* SP */
			location = rn + ((op_code & 0X00FF) * 4);                  /* x2 in shift */
			data = read_mem (location, 4, FALSE, FALSE, mem_data);
			put_reg (rd, data, reg_current);
			break;
			
	}
	return;
}

/*----------------------------------------------------------------------------*/

void sp_pc(unsigned int op_code)
{
	int rd, sp, data;
	
	if ((op_code & 0X1000) == 0)                                  /* ADD SP or PC */
	{
  rd = (op_code >> 8) & 7;
		
  if ((op_code & 0X0800) == 0)                                  /* ADD(5) -PC */
	  data = (get_reg(15, reg_current) & 0XFFFFFFFC) + ((op_code & 0X00FF) << 2);
		/* get_reg supplies PC + 2 */
  else                                                          /* ADD(6) -SP */
	  data = (get_reg(13, reg_current)) + ((op_code & 0X00FF) << 2);
  put_reg(rd, data, reg_current);
	}
	else                                                             /* Adjust SP */
	{
  switch (op_code & 0X0F00)
		{
			case 0X0000:
				if ((op_code & 0X0080) == 0)                              /* ADD(7) -SP */
					sp = get_reg(13, reg_current) + ((op_code & 0X7F) << 2);
				else                                                      /* SUB(4) -SP */
					sp = get_reg(13, reg_current) - ((op_code & 0X7F) << 2);
				put_reg(13, sp, reg_current);
				break;
				
			case 0X0400:
			case 0X0500:
			case 0X0C00:
			case 0X0D00:
			{
				int reg_list;
				
				reg_list = op_code & 0X000000FF;
				
				if ((op_code & 0X0800) == 0)                                    /* PUSH */
				{
					if ((op_code & 0X0100) != 0) reg_list = reg_list | 0X4000;
					stm(2, 13, reg_list, 1, 0);
				}
				else                                                             /* POP */
				{
					if ((op_code & 0X0100) != 0) reg_list = reg_list | 0X8000;
					ldm (1, 13, reg_list, 1, 0);
				}
			}
				break;
				
			case 0X0E00:                                                /* Breakpoint */
				//      fprintf(stderr, "Breakpoint\n");
				breakpoint();
				break;
				
			case 0X0100:
			case 0X0200:
			case 0X0300:
			case 0X0600:
			case 0X0700:
			case 0X0800:
			case 0X0900:
			case 0X0A00:
			case 0X0B00:
			case 0X0F00:
				//      fprintf(stderr, "Undefined\n");
				undefined();
				break;
		}
		
  return;
	}
}

/*----------------------------------------------------------------------------*/

void lsm_b(unsigned int op_code)
{
	unsigned int offset;
	
	if ((op_code & 0X1000) == 0)
	{
  if ((op_code & 0X0800) == 0)                                    /* STM (IA) */
	  stm(1, (op_code >> 8) & 7, op_code & 0X000000FF, 1, 0);
  else                                                            /* LDM (IA) */
	  ldm(1, (op_code >> 8) & 7, op_code & 0X000000FF, 1, 0);
	}
	else                                               /* conditional BRANCH B(1) */
	{
  if ((op_code & 0X0F00) != 0X0F00)                      /* Branch, not a SWI */
  {
	  if (check_cc (op_code >> 8) == TRUE)
	  {
		  offset = (op_code & 0X00FF) << 1;                        /* sign extend */
		  if ((op_code & 0X0080) != 0) offset = offset | 0XFFFFFE00;
		  
		  put_reg(15, get_reg(15, reg_current) + offset, reg_current);
		  /* get_reg supplies pc + 2 */
		  /*    fprintf(stderr, "%08X", address + 4 + offset);                          */
	  }
  }
  else                                                                 /* SWI */
  {
	  offset = op_code & 0X00FF;
	  /* bodge op_code to pass only SWI No. N.B. no copro in Thumb */
	  my_system(offset);
  }
	}
}


/*----------------------------------------------------------------------------*/

void thumb_branch1(unsigned int op_code, int exchange)
{
	int offset, lr;
	
	lr = get_reg (14, reg_current);              /* Retrieve first part of offset */
	offset = lr + ((op_code & 0X07FF) << 1);
	
	lr = get_reg (15, reg_current) - 2 + 1;         /* + 1 to indicate Thumb mode */
	
	if (exchange == TRUE)
	{
  cpsr = cpsr & ~tf_mask;                               /* Change to ARM mode */
  offset = offset & 0XFFFFFFFC;
	}
	
	put_reg(15, offset, reg_current);
	put_reg(14, lr, reg_current);
	
	return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void thumb_branch(unsigned int op_code)
{
	int offset;
	
	switch (op_code & 0X1800)
	{
  case 0X0000:                                            /* B -uncond. B(2)  */
			offset = (op_code & 0X07FF) << 1;
			if ((op_code & 0X0400) != 0) offset = offset | 0XFFFFF000; /* sign extend */
			put_reg (15, (get_reg (15, reg_current) + offset), reg_current);
			break;
			
  case 0X0800:                                                         /* BLX */
			if ((op_code & 0X0001) == 0) thumb_branch1 (op_code, TRUE);
			else fprintf(stderr, "Undefined\n");
			break;
			
  case 0X1000:                                                   /* BL prefix */
			BL_prefix = op_code & 0X07FF;
			offset = BL_prefix << 12;
			
			if ((BL_prefix & 0X0400) != 0) offset = offset | 0XFF800000; /* Sign ext. */
			offset = get_reg (15, reg_current) + offset;
			put_reg (14, offset, reg_current);
			break;
			
  case 0X1800:                                                          /* BL */
			thumb_branch1 (op_code, FALSE);
			break;
	}
	
	return;
}

/*----------------------------------------------------------------------------*/

/*------------------------------ Charlie's functions--------------------------*/

/*----------------------------------------------------------------------------*/

// Jesus wept.   What was wrong with "read_mem" and "write_mem"?  @@@
// If int < 32 bits the whole lot is broken anyway! @@@


unsigned int getmem32(int number)
{
	number = number % RAMSIZE;
	return memory[(number << 2)]           | memory[(number << 2) + 1] << 8
	| memory[(number << 2) + 2] << 16 | memory[(number << 2) + 3] << 24;
}

void setmem32(int number, unsigned int reg)
{
	number = number & (RAMSIZE - 1);
	memory[(number << 2) + 0] = (reg >> 0)  & 0xff;
	memory[(number << 2) + 1] = (reg >> 8)  & 0xff;
	memory[(number << 2) + 2] = (reg >> 16) & 0xff;
	memory[(number << 2) + 3] = (reg >> 24) & 0xff;
}


/*----------------------------------------------------------------------------*/
/* terminal support routines                                                  */

void init_buffer(ring_buffer *buffer)
{
	buffer->iHead = 0;
	buffer->iTail = 0;
	return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Measure buffer occupancy                                                   */

int count_buffer(ring_buffer *buffer)
{
	int i;
	i = buffer->iHead - buffer->iTail;
	if (i < 0) i = i + RING_BUF_SIZE;
	return i;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int put_buffer(ring_buffer *buffer, unsigned char c)
{
	int status;
	unsigned int temp;
	
	temp = (buffer->iHead + 1) % RING_BUF_SIZE;
	
	if (temp != buffer->iTail)
	{
  buffer->buffer[temp] = c;
  buffer->iHead = temp;
  status = (0 == 0);                                                  /* Okay */
	}
	else
  status = (0 == 1);                                           /* Buffer full */
	
	return status;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int get_buffer(ring_buffer *buffer, unsigned char *c)
{
	int status;
	
	if (buffer->iTail != buffer->iHead)
	{
  buffer->iTail = (buffer->iTail + 1) % RING_BUF_SIZE;/* Preincrement pointer */
  *c = buffer->buffer[buffer->iTail];
  status = (0 == 0);                                                  /* Okay */
	}
	else
  status = (0 == 1);                                          /* Nothing read */
	
	return status;
}


/*                                end of jimulator.c                          */
/*============================================================================*/
