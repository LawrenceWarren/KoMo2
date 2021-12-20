/* ':':':':':':':':':'@' '@':':':':':':':':':'@' '@':':':':':':':':':'| */
/* :':':':':':':':':':'@'@':':':':':':':':':':'@'@':':':':':':':':':':| */
/* ':':':': : :':':':':'@':':':':': : :':':':':'@':':':':': : :':':':'| */
/*  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/*                Name:           interface.h                           */
/*                Version:        1.5.0                                 */
/*                Date:           20/07/2007                            */
/*  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdio.h>

#define _(String) (String)

#define MAX_SERIAL_WORD 4
/* Currently the maximum size of a word sent to emulator or board */

/* The following define possible states of the board */

#define CLIENT_STATE_CLASS_MASK 0XC0
#define CLIENT_STATE_CLASS_STOPPED 0X40
#define CLIENT_STATE_CLASS_RUNNING 0X80
#define CLIENT_STATE_RESET 0X00
#define CLIENT_STATE_STOPPED 0X40
#define CLIENT_STATE_BREAKPOINT 0X41
#define CLIENT_STATE_WATCHPOINT 0X42
#define CLIENT_STATE_BYPROG 0X44
#define CLIENT_STATE_RUNNING 0X80
#define CLIENT_STATE_RUNNING_BL 0x81  //  @@@ NEEDS UPDATE
#define CLIENT_STATE_RUNNING_SWI 0x81
#define CLIENT_STATE_STEPPING 0X82

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* The following defines possible instructions that can be sent to the board  */
/*   as 1 byte each.                                                          */

/* Board instructions unsigned char */
/* need work on */
typedef enum {
  BR_NOP = 0x00,
  BR_PING = 0x01,
  BR_WOT_R_U = 0x02,
  BR_RESET = 0x04,
  BR_FR_WRITE = 0x12,
  BR_FR_READ = 0x13,
  BR_WOT_U_DO = 0x20,
  BR_STOP = 0x21,
  BR_PAUSE = 0x22,
  BR_CONTINUE = 0x23,
  BR_RTF_SET = 0x24,
  BR_RTF_GET = 0x25,
  BR_BP_WRITE = 0x30,
  BR_BP_READ = 0x31,
  BR_BP_SET = 0x32,
  BR_BP_GET = 0x33,
  BR_WP_WRITE = 0x34,
  BR_WP_READ = 0x35,
  BR_WP_SET = 0x36,
  BR_WP_GET = 0x37,
} BR_Instruction;

#endif

/*                                                                      */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
/*                     end of interface.h                               */
/************************************************************************/
