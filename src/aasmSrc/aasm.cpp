//  *** Check mnemonics file; had to regress version due to ftp cock-up ***

// AASM - ARM assembler  Alpha+ version  J. Garside  UofM 16/8/12
//  To do								@@@@@@@@
//  Symbol table lists
//  Can't do ORG top - (end - start) ... difficult (!!), but should be possible
//		Would require `virtual' assembler pointer or label values
// Some ELF work May 2004 - output file accepted by ARM's "fromelf" and komodo
// Thumb shift problem fixed Apr.'07
// 10/01/07  Fix of LDRH post index; fix of literal recogniser
// PUSH/POP added to ARM May '07 (Introduced bug removed June '07)
// Reorigining in ELF/DEFS bug found & fixed June '07.
// Data records added 9/7/07
// Oops! LDR= `brain fart' fixed 26/7/07
// Subtle LDR= bug found & believed fixed 21/12/07
// Added Verilog ROM output 30/4/09
// Fixed erroneous 'fed up' reporting 1/5/09
// Added binary IMPORT 2/7/09
// Symbol list bug fix 16/3/10
// ADRL PC trapped and forbidden 17/3/10
// Thumb BLX fixed + 1 improved error report 27/9/11
// C-style character escapes added to immediates (e.g. #'\n') 16/8/12
// Okay/error return value added 16/8/12
// Explicit mnemonic alternatives for shifts added 23/3/15

// To do:	ADRL fixed, "MOVX" etc added - some more shakedown tests (?)  @@
//              ADRL still causing problems :-(  'Length cycle' too great (4)
//              found
//		Pass counters increased but needs some attention to find 'real'
// answer (6/4/11)
//
//		Pack literal pool (halfwords) together rather than `in order'
//		Dump literals into space in ALIGN where possible
//		Not sure LDRSH rd, =nnnn behaviour is correct
//			No range check but doesn't alias -1 & FFFF either
//			I think range check; ST thinks ban it
//		LDMFDEQ as well as LDMEQFD etc. etc. etc.
//		Proper shakedown testing (improving)
//		Macros
//		Conditional assembly
//		"mnemonics" file not found if executed via $PATH; improve search
//		'record'/'structure' directive for creating offsets

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <array>
#include <string>

#define MAX_PASSES 30                  // No of reiterations before giving up
#define SHRINK_STOP (MAX_PASSES - 10)  // First pass where shrinkage forbidden
#define VERILOG_MAX 0x10000            // Maximum size of Verilog ROM output
#define IF_STACK_SIZE 10               // Maximum nesting of IF clauses

#define SYM_TAB_HASH_BITS 4
#define SYM_TAB_LIST_COUNT (1 << SYM_TAB_HASH_BITS)
#define SYM_TAB_LIST_MASK (SYM_TAB_LIST_COUNT - 1)

#define SYM_NAME_MAX 32
#define LINE_LENGTH 256

#define SYM_TAB_CASE_FLAG 1    // Bit mask for case insensitive flag
#define SYM_TAB_EXPORT_FLAG 2  // Keep whole table

#define SYM_REC_DEF_FLAG 0x0100     // Bit mask for `symbol defined' flag
#define SYM_REC_EXPORT_FLAG 0x0200  // Bit mask for `export' flag
#define SYM_REC_EQU_FLAG 0x0400     // Indicate `type' as EQU (abs)
#define SYM_REC_USR_FLAG 0x0800     // Indicate `type' as DEF (abs)
#define SYM_REC_EQUATED (SYM_REC_EQU_FLAG | SYM_REC_USR_FLAG)  // Either
#define SYM_REC_USR_FLAG 0x0800  // Indicate `type' as DEF (abs)

// Indicate label in `Thumb' area Lowest 8 bits used for pass count
#define SYM_REC_THUMB_FLAG 0x1000
#define SYM_REC_DATA_FLAG 0x2000  // Data space offset

#define ALLOW_ON_FIRST_PASS 0x00010000  // Bit masks to prevent errors
#define ALLOW_ON_INTER_PASS 0x00020000  // occurring when not wanted.
#define WARNING_ONLY 0x00040000
#define ALL_EXCEPT_LAST_PASS (ALLOW_ON_FIRST_PASS | ALLOW_ON_INTER_PASS)

#define SYM_NO_ERROR 0
#define SYM_ERR_SYNTAX 0x0100
#define SYM_ERR_NO_MNEM 0x0200
#define SYM_ERR_NO_EQU 0x0300
#define SYM_BAD_REG 0x0400
#define SYM_BAD_REG_COMB 0x0500
#define SYM_NO_REGLIST 0x0600
#define SYM_NO_RSQUIGGLE 0x0700
#define SYM_OORANGE (0x0800 | ALL_EXCEPT_LAST_PASS)
#define SYM_ENDLESS_STRING 0x0900
#define SYM_DEF_TWICE 0x0A00
#define SYM_NO_COMMA 0x0B00
#define SYM_NO_TABLE 0x0C00
#define SYM_GARBAGE 0x0D00
#define SYM_ERR_NO_EXPORT (0x0E00 | WARNING_ONLY)
#define SYM_INCONSISTENT 0x0F00
#define SYM_ERR_NO_FILENAME 0x1000
#define SYM_NO_LBR 0x1100
#define SYM_NO_RBR 0x1200
#define SYM_ADDR_MODE_ERR 0x1300
#define SYM_ADDR_MODE_BAD 0x1400
#define SYM_NO_LSQUIGGLE 0x1500
#define SYM_OFFSET_TOO_BIG (0x1600 | ALL_EXCEPT_LAST_PASS)
#define SYM_BAD_COPRO 0x1700
#define SYM_BAD_VARIANT 0x1800
#define SYM_NO_COND 0x1900
#define SYM_BAD_CP_OP 0x1A00
#define SYM_NO_LABELS (0x1B00 | WARNING_ONLY)
#define SYM_DOUBLE_ENTRY 0x1C00
#define SYM_NO_INCLUDE 0x1D00
#define SYM_NO_BANG 0x1E00
#define SYM_MISALIGNED (0x1F00 | ALL_EXCEPT_LAST_PASS)
#define SYM_OORANGE_BRANCH (0x2000 | ALL_EXCEPT_LAST_PASS)
#define SYM_UNALIGNED_BRANCH (0x2100 | ALL_EXCEPT_LAST_PASS)
#define SYM_VAR_INCONSISTENT 0x2200
#define SYM_NO_IDENTIFIER 0x2300
#define SYM_MANY_IFS 0x2400
#define SYM_MANY_FIS 0x2500
#define SYM_LOST_ELSE 0x2600
#define SYM_NO_HASH 0x2700
#define SYM_NO_IMPORT 0x2800
#define SYM_ADRL_PC 0x2900
#define SYM_ERR_NO_SHFT 0x2A00
#define SYM_NO_REG_HASH 0x2B00
#define SYM_ERR_BROKEN 0xFF00  // TEMP uncommitted @@@

// evaluate return error states
// #define EVAL_OKAY             0x0000		// Rationalise @@@

#define EVAL_OKAY SYM_NO_ERROR
#define EVAL_NO_OPERAND 0x3000
#define EVAL_NO_OPERATOR 0x3100
#define EVAL_NO_CLOSEBR 0x3200
#define EVAL_NO_OPENBR 0x3300
#define EVAL_MATHSTACK_LIMIT 0x3400
#define EVAL_NO_LIMIT (0x3500 | ALLOW_ON_FIRST_PASS)
#define EVAL_LABEL_UNDEF (0x3600 | ALL_EXCEPT_LAST_PASS)
#define EVAL_OUT_OF_RADIX 0x3700
#define EVAL_DIV_BY_ZERO (0x3800 | ALL_EXCEPT_LAST_PASS)
#define EVAL_OPERAND_ERROR 0x3900
#define EVAL_BAD_LOC_LAB 0x3A00
#define EVAL_NO_LABEL_YET 0x3B00  // Label not defined `above' (for `IF's)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Literal pool flags                                                         */

#define LIT_DEFINED 0x01  // Set when a literal pool entry has a value
#define LIT_NO_DUMP 0x02  // Set when record needn't be planted
#define LIT_HALF 0x04     // Set when value is halfword

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Expression evaluator                                                       */

#define MATHSTACK_SIZE 20
// Enumerations used for both unary and binary operators
#define PLUS 0
#define MINUS 1
#define NOT 2
#define MULTIPLY 3
#define DIVIDE 4
#define MODULUS 5
#define CLOSEBR 6
#define LEFT_SHIFT 7
#define RIGHT_SHIFT 8
#define AND 9
#define OR 10
#define XOR 11
#define EQUALS 12
#define NOT_EQUAL 13
#define LOWER_THAN 14  // Unsigned comparisons
#define LOWER_EQUAL 15
#define HIGHER_THAN 16
#define HIGHER_EQUAL 17
#define LESS_THAN 18  // Signed comparisons
#define LESS_EQUAL 19
#define GREATER_THAN 20
#define GREATER_EQUAL 21
#define LOG 22
#define END 23

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* List file formatting constants                                             */
#define LIST_LINE_LENGTH 120  // Total line length
#define LIST_LINE_ADDRESS 10  // Address field width
#define LIST_BYTE_COUNT 4     // Number of bytes per line
#define LIST_BYTE_FIELD (LIST_LINE_ADDRESS + 3 * LIST_BYTE_COUNT + 2)
#define LIST_LINE_LIST (LIST_LINE_LENGTH - 1 - LIST_BYTE_FIELD)

#define HEX_LINE_ADDRESS 10
#define HEX_BYTE_COUNT 16
#define HEX_LINE_LENGTH (HEX_LINE_ADDRESS + 3 * HEX_BYTE_COUNT)

#define ELF_TEMP_LENGTH 20

#define ELF_MACHINE 40          // ARM (?)
#define ELF_EHSIZE 52           // Defined in standard
#define ELF_PHENTSIZE (4 * 8)   // Defined in standard
#define ELF_SHENTSIZE (4 * 10)  // Defined in standard

#define ELF_SHN_ABS 0xFFF1  // Defined in standard

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define v3 0xFF
#define v3M 0xFE
#define v4 0xFC
#define v4xM 0xFD
#define v4T 0xF8
#define v4TxM 0xF9
#define v5 0xF0
#define v5xM 0xF1
#define v5T 0xF0
#define v5TxM 0xF1
#define v5TE 0xC0
#define v5TExP 0xE0

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

typedef enum { ARM, THUMB } instr_set;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

typedef enum { TYPE_BYTE, TYPE_HALF, TYPE_WORD, TYPE_CPRO } type_size;
typedef enum { NO_LABEL, MAYBE_SYMBOL, SYMBOL, LOCAL_LABEL } label_type;
typedef enum { ALL, EXPORTED, DEFINED, UNDEFINED } label_category;
typedef enum { ALPHABETIC, VALUE, DEFINITION, FOR_ELF } label_sort;

typedef enum {
  SYM_REC_ADDED,
  SYM_REC_DEFINED,
  SYM_REC_REDEFINED,
  SYM_REC_UNCHANGED,
  SYM_REC_ERROR
} defn_return;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

typedef struct sym_record_name  // Symbol record definition
{
  struct sym_record_name* pNext;  // Pointer to next record
  unsigned int count;             // Number of characters in name
  unsigned int hash;
  unsigned int flags;
  unsigned int identifier;   // Record identifier, also definition order
  unsigned int elf_section;  // Section number - purely for ELF driver
  int value;
  char name[SYM_NAME_MAX];  // Fixed field for name (quick)
} sym_record;

typedef struct  // Symbol table header definition
{
  char* name;
  unsigned int symbol_number;
  unsigned int flags;
  sym_record* pList[SYM_TAB_LIST_COUNT];
} sym_table;

typedef struct sym_table_item_name  // So we can make lists of symbol tables
{
  sym_table* pTable;                  // Pointer to symbol table (or NULL)
  struct sym_table_item_name* pNext;  // Pointer to next record  (or NULL)
} sym_table_item;

typedef struct local_label_name  // Local label element definition
{
  struct local_label_name* pNext;  // Pointer to next record
  struct local_label_name* pPrev;  // Pointer to previous record
  unsigned int label;              // The value of the label (number)
  unsigned int value;              // The label word value
  unsigned int flags;
} local_label;

typedef struct own_label_name  // Definition of label on current line
{
  label_type sort;                 // What `sort' of label, if any
  struct sym_record_name* symbol;  // Pointer to symbol record, if present
  struct local_label_name* local;  // Pointer to local label, if present
} own_label;

typedef struct literal_record_name  // Literal pool element holder definition
{
  struct literal_record_name* pNext;  // Pointer to next record
  unsigned int address;               // The address of the literal word
  unsigned int value;                 // The literal word value
  unsigned int flags;
} literal_record;

typedef struct elf_temp_name {
  struct elf_temp_name* pNext;
  bool continuation;
  unsigned int section;
  unsigned int address;
  unsigned int count;
  char data[ELF_TEMP_LENGTH];
} elf_temp;

typedef struct elf_info_name  // Section info collecting point
{                             // Just the bits I think need collecting
  struct elf_info_name* pNext;
  unsigned int name;
  unsigned int address;
  unsigned int position;
  unsigned int size;
} elf_info;

typedef struct size_record_name  // Size of variable length operation
{                                // (form an ordered list)
  struct size_record_name* pNext;
  unsigned int size;
} size_record;

/*----------------------------------------------------------------------------*/

const int SYM_RECORD_SIZE = sizeof(sym_record);
const int SYM_TABLE_SIZE = sizeof(sym_table);
const int SYM_TABLE_ITEM_SIZE = sizeof(sym_table_item);
const int LIT_RECORD_SIZE = sizeof(literal_record);
const int LOCAL_LABEL_SIZE = sizeof(local_label);
const int ELF_TEMP_SIZE = sizeof(elf_temp);
const int ELF_INFO_SIZE = sizeof(elf_info);
const int SIZE_RECORD_SIZE = sizeof(size_record);

/*----------------------------------------------------------------------------*/

bool set_options(int argc, char* argv[]);

bool input_line(FILE*, std::string&, unsigned int);
bool parse_mnemonic_line(std::string&, sym_table*, sym_table*, sym_table*);
unsigned int parse_source_line(std::string&,
                               sym_table_item*,
                               sym_table*,
                               int,
                               int,
                               char**,
                               char*);
void print_error(std::string&, unsigned int, unsigned int, char*, int);
unsigned int assemble_line(char*,
                           unsigned int,
                           unsigned int,
                           own_label*,
                           sym_table*,
                           int,
                           int,
                           char**,
                           char*);

int do_literal(instr_set, type_size, int*, bool, unsigned int*);
unsigned int find_partials(unsigned int, unsigned int*);
unsigned int variable_item_size(int, unsigned int);

int get_thing(char*, unsigned int*, sym_table*);
int get_reg(char*, unsigned int*);
int get_thumb_reg(char*, unsigned int*, unsigned int);
int get_creg(char*, unsigned int*);
int get_copro(char*, unsigned int*);
int get_psr(char*, unsigned int*);
int get_shift(char*, unsigned int*);
int data_op_imm(unsigned int);
unsigned int thumb_pc_load(unsigned int,
                           unsigned int,
                           unsigned int,
                           int,
                           int,
                           unsigned int*);

void redefine_symbol(char*, sym_record*, sym_table*);
void assemble_redef_label(unsigned int,
                          int,
                          own_label*,
                          unsigned int*,
                          int,
                          int,
                          int,
                          char*);

/*----------------------------------------------------------------------------*/

unsigned int evaluate(std::string, unsigned int*, int*, sym_table*);
int get_variable(char*, unsigned int*, int*, int*, bool*, sym_table*);
int get_operator(char*, unsigned int*, int*, int*);

/*----------------------------------------------------------------------------*/

sym_table* sym_create_table(char*, unsigned int);
int sym_delete_table(sym_table*, bool);
defn_return sym_define_label(char*,
                             unsigned int,
                             unsigned int,
                             sym_table*,
                             sym_record**);
int sym_locate_label(char*, unsigned int, sym_table*, sym_record**);
sym_record* sym_find_label_list(char*, sym_table_item*);
sym_record* sym_find_label(char*, sym_table*);
sym_record* sym_create_record(char*, unsigned int, unsigned int, unsigned int);
void sym_delete_record(sym_record*);
int sym_delete_record_list(sym_record**, int);
int sym_add_to_table(sym_table*, sym_record*);
sym_record* sym_find_record(sym_table*, sym_record*);
void sym_string_copy(char*, sym_record*, unsigned int);
char* sym_strtab(sym_record*, unsigned int, unsigned int*);
sym_record* sym_sort_symbols(sym_table*, label_category, label_sort);
unsigned int sym_count_symbols(sym_table*, label_category);
void sym_print_table(sym_table*, label_category, label_sort, int, char*);
void local_label_dump(local_label*, FILE*);
void lit_print_table(literal_record*, FILE*);

/*----------------------------------------------------------------------------*/

void byte_dump(unsigned int, unsigned int, char*, int);

void literal_dump(int, char*, unsigned int);

FILE* open_output_file(int, char*);
void close_output_file(FILE*, char*, int);
void hex_dump(unsigned int, char);
void hex_dump_flush(void);

void elf_dump(unsigned int, char);
void elf_new_section_maybe(void);
void elf_dump_out(FILE*, sym_table*);

void list_file_out(void);
void list_start_line(unsigned int, int);
void list_mid_line(unsigned int, char*, int);
void list_end_line(char*);
void list_symbols(FILE*, sym_table*);
void list_buffer_init(char*, unsigned int, int);
void list_hex(unsigned int, unsigned int, char*);

/*----------------------------------------------------------------------------*/

int skip_spc(char*, int);
char* file_path(char*);
char* pathname(char*, char*);
bool cmp_next_non_space(char*, int*, int, char);
bool test_eol(char);
unsigned int get_identifier(char*, unsigned int, char*, unsigned int);
bool alpha_numeric(char);
bool alphabetic(char);
unsigned char c_char_esc(unsigned char);
int get_num(char*, int*, int*, unsigned int);
int allow_error(unsigned int, bool, bool);

/*----------------------------------------------------------------------------*/
/* Global variables                                                           */

instr_set instruction_set;  // ARM or Thumb
char* input_file_name;
char* symbols_file_name;
char* list_file_name;
char* hex_file_name;
char* elf_file_name;
char* verilog_file_name;
FILE *fList, *fHex, *fElf, *fVerilog;
int symbols_stdout, list_stdout, hex_stdout, elf_stdout;  // Booleans
int verilog_stdout;
label_sort symbols_order;
int list_sym, list_kmd;

unsigned char Verilog_array[VERILOG_MAX];  // Assemble Verilog output in array
unsigned int verilog_mem_size;             // Size of buffer =used=

unsigned int sym_print_extras;  // <0> set for locals, <1> for literals

unsigned int arm_variant;

unsigned int assembly_pointer;  // Address at which to plant instruction
unsigned int def_increment;     // Offset reached from assembly_pointer
bool assembly_pointer_defined;
unsigned int entry_address;
bool entry_address_defined;
unsigned int data_pointer;     // Used for creating record offsets etc.
unsigned int undefined_count;  // Number of references to undefined variables
unsigned int defined_count;    // Number of variables defined this pass
unsigned int redefined_count;  // Number of variables redefined this pass
unsigned int pass_count;       // Pass number, starts at 0
unsigned int pass_errors;      // Errors occurred in this pass
bool div_zero_this_pass;       // Indicates /0 in pass, prevents code dump
bool dump_code;                // Allow output (false on last pass if error)

own_label* evaluate_own_label;  // Yuk! @@@@@
// Because evaluate needs to know if there is a local label on -current- line

literal_record* literal_list;  // Start of the list of literals from "LDR ="
literal_record* literal_head;  // The next literal record `expected'
literal_record* literal_tail;  // The last literal record `dumped'

local_label* loc_lab_list;      // Start of the list of local labels
local_label* loc_lab_position;  // The current local label position

size_record* size_record_list;     // Start of list of ADRL (etc.) lengths
size_record* size_record_current;  // Current record in above list
unsigned int size_changed_count;   // Number of `instruction' size changes

bool if_stack[IF_STACK_SIZE + 1];  // Used for nesting IF clauses
int if_SP;

unsigned int list_address;
unsigned int list_byte;
unsigned int list_line_position;  // Pos. in the src line copied to output
char list_buffer[LIST_LINE_LENGTH];

unsigned int hex_address;
bool hex_address_defined;
char hex_buffer[HEX_LINE_LENGTH];

int elf_section_valid;     // Flag: true if code dumped in elf_section
unsigned int elf_section;  // Current elf section number (for labels)
unsigned int elf_section_old;
bool elf_new_block;

elf_temp* elf_record_list;
elf_temp* current_elf_record;

sym_table* arch_table;  // Table of possible processor architectures
sym_table* operator_table;
sym_table* register_table;
sym_table* cregister_table;
sym_table* copro_table;
sym_table* shift_table;

// Recursion for INCLUDE files
void code_pass(FILE*& fHandle,
               char*& filename,
               std::string& line,
               unsigned int& error_code,
               sym_table_item*& thumb_mnemonic_list,
               sym_table_item*& arm_mnemonic_list,
               sym_table*& symbol_table,
               bool& last_pass,
               bool& finished) {
  unsigned int line_number;
  char* include_file_path;  // Path as far as directory of "filename"
  char* include_name;
  FILE* incl_handle;

  include_file_path = file_path(filename);  // Path to directory in use
  line_number = 1;

  while (!feof(fHandle)) {
    include_name = NULL;                     // Don't normally return anything
    input_line(fHandle, line, LINE_LENGTH);  // Errors ignored

    if (instruction_set == THUMB) {
      error_code =
          parse_source_line(line, thumb_mnemonic_list, symbol_table, pass_count,
                            last_pass, &include_name, include_file_path);
    } else {
      error_code =
          parse_source_line(line, arm_mnemonic_list, symbol_table, pass_count,
                            last_pass, &include_name, include_file_path);
    }

    if (error_code != EVAL_OKAY) {
      print_error(line, line_number, error_code, filename, last_pass);
    } else if (include_name != NULL) {
      char* pInclude;

      if (include_name[0] == '/') {
        pInclude = include_name;  // Absolute
      }
      // Relative path - create new string
      else {
        pInclude = pathname(include_file_path, include_name);  // Add path
      }

      if ((incl_handle = fopen(pInclude, "r")) == NULL) {
        print_error(line, line_number, SYM_NO_INCLUDE, filename, last_pass);
        fprintf(stderr, "Can't open \"%s\"\n", include_name);
        finished = true;
      } else {
        code_pass(incl_handle, pInclude, line, error_code, thumb_mnemonic_list,
                  arm_mnemonic_list, symbol_table, last_pass, finished);
        fclose(incl_handle);  // Doesn't leave file locked
      }
      if (pInclude != include_name) {
        free(pInclude);  // If allocated (yuk)
      }
      free(include_name);
    }
    line_number++;  // Local to file
  }

  free(include_file_path);
}

// Create and initialise a symbol table
sym_table* build_table(char* table_name,
                       unsigned int flags,
                       std::string* sym_names,
                       int* values) {
  sym_table* table;
  int i;
  sym_record* dummy;  // Don't want returned pointer

  table = sym_create_table(table_name, flags);

  for (i = 0; *(sym_names[i]) != '\0'; i++)  // repeat until "" found
    sym_define_label(sym_names[i], values[i], 0, table, &dummy);

  return table;
}

/**
 * @brief Entry point 🎉
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char* argv[]) {
  FILE *fMnemonics, *fSource;
  std::string line(LINE_LENGTH + 1, '\0');

  sym_table *arm_mnemonic_table, *thumb_mnemonic_table, *directive_table;
  sym_table* symbol_table;
  sym_table_item *arm_mnemonic_list, *thumb_mnemonic_list;  // Real lists
  bool finished, last_pass;
  unsigned int error_code;

  arm_mnemonic_list = NULL;
  thumb_mnemonic_list = NULL;
  symbols_file_name = ""; /* Defaults */
  list_file_name = "";
  hex_file_name = "";
  elf_file_name = "";
  verilog_file_name = "";
  symbols_stdout = false;
  list_stdout = false;
  hex_stdout = false;
  elf_stdout = false;
  verilog_stdout = false;
  verilog_mem_size = VERILOG_MAX; /* Default to maximum size */

  int result = set_options(argc, argv);

  if (result == 144) {
    exit(443);
  }

  if (result) /* Parse command line and set options accordingly */
  {           /* We have a source file name, at least! */
    char *pChar, full_name[200];  // Size? @@

    /* Set up tables of operators, etc. */

    { /* Architecture names */
      std::string arch_name[] = {"v3",    "v3m",    "v4",   "v4xm", "v4t",
                                 "v4txm", "v5",     "v5xm", "v5t",  "v5txm",
                                 "v5te",  "v5texp", "all",  "any",  ""};

      int arch_value[] = {v3,  v3M,   v4,   v4xM,   v4T, v4TxM, v5, v5xM,
                          v5T, v5TxM, v5TE, v5TExP, 0,   0,     -1};

      arch_table = build_table("Architectures", SYM_TAB_CASE_FLAG, arch_name,
                               arch_value);
    }

    { /* Diadic expression operator definitions */
      std::string op_name[] = {"and", "or",  "xor", "eor", "shl", "lsl", "shr",
                               "lsr", "div", "mod", "eq",  "ne",  "lo",  "ls",
                               "hi",  "hs",  "lt",  "le",  "gt",  "ge",  ""};
      int op_value[] = {AND,        OR,          XOR,          XOR,
                        LEFT_SHIFT, LEFT_SHIFT,  RIGHT_SHIFT,  RIGHT_SHIFT,
                        DIVIDE,     MODULUS,     EQUALS,       NOT_EQUAL,
                        LOWER_THAN, LOWER_EQUAL, HIGHER_THAN,  HIGHER_EQUAL,
                        LESS_THAN,  LESS_EQUAL,  GREATER_THAN, GREATER_EQUAL,
                        -1};

      operator_table =
          build_table("Operators", SYM_TAB_CASE_FLAG, op_name, op_value);
    }

    { /* Register name definitions */
      std::string reg_name[] = {"r0",  "r1",  "r2", "r3",  "r4",  "r5",  "r6",
                                "r7",  "r8",  "r9", "r10", "r11", "r12", "r13",
                                "r14", "r15", "sp", "lr",  "pc",  "a1",  "a2",
                                "a3",  "a4",  "v1", "v2",  "v3",  "v4",  "v5",
                                "sb",  "v6",  "sl", "v7",  "fp",  "ip",  ""};
      int reg_value[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                         12, 13, 14, 15, 13, 14, 15, 0,  1,  2,  3,  4,
                         5,  6,  7,  8,  9,  9,  10, 10, 11, 12, -1};

      register_table =
          build_table("Registers", SYM_TAB_CASE_FLAG, reg_name, reg_value);
    }

    { /* Coprocessor register name definitions */
      std::string creg_name[] = {
          "cr0", "cr1",  "cr2",  "cr3",  "cr4",  "cr5",  "cr6",  "cr7", "cr8",
          "cr9", "cr10", "cr11", "cr12", "cr13", "cr14", "cr15", "c0",  "c1",
          "c2",  "c3",   "c4",   "c5",   "c6",   "c7",   "c8",   "c9",  "c10",
          "c11", "c12",  "c13",  "c14",  "c15",  ""};
      int creg_value[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                          11, 12, 13, 14, 15, 0,  1,  2,  3,  4,  5,
                          6,  7,  8,  9,  10, 11, 12, 13, 14, 15, -1};

      cregister_table = build_table("Copro_Registers", SYM_TAB_CASE_FLAG,
                                    creg_name, creg_value);
    }

    { /* Coprocessor name definitions */
      std::string copro_name[] = {
          "p0",   "p1",   "p2",   "p3",   "p4",   "p5",  "p6",  "p7",  "p8",
          "p9",   "p10",  "p11",  "p12",  "p13",  "p14", "p15", "cp0", "cp1",
          "cp2",  "cp3",  "cp4",  "cp5",  "cp6",  "cp7", "cp8", "cp9", "cp10",
          "cp11", "cp12", "cp13", "cp14", "cp15", ""};
      int copro_value[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                           11, 12, 13, 14, 15, 0,  1,  2,  3,  4,  5,
                           6,  7,  8,  9,  10, 11, 12, 13, 14, 15, -1};

      copro_table = build_table("Coprocessors", SYM_TAB_CASE_FLAG, copro_name,
                                copro_value);
    }

    { /* Shift definitions */
      std::string shift_name[] = {"lsl", "asl", "lsr", "asr", "ror", "rrx", ""};
      int shift_value[] = {0, 0, 1, 2, 3, 7, -1};
      shift_table =
          build_table("Shifts", SYM_TAB_CASE_FLAG, shift_name, shift_value);
    }

    arm_mnemonic_table = sym_create_table("ARM Mnemonics", SYM_TAB_CASE_FLAG);
    thumb_mnemonic_table =
        sym_create_table("Thumb Mnemonics", SYM_TAB_CASE_FLAG);
    directive_table = sym_create_table("Directives", SYM_TAB_CASE_FLAG);

    /* Following is crude hack for test/commissioning purposes.        @@@@@ */
    // realpath(argv[0], full_name);  // full path to binary (?) @@

    // This if statement was added by Lawrence Warren 27/11/2020.
    // Depends on using a Unix system which contains `proc/self/exe`
    // To find the absolute filepath of an executable.
    if (readlink("/proc/self/exe", full_name, sizeof(full_name)) == -1) {
      printf("Getting file path failed!");
      exit(1);
    }

    for (pChar = full_name; *pChar != '\0'; pChar++)
      ;  // find end of string @@
    while (*pChar != '/')
      pChar--;                       // Cut off last element
    pChar[1] = '\0';                 // Terminate
    strcat(full_name, "mnemonics");  // Then append filename

    if ((fMnemonics = fopen(full_name, "r")) == NULL) /* Read mnemonics */
      fprintf(stderr, "Can't open %s\n", "mnemonics");
    else {
      while (!feof(fMnemonics)) {
        input_line(fMnemonics, line, LINE_LENGTH); /* Errors ignored @@@ */
        if (!parse_mnemonic_line(line, arm_mnemonic_table, thumb_mnemonic_table,
                                 directive_table))
          fprintf(stderr, "Mnemonic file error\n %s\n", &line[0]);
      }
      /* no error checks @@@ */
      fclose(fMnemonics);

      { /* Make up mnemonic table lists */
        sym_table_item *pMnem, *pDir;

        pMnem = (sym_table_item*)malloc(SYM_TABLE_ITEM_SIZE); /* ARM defns. */
        pDir = (sym_table_item*)malloc(SYM_TABLE_ITEM_SIZE);  /* (hand built) */
        pMnem->pTable = arm_mnemonic_table;
        pMnem->pNext = pDir;
        pDir->pTable = directive_table;
        pDir->pNext = NULL;
        arm_mnemonic_list = pMnem;

        pMnem = (sym_table_item*)malloc(SYM_TABLE_ITEM_SIZE); /* Thumb defns. */
        pDir = (sym_table_item*)malloc(SYM_TABLE_ITEM_SIZE);  /* (hand built) */
        pMnem->pTable = thumb_mnemonic_table;
        pMnem->pNext = pDir;
        pDir->pTable = directive_table;
        pDir->pNext = NULL;
        thumb_mnemonic_list = pMnem;
      }

      symbol_table =
          sym_create_table("Labels", 0);  // Labels are case sensitive
      literal_list = NULL;
      loc_lab_list = NULL;
      size_record_list = NULL;

      pass_count = 0;
      finished = false;
      last_pass = false;
      dump_code = false;

      fHex = open_output_file(hex_stdout, hex_file_name);    /* Open required */
      fList = open_output_file(list_stdout, list_file_name); /*  output files */
      fElf = open_output_file(elf_stdout, elf_file_name);
      fVerilog = open_output_file(verilog_stdout, verilog_file_name);

      if ((fList != NULL) && list_kmd)
        fprintf(fList, "KMD\n"); /* KMD marker */

      if ((fSource = fopen(input_file_name, "r")) == NULL) /* Read file in */
      {
        fprintf(stderr, "Can't open %s\n", input_file_name);
        exit(144);  // Return 144 for can't open input
        finished = true;
      } else
        finished = false;

      if (fVerilog != NULL) {
        int i;
        for (i = 0; i < verilog_mem_size; i++)
          Verilog_array[i] = 0;
      }

      while (!finished) {
        instruction_set = ARM; /* Default */
        assembly_pointer = 0;  /* Default */
        data_pointer = 0;
        entry_address = 0;
        assembly_pointer_defined = true; /* ??? @@@@  Okay for us! */
        entry_address_defined = false;
        arm_variant = 0; /* Default to any ARM architecture */
        pass_errors = 0;
        div_zero_this_pass = false;
        hex_address_defined = false;
        elf_new_block = true;
        undefined_count = 0; /* Reads of undefined variables on this pass */
        defined_count = 0;   /* Labels newly defined on this pass */
        redefined_count = 0; /* Labels with values changed on this pass */
        if_SP = 0;
        if_stack[0] = true;
        elf_section_valid = false; /* No bytes dumped yet */
        elf_section = 1;
        literal_head = NULL;
        literal_tail = NULL;
        loc_lab_position = NULL;
        size_record_current = size_record_list; /* Go to front of list */
        size_changed_count = 0;

        rewind(fSource); /* Ensure at start of file */

        code_pass(fSource, input_file_name);
        /* no error checks @@@ */

        if (literal_tail != literal_head) /* Clear the literal pool */
        {
          char* literals = "Remaining literals";

          if (fList != NULL)
            list_start_line(assembly_pointer, false);
          literal_dump(last_pass, literals, 0); /*  Much like an instruction */
          if (fList != NULL)
            list_end_line(literals);
        }

        hex_dump_flush(); /* Ensure buffer is cleared */

        if (pass_errors != 0) {
          finished = true;
          printf("Pass %2d: terminating due to %d error", pass_count,
                 pass_errors);
          if (pass_errors == 1)
            printf("\n");
          else
            printf("s\n");
        } else {
          if (last_pass || (pass_count > MAX_PASSES))
            finished = true;
          else {
            if ((defined_count == 0) && (redefined_count == 0) &&
                (undefined_count == 0)) {
              last_pass = true;                /* One more time ... */
              dump_code = !div_zero_this_pass; /* If error don't plant code */
            }
            pass_count++;
          }
        }

        if (if_SP != 0) {
          printf("Pass completed with IF clause still open; terminating\n");
          finished = true;
        }

      } /* End of WHILE */

      if (fSource != NULL)
        fclose(fSource);

      if ((fList != NULL) && list_sym)
        list_symbols(fList, symbol_table);
      /* Symbols into list file */

      if (fVerilog != NULL) /* Dump memory image to hex file */
      {
        int i;
        for (i = 0; i < verilog_mem_size; i++) {
          fprintf(fVerilog, "%02X", Verilog_array[i ^ 3]); /* Big endian */
          if ((i & 3) == 3)
            fprintf(fVerilog, "\n");
        }
      }

      close_output_file(fList, list_file_name, pass_errors != 0);
      close_output_file(fHex, hex_file_name, pass_errors != 0);
      close_output_file(fVerilog, verilog_file_name, pass_errors != 0);

      if (fElf != NULL)
        elf_dump_out(fElf, symbol_table); /* Organise & o/p ELF */

      if (pass_count > MAX_PASSES) {
        printf("Couldn't do it ... fed up!\n\n");
        printf("Undefined labels:\n");
        sym_print_table(symbol_table, UNDEFINED, ALPHABETIC, true, "");
      } else {
        if (symbols_stdout || (symbols_file_name[0] != '\0')) {
          sym_print_table(symbol_table, ALL, symbols_order, symbols_stdout,
                          symbols_file_name);
          if (!symbols_stdout)
            printf("Symbols in file: %s\n", symbols_file_name);
        }

        if (pass_errors == 0) {
          if (list_file_name[0] != '\0')
            printf("List file in: %s\n", list_file_name);
          if (hex_file_name[0] != '\0')
            printf("Hex dump in: %s\n", hex_file_name);
          if (elf_file_name[0] != '\0')
            printf("ELF file in: %s\n", elf_file_name);
          if (verilog_file_name[0] != '\0') {
            printf("Verilog file in: %s", verilog_file_name);
            printf("  size: %X (decimal %d) words\n", verilog_mem_size,
                   verilog_mem_size);
          }
        } else
          printf("No output generated.\n"); /* Errors => trash output files */

        if (pass_count == 1)
          printf("\n1 pass performed.\n");
        else
          printf("\nComplete.  %d passes performed.\n", pass_count);
      }

      { /* Free local label list */
        local_label* pTemp;
        while ((pTemp = loc_lab_list) != NULL) /* Syntactically grubby! */
        {
          loc_lab_list = loc_lab_list->pNext;
          free(pTemp);
        }
      }

      { /* Free literal list */
        literal_record* pTemp;
        while ((pTemp = literal_list) != NULL) /* Syntactically grubby! */
        {
          literal_list = literal_list->pNext;
          free(pTemp);
        }
      }

      { /* Free size list */
        size_record* pTemp;
        while ((pTemp = size_record_list) != NULL) /* Syntactically grubby! */
        {
          size_record_list = size_record_list->pNext; /* Cut out first record */
          free(pTemp);                                /*  and delete it */
        }
      }

      { /* Clear away lists of symbol tables */
        sym_table_item *p1, *p2;

        p1 = arm_mnemonic_list;
        while (p1 != NULL) {
          p2 = p1;
          p1 = p1->pNext;
          free(p2);
        }
        p1 = thumb_mnemonic_list;
        while (p1 != NULL) {
          p2 = p1;
          p1 = p1->pNext;
          free(p2);
        }
      }

      sym_delete_table(symbol_table, false);
      sym_delete_table(directive_table, false);
      sym_delete_table(arm_mnemonic_table, false);
      sym_delete_table(thumb_mnemonic_table, false);
      sym_delete_table(arch_table, false);
      sym_delete_table(operator_table, false);
      sym_delete_table(register_table, false);
      sym_delete_table(cregister_table, false);
      sym_delete_table(copro_table, false);
      sym_delete_table(shift_table, false);
    }
  } else
    printf("No input file specified\n");

  if (pass_errors == 0)
    exit(0);
  else
    exit(-1);
}

/*----------------------------------------------------------------------------*/
/*					// Allow omission of spaces? @@@@
                                        // Allow filename first ?    @@@@*/
bool set_options(int argc, char* argv[]) {
  void file_option(int* std_out, char** filename, char* err_mss) {
    if (argc > 2) {
      if ((argv[1])[0] == '-')
        *std_out = true;
      else {
        *filename = &(*argv[1]);
        argc--;
        argv++;
      }
    } else
      printf("%s filename omitted\n", err_mss);
    return;
  }

  bool okay;
  char c;

  okay = false;

  if (argc == 1) {
    printf("ARM assembler v0.28 (23/03/15)\n");
    printf("Usage: %s <options> filename\n", argv[0]);
    printf("Options:    -e <filename>  specify ELF output file\n");
    printf("            -h <filename>  specify hex dump file\n");
    printf("            -l <filename>  specify list file\n");
    printf("                -ls appends symbol table\n");
    printf("                -lk produces a KMD file\n");
    printf("            -s <filename>  specify symbol table file\n");
    printf("                -sd gives symbols in order of definition\n");
    printf("                -sv gives symbols sorted by value\n");
    printf("                -sl includes local labels\n");
    printf("                -sp includes automatically generated literals\n");
    printf("            -v <filename>  specify Verilog readmemh file\n");
    printf("                [<hex number>] following name sets file size\n");
    printf("                    default (max) = %08X\n", VERILOG_MAX);
    printf("                    (output aliases modulo this size)\n");
    printf("Omitting a filename (or using '-') directs to stdout\n");
  } else {
    argv++; /* Next pointer */

    while ((argc > 1) && ((*argv)[0] == '-')) {
      c = (*argv)[1];
      switch (c) {
        case '\0':
          break; /* Can be used as a non-filename */

        case 'E':
        case 'e':
          file_option(&elf_stdout, &elf_file_name, "Elf file");
          break;

        case 'H':
        case 'h':
          file_option(&hex_stdout, &hex_file_name, "Hex dump");
          break;

        case 'L':
        case 'l':
          list_sym =
              ((((*argv)[2] & 0xDF) == 'S') || (((*argv)[2] & 0xDF) == 'K'));
          /* 'S' or 'K' dumps symbols too */
          list_kmd = (((*argv)[2] & 0xDF) == 'K'); /* K inserts "KMD" header */
          file_option(&list_stdout, &list_file_name, "List");
          break;

        case 'S':
        case 's': {
          int pos;
          pos = 2;
          switch ((*argv)[pos]) {
            case 'v':
            case 'V':
              symbols_order = VALUE;
              pos++;
              break;
            case 'd':
            case 'D':
              symbols_order = DEFINITION;
              pos++;
              break;
            default:
              symbols_order = ALPHABETIC;
              break;
          }

          while ((*argv)[pos] != '\0') {
            if (((*argv)[pos] == 'l') || ((*argv)[pos] == 'L'))
              sym_print_extras |= 1;
            if (((*argv)[pos] == 'p') || ((*argv)[pos] == 'P'))
              sym_print_extras |= 2;
            pos++;
          }
          file_option(&symbols_stdout, &symbols_file_name, "Symbol");
        } break;

        case 'V':
        case 'v':
          file_option(&verilog_stdout, &verilog_file_name,
                      "Verilog memory format");

          if (argc > 2) {
            if ((argv[1])[0] == '[') {
              unsigned int x, p;
              x = 0;
              p = 1;

              if (get_num(argv[1], &p, &x, 16)) /* Input value */
                if (x < verilog_mem_size)
                  verilog_mem_size = x; /* Clip to max.*/

              if ((argv[1])[p] != ']')
                printf("Please close the brackets!\n");

              argc--;
              argv++;
            }
          }
          break;

        default:
          printf("Unknown option %c\n", c);
          break;
      }
      argc--; /* Remove parameter from count */
      argv++; /* Next pointer */
    }

    if (argc > 1) {
      input_file_name = *argv;

      // Checks if the file path is valid
      FILE* temp = fopen(input_file_name, "r");

      if (temp == NULL) {
        fclose(temp);
        return 144;
      }

      printf("Input file: %s\n", input_file_name);
      okay = true;
    }
  }
  return okay;
}

/*----------------------------------------------------------------------------*/

void print_error(std::string& line,
                 unsigned int line_no,
                 unsigned int error_code,
                 char* filename,
                 bool last_pass) {
  unsigned int position;
  int i;

  if ((error_code & WARNING_ONLY) != 0) {
    if (!last_pass)
      return; /* Barf! */
    else
      printf("Warning: ");
  } else
    pass_errors++; /* Don't tally warnings */

  /*  The position on the line is in the bottom 8 bits; 0 indicates undefined.
   */
  position = error_code & 0x000000FF;

  switch (error_code & 0xFFFFFF00) {
    case SYM_ERR_SYNTAX:
      printf("Syntax error");
      break;
    case SYM_ERR_NO_MNEM:
      printf("Mnemonic not found");
      break;
    case SYM_ERR_NO_EQU:
      printf("Label missing");
      break;
    case SYM_BAD_REG:
      printf("Bad register");
      break;
    case SYM_BAD_REG_COMB:
      printf("Illegal register combination");
      break;
    case SYM_NO_REGLIST:
      printf("Register list required");
      break;
    case SYM_NO_RSQUIGGLE:
      printf("Missing '}'");
      break;
    case SYM_OORANGE:
      printf("Value out of range");
      break;
    case SYM_ENDLESS_STRING:
      printf("String unterminated");
      break;
    case SYM_DEF_TWICE:
      printf("Label redefined");
      break;
    case SYM_NO_COMMA:
      printf("',' expected");
      break;
    case SYM_GARBAGE:
      printf("Garbage");
      break;
    case SYM_ERR_NO_EXPORT:
      printf("Exported label not defined");
      break;
    case SYM_INCONSISTENT:
      printf("Label redefined inconsistently");
      break;
    case SYM_ERR_NO_FILENAME:
      printf("Filename missing");
      break;
    case SYM_NO_LBR:
      printf("'[' expected");
      break;
    case SYM_NO_RBR:
      printf("']' expected");
      break;
    case SYM_ADDR_MODE_ERR:
      printf("Error in addressing mode");
      break;
    case SYM_ADDR_MODE_BAD:
      printf("Illegal addressing mode");
      break;
    case SYM_NO_LSQUIGGLE:
      printf("'{' expected");
      break;
    case SYM_OFFSET_TOO_BIG:
      printf("Offset out of range");
      break;
    case SYM_BAD_COPRO:
      printf("Coprocessor specifier expected");
      break;
    case SYM_BAD_VARIANT:
      printf("Instruction not available");
      break;
    case SYM_NO_COND:
      printf("Conditional execution forbidden");
      break;
    case SYM_BAD_CP_OP:
      printf("Bad coprocessor operation");
      break;
    case SYM_NO_LABELS:
      printf("No labels! Position uncertain");
      break;
    case SYM_DOUBLE_ENTRY:
      printf("Entry already defined");
      break;
    case SYM_NO_INCLUDE:
      printf("Include file missing");
      break;
    case SYM_NO_BANG:
      printf("'!' expected");
      break;
    case SYM_MISALIGNED:
      printf("Offset misaligned");
      break;
    case SYM_OORANGE_BRANCH:
      printf("Branch out of range");
      break;
    case SYM_UNALIGNED_BRANCH:
      printf("Branch to misaligned target");
      break;
    case SYM_VAR_INCONSISTENT:
      printf("Variable redefined inconsistently");
      break;
    case SYM_NO_IDENTIFIER:
      printf("Identifier expected");
      break;
    case SYM_MANY_IFS:
      printf("Too many nested IFs");
      break;
    case SYM_MANY_FIS:
      printf("ENDIF without an IF");
      break;
    case SYM_LOST_ELSE:
      printf("Floating ELSE");
      break;
    case SYM_NO_HASH:
      printf("'#' expected");
      break;
    case SYM_NO_IMPORT:
      printf("Import file missing");
      break;
    case SYM_ADRL_PC:
      printf("Only ADR allowed with destination PC");
      break;
    case SYM_ERR_NO_SHFT:
      printf("Shift operator expected");
      break;
    case SYM_NO_REG_HASH:
      printf("'#' or register expected");
      break;
    case EVAL_NO_OPERAND:
      printf("Operand expected");
      break;
    case EVAL_NO_OPERATOR:
      printf("Operator expected");
      break;
    case EVAL_NO_CLOSEBR:
      printf("Missing ')'");
      break;
    case EVAL_NO_OPENBR:
      printf("Extra ')'");
      break;
    case EVAL_MATHSTACK_LIMIT:
      printf("Math stack overflow");
      break;
    case EVAL_NO_LIMIT:
      printf("Label not found");
      break;
    case EVAL_LABEL_UNDEF:
      printf("Label undefined");
      break;
    case EVAL_OUT_OF_RADIX:
      printf("Number out of radix");
      break;
    case EVAL_DIV_BY_ZERO:
      printf("Division by zero");
      break;
    case EVAL_OPERAND_ERROR:
      printf("Operand error");
      break;
    case EVAL_BAD_LOC_LAB:
      printf("Bad local label");
      break;
    case EVAL_NO_LABEL_YET:
      printf("Label not defined before this point");
      break;

    default:
      printf("Strange error");
      break;
  }
  printf(" on line %d of file: %s\n", line_no, filename);

  /*printf(line); printf("\n");           /* This suppresses '%' characters :-(
   */
  for (i = 0; line[i] != '\0'; i++)
    printf("%c", line[i]);
  printf("\n"); /* Yuk! */

  if (position > 0) /* else position not well defined */
  {
    for (i = 0; i <= position - 1; i++) /* 1 space less than the posn. */
      if (line[i] == '\t')
        printf("\t");
      else
        printf(" ");
    printf("^\n"); /* Mirrors TAB expansion (non-printing chars too? @@) */
  }

  return;
}

/*----------------------------------------------------------------------------*/

bool input_line(FILE* file, std::string& buffer, unsigned int max) {
  int i;
  char c;

  if (file != NULL) {
    i = 0;
    do {
      c = getc(file);
      if (!feof(file) && (i <= max - 1))
        buffer[i++] = c;
    } while ((c != '\n') && (c != '\r') && !feof(file));

    buffer[i] = '\0';  // Terminate
    if ((i > 0) && ((buffer[i - 1] == '\n') || (buffer[i - 1] == '\r'))) {
      buffer[i - 1] = '\0';  // Strip LF
    }

    if (c == '\r') {
      c = getc(file);  // Strip off any silly DOS-iness
    }
    if (c != '\n') {
      ungetc(c, file);  // Yuk! In case there's -just- a CR
    }

    return true;
  } else {
    return false;  // file not valid
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

bool parse_mnemonic_line(std::string& line,
                         sym_table* a_table,
                         sym_table* t_table,
                         sym_table* d_table) {
  int i, j, k, okay;
  unsigned int value, token;
  sym_record* dummy;
  char
      buffer[SYM_NAME_MAX + 5]; /* Largest suffix is 5 bytes, inc. terminator */
  char* conditions[] = {"eq", "ne", "cs hs", "cc lo", "mi", "pl", "vs", "vc",
                        "hi", "ls", "ge",    "lt",    "gt", "le", "al"};
  char* pCC;

  /* Wrapper to allow more suffixes (e.g. "S") on mnemonics  */
  void parse_mnem_variant(char* name2, unsigned int eos, unsigned int token2,
                          unsigned int variation) {
    void extra_letter(char letter,
                      unsigned int mask) /* Convenient abstraction */
    {
      buffer[eos] = letter;
      buffer[eos + 1] = '\0'; /* Terminator */
      sym_define_label(name2, token2 | mask, 0, a_table, &dummy);
      return;
    }

    switch (variation) {
      case 0x1: /* Arithmetic 'S' */
      case 0xD:
        sym_define_label(name2, token2, 0, a_table, &dummy);
        extra_letter('s', 0x00100000);
        break;

      case 0x2: /* Multiplies 'S' + others with just register lists */
        sym_define_label(name2, token2, 0, a_table, &dummy);
        switch (token2 & 0x0000F000) {
          case 0x00001000:
            extra_letter('s', 0x00100000);
            break; /* MUL etc. */
          case 0x00002000:
            extra_letter('b', 0x00400000);
            break; /* SWP      */
          case 0x00004000:
            extra_letter('s', 0x00100000);
            break; /* Shifts   */
          default:
            break; /* 0 - CLZ, 3 - QADD, 8-B - SMUL */
        }
        break;

      case 0x3: /* LDR/STR - not LDRH etc. */
        sym_define_label(name2, token2, 0, a_table, &dummy);
        extra_letter('t', 0x00001000); /* suffix 'T' */
        extra_letter('b', 0x00400000); /* suffix 'B' */
        eos++; /* Grubbily leaves previous 'B' in place */
        extra_letter('t', 0x00401000); /* suffix 'BT' */
        break;

      case 0x6: /* LDM/STM */
      {
        char* ldm_mode[] = {"da fa", "ia fd", "db ea", "ib ed"};
        char* stm_mode[] = {"da ed", "ia ea", "db fd", "ib fa"};
        char* pMode;
        int mode, k;

        for (mode = 0; mode < 4;
             mode++) /* Loop over possible addressing modes */
        {
          if ((token2 & 0x00100000) == 0)
            pMode = stm_mode[mode];
          else
            pMode = ldm_mode[mode];

          while (*pMode != '\0') /* While some items remain in string */
          {
            k = eos; /* Points at `original' terminator in buffer */
            while ((*pMode != '\0') && (*pMode != ' '))
              buffer[k++] = *(pMode++);
            buffer[k] = '\0'; /* Copy suffix and terminate it */
            sym_define_label(name2, token2 | (mode << 23), 0, a_table, &dummy);

            while (*pMode == ' ')
              pMode++; /* Skip spaces - next suffix (if any) */
          }
        }
      } break;

      case 0x9:                         /* LDRH etc. */
        extra_letter('h', 0x00000000);  /* suffix 'H' */
        if ((token2 & 0x00100000) != 0) /* Loads, only */
        {
          buffer[eos++] = 's';
          extra_letter('b', 0x00001000); /* suffix "SB" */
          extra_letter('h', 0x00002000); /* suffix "SH" */
        }
        break;

      case 0xB: /* LDC/STC */
        sym_define_label(name2, token2, 0, a_table, &dummy);
        extra_letter('l', 0x00400000);
        break;

      case 0xC:                        /* LDRD/STRD */
        extra_letter('d', 0x00000000); /* suffix 'D' */
        break;

      case 0x4: /* Branch */
      case 0x5: /* Miscellaneous */
      case 0x7: /* MRS/MSR */
      case 0x8: /* ADR */
      case 0xA: /* CDP + MCR/MRC */
      default:
        sym_define_label(name2, token2, 0, a_table, &dummy);
        break;
    }

    return;
  }

  i = skip_spc(line, 0);
  j = 0; /* Indicates end of `root' mnemonic */

  if (!test_eol(line[i])) /* Something on line - not comment */
  {
    while (alpha_numeric(line[i]) && (j < SYM_NAME_MAX))
      buffer[j++] = line[i++]; /* Mnemonics may start with numeric */
    buffer[j] = '\0';          /* Add terminator */

    okay = get_num(line, &i, &value, 16); /* Get hex number */
    /*  use evaluate() - mark real symbols for export and decimate before use
     * @@@ */

    if (okay) {
      if ((value & 0xF0000000) == 0xF0000000) /* Straight directive */
        sym_define_label(&buffer[0], value, 0, d_table, &dummy);
      else if ((value & 0x00000100) != 0) /* Thumb mnemonic */
        sym_define_label(&buffer[0], value, 0, t_table, &dummy);
      else {
        token = value & 0x0FFFFFFF;
        parse_mnem_variant(&buffer[0], j, 0xE0000000 | token,
                           (value >> 16) & 0xF);
        /* Straightforward "always" */

        if ((value & 0x40000000) != 0) /* Conditions too? */
        {
          for (i = 0; i < 0xF; i++) {
            pCC = conditions[i];
            while (*pCC != '\0') {
              k = j; /* Points at original terminator in buffer */
              while ((*pCC != '\0') && (*pCC != ' '))
                buffer[k++] = *(pCC++);
              buffer[k] = '\0'; /* Copy and terminate */
              parse_mnem_variant(&buffer[0], k, (i << 28) | token,
                                 (value >> 16) & 0xF);

              while (*pCC == ' ')
                pCC++; /* Skip spaces */
            }
          }
        }
      }
    }
  } else
    okay = true; /* Blank line or comment */

  return okay;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

unsigned int parse_source_line(std::string& line,
                               sym_table_item* mnemonic_list,
                               sym_table* symbols,
                               int pass_count,
                               bool last_pass,
                               char** include_name,
                               char* include_file_path) {
  int pos, j;
  own_label label_this_line;
  sym_record* ptr;
  char buffer[LINE_LENGTH];
  unsigned int value, error_code;
  bool mnemonic;

  error_code = SYM_NO_ERROR; /* @@@@ */
  mnemonic = false;
  label_this_line.sort = NO_LABEL;
  pos = skip_spc(line, 0);

  if (last_pass && (fList != NULL))
    list_start_line(assembly_pointer, false);

  if (!test_eol(line[pos])) /* Something on line - not comment */
  {
    if (get_num(line, &pos, &value,
                10)) /* Look for a `local' (numeric) label */
    {
      pos = skip_spc(line, pos);

      if (pass_count == 0) {
        local_label* pTemp;

        pTemp = (local_label*)malloc(LOCAL_LABEL_SIZE); /* New entry */
        pTemp->pNext = NULL;
        pTemp->pPrev = loc_lab_position;
        if (loc_lab_position == NULL)
          loc_lab_list = pTemp; /* First entry */
        else
          loc_lab_position->pNext = pTemp; /* Subsequent entry */
        loc_lab_position = pTemp;
        loc_lab_position->label = value;
        loc_lab_position->flags = 0;
      } else /* After the first pass */
      {
        if (loc_lab_position == NULL)
          loc_lab_position = loc_lab_list; /* 1st entry*/
        else
          loc_lab_position = loc_lab_position->pNext; /* Subsequent entry */
      }
      label_this_line.sort = LOCAL_LABEL;
      label_this_line.local = loc_lab_position;
    } else {
      if ((j = get_identifier(line, pos, buffer, LINE_LENGTH)) != 0)
      /* Element=>buffer */
      {
        pos = pos + j; /* Move position in line */
        if ((ptr = sym_find_label_list(buffer, mnemonic_list)) ==
            NULL) { /* Not a mnemonic */
          if (sym_locate_label(
                  buffer, /* Pass in flag if in Thumb area */
                  instruction_set == THUMB ? SYM_REC_THUMB_FLAG : 0, symbols,
                  &(label_this_line.symbol)))
            label_this_line.sort = SYMBOL; /* Found */
          else {
            if (pass_count == 0) /* First pass */
              label_this_line.sort =
                  MAYBE_SYMBOL; /* Could be reg. name, etc. */
          }
        } else
          mnemonic = true; /* Mnemonic first - no label on this line */
      } else
        error_code = pos | SYM_ERR_SYNTAX; /* 1st char. on line is non-alpha. */
    }
    /* If all is well, at this point the first item on the */
    /*  line has been identified, classified and stripped. */

    if ((error_code == EVAL_OKAY) && !mnemonic)
    /* Could check for other symbols (e.g. "=") first     @@@@ */
    {
      pos = skip_spc(line, pos); /* Find next item on line */
      if ((j = get_identifier(line, pos, buffer, LINE_LENGTH)) !=
          0) { /* Possible identifier found */
        if ((ptr = sym_find_label_list(buffer, mnemonic_list)) == NULL)
          error_code = pos | SYM_ERR_NO_MNEM; /*	//### */
        else {                                /* Mnemonic found */
          mnemonic = true;
          pos = pos + j; /* Move position in line */
        }
      } else {                    /* Nothing recognisable found on line */
        if (!test_eol(line[pos])) /* Effective EOL? */
        { /* Label(?) followed by something unexpected */
          error_code = pos | SYM_ERR_NO_MNEM;
        } else /* Just a label on this line */
        {
          if (label_this_line.sort ==
              MAYBE_SYMBOL) /* Uncertain only on 1st pass */
            sym_add_to_table(symbols, label_this_line.symbol);

          assemble_redef_label(assembly_pointer, assembly_pointer_defined,
                               &label_this_line, &error_code, 0, pass_count,
                               last_pass, line);
        }
      }
    }

    if ((error_code == EVAL_OKAY) && mnemonic) {
      /* Check lower bits of token against current instruction set */
      if ((ptr->value & arm_variant & 0x00000FFF) != 0)
        error_code = SYM_BAD_VARIANT; /* Disallowed in selected ARM variant */
      else
        error_code = assemble_line(line, pos, ptr->value, &label_this_line,
                                   symbols, pass_count, last_pass, include_name,
                                   include_file_path);
    }
  }

  if (last_pass) {
    if (fList != NULL)
      list_end_line(&line[0]);

    if ((label_this_line.sort == SYMBOL) &&
        ((label_this_line.symbol->flags & SYM_REC_EQUATED) == 0))
      label_this_line.symbol->elf_section = elf_section; /* Purely for ELF */
  }

  return error_code;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Do almost all the processing when an "LDR Rd, =value" is met.              */
/* On entry: instr_set is the current instruction set                         */
/*           size is the transfer size                                        */
/*           ext_value points to a location for the return value              */
/* On exit:  *ext_value will have been modified as appropriate to the         */
/*             returned value.                                                */
/* Returns:  0 if value can fit in a single MOV instruction                   */
/*             ext_value contains the immediate field                         */
/*           1 if value can fit in a single MVN instruction                   */
/*             ext_value contains the immediate field                         */
/*           2 if a load is required (data structures have been set up)       */
/*             ext_value contains the address to load                         */
/*          -1 on error                                                       */
/*             ext_value is undefined                                         */
/* External variables:  literal_head, literal_list, defined_count             */

int do_literal(instr_set instr_type,
               type_size size,
               int* ext_value,
               bool first_pass,
               unsigned int* pError) {
  unsigned int value;
  int what, PC_inc;
  literal_record* pTemp;

  value = *ext_value;

  if (instr_type == THUMB)
    PC_inc = 4;
  else
    PC_inc = 8;

  if (first_pass) { /* Create new literal entry */
    pTemp = (literal_record*)malloc(LIT_RECORD_SIZE);
    pTemp->pNext = NULL;
    if (literal_head != NULL)
      literal_head->pNext = pTemp;
    literal_head = pTemp;
    if (literal_list == NULL)
      literal_list = pTemp;

    if (*pError == EVAL_OKAY) {
      literal_head->flags = LIT_DEFINED; /* Zeros other flags */
      defined_count++;                   /* Is this a "label"?? @@@ */
      if (size == TYPE_HALF)
        literal_head->flags |= LIT_HALF;
      literal_head->value = value;
      if ((data_op_imm(value) >= 0) || (data_op_imm(~value) >= 0))
        literal_head->flags |= LIT_NO_DUMP; /* Short form */
    } /* Doesn't check for duplicate values on first pass */
    else
      literal_head->flags = 0; /* Not defined - or anything else */
  } else {                     /* First move definition pointer */
    if (literal_head == NULL)
      literal_head = literal_list;
    else
      literal_head = literal_head->pNext;

    if (*pError == EVAL_OKAY) {
      if ((literal_head->flags & LIT_DEFINED) == 0) /* undef? */
        defined_count++; /* Is this a "label"?? @@@ */
      else if (literal_head->value != value)
        redefined_count++;
      /*						// Is this a "label"??
       * @@@ */

      literal_head->flags |= LIT_DEFINED; /* Is, now ... */
      literal_head->value = value;        /* ... and this is it */

      if (((value & ~0x000000FF) == 0) /* Thumb test - all ARM values pass */
          || ((instr_type == ARM) && (data_op_imm(value) >= 0)))
      //                              || (data_op_imm(~value) >= 0))))
      {
        what = 0;                     /* Can do a MOV */
        *ext_value = value;           /* Return value */
        if (pass_count < SHRINK_STOP) /* If object code can still shrink ... */
          literal_head->flags |= LIT_NO_DUMP; /*  ... save word in lit. pool */
      } else {
        if ((instr_type == ARM) && (data_op_imm(~value) >= 0)) {
          what = 1;           /* Can do a MVN */
          *ext_value = value; /* Return value */
          if (pass_count <
              SHRINK_STOP) /* If object code can still shrink ... */
            literal_head->flags |=
                LIT_NO_DUMP; /*  ... save word in lit. pool */
        } else {
          bool found;

          what = 2;                            /* Needs a load */
          literal_head->flags &= ~LIT_NO_DUMP; /* Long form */
          /* -Needed- here to guarantee termination of loop below */
          pTemp = literal_list; /* Guaranteed not NULL */
          found = false;

          /* This searches whole assembly, no just currently pending pool */
          while (!found) /* Search for earlier, duplicate value */
          {              /* Always finds itself, if nothing else */
            while ((pTemp->value != literal_head->value) /* Search ...*/
                   || ((pTemp->flags & LIT_NO_DUMP) != 0))
              pTemp = pTemp->pNext; /* Find own value in list */

            if (!(found = (pTemp == literal_head))) /* Flag if self */
            { /*  else see if alternative `nearby' */
              if (((pTemp->flags & LIT_HALF) == 0) /* Alias is 32-bit */
                  || (size == TYPE_HALF))          /*  or I'm only 16-bit */
              {                                    /* Range check to alias */
                int range;

                range = pTemp->address - (assembly_pointer + PC_inc);
                if (instr_type == ARM) /* Range check of possible share */
                {
                  if (range < 0)
                    range = -range;
                  switch (size) {
                    case TYPE_WORD:
                      found = range < 4096;
                      break;
                    case TYPE_HALF:
                      found = range < 256;
                      break;
                    case TYPE_CPRO:
                      found = range < 1024;
                      break;
                  }
                } else /* Thumb is -much- more constrained */
                  found = (range >= 0) && (range < 1024);

              } /*  else not found (word can't alias to halfword) */
            }
            if (!found)
              pTemp = pTemp->pNext; /* Unlucky - try again */
          }                         /* End of outer `while' */

          if (pTemp != literal_head)
            literal_head->flags |= LIT_NO_DUMP; /* Shares literal */
          else
            literal_head->flags &= ~LIT_NO_DUMP; /* Unique */

          *ext_value = pTemp->address; /* Return value */
        }
      }
    }
  }

  if (*pError == EVAL_OKAY)
    return what;
  else
    return -1;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Break immediate into a set of ARM immediate fields in "buffer"             */
/* Returns number of entries required                                         */
// Compare & maybe amalgamate with "adr_loop"	@@@

unsigned int find_partials(unsigned int imm, unsigned int* buffer) {
  unsigned int i, count;

  if (imm == 0) {
    count = 1;
    buffer[0] = 0;
  } else {
    count = 0;
    while (imm != 0) /* Something to go at? */
    {
      i = 0;
      if (imm != 0)
        while ((imm & (3 << (2 * i))) == 0)
          i++;
      /* Find LS bit pair, unless zero */
      buffer[count] =
          data_op_imm(imm & (0xFF << (2 * i))); /* Isolate one byte */
      imm = imm & ~(0xFF << (2 * i));           /* Peel off that byte */
      count = count + 1;
    }
  }
  return count;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Make/maintain list of variable size items.                                 */
/* Inputs: first_pass - build or use flag                                     */
/*         size - desired size of item                                        */
/* Returns: size allocated (may be larger than desired)                       */
/* Globals: size_record_list, size_record_current, size_changed_count         */

unsigned int variable_item_size(int first_pass, unsigned int size) {
  if (first_pass) /* Build list of variable size elements */
  {
    size_record* pTemp;

    pTemp = (size_record*)malloc(SIZE_RECORD_SIZE); /* Append new record */
    pTemp->pNext = NULL;
    pTemp->size = size;
    if (size_record_list == NULL)
      size_record_list = pTemp; /* Link in first */
    else
      size_record_current->pNext = pTemp; /* or subsequent */
    size_record_current = pTemp;          /* Pointer to last in list */
  } else { /* Check for changes in object code size */
    if (size_record_current->size != size) {
      if ((pass_count < SHRINK_STOP)             /* Can still shrink */
          || (size_record_current->size < size)) /*  or grow */
      {
        size_record_current->size = size;
        size_changed_count++; /* Superfluous? @@@ */
      } else
        size = size_record_current->size; /* Size fixed */
    }
    size_record_current = size_record_current->pNext; /* Global ptr to next */
  }

  return size;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* If an INCLUDE <file> is found a string is allocated and pointed to by      */
/* include_name.                                                              */

unsigned int assemble_line(char* line,
                           unsigned int position,
                           unsigned int token,
                           own_label* my_label,
                           sym_table* symbol_table,
                           int pass_count,
                           bool last_pass,
                           char** include_name,
                           char* include_file_path) {
  unsigned int operand, error_code;
  unsigned int temp;
  int first_pass;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/

  void assemble_define(int size) { /* Elongated by string definition */
    bool terminate, escape;
    char delimiter, c;

    terminate = false;

    while (!terminate) {
      position = skip_spc(line, position);
      if (((line[position] == '"') || (line[position] == '\'') /*  String    */
           || (line[position] == '/') ||
           (line[position] == '`')))  /* delimiters */
      {                               /* Input string */
        delimiter = line[position++]; /* Strip & record delimiter */
        while ((line[position] != delimiter) && !terminate) {
          c = line[position];
          if (escape = (c == '\\')) /* C-style escape code */
            c = line[++position];   /* Get next character */

          if (c != '\0') {
            if (last_pass) { /* Bytes only */
              if (escape)
                c = c_char_esc(c);
              byte_dump(assembly_pointer + def_increment, c, line, size);
            }
            def_increment = def_increment + size; /* Always one address here */
            position++;
          } else { /* Line finished before string did */
            error_code = SYM_ENDLESS_STRING;
            terminate = true;
          }
        }
        if (!terminate)
          position = skip_spc(line, position + 1); /*Skip delimiter*/
      } else {
        error_code = evaluate(line, &position, &temp, symbol_table);
        /* Parse expression */
        if ((error_code == EVAL_OKAY) ||
            allow_error(error_code, first_pass, last_pass)) {
          if ((error_code == EVAL_OKAY) && last_pass) /* Plant, ltl endian */
            byte_dump(assembly_pointer + def_increment, temp, line, size);

          if (!last_pass)
            error_code = EVAL_OKAY; /* Pretend it's okay */
          def_increment += size;    /* Continue, even if missing values */
        } else
          terminate = true;
      }

      if (!terminate) {
        if (line[position] == ',')
          position++; /* Another element? */
        else
          terminate = true;
      }
    } /* End of WHILE */
    //##
    if (if_stack[if_SP])
      assembly_pointer += def_increment; /* Add total size at end */
    return;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/

  void fill_space(unsigned int count) { /* Fill with value */
    unsigned int fill;
    int i;

    position++; /* Skip comma */
    error_code = evaluate(line, &position, &fill, symbol_table);
    if (allow_error(error_code, first_pass, last_pass))
      error_code = EVAL_OKAY;

    if (last_pass && (error_code == EVAL_OKAY))
      for (i = 0; i < operand; i++)
        byte_dump(assembly_pointer + i, fill & 0xFF, line, 1);

    return;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/
  /* Read, range check and insert a 8-bit immediate offset for a load or store
   */
  /* "error_code" is `global' wrt this routine. */
  /* `variation' indicates the type of instruction */

  unsigned int ldr_offset(unsigned int op_code, unsigned int value,
                          type_size variation) {
    int x;

    if (!last_pass)
      return op_code; /* Crude, but clear! */

    if (error_code == EVAL_OKAY) {
      x = (int)value; /* Cast for convenience */
      if (x < 0)
        x = -x; /* Make offset positive */
      else
        op_code = op_code | 0x00800000; /*  or set U bit */

      switch (variation) /* Select on load/store type */
      {
        case TYPE_WORD:              /* LDR, LDRB, etc. */
          if ((x & 0xFFFFF000) == 0) /* Used to trap the `min_int' case too */
            op_code = op_code | x;
          else if (last_pass)
            error_code = SYM_OFFSET_TOO_BIG;
          break;

        case TYPE_HALF:              /* LDRH, LDRSB, etc. */
          if ((x & 0xFFFFFF00) == 0) /* Used to trap the `min_int' case too */
            op_code = op_code | ((x & 0xF0) << 4) | (x & 0x0F);
          else if (last_pass)
            error_code = SYM_OFFSET_TOO_BIG;
          break;

        case TYPE_CPRO:              /* LDC, STC, etc. */
          if ((x & 0xFFFFFC03) == 0) /* Used to trap the `min_int' case too */
            op_code = op_code | (x >> 2);
          else if (last_pass) {
            if ((x & 3) != 0)
              error_code = SYM_MISALIGNED;
            else
              error_code = SYM_OFFSET_TOO_BIG;
          }
          break;

        default:
          break;
      }
    }
    return op_code;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/
  /* Parse a data operation immediate field, explicitly or implicitly specified
   */
  /* The Boolean `translate' flag allows a second data operation to be tried if
   */
  /* the first attempt is out of range. */

  unsigned int data_op_immediate(char* line, unsigned int* pPosition,
                                 unsigned int op_code, int translate) {
    unsigned int value, ror_value;
    int x;

    error_code = evaluate(line, pPosition, &value, symbol_table);
    if (error_code == EVAL_OKAY) {
      if (line[*pPosition] == ',') /* Allow "ROR" instead/too? @@@ */
      {                            /* Explicit rotation code */
        (*pPosition)++;            /* Skip comma */
        error_code = evaluate(line, pPosition, &ror_value, symbol_table);
        if (error_code == EVAL_OKAY) {
          if (((value & 0xFFFFFF00) == 0) /* Check value ranges */
              && ((ror_value & 0xFFFFFFE1) == 0))
            op_code = op_code | (ror_value << 7) | value;
          else
            error_code = SYM_OORANGE;
        }
      } else {                  /* Rotation implicit */
        x = data_op_imm(value); /* Transform into rotate+field code */
        if (x >= 0)
          op_code = op_code | x;
        else { /* Out of range */
          int trans_type[] = {0,  -1, 1, -1, 1,  0, 0, -1,
                              -1, -1, 1, 1,  -1, 0, 0, 0};
          int trans_instr[] = {0xE, -1, 0x4, -1,  0x2, 0x6, 0x5, -1,
                               -1,  -1, 0xB, 0xA, -1,  0xF, 0x0, 0xD};
          int i;

          i = (token >> 21) & 0xF; /* Data operation specifier */

          if (translate &&
              (trans_type[i] >= 0)) {   /* Maybe can do as a different op. */
            x = ~value + trans_type[i]; /* 0 = not, 1 = negate */
            x = data_op_imm(x);         /* Try a transform ... */
            if (x >= 0) { /* Okay, so use new immediate and change operation */
              op_code = op_code | x;
              op_code = (op_code & 0xFE1FFFFF) | (trans_instr[i] << 21);
            } else
              error_code = SYM_OORANGE;
          } else
            error_code = SYM_OORANGE;
        }
      }
    }

    return op_code | 0x02000000; /* Set I bit */
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/
  /* Look for a register shift specifier from line[*pPosition] */
  /* Append the code, if found, to op_code, and return it (stripped from input)
   */
  /* reg_shift is a Boolean which allows a register as the distance specifier */
  /* "error_code" and "symbol_table" are `global' wrt this routine. */

  unsigned int addr_shift(char* line, unsigned int* pPosition,
                          unsigned int op_code, int reg_shift) {
    int shift;
    unsigned int value;

    shift = get_shift(line, pPosition);
    if (shift >= 0) /* Legitimate shift code */
    {
      if ((shift & 4) != 0)             /* Extended code */
        op_code = op_code | 0x00000060; /* RRX */
      else {
        op_code = op_code | (shift << 5); /* Insert shift op. code */
                                          /* (removed later if #0) */

        if (cmp_next_non_space(line, pPosition, 0, '#')) /* Check/strip '#' */
        {
          error_code = evaluate(line, pPosition, &value, symbol_table);

          if (error_code == EVAL_OKAY) {
            if (value == 0)
              op_code = op_code & 0xFFFFFF9F; /* Back to LSL */
            else if ((value & 0xFFFFFFC0) != 0)
              error_code = SYM_OORANGE; /* Coarse filter */
            else
              switch (shift) {
                case 0: /* LSL and ROR (#0 already done) */
                case 3:
                  if (value <= 31)
                    op_code = op_code | (value << 7);
                  else
                    error_code = SYM_OORANGE;
                  break;

                case 1: /* LSR and ASR */
                case 2:
                  if (value <= 32)
                    op_code = op_code | ((value & 0x1F) << 7);
                  else
                    error_code = SYM_OORANGE;
                  break;

                default:
                  break; /* Unreachable :-/ */
              }
          }
        } else {
          if (reg_shift) /* Allowed on data ops. but not load/store */
          {
            int reg;

            if ((reg = get_reg(line, &position)) >= 0)
              op_code = op_code | 0x00000010 | (reg << 8);
            else
              error_code = SYM_BAD_REG | position;
          } else
            error_code = SYM_ADDR_MODE_ERR | *pPosition; /* Only '#' legit. */
        }
      }
    } else /* Check for `dangling' end after comma */
      if (shift < -1)
        error_code = SYM_ERR_NO_SHFT | *pPosition;

    return op_code;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/
  /* Parses a LDM-style register list and returns bit vector indicating which */
  /* registers are found.  Checks validity against an `allowed' vector and */
  /* modifies (global) error_code if a bad register is requested. */

  unsigned int parse_reg_list(unsigned int allowed) {
    unsigned int list, RegA, RegB; /* Bit positions/mask */
    int reg;

    list = 0; /* Bitmask for collecting registers */

    do {
      if (line[position] == ',')
        position++; /* Applies after 1st iter.*/
      if ((reg = get_thumb_reg(line, &position, allowed)) >= 0) {
        RegA = 1 << reg;    /* Bit position */
        list = list | RegA; /* Put this register into mask */

        if (cmp_next_non_space(line, &position, 0,
                               '-')) { /* Register list coming up */
          if ((reg = get_thumb_reg(line, &position, allowed)) >= 0) {
            RegB = 1 << reg; /* Last position in list */

            while (RegA != RegB) /* Fill in the bits between */
            {                    /* Iterate up or down, as required */
              if (RegA < RegB)
                RegA = RegA << 1;
              else
                RegA = RegA >> 1;
              list = list | RegA;
            }
          } else
            error_code = SYM_BAD_REG | position; /* Bad list end reg. */
        } /* End of register list processing */
      } else
        error_code = SYM_BAD_REG | position; /* Bad individual/start reg. */
    } /* Iterate while no errors and comma separators encountered */
    while ((error_code == EVAL_OKAY) &&
           cmp_next_non_space(line, &position, 0, ','));

    if ((list & ~allowed) != 0)
      error_code = SYM_BAD_REG; /* Posn. uncertain */
    /* Else breakage possible if range used in Thumb (e.g. PUSH {R0-LR} ) */

    return list;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/
  /* Wrapper for parse_reg_list to deal with '{' '}' syntax (ARM only) */

  unsigned int parse_ARM_reg_list(unsigned int op_code,
                                  unsigned int line_offset) {
    if (!cmp_next_non_space(line, &position, line_offset, '{'))
      error_code = SYM_NO_LSQUIGGLE | position;
    else {
      op_code |= parse_reg_list(0x0000FFFF); /* Get bitmask of registers */

      if (error_code == EVAL_OKAY) {
        if (line[position] == '}') /* Check list terminated cleanly */
        {
          if (cmp_next_non_space(line, &position, 1, '^'))
            op_code = op_code | 0x00400000; /* Add S bit if required */
        } else
          error_code = SYM_NO_RSQUIGGLE | position;
      }
    }
    return op_code;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/
  /* Parse an addressing mode */
  /* size specifies the type of instruction and hence the particular syntax */
  /* Returns op_code with the new coding appended */

  unsigned int addressing_mode(unsigned int op_code, type_size size)

  { /* First, the parameters which differentiate LDR and LDRH */
    unsigned int word_masks[] = {0x01000000, 0x02000000, 0x03000000,
                                 0x010F0000};
    unsigned int half_masks[] = {0x01400000, 0x00000000, 0x01000000,
                                 0x014F0000};
    unsigned int cpro_masks[] = {0x01000000, 0x00000000, 0x01000000,
                                 0x010F0000};
    unsigned int* parameter;
    unsigned int value;
    int reg;

    switch (size) {
      case TYPE_WORD:
        parameter = word_masks;
        break;
      case TYPE_HALF:
        parameter = half_masks;
        break;
      case TYPE_CPRO:
        parameter = cpro_masks;
        break;
    }

    if (cmp_next_non_space(line, &position, 0, '[')) {
      if ((reg = get_reg(line, &position)) >= 0) {
        op_code = op_code | (reg << 16); /* Base register (Rn) */
        position = skip_spc(line, position);

        switch (line[position]) /* What succeeds base register? */
        {
          case ']': /* Post indexed or just "[Rn]" */
            if (!cmp_next_non_space(line, &position, 1, ','))
              op_code = op_code | parameter[0]; /* Just "[Rn]" */
            else {                              /* Post-indexed mode */
              if (cmp_next_non_space(line, &position, 0, '#')) /* Imm. ? */
              {
                error_code = evaluate(line, &position, &value, symbol_table);
                op_code = ldr_offset(op_code, value, size);
                switch (size) {
                  case TYPE_HALF:
                    op_code |= 0x00400000;
                    break; /* I bit */
                  case TYPE_CPRO:
                    op_code |= 0x00200000;
                    break; /* W bit */
                  default:
                    break;
                }
              } else { /* Register offset, post-indexed */
                if (size != TYPE_CPRO) {
                  op_code = op_code | parameter[1];
                  if (line[position] == '-')
                    position++; /* + or - op.? */
                  else {
                    op_code = op_code | 0x00800000; /* Set 'U' bit */
                    if (line[position] == '+')
                      position++;
                  }

                  if ((reg = get_reg(line, &position)) >= 0) /* Rm */
                  {
                    if (size == TYPE_WORD)
                      op_code =
                          addr_shift(line, &position, op_code | reg, false);
                    else
                      op_code = op_code | reg; /* No shift for LDRH */
                  } else
                    error_code = SYM_BAD_REG | position; /* No offset reg. */
                } else { /* LDC/STC "unindexed" mode */
                  if (line[position] == '{') {
                    position++;
                    error_code =
                        evaluate(line, &position, &value, symbol_table);
                    if (error_code == EVAL_OKAY) {
                      if ((value & 0xFFFFFF00) == 0x00000000) {
                        op_code = op_code | value | 0x00800000; /* Set U bit */
                        if (!cmp_next_non_space(line, &position, 0, '}'))
                          error_code = SYM_NO_RSQUIGGLE | position;
                      } else
                        error_code = SYM_OORANGE;
                    }
                  } else
                    error_code = SYM_NO_LSQUIGGLE | position;
                }
              }
            }
            break;

          case ',':
            if (cmp_next_non_space(line, &position, 1,
                                   '#')) { /* Pre-index immediate */
              op_code = op_code | parameter[0];
              error_code = evaluate(line, &position, &value, symbol_table);
              op_code = ldr_offset(op_code, value, size);
              /* Offset */
              if ((error_code == EVAL_OKAY) ||
                  allow_error(error_code, first_pass,
                              last_pass)) { /* May need to tolerate an error to
                                               check syntax */
                if (line[position] == ']') {
                  if (cmp_next_non_space(line, &position, 1, '!'))
                    op_code = op_code | 0x00200000; /* Set 'W' bit */
                } else
                  error_code = SYM_NO_RBR | position;
              }
            } else { /* Register offset, pre-indexed */
              if (size != TYPE_CPRO) {
                op_code = op_code | parameter[2];
                if (line[position] == '-')
                  position++; /* + or - operator? */
                else {
                  if (line[position] == '+')
                    position++;
                  op_code = op_code | 0x00800000; /* Set 'U' bit */
                }

                if ((reg = get_reg(line, &position)) >= 0) {
                  op_code = op_code | reg; /* Rm */
                  if (!cmp_next_non_space(line, &position, 0,
                                          ']')) { /* Something else here ... */
                    if (size == TYPE_WORD) {      /* Shift */
                      op_code = addr_shift(line, &position, op_code, false);

                      if ((error_code == EVAL_OKAY) ||
                          allow_error(error_code, first_pass, last_pass)) {
                        if (line[position] == ']')
                          position++;
                        else
                          error_code = SYM_NO_RBR | position;
                      }
                    } else
                      error_code = SYM_NO_RBR | position; /* LDRH */
                  }

                  if (cmp_next_non_space(line, &position, 0, '!'))
                    op_code = op_code | 0x00200000; /* Set 'W' bit */

                } else
                  error_code = SYM_BAD_REG | position; /* Rm not IDed */
              } else
                error_code = SYM_ADDR_MODE_BAD; /* Bad mode for LDC/STC */
            }
            break;

          default:
            error_code = SYM_ADDR_MODE_ERR;
            break;
        }

      } else
        error_code = SYM_BAD_REG | position; /* Base register not found */
    } else /* Addressing mode does not begin with '[' */
    {
      if (line[position] == '=') { /* Literal pool stuff */
        if ((size == TYPE_CPRO) || ((op_code & 0x00100000) == 0))
          error_code = SYM_ADDR_MODE_ERR; /* Disallowed in LDC and all stores */
        else {
          position++;
          error_code = evaluate(line, &position, &value, symbol_table);

          if ((error_code == EVAL_OKAY) ||
              allow_error(error_code, first_pass, last_pass)) {
            switch (do_literal(ARM, size, &value, first_pass, &error_code))
            /* Constant size? */
            {         /*  (literal table appended as necessary) */
              case 0: /* Will fit in instruction */
                op_code = op_code & 0xF000F000; /* Fields to keep */
                op_code = op_code | 0x03A00000 | data_op_imm(value); /* MOV */
                break;

              case 1:                           /* Will fit in instruction */
                op_code = op_code & 0xF000F000; /* Fields to keep */
                op_code = op_code | 0x03E00000 | data_op_imm(~value); /* MVN */
                break;

              case 2: /* Needs full-blooded LDR */
                op_code = ldr_offset(op_code | parameter[3],
                                     value - (assembly_pointer + 8), size);
                break;

              default:
                break;
            }
          }
        }
      } else { /* Try for `absolute' address */
        error_code = evaluate(line, &position, &value, symbol_table);
        if (error_code == EVAL_OKAY)
          op_code = ldr_offset(op_code | parameter[3],
                               value - (assembly_pointer + 8), size);
      }
    }
    return op_code;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/

  void arm_mnemonic() { /* Instructions, rather than directives */
    unsigned int op_code, value, extras;
    int reg;
    bool rrx; /* Flag (of convenience) identifying short RRX */

    extras = 0; /* Instruction length - 4 */

    if (first_pass || last_pass || ((token & 0x00008000) != 0)) /* -skip- */
    { /* Only do difficult stuff on first pass (syntax) and last pass (code
dump) unless instruction may cause file length to vary (e.g. LDR Rd, =###) */
      op_code = token & 0xFFF00000;

      rrx = ((token & 0x000FFF00) == 0x00024E00); /* Nasty irregular case */

      if (!rrx && ((token & 0x00000800) != 0))
        op_code |= 0xF0000000;
      /*`New' always codes */

      switch (token & 0x000F0000) {
        case 0x00000000: /* NOP (etc.) */
          switch (token & 0x00F00000) {
            case 0x00000000:
              op_code = 0xE1A00000;
              break;         /* NOP */
            case 0x00100000: /* undefined */
              op_code = (op_code & 0xF0000000) | 0x06000010;
              break;

            default:
              op_code = 0xE1A00000;
              break; /* NOP */
          }

          break;

        case 0x00010000:                 /* Data operations */
          if ((token & 0x00004000) != 0) /* Destination register wanted? */
            if ((reg = get_reg(line, &position)) >= 0) {
              op_code = op_code | (reg << 12);
              if (!cmp_next_non_space(line, &position, 0, ','))
                error_code = SYM_NO_COMMA | position;
            } else
              error_code = SYM_BAD_REG | position;

          if (error_code == EVAL_OKAY) {
            if ((token & 0x00002000) != 0) /* Source Rn wanted? */
              if ((reg = get_reg(line, &position)) >= 0) {
                op_code = op_code | (reg << 16);
                if (!cmp_next_non_space(line, &position, 0, ','))
                  error_code = SYM_NO_COMMA | position;
              } else
                error_code = SYM_BAD_REG | position;

            if (error_code ==
                EVAL_OKAY) { /* Rm always present, so check omitted */
              if (cmp_next_non_space(line, &position, 0, '#')) /* Imm. mode */
                op_code = data_op_immediate(line, &position, op_code, true);
              else { /* Register mode */
                if ((reg = get_reg(line, &position)) >= 0)
                  op_code = addr_shift(line, &position, op_code | reg, true);
                else
                  error_code = SYM_BAD_REG | position;
              }
            }
          }
          break;

        case 0x00020000:                        /* Multiply operations */
        {                                       /* and misc. later add-ons */
          int mul_regs[] = {16, 0, 8, -1};      /* Bit positions of */
          int mla_regs[] = {16, 0, 8, 12, -1};  /*    the various   */
          int mull_regs[] = {12, 16, 0, 8, -1}; /*  register fields */
          int swp_regs[] = {12, 0, -1};         /* First 2 for SWP */
          int clz_regs[] = {12, 0, -1};
          int qadd_regs[] = {12, 0, 16, -1};
          int shift_regs[] = {12, 0, 8, -1};
          int not_def[] = {-1};
          int *pRegs, i;

          switch (token & 0x0000F000) {
            case 0x00000000:
              pRegs = clz_regs;
              op_code |= 0x000F0F10;
              break;
            case 0x00002000:
              pRegs = swp_regs;
              op_code |= 0x00000090;
              break;
            case 0x00003000:
              pRegs = qadd_regs;
              op_code |= 0x00000050;
              break;
            case 0x00001000:
              op_code = op_code | 0x00000090;
              switch (token & 0x00E00000) /* Decide where the reg. fields go */
              {
                case 0x00000000:
                  pRegs = mul_regs;
                  break;
                case 0x00200000:
                  pRegs = mla_regs;
                  break;
                case 0x00800000:
                case 0x00A00000:
                case 0x00C00000:
                case 0x00E00000:
                  pRegs = mull_regs;
                  break;
                default:
                  pRegs = not_def;
                  break;
              }
              break;

            case 0x00004000: /* Alternative shift mnemonics */
              pRegs = shift_regs;
              op_code |= (token & 0x600) >> 4 | 0x00000010;
              break;

            case 0x00008000:
            case 0x00009000:
            case 0x0000A000:
            case 0x0000B000:
              op_code = op_code | ((token >> 7) & 0x60) | 0x00000080;
              switch (token & 0x00600000) /* Decide where the reg. fields go */
              {
                case 0x00000000:
                  pRegs = mla_regs;
                  break;
                case 0x00200000:
                  if ((token & 0x00001000) == 0)
                    pRegs = mla_regs;
                  else
                    pRegs = mul_regs;
                  break;
                case 0x00400000:
                  pRegs = mull_regs;
                  break;
                case 0x00600000:
                  pRegs = mul_regs;
                  break;
              }
              break;

            default:
              pRegs = not_def;
              break;
          }

          i = 0;

          while ((error_code == EVAL_OKAY) && (pRegs[i] >= 0)) {
            if ((reg = get_reg(line, &position)) >= 0) {
              if (rrx && (i == 2))
                error_code = SYM_ERR_SYNTAX; /* RRX, short form, has no Rs */
              else {
                op_code = op_code | (reg << pRegs[i]);
                i++;
                if (pRegs[i] >= 0) /* If there is a next register ... */
                {
                  if (!cmp_next_non_space(line, &position, 0, ',')) {
                    if (rrx) /* RRX, short form */
                    {
                      op_code &= 0xFFFFFFEF; /* Clear 'register' bit */
                      i++; /* Will move to control array terminator */
                    } else
                      error_code = SYM_NO_COMMA | position;
                  }
                } else if ((token & 0x0000F000) ==
                           0x00002000) /* SWP addr. bodged */
                {
                  if (!cmp_next_non_space(line, &position, 0, ','))
                    error_code = SYM_NO_COMMA | position;
                  else if (!cmp_next_non_space(line, &position, 0, '['))
                    error_code = SYM_NO_LBR | position;
                  else if ((reg = get_reg(line, &position)) >= 0) {
                    op_code = op_code | (reg << 16);
                    if (!cmp_next_non_space(line, &position, 0, ']'))
                      error_code = SYM_NO_RBR | position;
                  } else
                    error_code = SYM_BAD_REG | position;
                }
              }
            } else {
              if ((token & 0x000FF000) == 0x00024000) /* Shift immediate? */
              {
                if ((line[position] == '#') && (i == 2)) {
                  op_code &= 0xFFFFFFEF; /* Clear 'register' bit */
                  position++;
                  i++;
                  error_code = evaluate(line, &position, &value, symbol_table);
                  switch (token & 0x00000E00) {
                    case 0x000: /* LSL */
                      if (value > 31)
                        error_code = SYM_OORANGE;
                      break;
                    case 0x600: /* ROR */
                      if ((value < 1) || (value > 31))
                        error_code = SYM_OORANGE;
                      break;
                    case 0xE00: /* RRX */
                      if (value != 1)
                        error_code = SYM_OORANGE;
                      else
                        value = 0;
                      break;
                    default: /* Other shift types */
                      if ((value < 1) || (value > 32))
                        error_code = SYM_OORANGE;
                      else
                        value = value & 0x1F;
                      break;
                  }
                  op_code |= value << 7;
                } else { /* Over detailed error messages?! */
                  if (i < 2)
                    error_code = SYM_BAD_REG | position;
                  else if (!rrx)
                    error_code = SYM_NO_REG_HASH | position;
                  else
                    error_code = SYM_NO_HASH | position;
                }
              } else
                error_code = SYM_BAD_REG | position;
            }
          }
        } break;

        case 0x00030000: /* Normal LDR/STR operations */
        case 0x00090000: /* LDRH etc. */
        case 0x000C0000: /* LDRD/STRD */
        {
          int err_pos; /* Used if real register Rd is disallowed */

          err_pos = position;
          if ((reg = get_reg(line, &position)) >= 0) /* Reg. to transfer (Rd) */
          {
            if (((token & 0x000F0000) == 0x000C0000) && ((reg & 1) != 0))
              error_code = SYM_BAD_REG | skip_spc(line, err_pos);
            /* Even regs. only in LDRD; rederive position only if needed */
            else {
              op_code = op_code | (reg << 12);
              if (cmp_next_non_space(line, &position, 0, ',')) /* Comma found */
              {
                if ((token & 0x000F0000) == 0x00030000) /* LDR/STR */
                  op_code = addressing_mode(op_code, TYPE_WORD);
                else /* LDRH etc./LDRD */
                  op_code = addressing_mode(op_code, TYPE_HALF);
              } else
                error_code = SYM_NO_COMMA | position; /*',' after Rd missing*/
            }
          } else
            error_code = SYM_BAD_REG | position; /* Rd not found */

          switch (token & 0x000F0000) {
              /* Note: the following trap allows "[Rn]" for LDRT but also allows
               * "[Rn, #0]" */
              /*       to be translated too.  Is this acceptable?  @@ */
            case 0x00030000: /* LDR/STR only */
              if ((error_code == EVAL_OKAY) &&
                  ((token & 0x00001000) != 0)) {           /* 'T' option */
                if (((op_code & 0x01200000) == 0x00000000) /* Post ind. mode? */
                    ||
                    ((op_code & 0x02000FFF) == 0x00000000)) /*  or #0 offset? */
                  op_code = (op_code & 0xFEFFFFFF) | 0x00200000;
                /* Redefine addressing mode */
                else
                  error_code = SYM_ADDR_MODE_BAD;
              }
              break;

            case 0x00090000:              /* Short-form LD/ST */
              switch (token & 0x00007000) /* Which LDRH... instruction? */
              {
                case 0x00000000:
                  op_code = op_code | 0x000000B0;
                  break; /*LDRH */
                case 0x00001000:
                  op_code = op_code | 0x000000D0;
                  break; /*LDRSB*/
                case 0x00002000:
                  op_code = op_code | 0x000000F0;
                  break; /*LDRSH*/
                default:
                  break;
              }
              break;

            case 0x000C0000: /* LDRD/STRD */
              if ((token & 0x00001000) == 0)
                op_code |= 0x000000F0; /* STRD */
              else
                op_code |= 0x000000D0; /* LDRD */
              break;
          }
        } break;

        case 0x00040000: /* B and BL */
          error_code = evaluate(line, &position, &value, symbol_table);
          value = value - (assembly_pointer + 8);
          if ((value & 3) != 0) /* Not word aligned */
            error_code = SYM_UNALIGNED_BRANCH;
          else if (((value & 0xFE000000) == 0x00000000) /*  or out of range */
                   || ((value & 0xFE000000) == 0xFE000000))
            op_code = op_code | ((value >> 2) & 0x00FFFFFF);
          else
            error_code = SYM_OORANGE_BRANCH;
          break;

        case 0x00050000: /* Miscellany */
          switch (token & 0x0000F000) {
            case 0x00000000: /* SWI */
              error_code = evaluate(line, &position, &value, symbol_table);
              if ((value & 0xFF000000) == 0)
                op_code = op_code | value;
              else
                error_code = SYM_OORANGE;
              break;

            case 0x00001000: /* BX */
              if ((reg = get_reg(line, &position)) >= 0)
                op_code = op_code | 0x000FFF10 | reg;
              else
                error_code = SYM_BAD_REG | position; /* Rm not found */
              break;

            case 0x00002000: /* BKPT */
              error_code = evaluate(line, &position, &value, symbol_table);
              if ((value & 0xFFFF0000) == 0)
                op_code =
                    op_code | ((value & 0xFFF0) << 4) | 0x70 | (value & 0x000F);
              else
                error_code = SYM_OORANGE;
              break;

            case 0x00003000: /* BLX */
              if ((reg = get_reg(line, &position)) >= 0)
                op_code = op_code | 0x012FFF30 | reg;
              else                                      /* Rm not found */
                if ((token & 0xF0000000) == 0xE0000000) /* Only unconditional */
                {
                  error_code = evaluate(line, &position, &value, symbol_table);
                  value = value - (assembly_pointer + 8);

                  if ((value & 1) != 0) /* Not halfword aligned */
                    error_code = SYM_UNALIGNED_BRANCH;
                  else if (((value & 0xFE000000) == 0x00000000) /* In range? */
                           || ((value & 0xFE000000) == 0xFE000000))
                    op_code = 0xFA000000 | ((value & 0x03FFFFFC) >> 2) |
                              ((value & 2) << 23);
                  else
                    error_code = SYM_OORANGE_BRANCH;
                } else
                  error_code = SYM_NO_COND;
              break;

            case 0x00004000: /* PLD */
              op_code = addressing_mode(op_code | 0x0000F000, TYPE_WORD);
              if ((op_code & 0x01200000) !=
                  0x01000000) { /* Modes which write back are disallowed */
                op_code = 0x00000000;
                error_code = SYM_ADDR_MODE_BAD;
              }
              break;

            case 0x00005000: /* PUSH/POP */
              op_code = op_code | 0x000D0000 | parse_ARM_reg_list(op_code, 0);
              break;

            default:
              break;
          }
          break; /* End of case 0x00050000 "Miscellany" */

        case 0x00060000:                             /* STM/LDM */
          if ((reg = get_reg(line, &position)) >= 0) /* Base register (Rn) */
          {
            op_code = op_code | (reg << 16);

            if (cmp_next_non_space(line, &position, 0, '!')) {
              op_code = op_code | 0x00200000; /* Set W bit */
              position = skip_spc(line, position);
            }

            if (line[position] == ',')
              op_code = op_code | parse_ARM_reg_list(op_code, 1);
            else
              error_code = SYM_NO_COMMA | position;
          } else
            error_code = SYM_BAD_REG | position; /* Rn not found */

          break;

        case 0x00070000:                 /* MRS/MSR */
          if ((token & 0x00200000) == 0) /* MRS */
          {
            if ((reg = get_reg(line, &position)) >= 0) /* Register (Rd) */
            {
              op_code = op_code | (reg << 12);

              if (cmp_next_non_space(line, &position, 0, ',')) {
                unsigned int old_pos;

                position = skip_spc(line, position);
                old_pos = position;             /* Remember in case of error */
                reg = get_psr(line, &position); /* PSR */
                if ((reg >= 0) && ((reg & 0x0F) == 0x0F)) /* Reg. found */
                  op_code = op_code | (reg << 16);        /*  and legal here */
                /* The lower `field mask' fills the SBO field <19>-<16> */
                else
                  error_code = SYM_BAD_REG | old_pos;
              } else
                error_code = SYM_NO_COMMA | position;
            } else
              error_code = SYM_BAD_REG | position; /* Rd not found */
          } else                                   /* MSR */
          {
            position = skip_spc(line, position);
            reg = get_psr(line, &position); /* PSR */
            if (reg >= 0) {
              op_code = op_code | (reg << 16) | 0x0000F000;
              if (cmp_next_non_space(line, &position, 0, ',')) {
                if (cmp_next_non_space(line, &position, 0, '#')) /* Skip ',' */
                  op_code = data_op_immediate(line, &position, op_code, false);
                else {
                  if ((reg = get_reg(line, &position)) >= 0) /* Register (Rm) */
                    op_code = op_code | reg;
                  else
                    error_code = SYM_BAD_REG | position; /*Missing source reg.*/
                }
              } else
                error_code = SYM_NO_COMMA | position;
            } else
              error_code = SYM_BAD_REG | position;
          }
          break;

        case 0x00080000: /* ADR */
        {
          void adr_loop(int value, int count) /* Local procedure */
          {
            int i, fixed;

            fixed = count >= 0; /* Flag for loop termination */

            while
              true /* Something to go at? */
              {
                i = 0;
                if (value != 0)
                  while ((value & (3 << (2 * i))) == 0)
                    i++;
                /* Find LS bit pair, unless zero */

                op_code = op_code | data_op_imm(value & (0xFF << (2 * i)));
                value =
                    value & ~(0xFF << (2 * i)); /* Peel off byte from offset */

                /* Finish if dealing with last word */
                if (fixed) {
                  count = count - 1;
                  if (count < 0)
                    break;
                } else if (value == 0)
                  break;
                /* otherwise plant word and continue */

                if (last_pass)
                  byte_dump(assembly_pointer + extras, op_code, line, 4);
                extras = extras + 4;             /* Count extra word(s) */
                op_code = (op_code & 0xFFF0F000) /* Modify op-code */
                          | ((op_code & 0x0000F000) << 4);
              } /* Closure of loop */

            if (value != 0)
              error_code = SYM_OORANGE; /* Bits remain: error */

            return;
          }

          if ((reg = get_reg(line, &position)) >= 0) {
            if ((reg == 15) && ((token & 0x0000F000) != 0x00000000))
              error_code = SYM_ADRL_PC; /* Only ADR with PC */
            else {
              op_code = op_code | (reg << 12);
              if (cmp_next_non_space(line, &position, 0, ',')) {
                int x;
                error_code = evaluate(line, &position, &value, symbol_table);

                if (error_code == EVAL_OKAY) {
                  x = (int)(value - (assembly_pointer + 8)); /* Offset */
                  if (x < 0) {
                    op_code = op_code | 0x024F0000;
                    x = -x;
                  } else
                    op_code = op_code | 0x028F0000; /* Sign and mag.*/
                }

                if ((token & 0x0000B000) == 0x00008000) /* Var. length: ADRL */
                { /* Crunch without restriction */
                  if (!last_pass) {
                    if (error_code == EVAL_OKAY)
                      adr_loop(x, -1);
                    else
                      extras = 12; /* If unknown assume 3 extra words needed*/
                  } else
                    adr_loop(x, size_record_current->size / 4);

                  extras = variable_item_size(first_pass, extras);
                } else /* Fixed length: ADR(n) */
                  if (error_code == EVAL_OKAY)
                    adr_loop(x,
                             (token >> 12) & 3); /* Pass length to cruncher */
                  else
                    extras = (token >> 10) & 0xC; /* Length as specified */
              } else
                error_code = SYM_NO_COMMA | position; /* ',' not found */
            }
          } else
            error_code = SYM_BAD_REG | position; /* Rd not found */
        } break;

        case 0x000A0000: /* CDP + MCR/MRC */
        {
          int cdp_parameters[] = {0xFFFFFFF0, 20, 1, 1, 2};
          int mcr_parameters[] = {0xFFFFFFF8, 21, 0, 1, 1};
          int mcrr_parameters[] = {0xFFFFFFF0, 4, 0, 0, 0};
          int* parameters;
          int CDP;

          CDP = ((token & 0x0000F000) == 0x00000000); /* Differ. from MCR/MRC */

          switch (token & 0x0000F000) {
            case 0x00000000:
              parameters = cdp_parameters;
              break;
            case 0x00001000:
              parameters = mcr_parameters;
              op_code |= 0x00000010;
              break;
            case 0x00002000:
              parameters = mcrr_parameters;
              break;
          }

          if ((reg = get_copro(line, &position)) >=
              0) /* `reg' is a temp var. */
          {
            op_code = op_code | (reg << 8); /* cp_num */
            if (cmp_next_non_space(line, &position, 0, ',')) {
              error_code = evaluate(line, &position, &value, symbol_table);
              if ((error_code == EVAL_OKAY) ||
                  (first_pass && ((error_code & ALLOW_ON_FIRST_PASS) != 0))) {
                if ((value & parameters[0]) == 0) /* opcode_1 */
                {
                  op_code = op_code | (value << parameters[1]);
                  if (cmp_next_non_space(line, &position, 0, ',')) {
                    if (parameters[2] == 0)
                      reg = get_reg(line, &position);
                    else
                      reg = get_creg(line, &position);
                    if (reg >= 0) {
                      op_code = op_code | (reg << 12); /* CRd */
                      if (cmp_next_non_space(line, &position, 0, ',')) {
                        if (parameters[3] == 0)
                          reg = get_reg(line, &position);
                        else
                          reg = get_creg(line, &position);
                        if (reg >= 0) {
                          op_code = op_code | (reg << 16); /* CRn */
                          if (cmp_next_non_space(line, &position, 0, ',')) {
                            if ((reg = get_creg(line, &position)) >= 0) {
                              op_code = op_code | reg; /* CRm */
                              if (parameters[4] > 0) /* else finished (MCRR) */
                              {
                                if (cmp_next_non_space(line, &position, 0,
                                                       ',')) {
                                  error_code = evaluate(line, &position, &value,
                                                        symbol_table);
                                  if ((error_code == EVAL_OKAY) ||
                                      (first_pass &&
                                       ((error_code & ALLOW_ON_FIRST_PASS) !=
                                        0))) {
                                    if ((value & 0xFFFFFFF8) ==
                                        0) /* opcode_2 */
                                      op_code = op_code | (value << 5);
                                    else
                                      error_code = SYM_BAD_CP_OP; /*Bad Op. #2*/
                                  }
                                } else /* Last field optional for MCR/MRC */
                                  if (parameters[4] == 2) /* CDP only */
                                    error_code = SYM_NO_COMMA | position;
                              }
                            } else
                              error_code = SYM_BAD_REG | position; /*No CRm*/
                          } else
                            error_code = SYM_NO_COMMA | position;
                        } else
                          error_code = SYM_BAD_REG | position; /*CRn missing*/
                      } else
                        error_code = SYM_NO_COMMA | position;
                    } else
                      error_code = SYM_BAD_REG | position; /* CRd missing */
                  } else
                    error_code = SYM_NO_COMMA | position;
                } else
                  error_code = SYM_BAD_CP_OP; /* Opcode 1 out of range */
              }
            } else
              error_code = SYM_NO_COMMA | position;
          } else
            error_code = SYM_BAD_COPRO | position; /* Copro. field missing */
        } break;

        case 0x000B0000: /* LDC/STC */
          if ((reg = get_copro(line, &position)) >=
              0) /* `reg' is a temp var. */
          {
            op_code = op_code | (reg << 8); /* cp_num */
            if (cmp_next_non_space(line, &position, 0, ',')) {
              if ((reg = get_creg(line, &position)) >= 0) {
                op_code = op_code | (reg << 12); /* Rd */
                if (cmp_next_non_space(line, &position, 0, ','))
                  op_code = addressing_mode(op_code, TYPE_CPRO);
                else
                  error_code = SYM_NO_COMMA | position;
              } else
                error_code = SYM_BAD_REG | position; /* CRd missing */
            } else
              error_code = SYM_NO_COMMA | position;
          } else
            error_code = SYM_BAD_COPRO | position; /* Copro. field missing */
          break;

#define TOKEN_ANDX 0x02000000 /* Just to aid readability below */
#define TOKEN_EORX 0x02200000
#define TOKEN_SUBX 0x02400000
#define TOKEN_RSBX 0x02600000
#define TOKEN_ADDX 0x02800000
#define TOKEN_ADCX 0x02A00000
#define TOKEN_SBCX 0x02C00000
#define TOKEN_RSCX 0x02E00000
#define TOKEN_ORRX 0x03800000
#define TOKEN_MOVX 0x03A00000
#define TOKEN_BICX 0x03C00000

        case 0x000D0000: /* Long data-op. variants */
        {
          unsigned int Rd, Rn, imm;
          unsigned int partial[4], alternate[4]; /* Buffers for imm. fields */
          unsigned int count, alt_count, true_count, *ptr;
          unsigned int op_base;

          if ((Rd = get_reg(line, &position)) >= 0) /* Syntax parsing */
          {
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            else {
              if ((token & 0x00002000) != 0) /* Source Rn wanted? */
              {                              /* (all except MOVL) */
                if ((Rn = get_reg(line, &position)) >= 0) {
                  if (!cmp_next_non_space(line, &position, 0, ','))
                    error_code = SYM_NO_COMMA | position;
                } else
                  error_code = SYM_BAD_REG | position;
              } else
                Rn = 0; /* `Rn' for MOV/MVN shoud be R0 */
            }
          } else
            error_code = SYM_BAD_REG | position;

          if (error_code == EVAL_OKAY) /* Get the immediate value */
          {
            if (!cmp_next_non_space(line, &position, 0, '#'))
              error_code = SYM_NO_HASH | position;
            else
              error_code = evaluate(line, &position, &imm, symbol_table);
          }

          ptr = partial;               /* Default setting */
          if (error_code == EVAL_OKAY) /* Got the (possible) immediate value */
          {
            if ((token & 0x0FE00000) == TOKEN_ANDX /* AND must be single op. */
                && find_partials(imm, partial) != 1) /*  else ANDX => BICX */
            {
              token = token & 0xF01FFFFF | TOKEN_BICX;
              imm = ~imm;
            }

            op_base = token & 0xFFF00000;
            count = find_partials(imm, partial); /* Break up imm */

            switch (token & 0x0FE00000) /* Check for shorter alternatives */
            {
              case TOKEN_ANDX: /* ANDX */
              case TOKEN_EORX: /* EORX */
              case TOKEN_ORRX: /* ORRX */
              case TOKEN_BICX: /* BICX */
              case TOKEN_RSBX: /* RSBX */
              case TOKEN_RSCX: /* RSCX */
                break;

              case TOKEN_SUBX:                              /* SUBX */
              case TOKEN_ADDX:                              /* ADDX */
              case TOKEN_ADCX:                              /* ADCX */
              case TOKEN_SBCX:                              /* SBCX */
                alt_count = find_partials(-imm, alternate); /* Negate */
                if (alt_count < count) /* Use alternative op. */
                {
                  count = alt_count;
                  ptr = alternate;
                  switch (token & 0x0FE00000) /* Change op. code */
                  {
                    case TOKEN_SUBX:
                      op_base = op_base & 0xFE100000 | TOKEN_ADDX;
                      break;
                    case TOKEN_ADDX:
                      op_base = op_base & 0xFE100000 | TOKEN_SUBX;
                      break;
                    case TOKEN_ADCX:
                      op_base = op_base & 0xFE100000 | TOKEN_SBCX;
                      break;
                    case TOKEN_SBCX:
                      op_base = op_base & 0xFE100000 | TOKEN_ADCX;
                      break;
                  }
                }
                break;

                /* Note: doesn't try 16-bit MOVs from v6T2 */
              case TOKEN_MOVX:                              /* MOVX */
                alt_count = find_partials(~imm, alternate); /* One's comp. */
                if (alt_count < count) /* Use alternative op. */
                {
                  count = alt_count;
                  ptr = alternate;
                  op_base = op_base | 0x00400000; /* MVN */
                }
                break;

              default:
                printf("Unknown `long' operation\n");
                break;
            }
          } else
            count = 4;

          true_count = variable_item_size(first_pass, count);
          /* Account for variable length sequence */

          if (!last_pass)
            extras = extras + 4 * (true_count - 1);
          else /* Only derive op. code(s) on last pass */
          {
            unsigned int i;
            op_base = op_base | Rn << 16 | Rd << 12;

            /* Padding only used if length is `wrong' */
            for (i = count; i < true_count; i++)
              ptr[i] = 0; /* Pad with 0 */
                          /* Zero is safe for any op. after 1st in sequence */
            for (i = 0; i < true_count;
                 i++) { /* After first word accumulate within Rd */
              op_code = op_base | ptr[i];
              if (i != true_count - 1) /* If not last word to plant ... */
              {
                op_code = op_code & 0xFFEFFFFF; /* S bit only on last op. */
                byte_dump(assembly_pointer + extras, op_code, line, 4);
                extras = extras + 4; /* Count extra word(s) */
              }

              /* Now prepare for follow up operations */
              if (i != 0)
                continue; /* Not -needed- probably faster */
              op_base = op_base & 0xFFF0F000 | Rd << 16; /* Accumulator */
              switch (token & 0x0FE00000)                /* Change op. code? */
              { /* No carry after first operation */
                case TOKEN_SBCX:
                  op_base = op_base & 0xFE1FFFFF | TOKEN_SUBX;
                  break;
                case TOKEN_RSBX: /* Remaining immediate is added on */
                case TOKEN_RSCX:
                case TOKEN_ADCX:
                  op_base = op_base & 0xFE1FFFFF | TOKEN_ADDX;
                  break;
                case TOKEN_MOVX:
                  op_base = op_base & 0xFFDFFFFF;
                  break;
              }
            } /* End of `for' loop */
          }
        } break;

        default:
          printf("Unprocessable opcode!\n");
          break;
      }

    } /* end of -skip- */

    if (error_code == EVAL_OKAY) {
      if (last_pass)
        byte_dump(assembly_pointer + extras, op_code, line, 4);
    } else {
      if (!last_pass) {
        if (allow_error(error_code, first_pass, last_pass))
          error_code = EVAL_OKAY; /* Pretend we're okay */
      } else                      /* Error on final pass */
        byte_dump(assembly_pointer + extras, 0, line, 4);
      /* Dump 0x00000000 place holder */
    }
    //##
    if (if_stack[if_SP])
      assembly_pointer = assembly_pointer + 4 + extras;

    return;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/

  void thumb_mmemonic() {
    unsigned int op_code, value, extras;
    int reg;

    extras = 0; /* Instruction length - 2 */

    if (first_pass || last_pass || ((token & 0x00008000) != 0)) /* -skip- */
    { /* Only do difficult stuff on first pass (syntax) and last pass (dump)
unless instruction may cause file length to vary (e.g. LDR Rd, =###) */
      op_code = (token >> 20) & 0x00000FFF;

      switch (token & 0x000F0000) {
        case 0x00000000: /* Straightforward data operations */
        { /* ADC, AND, BIC, CMN, EOR, MUL, MVN, NEG, ORR, ROR, SBC, TST */
          op_code = 0x4000 | (op_code << 6);

          reg = get_thumb_reg(line, &position, 0x00FF); /* First register */
          if (reg >= 0) {
            op_code = op_code | reg;
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
          } else
            error_code = SYM_BAD_REG | position;

          if (error_code == EVAL_OKAY) {
            reg = get_thumb_reg(line, &position, 0x00FF); /* Second register */
            if (reg >= 0)
              op_code = op_code | (reg << 3);
            else
              error_code = SYM_BAD_REG | position;
          }

        } break;

        case 0x00010000: /* Shifts {ASR, LSL, LSR} */
        {                /*  (ROR is different format) */
          unsigned int temp;

          reg = get_thumb_reg(line, &position, 0x00FF); /* First register */
          if (reg >= 0) {
            temp = reg;
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
          } else
            error_code = SYM_BAD_REG | position;

          if (error_code == EVAL_OKAY) /* First register specifier read okay */
          {
            reg = get_thumb_reg(line, &position, 0x00FF);
            if (reg >= 0) /* Second operand is a register too */
            {
              temp = temp | (reg << 3);

              if (cmp_next_non_space(line, &position, 0, ',')) /* Any more? */
              {
                if (cmp_next_non_space(line, &position, 0, '#')) /* (1) */
                { /* Comma so immediate shift (probably) */
                  error_code = evaluate(line, &position, &value, symbol_table);
                  if (error_code == EVAL_OKAY) {
                    unsigned int x;
                    if ((token & 0x00300000) == 0x00000000)
                      x = value; /* LSL */
                    else
                      x = value - 1;      /* ASR, LSR */
                    if ((x & ~0x1F) == 0) /* Range check (0-31 or 1-32) */
                      op_code = (op_code << 11) | ((value & 0x1F) << 6) | temp;
                    else
                      error_code = SYM_OORANGE;
                  }
                } else
                  error_code = SYM_ERR_SYNTAX | position;
              } else /* No comma so register shift */
                op_code = 0x4000 | ((op_code + 2) << 6) | temp; /* (2) */
            } else
              error_code = SYM_BAD_REG | position; /* 2nd op. not register */
          }

        } break;

        case 0x00020000: /* ADD/SUB */
        {
          unsigned int Rd_pos, Rn_pos;
          int Rd, Rn, Rm, imm;

          Rd_pos = position;
          Rd = get_reg(line, &position);
          if (Rd >= 0) {
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            else if (cmp_next_non_space(line, &position, 0, '#')) /* Rd, #nn */
            {
              error_code = evaluate(line, &position, &value, symbol_table);

              if (error_code == EVAL_OKAY) {
                if (Rd < 8) /* (2) */
                {
                  if ((value & ~0xFF) == 0) {
                    if (op_code == 0)
                      op_code = 0x3000 | (Rd << 8) | value;
                    else
                      op_code = 0x3800 | (Rd << 8) | value;
                  } else
                    error_code = SYM_OORANGE;
                } else {
                  if (Rd = 13) /* (7) */
                  {
                    if ((value & ~0x1FC) == 0) {
                      if (op_code == 0)
                        op_code = 0xB000 | (value >> 2);
                      else
                        op_code = 0xB080 | (value >> 2);
                    } else
                      error_code = SYM_OORANGE;
                  } else
                    error_code = SYM_ERR_SYNTAX;
                }
              }
            } else /* Rd, Rn ... */
            {
              Rn_pos = position;
              Rn = get_reg(line, &position);
              if (Rn >= 0) /* Should be another register by now */
              {
                if (!cmp_next_non_space(line, &position, 0, ',')) /* (4) */
                {
                  if (op_code == 0) /* ADD */
                  {
                    if (((Rd | Rn) & 8) == 0) /* Both low registers? */
                      op_code = 0x1800 | (Rn << 6) | (Rn << 3) | Rd; /* (3) */
                    else
                      op_code = 0x4400 | ((Rd & 8) << 4) | (Rn << 3) | (Rd & 7);
                  } else
                    error_code = SYM_BAD_VARIANT; /* Not SUB */
                } else                            /* Third operand reached */
                {
                  imm = cmp_next_non_space(line, &position, 0, '#'); /* below */

                  if (Rd < 8) /* Should be so, here */
                  {
                    if (Rn < 8) /* (1) or (3) */
                    {
                      if (op_code == 0)
                        op_code = 0x1800;
                      else
                        op_code = 0x1A00;

                      if (imm) /* (1) */
                      {
                        error_code =
                            evaluate(line, &position, &value, symbol_table);
                        if (error_code == EVAL_OKAY) {
                          if ((value & ~0x7) == 0)
                            op_code |= 0x0400 | (value << 6) | (Rn << 3) | Rd;
                          else
                            error_code = SYM_OORANGE;
                        }
                      } else /* (3) */
                      {
                        Rm = get_thumb_reg(line, &position, 0x00FF);
                        if (Rm >= 0)
                          op_code |= (Rm << 6) | (Rn << 3) | Rd;
                        else
                          error_code = SYM_BAD_REG | position;
                      }
                    } else if (op_code == 0) /* ADD */
                    {
                      if ((Rn | 0x2) == 0xF) /* (5) or (6) */
                      {
                        if (imm) {
                          if (Rn == 15)
                            op_code = 0xA000; /* PC (5) */
                          else
                            op_code = 0xA800; /* SP (6) */
                          error_code =
                              evaluate(line, &position, &value, symbol_table);
                          if (error_code == EVAL_OKAY) {
                            if ((value & ~0x3FC) == 0)
                              op_code = op_code | Rd << 8 | (value >> 2);
                            else
                              error_code = SYM_OORANGE;
                          }
                        } else
                          error_code = SYM_ERR_SYNTAX | position;
                      } else
                        error_code = SYM_BAD_REG | skip_spc(line, Rn_pos);
                    } else
                      error_code = SYM_BAD_VARIANT; /* Can't "SUB" */
                  } else
                    error_code = SYM_BAD_REG | skip_spc(line, Rd_pos);
                }
              } else
                error_code = SYM_BAD_REG | skip_spc(line, Rn_pos);
            }
          } else
            error_code = SYM_BAD_REG | skip_spc(line, Rd_pos);

        } break;

        case 0x00030000: /* MOV/CMP */
        {
          int Rd, Rm;
          unsigned int Rd_pos, Rm_pos;

          Rd_pos = position;
          Rd = get_reg(line, &position);
          if (Rd >= 0) {
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            else if (cmp_next_non_space(line, &position, 0, '#')) /* Rd, #nn */
            {                                                     /* (1) */
              if (Rd < 8) {
                error_code = evaluate(line, &position, &value, symbol_table);

                if (error_code == EVAL_OKAY) {
                  if ((value & ~0xFF) == 0) {
                    if (op_code == 0)
                      op_code = 0x2000 | (Rd << 8) | value;
                    else
                      op_code = 0x2800 | (Rd << 8) | value;
                  } else
                    error_code = SYM_OORANGE;
                }
              } else
                error_code = SYM_BAD_REG | skip_spc(line, Rd_pos);
            } else {
              Rm_pos = position;
              Rm = get_reg(line, &position);
              if (Rm >= 0) {
                if (((Rm | Rd) & 8) == 0) /* Both low registers? */
                {                         /* (2) */
                  if (op_code == 0)
                    op_code = 0x1C00;
                  else
                    op_code = 0x4280;
                  op_code = op_code | (Rm << 3) | Rd; /* MOV is really ADD # */
                } else                                /* (3) */
                {
                  if (op_code == 0)
                    op_code = 0x4600;
                  else
                    op_code = 0x4500;
                  op_code |= ((Rd & 8) << 4) | (Rm << 3) | (Rd & 7);
                }
              } else
                error_code = SYM_BAD_REG | skip_spc(line, Rm_pos);
            }
          } else
            error_code = SYM_BAD_REG | skip_spc(line, Rd_pos);

        } break;

        case 0x00040000: /* Branches */
          error_code = evaluate(line, &position, &value, symbol_table);

          if (error_code == EVAL_OKAY) {
            value = value - (assembly_pointer + 4);
            if ((value & 1) != 0) /* Not halfword aligned */
              error_code = SYM_UNALIGNED_BRANCH;
            else {
              if ((op_code & 0xF) == 0x000E) /* 11 bit branch range */
              {
                if (((value & 0xFFFFF800) == 0x00000000) /* Out of range? */
                    || ((value & 0xFFFFF800) == 0xFFFFF800))
                  op_code = 0xE000 | ((value >> 1) & 0x000007FF);
                else
                  error_code = SYM_OORANGE_BRANCH;
              } else {
                if (((value & 0xFFFFFF00) == 0x00000000) /* Out of range? */
                    || ((value & 0xFFFFFF00) == 0xFFFFFF00))
                  op_code =
                      0xD000 | ((op_code & 0xF) << 8) | ((value >> 1) & 0xFF);
                else
                  error_code = SYM_OORANGE_BRANCH;
              }
            }
          }
          break;

        case 0x00050000: /* BX */
          reg = get_reg(line, &position);
          if (reg >= 0)
            op_code = 0x4700 | (reg << 3);
          else
            error_code = SYM_BAD_REG | position;
          break;

        case 0x00060000: /* BL/BLX */
        {
          unsigned int op, old_pos;

          op = op_code; /* Copy of operation type */
          old_pos = position;
          reg = get_reg(line, &position);
          /* Don't prefilter here - want to know if PC specified */
          if (reg >= 15)
            error_code = SYM_BAD_REG | skip_spc(line, old_pos);
          else {
            if (reg >= 0) {
              if (op != 0x0001)
                error_code = SYM_BAD_VARIANT; /* Not BLX */
              else
                op_code = 0x4780 | (reg << 3);
            } else /* Fixed offset form */
            {
              error_code = evaluate(line, &position, &value, symbol_table);

              if (error_code == EVAL_OKAY) {
                value = value - (assembly_pointer + 4);
                if (op != 0x0000)
                  value += 2;         /* BLX correction */
                if ((value & 1) != 0) /* Not halfword aligned */
                  error_code = SYM_UNALIGNED_BRANCH;
                else if (((value & 0xFFC00000) == 0x00000000) ||
                         ((value & 0xFFC00000) == 0xFFC00000)) {
                  if (last_pass) {
                    op_code = 0xF000 | ((value >> 12) & 0x07FF);
                    byte_dump(assembly_pointer, op_code, line, 2);
                  }
                  extras = extras + 2; /* Count extra halfword */
                  if (op == 0x0000)
                    op_code = 0xF800 | ((value >> 1) & 0x07FF);
                  else
                    op_code = 0xE800 | ((value >> 1) & 0x07FE);
                } /* NB Bit 0 is cleared for BLX */
                else
                  error_code = SYM_OORANGE_BRANCH;
              }
            }
          }
        } break;

        case 0x00070000: /* SWI/BKPT */
          error_code = evaluate(line, &position, &value, symbol_table);

          if (error_code == EVAL_OKAY) {
            if ((value & ~0xFF) == 0)
              op_code = (op_code << 8) | value;
            else
              error_code = SYM_OORANGE;
          }
          break;

        case 0x00080000: /* Miscellany */
          switch (op_code) {
            case 0x0000:
              op_code = 0x46C0;
              break; /* NOP (mov r8, r8) */
            case 0x0001:
              op_code = 0xDE00;
              break; /* Undefined */
            default:
              break;
          }
          break;

        case 0x00090000: /* Single register transfers */
        {
          unsigned int Rd;
          bool imm, no_offset;

          no_offset = false; /* Used to amalgamate different syntaxes */

          Rd =
              get_thumb_reg(line, &position, 0x00FF); /* Register to transfer */
          if (Rd >= 0) {
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_ERR_SYNTAX | position;
            else {
              if (cmp_next_non_space(line, &position, 0, '[')) {
                unsigned int old_pos;

                old_pos = position; /* Survived to try for base register */
                reg = get_reg(line, &position);
                /* Want `old_pos' in case mode disallowed */
                if (!cmp_next_non_space(line, &position, 0, ',')) {
                  if (line[position] == ']')
                    no_offset = true; /* No offset */
                  else
                    error_code = SYM_ERR_SYNTAX | position;
                }

                if (error_code == EVAL_OKAY) {
                  imm = no_offset /* No offset */
                        || cmp_next_non_space(line, &position, 0,
                                              '#'); /* Immediate */

                  if (reg < 0)
                    error_code = SYM_BAD_REG | skip_spc(line, old_pos);
                  else {
                    switch (reg) /* Select according to base register */
                    {
                      case 0:
                      case 1:
                      case 2:
                      case 3:
                      case 4:
                      case 5:
                      case 6:
                      case 7:
                        if (imm) /* R0-R7, immediate offset */
                        {
                          if ((token & 0x00004000) != 0) /* Legal mode? */
                          {
                            if (no_offset)
                              value = 0; /* [Rn] form */
                            else
                              error_code = evaluate(line, &position, &value,
                                                    symbol_table);

                            if (error_code == EVAL_OKAY) {
                              int shifts; /* For immediate field justification
                                           */

                              if (!cmp_next_non_space(line, &position, 0, ']'))
                                error_code = SYM_NO_RBR | position;
                              else {
                                switch (token & 0x00700000) {
                                  case 0x00000000:
                                    op_code = 0x6000;
                                    break; /*STR */
                                  case 0x00100000:
                                    op_code = 0x8000;
                                    break; /*STRH*/
                                  case 0x00200000:
                                    op_code = 0x7000;
                                    break; /*STRB*/
                                  case 0x00300000:
                                    op_code = 0x0000;
                                    break;
                                  case 0x00400000:
                                    op_code = 0x6800;
                                    break; /*LDR */
                                  case 0x00500000:
                                    op_code = 0x8800;
                                    break; /*LDRH*/
                                  case 0x00600000:
                                    op_code = 0x7800;
                                    break; /*LDRB*/
                                  case 0x00700000:
                                    op_code = 0x0000;
                                    break;
                                }
                                op_code = op_code | (reg << 3) | Rd;

                                shifts = (token >> 24) & 3;

                                if ((value & ~(0x1F << shifts)) ==
                                    0) /* Magic! */
                                  op_code = op_code | value << (6 - shifts);
                                else
                                  error_code = SYM_OORANGE;
                              }
                            }
                          } else
                            error_code = SYM_BAD_VARIANT;
                        } else { /* Register offset */
                          op_code =
                              0x5000 | ((op_code & 7) << 9) | (reg << 3) | Rd;

                          reg = get_thumb_reg(line, &position, 0x00FF);

                          if (reg < 0)
                            error_code = SYM_BAD_REG | position;
                          else {
                            op_code = op_code | (reg << 6);
                            if (!cmp_next_non_space(line, &position, 0, ']'))
                              error_code = SYM_NO_RBR | position;
                          }
                        }
                        break;

                      case 13:
                      case 15: /* Other legal Thumb base registers */
                        if (imm &&
                            (((reg == 15) && ((token & 0x00002000) != 0)) ||
                             ((reg == 13) && ((token & 0x00001000) != 0)))) {
                          if ((token & 0x00400000) == 0)
                            op_code = 0x9000; /*STR [SP */
                          else if (reg == 13)
                            op_code = 0x9800; /*LDR [SP */
                          else
                            op_code = 0x4800; /*LDR [PC */

                          op_code = op_code | (Rd << 8); /* Insert Rd field */

                          error_code =
                              evaluate(line, &position, &value, symbol_table);

                          if (error_code == EVAL_OKAY) {
                            if ((value & ~0x3FC) == 0) {
                              op_code |= value >> 2;
                              if (!cmp_next_non_space(line, &position, 0, ']'))
                                error_code = SYM_NO_RBR | position;
                            } else
                              error_code = SYM_OORANGE;
                          }
                        } else
                          error_code = SYM_BAD_VARIANT;
                        break;

                      default:
                        error_code = SYM_BAD_VARIANT;
                        break;
                    }
                  }
                }
              } else {
                if (line[position] == '=') { /* Literal pool stuff */
                  if ((token & 0xFFF00000) != 0x02400000) /* Only for LDR */
                    error_code = SYM_ADDR_MODE_BAD;
                  else {
                    position++;
                    error_code =
                        evaluate(line, &position, &value, symbol_table);

                    if ((error_code == EVAL_OKAY) ||
                        allow_error(error_code, first_pass, last_pass)) {
                      switch (do_literal(THUMB, TYPE_WORD, &value, first_pass,
                                         &error_code)) {
                        case 0: /* Will fit in instruction */
                          op_code = 0x2000 | (Rd << 8) | value; /* => MOV */
                          break;

                        case 1:
                          printf("Assembler error: Thumb LDR=/MVN\n");
                          break;

                        case 2: /* Needs full-blooded LDR */
                          op_code =
                              thumb_pc_load(assembly_pointer, value, 0x4800, Rd,
                                            last_pass, &error_code);
                          break;

                        default:
                          break;
                      }
                    }
                  }
                } else /* Not '[' or '=' */
                {      /* Try for `absolute' address */
                  error_code = evaluate(line, &position, &value, symbol_table);

                  if (error_code == EVAL_OKAY)
                    op_code = thumb_pc_load(assembly_pointer, value, 0x4800, Rd,
                                            last_pass, &error_code);
                }
              }
            }
          } else
            error_code = SYM_BAD_REG | position;

        } break;

        case 0x000A0000: /* Multiple load/stores */
        {
          unsigned int list;

          op_code = (op_code & 0xFF) << 8;
          if ((token & 0x0F000000) == 0x0C000000) /* LDM/STM */
          {
            list = 0x00FF; /* Just R7-R0 */

            reg = get_thumb_reg(line, &position, 0x00FF);

            if (reg < 0)
              error_code = SYM_BAD_REG | position;
            else {
              op_code = op_code | (reg << 8); /* Include base register */

              if (!cmp_next_non_space(line, &position, 0, '!'))
                error_code = SYM_NO_BANG | position;
              else if (!cmp_next_non_space(line, &position, 0, ','))
                error_code = SYM_NO_COMMA | position;
            }
          } else /* PUSH/POP */
          {
            if ((token & 0x0FF00000) == 0x0B400000)
              list = 0x40FF; /* PUSH: LR */
            else
              list = 0x80FF; /* POP:  PC */
          }

          if (error_code == EVAL_OKAY) /* Should be ready for register list */
          {
            if (cmp_next_non_space(line, &position, 0, '{')) {
              list =
                  parse_reg_list(list); /* Replace allowed with actual list */
              if (error_code == EVAL_OKAY) {
                if ((list & 0xC000) != 0)
                  op_code = op_code | 0x0100;
                op_code = op_code | (list & 0xFF);
                if (!cmp_next_non_space(line, &position, 0, '}'))
                  error_code = SYM_NO_RSQUIGGLE | position;
              }
            } else
              error_code = SYM_NO_REGLIST | position;
          }

        } break;

        case 0x000B0000: /* ADR */
        {                /* Only single instruction ops. at present */
          if ((reg = get_thumb_reg(line, &position, 0x00FF)) >= 0) {
            if (cmp_next_non_space(line, &position, 0, ',')) {
              error_code = evaluate(line, &position, &value, symbol_table);
              if (error_code == EVAL_OKAY)
                op_code = thumb_pc_load(assembly_pointer, value, 0xA000, reg,
                                        last_pass, &error_code);
            } else
              error_code = SYM_NO_COMMA | position; /* ',' not found */
          } else
            error_code = SYM_BAD_REG | position; /* Rd not found */
        } break;

        default:
          printf("Unprocessable opcode!\n");
          break;
      }
    }

    if (error_code == EVAL_OKAY) {
      if (last_pass)
        byte_dump(assembly_pointer + extras, op_code, line, 2);
    } else {
      if (!last_pass) {
        if (allow_error(error_code, first_pass, last_pass))
          error_code = EVAL_OKAY; /* Pretend we're okay */
      } else                      /* Error on final pass */
        byte_dump(assembly_pointer + extras, 0, line, 2);
      /* Dump 0x0000 place holder */
    }
    //##
    if (if_stack[if_SP])
      assembly_pointer = assembly_pointer + 2 + extras;

    return;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/
  /* Separated out so other functions (e.g. "ARM") can call it too. */

  void do_align() {
    int fill;

    error_code = evaluate(line, &position, &operand, symbol_table);
    if ((error_code & 0xFFFFFF00) == EVAL_NO_OPERAND) /* Code - position */
    {
      error_code = EVAL_OKAY;
      if (instruction_set == THUMB)
        operand = 2; /* Thumb default = 2 */
      else
        operand = 4; /* else default = 4 */
    }

    if (error_code == EVAL_OKAY) {
      if (operand != 0) { /* (ALIGN 0 has no effect) */
        temp = (assembly_pointer - 1) % operand;
        operand = operand - (temp + 1); /* No. of elements to skip(/fill) */
      }

      if (fill = (line[position] == ',')) /*Note where any label should go*/
        fill_space(operand);              /* Fill with value (?) */
      else                                /* Start new section if leaving gap */
      {
        // dumping literals inside ALIGN @@@
        // literal_dump(last_pass, line, assembly_pointer + operand);
        // printf("Hello?\n");
        if (fList != NULL)
          list_start_line(assembly_pointer + operand, false);
        /* Revise list file address */

        if (operand != 0)
          elf_new_section_maybe(); /* Only reorigin in needed */
      }
    }

    if (error_code == EVAL_OKAY) /* Still OK? */
    {
      if (fill) /* Any label is at source point */
        assemble_redef_label(assembly_pointer, assembly_pointer_defined,
                             my_label, &error_code, 0, pass_count, last_pass,
                             line);
      else /* Any label is after alignment */
        assemble_redef_label(assembly_pointer + operand,
                             assembly_pointer_defined, my_label, &error_code, 0,
                             pass_count, last_pass, line);
      //##
      if (if_stack[if_SP])
        assembly_pointer = assembly_pointer + operand;
    }

    return;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   * -*/

  error_code = EVAL_OKAY; /* #DEFINE ??   @@@@ */

  first_pass = (pass_count == 0);
  evaluate_own_label = my_label; /* Yuk! @@@@@ */
  /* Global, to pass local label definition to evaluate */

  if (((token & 0xF4000000) != 0xF4000000) && (my_label->sort == MAYBE_SYMBOL))
    sym_add_to_table(symbol_table, my_label->symbol); /* Must be a label */

  if (((token & 0xF8000000) != 0xF8000000) && (my_label->sort != NO_LABEL))
    /* Redefine label if present/required unless directive such as EQU */
    assemble_redef_label(assembly_pointer, assembly_pointer_defined, my_label,
                         &error_code, 0, pass_count, last_pass, line);

  def_increment = 0; /* Default to first position/item on line */

  if (error_code == EVAL_OKAY) {  //###	Check this error trap correct.

    if ((token & 0xF0000000) == 0xF0000000) { /* Directive */
      switch (token) {                        /* Defining code */
        case 0xF0000000:
          assemble_define(1);
          break; /* DEFB */
        case 0xF0010000:
          assemble_define(2);
          break; /* DEFH */
        case 0xF0020000:
          assemble_define(4);
          break;         /* DEFW */
        case 0xF0030000: /* DEFS */
          error_code = evaluate(line, &position, &operand, symbol_table);
          if (allow_error(error_code, first_pass, last_pass))
            error_code = EVAL_OKAY;

          if (error_code == EVAL_OKAY) {
            if (line[position] == ',')
              fill_space(operand); /*Fill with value (?)*/
            else                   /* Reorigin ELF by starting new section */
              if (operand != 0)
                elf_new_section_maybe(); /* Reorigin in needed */

            //##
            if (if_stack[if_SP])
              if (error_code == EVAL_OKAY)
                assembly_pointer += operand; /*Still OK?*/
          }
          break;

        case 0xF0040000: /* EXPORT */
        {
          bool terminate;
          int i;
          char ident[LINE_LENGTH];
          sym_record* symbol;

          terminate = false;

          while (!terminate) {
            position = skip_spc(line, position);
            if ((i = get_identifier(line, position, ident, LINE_LENGTH)) > 0) {
              if (last_pass) /* Only care when about to complete */
              {
                if ((i == 3) && ((ident[0] & 0xDF) == 'A') /* Bodgy test */
                    && ((ident[1] & 0xDF) == 'L') && ((ident[2] & 0xDF) == 'L'))
                  symbol_table->flags |=
                      SYM_TAB_EXPORT_FLAG; /*Mark whole table*/
                else {
                  if ((symbol = sym_find_label(ident, symbol_table)) != NULL)
                    symbol->flags |= SYM_REC_EXPORT_FLAG;
                  else
                    error_code = SYM_ERR_NO_EXPORT;
                }
              }
              position = position + i;
            } else {
              error_code = SYM_ERR_SYNTAX | position;
              terminate = true;
            }
            if (!cmp_next_non_space(line, &position, 0, ','))
              terminate = true;
          }
        } break;

        case 0xF0050000: /* INCLUDE */
        {                // Allow " " around name?  @@@
          int i;

          position = skip_spc(line, position);
          if (test_eol(line[position])) /* Effective EOL? */
          {
            error_code = SYM_ERR_NO_FILENAME | position; /* Filename missing */
          } else { /* Got name; make and fill buffer */
            *include_name =
                (char*)malloc(LINE_LENGTH + 1); /* Overkill-so what? */
            i = 0;
            while (!test_eol(line[position]) && (line[position] != ' ') &&
                   (line[position] != '\t'))
              (*include_name)[i++] = line[position++];
            (*include_name)[i] = '\0'; /* Terminate filename string */
          }
        } break;

        case 0xF0060000: /* LITERAL */
          literal_dump(last_pass, line,
                       0); /* Get rid of any pending literals */
          if (my_label->sort != NO_LABEL)
            error_code = SYM_NO_LABELS;
          break;

        case 0xF0070000: /* ARCH */
        {
          int arch;
          arch = get_thing(line, &position, arch_table);
          if (arch >= 0)
            arm_variant = arch;
          else
            error_code = SYM_ERR_SYNTAX | position;
        } break;

        case 0xF0080000: /* ENTRY */
          if (!entry_address_defined) {
            entry_address = assembly_pointer;
            entry_address_defined = true;
          } else
            error_code = SYM_DOUBLE_ENTRY;
          break;

        case 0xF0090000: /* ARM */
          instruction_set = ARM;
          do_align(); /* Automatic realignment (correct choice of action?) */
          break;

        case 0xF00A0000: /* THUMB */
          instruction_set = THUMB;
          break;

        case 0xF00B0000: /* <option removed> */
          break;

        case 0xF00C0000: /* IF */
        {
          int condition;

          if (if_SP >= IF_STACK_SIZE)
            error_code = SYM_MANY_IFS;
          else {
            error_code = evaluate(line, &position, &condition, symbol_table);
            if (error_code != EVAL_OKAY) {
              //## printf("IF: error %08X\n", error_code);
              condition = -1;
              if ((error_code & 0xFFFFFF00) ==
                  EVAL_NO_LIMIT) /* Improve error */
                error_code =
                    EVAL_NO_LABEL_YET | (error_code & 0xFF); /* message */
            }
            if_stack[++if_SP] = condition;
            //## printf("IF: Push value %08X\n", condition);
            //            }
            //          else
            //            error_code = SYM_NO_IDENTIFIER | position;
          }
        } break;

        case 0xF00D0000: /* FI */
          if (if_SP > 0)
            if_SP--;
          else
            error_code = SYM_MANY_FIS;
          //## printf("IF: Pop value\n");
          break;

        case 0xF00E0000: /* ELSE */
          if (if_SP > 0)
            if_stack[if_SP] = ~if_stack[if_SP];
          else
            error_code = SYM_LOST_ELSE;
          //## printf("IF: flip value %08X\n", if_stack[if_SP]);
          break;

        case 0xF00F0000: /* IMPORT */
        {                // Allow " " around name?  @@@
          int i;
          char* import_name;      /* Name extracted from source file */
          char* import_full_name; /* Path to file from 'here' */
          FILE* import_handle;
          char byte;

          position = skip_spc(line, position);
          if (test_eol(line[position])) /* Effective EOL? */
          {
            error_code = SYM_ERR_NO_FILENAME | position; /* Filename missing */
          } else { /* Got name; make and fill buffer */
            import_name =
                (char*)malloc(LINE_LENGTH + 1); /* Overkill-so what? */
            i = 0;
            while (!test_eol(line[position]) && (line[position] != ' ') &&
                   (line[position] != '\t'))
              import_name[i++] = line[position++];
            import_name[i] = '\0'; /* Terminate filename string */

            if (import_name[0] == '/') /* Absolute path specified */
              import_full_name = import_name;
            else /* Add path from invocation position */
              import_full_name = pathname(include_file_path, import_name);

            if (import_name[0] != '\0') {
              import_handle =
                  fopen(import_full_name, "r");  // Ignores errors if any  @@@
              if (import_handle != NULL) {
                while (!feof(import_handle)) {
                  byte = getc(import_handle);

                  if (!feof(import_handle)) {
                    if (last_pass)
                      byte_dump(assembly_pointer + def_increment, byte, line,
                                1);
                    def_increment++;
                  }
                }

                assembly_pointer += def_increment; /* Add total size at end */
                fclose(import_handle);
              } else {
                error_code = SYM_NO_IMPORT; /* Filename missing */
              }
            }
            if (import_full_name != import_name)
              free(import_full_name);
            free(import_name);
          }
        } break;

          /* Defining label */
        case 0xF8000000: /* EQU */
        case 0xF8030000: /* DEF */
          error_code = evaluate(line, &position, &temp, symbol_table);
          if (my_label->symbol != NULL) {
            if (token == 0xF8000000)
              assemble_redef_label(temp, true, my_label, &error_code,
                                   SYM_REC_EQU_FLAG, pass_count, last_pass,
                                   line);
            else
              assemble_redef_label(temp, true, my_label, &error_code,
                                   SYM_REC_USR_FLAG, pass_count, last_pass,
                                   line);
          } else
            error_code = SYM_ERR_NO_EQU;

          break;

        case 0xF8010000: /* ORG */
          error_code =
              evaluate(line, &position, &assembly_pointer, symbol_table);
          assembly_pointer_defined = (error_code == EVAL_OKAY);
          /* Result may be `undefined' */
          if (allow_error(error_code, first_pass, last_pass))
            error_code =
                SYM_NO_ERROR; /* ORG undefined -itself- is not an error */
          if (fList != NULL)
            list_start_line(assembly_pointer, false);
          /* Revise list file address */
          assemble_redef_label(assembly_pointer, assembly_pointer_defined,
                               my_label, &error_code, 0, pass_count, last_pass,
                               line);

          elf_new_section_maybe(); /* else reuse previous (unused) number */
          break;

        case 0xF8020000: /* ALIGN */
          do_align();
          break;

        case 0xF8040000: /* RECORD */
          error_code = evaluate(line, &position, &temp, symbol_table);
          if (((error_code & 0xFFFFFF00) == EVAL_NO_OPERAND) ||
              allow_error(error_code, first_pass, last_pass)) {
            temp = 0; /* If no operand found then assume zero start */
            error_code = EVAL_OKAY;
          }

          if (error_code == EVAL_OKAY) {
            data_pointer = temp;
            assemble_redef_label(data_pointer, true, my_label, &error_code,
                                 SYM_REC_DATA_FLAG, pass_count, last_pass,
                                 line);
          }
          break;

        case 0xF8050000: /* REC_ALIGN */
          error_code = evaluate(line, &position, &temp, symbol_table);
          if (((error_code & 0xFFFFFF00) == EVAL_NO_OPERAND) ||
              allow_error(error_code, first_pass, last_pass)) {
            temp = 4;
            error_code = EVAL_OKAY;
          }

          if (error_code == EVAL_OKAY) {
            if (temp != 0)
              data_pointer = data_pointer - (data_pointer % temp) + temp;
            /* Any label is after alignment */
            assemble_redef_label(data_pointer, true, my_label, &error_code,
                                 SYM_REC_DATA_FLAG, pass_count, last_pass,
                                 line);
          }
          break;

        case 0xF8100000: /*    ALIAS */
        case 0xF8110000: /*     BYTE */
        case 0xF8120000: /* HALFWORD */
        case 0xF8140000: /*     WORD */
        case 0xF8180000: /*   DOUBLE */
        {
          unsigned int size;

          size = (token >> 16) & 0xF; /* Size of one element */
          error_code = evaluate(line, &position, &temp, symbol_table);
          if (((error_code & 0xFFFFFF00) == EVAL_NO_OPERAND) ||
              allow_error(error_code, first_pass, last_pass)) {
            temp = 1; /* If no operand found then assume one element */
            error_code = EVAL_OKAY;
          }

          if (error_code == EVAL_OKAY) {
            assemble_redef_label(data_pointer, true, my_label, &error_code,
                                 SYM_REC_DATA_FLAG, pass_count, last_pass,
                                 line);
            data_pointer = data_pointer + (temp * size);
          }

        } break;

        case 0xF4000000:  /* RN */
          if (first_pass) /* Must be resolved on first pass */
          {
            if (my_label->sort == MAYBE_SYMBOL) {
              if ((my_label->symbol->value = get_reg(line, &position)) >= 0)
                redefine_symbol(line, my_label->symbol, register_table);
              else
                error_code = SYM_BAD_REG | position;
            } else
              error_code = SYM_ERR_NO_EQU;
          }
          break;

        case 0xF4010000:  /* CN */
          if (first_pass) /* Must be resolved on first pass */
          {
            if (my_label->sort == MAYBE_SYMBOL) {
              if ((my_label->symbol->value = get_creg(line, &position)) >= 0)
                redefine_symbol(line, my_label->symbol, cregister_table);
              else
                error_code = SYM_BAD_REG | position;
            } else
              error_code = SYM_ERR_NO_EQU;
          }
          break;

        case 0xF4020000:  /* CP */
          if (first_pass) /* Must be resolved on first pass */
          {
            if (my_label->sort == MAYBE_SYMBOL) {
              if ((my_label->symbol->value = get_copro(line, &position)) >= 0)
                redefine_symbol(line, my_label->symbol, copro_table);
              else
                error_code = SYM_BAD_COPRO | position;
            } else
              error_code = SYM_ERR_NO_EQU;
          }
          break;

        default:
          error_code = SYM_ERR_BROKEN;
          break;
      }
    } else {
      switch (instruction_set) {
        case ARM:
          arm_mnemonic();
          break;
        case THUMB:
          thumb_mmemonic();
          break;
        default:
          printf("Got into an undefined instruction set :-(\n");
          break;
      }
    }

  }  //  ###
  if (first_pass &&
      (error_code ==
       EVAL_OKAY)) { /* Check that nothing remains on line (first pass only) */
    position = skip_spc(line, position);
    if (!test_eol(line[position]))
      error_code = SYM_GARBAGE | position;
  }

  return error_code;
}

/*----------------------------------------------------------------------------*/
/* Fully parameterised utility routines                                       */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Look up a value in a symbol table.                                         */
/* Intended to recover positive values only; returns -1 if not found.         */

#define THING_BUFFER_LENGTH 16

int get_thing(char* line, unsigned int* pos, sym_table* table) {
  int i, result;
  char buffer[THING_BUFFER_LENGTH];
  sym_record* ptr;

  *pos = skip_spc(line, *pos);
  result = -1; /* Not found code */

  if ((i = get_identifier(line, *pos, buffer, THING_BUFFER_LENGTH)) >
      0) {                                               /* Something taken */
    if ((ptr = sym_find_label(buffer, table)) != NULL) { /* Symbol recognised */
      result = ptr->value;
      *pos += i;
    }
  }

  return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int get_reg(char* line, unsigned int* pos) /* Expand into code? @@@@ */
{
  return get_thing(line, pos, register_table);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Like get_reg but accepts a mask of which subset of registers are allowed.  */
/* If mask doesn't match then leaves *pos at start of symbol.                 */

int get_thumb_reg(char* line, unsigned int* pos, unsigned int reg_mask) {
  int reg;
  unsigned int start_pos;

  start_pos = *pos;
  reg = get_thing(line, pos, register_table);
  if ((reg >= 0) && (((1 << reg) & reg_mask) == 0)) /* Allowed register? */
  {
    reg = -1;
    *pos = skip_spc(line, start_pos); /* Back off pointer (to error position) */
  }

  return reg;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int get_creg(char* line, unsigned int* pos) {
  return get_thing(line, pos, cregister_table);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int get_copro(char* line, unsigned int* pos) {
  return get_thing(line, pos, copro_table);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int get_psr(char* line, unsigned int* pPos) {
  int reg;

  if ((line[*pPos] & 0xDF) == 'C')
    reg = 0x00; /* Possibly CPSR */
  else if ((line[*pPos] & 0xDF) == 'S')
    reg = 0x40; /* Possibly SPSR */
  else
    reg = -1; /* Definitely not found */

  if (reg >= 0) {
    if (((line[(*pPos) + 1] & 0xDF) == 'P') &&
        ((line[(*pPos) + 2] & 0xDF) == 'S') &&
        ((line[(*pPos) + 3] & 0xDF) == 'R')) {
      *pPos = *pPos + 4;
      if (line[(*pPos)] != '_')
        reg = reg | 0x0F; /* Assume "_all" as default */
      else                /* Search out fields and assemble field mask */
      {
        bool okay;

        okay = true;
        (*pPos)++;
        while (okay)
          switch (line[*pPos]) {
            case 'F':
            case 'f':
              reg = reg | 0x08;
              (*pPos)++;
              break;
            case 'S':
            case 's':
              reg = reg | 0x04;
              (*pPos)++;
              break;
            case 'X':
            case 'x':
              reg = reg | 0x02;
              (*pPos)++;
              break;
            case 'C':
            case 'c':
              reg = reg | 0x01;
              (*pPos)++;
              break;
            default:
              okay = false;
              break;
          }
      }
    } else
      reg = -1; /* Not found after all */
  }

  return reg;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int get_shift(char* line, unsigned int* pos) {
  int result, flag;

  *pos = skip_spc(line, *pos);
  if (line[*pos] == ',') {
    flag = -1;
    (*pos)++;
  } /* Allow -optional- ',' */
  else {
    flag = 0;
  }

  result = get_thing(line, pos, shift_table);

  if (result < 0)
    result = result + flag; /* Signify ',' but not shift found */

  return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Change a 32-bit value into a 4-bit ROR + 8-bit immediate form; -1 if can't */

int data_op_imm(unsigned int value) /* Not particularly efficient */
{
  unsigned int i;
  bool found;

  unsigned int rol32(unsigned int x, unsigned int j) {
    return (((x & 0xFFFFFFFF) >> (32 - j)) | (x << j)) & 0xFFFFFFFF;
  }

  found = false;
  i = 0;

  while ((i < 16) && !found) {
    if ((rol32(value, 2 * i) & 0xFFFFFF00) == 0)
      found = true;
    else
      i++;
  }

  if (found)
    return rol32(value, 2 * i) | (i << 8);
  else
    return -1;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

unsigned int thumb_pc_load(unsigned int here,
                           unsigned int there,
                           unsigned int op_code,
                           int reg,
                           bool last_pass,
                           unsigned int* error_code) {
  there = there - ((here & ~0x00000002) + 4); /* Calculate offset */

  if ((there & ~0x000003FC) == 0)
    op_code = op_code | (reg << 8) | (there >> 2);
  else if (last_pass) {
    if ((there & 3) != 0)
      *error_code = SYM_MISALIGNED;
    else
      *error_code = SYM_OFFSET_TOO_BIG;
  }

  return op_code;
}

/*----------------------------------------------------------------------------*/
/* Refetch first identifier from source line (in case it was truncated) and   */
/* re-hash into old symbol with appropriate rules.                            */

void redefine_symbol(char* line, sym_record* record, sym_table* table) {
  char ident[LINE_LENGTH];

  get_identifier(line, skip_spc(line, 0), ident, LINE_LENGTH);
  sym_string_copy(ident, record, table->flags);
  sym_add_to_table(table, record);

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Redefine a label on the current line                                       */

void assemble_redef_label(unsigned int value,
                          int defined,
                          own_label* my_label,
                          unsigned int* error_code,
                          int type_change,
                          int pass_count,
                          bool last_pass,
                          char* line) {
  int value_defined; /* Genuine value supplied */
  unsigned int old_value;
  int flags;

  if (my_label->sort != NO_LABEL) /* Reject cases that don't apply */
  {
    if ((my_label->sort == SYMBOL) ||
        (my_label->sort ==
         MAYBE_SYMBOL)) { /* Symbolic (ordinary) label on current line */
      old_value = my_label->symbol->value;
      flags = my_label->symbol->flags;
    } else if (my_label->sort ==
               LOCAL_LABEL) { /* Local label on current line */
      old_value = my_label->local->value;
      flags = my_label->local->flags;
    }

    value_defined =
        (*error_code == EVAL_OKAY) && defined;  // Clumsy - 2 parameters @@

    if ((pass_count != (flags & 0xFF))        /* First encounter this pass? */
        || ((flags & SYM_REC_DEF_FLAG) == 0)) /*  or was undefined */
    {                                         /* Treat as new encounter */
      if (value_defined)                      /* New value is defined value */
      {
        if (((flags & SYM_REC_DEF_FLAG) == 0) /* Undefined ... */
            || (old_value != value))          /*  ...or changed */
        {
          if ((flags & SYM_REC_DEF_FLAG) == 0)
            defined_count++; /* Undefined */
          else
            redefined_count++;
          /* Note what was done */
          flags |= SYM_REC_DEF_FLAG; /* Mark as defined */
        }
      } else {
        flags &= ~SYM_REC_DEF_FLAG; /* Mark label as undefined */

        if (allow_error(*error_code, pass_count == 0, last_pass))
          *error_code = SYM_NO_ERROR;  // But flag up something @@@@
      }
    } else { /* Repeat encounter with defined label */
      if (!value_defined || (value != old_value)) /* Different! */
        *error_code = SYM_INCONSISTENT;
    }

    flags = (flags & 0xFFFFFF00) | pass_count;

    if ((my_label->sort == SYMBOL) ||
        (my_label->sort ==
         MAYBE_SYMBOL)) { /* Symbolic (ordinary) label on current line */
      my_label->symbol->value = value;
      my_label->symbol->flags = flags;
    } else if (my_label->sort ==
               LOCAL_LABEL) { /* Local label on current line */
      my_label->local->value = value;
      my_label->local->flags = flags;
    }

    if (type_change != 0) /* Maybe want to override type */
    {
      if (my_label->sort == LOCAL_LABEL)
        my_label->local->flags |= type_change; /* `type' indicator */
      else
        my_label->symbol->flags |= type_change; /* `type' indicator */
    }
  }

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void byte_dump(unsigned int address, unsigned int value, char* line, int size) {
  int i;

  //## printf("Should I? : %d %08X %08X %d\n", if_SP, if_stack[if_SP], value,
  // size);

  if (dump_code && if_stack[if_SP]) {
    if (fList != NULL)
      list_mid_line(value, line, size);

    for (i = 0; i < size; i++) {
      if (fHex != NULL)
        hex_dump(address + i, (value >> (8 * i)) & 0xFF);
      if (fElf != NULL)
        elf_dump(address + i, (value >> (8 * i)) & 0xFF);
      if (fVerilog != NULL)
        Verilog_array[(address + i) % VERILOG_MAX] = (value >> (8 * i)) & 0xFF;
    }
  }
  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Dump any pending literal records into code                                 */
/* `last_pass' is a bool which enables code dumping                        */
/* `line' is the input source line                                            */
/* `limit' is a dump address not to exceed, unless 0 which indicates no limit */

void literal_dump(bool last_pass, char* line, unsigned int limit) {
  unsigned int address; /* Needed because assembly pointer is static */
                        /*  for each `instruction' in the list file */
  unsigned int size;    /* Each plant aligns to the appropriate boundary */
  int i;
  char* align_message = "(padding)";
  char* my_message;

  address = assembly_pointer;
  my_message = align_message; /* In case we need to align first */

  // For dumping inside ALIGN etc.	@@@
  // while ((literal_tail != literal_head)  /* Strip any unwanted entries at
  // start */
  //   && (literal_tail != NULL)
  //   && ((literal_tail->flags & LIT_NO_DUMP) != 0))
  //  if (literal_tail == NULL) literal_tail = literal_list;
  //  else                      literal_tail = literal_tail->pNext;

  while ((literal_tail != literal_head) &&
         ((limit == 0) ||
          ((address + (((literal_tail->flags & LIT_HALF) != 0) ? 2 : 4)) <=
           limit))) { /* Something to dump and space to dump it in */
    if (literal_tail == NULL)
      literal_tail = literal_list;
    else
      literal_tail = literal_tail->pNext;

    if ((literal_tail->flags & LIT_HALF) == 0)
      size = 4;
    else
      size = 2;

    if ((literal_tail->flags & LIT_NO_DUMP) ==
        0) /* If -not- converted to MOV */
    {
      for (i = 0; ((address + i) & (size - 1)) != 0; i++) /* Align */
        if (last_pass)
          byte_dump(address + i, 0, my_message, 1); /* Padded */
      /* Padding avoids need to mess about with sections in elf output */
      address = address + i; /* Step, even if not planting */

      if ((fList != NULL) &&
          ((i != 0) /* Needed to align first */
           || ((size == 4) &&
               ((list_byte % 4) != 0)))) /*  or unaligned for word */
      { /* Start new list line if alignment was needed */
        list_end_line(my_message);
        list_start_line(address, (my_message == line)); /* May *continue' */
      }
    } else if ((literal_tail->flags & LIT_DEFINED) == 0)
      undefined_count++;  // Is this a "label"?? @@@
    else {
      if (address != literal_tail->address)
        redefined_count++;  // Is this a "label"?? @@@
      /* Addr. change => offset may change => other literals (/labels) may move
       */
    }

    literal_tail->address = address; /* Note dump address in record anyway */

    if ((literal_tail->flags & LIT_NO_DUMP) ==
        0) /* If -not- converted to MOV */
    {
      my_message = line; /* Real output line (if any) now */
      if (last_pass)
        byte_dump(address, literal_tail->value, line, size); /*Plant*/
      address = address + size;                              /* Step on */
    }
  }

  for (i = 0; ((address + i) & 3) != 0; i++) /* Realign to word boundary */
    if (last_pass)
      byte_dump(address + i, 0, line, 1); /*  padding, if needed */

  //##
  if (if_stack[if_SP])
    assembly_pointer = address + i; /* Finally, allow global modification */

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

FILE* open_output_file(int std, char* filename) {
  if (std)
    return stdout;
  else if (filename[0] != '\0')
    return fopen(filename, "w");  // Ignores errors if any  @@@
  else
    return NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void close_output_file(FILE* handle, char* filename, int errors) {
  if ((handle != stdout) && (handle != NULL)) {
    fclose(handle);
    if (errors)
      remove(filename);
  }
  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void hex_dump(unsigned int address, char value) {
  int i;

  if (hex_address_defined && (address == hex_address)) { /* Expected address */
    list_hex(value, 2,
             &hex_buffer[HEX_LINE_ADDRESS + 3 * (address % HEX_BYTE_COUNT)]);
    hex_address++;
  } else { /* New or unexpected address */
    if (hex_address_defined)
      fprintf(fHex, "%s\n", hex_buffer);

    for (i = 0; i < HEX_LINE_LENGTH - 1; i++)
      hex_buffer[i] = ' ';
    hex_buffer[i] = '\0';
    list_hex(address, 8, &hex_buffer[0]);
    hex_buffer[8] = ':';

    list_hex(value, 2,
             &hex_buffer[HEX_LINE_ADDRESS + 3 * (address % HEX_BYTE_COUNT)]);

    hex_address = address + 1;
    hex_address_defined = true;
  }

  if ((hex_address % HEX_BYTE_COUNT) == 0) /* If end of line, dump */
  {
    fprintf(fHex, "%s\n", hex_buffer);
    hex_address_defined = false;
  }

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void hex_dump_flush(void) {
  if (hex_address_defined)
    fprintf(fHex, "%s\n", hex_buffer);
  hex_address_defined = false;
  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void elf_dump(unsigned int address, char value) {
  elf_temp* pTemp;

  elf_section_valid = true; /* Note that we've dumped -something- in section */

  if (elf_new_block ||
      (elf_section != elf_section_old)) {     /* New or unexpected address */
    pTemp = (elf_temp*)malloc(ELF_TEMP_SIZE); /* Allocate block */
    pTemp->pNext = NULL;                      /* Initialise new record */
    pTemp->continuation = (elf_section == elf_section_old);
    pTemp->section = elf_section;
    pTemp->address = address;
    pTemp->count = 0;

    if (current_elf_record == NULL) { /* First record */
      elf_record_list = pTemp;
      pTemp->continuation = false;
    } else
      current_elf_record->pNext = pTemp;

    current_elf_record = pTemp; /* Move on */
  }

  current_elf_record->data[(current_elf_record->count)++] = value;

  elf_section_old = elf_section;

  elf_new_block = (current_elf_record->count >= ELF_TEMP_LENGTH);

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Start a new ELF section if one is already in use                           */

void elf_new_section_maybe(void) {
  if (elf_section_valid) {
    elf_section++;
    elf_section_valid = false;
  }
  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void elf_dump_out(FILE* fElf, sym_table* table) {
  void elf_dump_word(FILE * fElf, unsigned int word) {
    fprintf(fElf, "%c%c%c%c", word & 0xFF, (word >> 8) & 0xFF,
            (word >> 16) & 0xFF, (word >> 24) & 0xFF);
    return;
  }

  void elf_dump_SH(FILE * fElf, unsigned int name, unsigned int type,
                   unsigned int flags, unsigned int addr, unsigned int pos,
                   unsigned int size, unsigned int link, unsigned int info,
                   unsigned int align, unsigned int size2) {
    elf_dump_word(fElf, name);
    elf_dump_word(fElf, type);
    elf_dump_word(fElf, flags);
    elf_dump_word(fElf, addr);
    elf_dump_word(fElf, pos);
    elf_dump_word(fElf, size);
    elf_dump_word(fElf, link);
    elf_dump_word(fElf, info);
    elf_dump_word(fElf, align);
    elf_dump_word(fElf, size2);
    return;
  }

  elf_temp *pTemp, *pTemp_old;
  elf_info *pInfo_head, *pInfo, *pInfo_new;
  unsigned int fragments, total, pad_to_align, i, j, temp;
  unsigned int symtab_count, symtab_length, strtab_length, shstrtab_length;
  unsigned int symtab_local_count;
  unsigned int prog_start;
  unsigned int code_SHstr_offset, sym_SHstr_offset;
  unsigned int str_SHstr_offset, SHstr_SHstr_offset;
  char *strings, *SHstrings;
  sym_record *head, *ptr;

  char* sym_sectionname = "symtab"; /* Predefined section names */
  char* str_sectionname = "strtab";
  char* shs_sectionname = "shstrtab";

  pInfo = NULL;
  pInfo_head = NULL; /* Needed in case there's nothing to write (e.g. error) */
  pTemp = elf_record_list;
  fragments = 0; /* Number of ORGs */
  total = 0;     /* Length of all code bytes */

  while (pTemp != NULL) /* Make an code output section list */
  {
    if (!pTemp->continuation) {
      pInfo_new = (elf_info*)malloc(ELF_INFO_SIZE); /* Allocate block */
      pInfo_new->pNext = NULL;
      pInfo_new->address = pTemp->address;
      pInfo_new->position = total;
      if (pInfo == NULL)
        pInfo_head = pInfo_new; /* Start ...          */
      else
        pInfo->pNext = pInfo_new; /* ... or add to list */

      pInfo = pInfo_new;
      pInfo->size = 0;
      fragments++;
    }
    pInfo->size = pInfo->size + pTemp->count;
    total = total + pTemp->count;
    pTemp = pTemp->pNext;
  }

  head = sym_sort_symbols(table, ALL, FOR_ELF); /* Make temp. symbol list */

  prog_start = ELF_EHSIZE;
  symtab_count = sym_count_symbols(table, ALL) + 1; /* Number of entries */
  /* " + 1" is for dummy first entry */
  symtab_local_count = symtab_count - sym_count_symbols(table, EXPORTED);
  strings = sym_strtab(head, symtab_count, &strtab_length);

  shstrtab_length = 1; /* Build shstrtab in memory */
  i = 0;
  do {
    shstrtab_length++;
  } while (elf_file_name[i++] != '\0');
  i = 0;
  do {
    shstrtab_length++;
  } while (sym_sectionname[i++] != '\0');
  i = 0;
  do {
    shstrtab_length++;
  } while (str_sectionname[i++] != '\0');
  i = 0;
  do {
    shstrtab_length++;
  } while (shs_sectionname[i++] != '\0');
  shstrtab_length = (shstrtab_length + 3) & 0xFFFFFFFC;

  SHstrings = (char*)malloc(shstrtab_length); /* Allocate block */
  j = 0;                                      /* Fill block */
  SHstrings[j++] = '\0';
  code_SHstr_offset = j;
  i = 0;
  do {
    SHstrings[j++] = elf_file_name[i];
  } while (elf_file_name[i++] != '\0');
  sym_SHstr_offset = j;
  i = 0;
  do {
    SHstrings[j++] = sym_sectionname[i];
  } while (sym_sectionname[i++] != '\0');
  str_SHstr_offset = j;
  i = 0;
  do {
    SHstrings[j++] = str_sectionname[i];
  } while (str_sectionname[i++] != '\0');
  SHstr_SHstr_offset = j;
  i = 0;
  do {
    SHstrings[j++] = shs_sectionname[i];
  } while (shs_sectionname[i++] != '\0');
  while ((j & 3) != 0)
    SHstrings[j++] = '\0'; /* Pad to word align */

  pad_to_align = -(total % 4) & 3;
  total = total + pad_to_align;                     /* Word align */
  strtab_length = (strtab_length + 3) & 0xFFFFFFFC; /* Word align */
  symtab_length = 16 * symtab_count;                /* True length */

  elf_dump_word(
      fElf, 0x7F | ('E' << 8) | ('L' << 16) | ('F' << 24)); /* File header */
  elf_dump_word(fElf, 0x00010101);
  elf_dump_word(fElf, 0);
  elf_dump_word(fElf, 0);
  elf_dump_word(fElf, 2 + (ELF_MACHINE << 16));
  elf_dump_word(fElf, 1);
  elf_dump_word(fElf, entry_address);
  elf_dump_word(fElf, prog_start + total + symtab_length + strtab_length +
                          shstrtab_length);
  elf_dump_word(fElf, prog_start + total + symtab_length + strtab_length +
                          shstrtab_length + (ELF_PHENTSIZE * fragments));
  elf_dump_word(fElf, 0);  // Flags @@@
  elf_dump_word(fElf, ELF_EHSIZE + (ELF_PHENTSIZE << 16));
  elf_dump_word(fElf, fragments + (ELF_SHENTSIZE << 16));
  elf_dump_word(fElf, (fragments + 4) + ((fragments + 3) << 16));  // @@@

  pTemp = elf_record_list;

  while (pTemp != NULL) /* Dump the code sections */
  {
    for (i = 0; i < pTemp->count; i++) {
      fprintf(fElf, "%c", pTemp->data[i]);
    }
    pTemp = pTemp->pNext;
  }
  for (i = 0; i < pad_to_align; i++)
    fprintf(fElf, "%c", 0); /* Align */

  /* Symbol table - values et alia */
  for (i = 0; i < 4; i++)
    elf_dump_word(fElf, 0); /* Dummy first symbol */

  ptr = head;
  j = 0;
  for (i = 1; i < symtab_count; i++) {
    while (strings[j++] != '\0')
      ; /* Point beyond next '\0' */
    elf_dump_word(fElf, j);
    elf_dump_word(fElf, ptr->value);
    elf_dump_word(fElf, 0x00000000);

    if ((ptr->flags & SYM_REC_EXPORT_FLAG) == 0)
      temp = 0x00;
    else
      temp = 0x10;
    /* Binding */
    if ((ptr->flags & SYM_REC_EQUATED) == 0)
      elf_dump_word(fElf, (ptr->elf_section << 16) | temp);
    else /* EQU, so regard as absolute */
      elf_dump_word(fElf, (ELF_SHN_ABS << 16) | temp);
    ptr = ptr->pNext;
  }

  sym_delete_record_list(&head, false); /* Destroy temporary list */

  // printf("SHStrtab start:  %08X\n",
  // ELF_EHSIZE+total+symtab_length+strtab_length);

  for (i = 0; i < strtab_length; i++)
    fprintf(fElf, "%c", strings[i]);
  for (i = 0; i < shstrtab_length; i++)
    fprintf(fElf, "%c", SHstrings[i]);

  pInfo = pInfo_head;

  for (j = 0; j < fragments; j++)  // PHeader	@@@@@
  {  // Should one -segment- magically cover all these -sections- ?@@@
    elf_dump_word(fElf, 1);
    elf_dump_word(fElf, ELF_EHSIZE + pInfo->position);
    elf_dump_word(fElf, pInfo->address);
    elf_dump_word(fElf, pInfo->address);
    elf_dump_word(fElf, pInfo->size);
    elf_dump_word(fElf, pInfo->size);
    elf_dump_word(fElf, 0x00000000);
    elf_dump_word(fElf, 1);
    pInfo = pInfo->pNext;
  }

  elf_dump_SH(fElf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); /* Dump section table */

  pInfo = pInfo_head;
  for (i = 0; i < fragments; i++) {
    elf_dump_SH(fElf, code_SHstr_offset, 1, 0x07, pInfo->address,
                ELF_EHSIZE + pInfo->position, pInfo->size, 0, 0, 0, 0);
    pInfo = pInfo->pNext;
  }

  elf_dump_SH(fElf, sym_SHstr_offset, 2, 0, 0, ELF_EHSIZE + total,
              symtab_length, fragments + 2, symtab_local_count, 0, 16);  // @@@

  elf_dump_SH(fElf, str_SHstr_offset, 3, 0, 0,
              ELF_EHSIZE + total + symtab_length, strtab_length, 0, 0, 0, 0);

  elf_dump_SH(fElf, SHstr_SHstr_offset, 3, 0, 0,
              ELF_EHSIZE + total + symtab_length + strtab_length,
              shstrtab_length, 0, 0, 0, 0);

  close_output_file(fElf, elf_file_name, pass_errors != 0);

  /* Trash temporary data structures */
  pInfo = pInfo_head;

  while (pInfo != NULL) {
    pInfo_new = pInfo->pNext;
    free(pInfo);
    pInfo = pInfo_new;
  }

  pTemp = elf_record_list;
  while (pTemp != NULL) {
    pTemp_old = pTemp;
    pTemp = pTemp->pNext;
    free(pTemp_old);
  }

  free(strings);
  free(SHstrings);

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Prepare the list output buffer at the start of a line.                     */

void list_start_line(unsigned int address, int cont) {
  list_byte = 0;
  if (!cont)
    list_line_position = 0;
  list_address = address;
  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// Parameterisation a bit dubious; due for re-analysis & revision @@@@

void list_mid_line(unsigned int value, char* line, int size) {
  if (list_byte == 0)                         /* At start of first line */
    list_buffer_init(line, list_byte, true);  /* Zero output buffer */
  else if (((list_byte % 4) == 0)             /* Start of another line */
           || (((list_byte % 4) + size) > 4)) /*  or about to overflow */
  {
    if (list_byte != 0)
      list_file_out();                       /* Dump buffer (?) */
    list_buffer_init(line, list_byte, true); /* Zero output buffer */
  }

  list_hex(value, 2 * size, &list_buffer[10 + 3 * (list_byte % 4)]);
  list_byte = list_byte + size;
  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void list_end_line(char* line) {
  if (list_byte == 0)
    list_buffer_init(line, list_byte, true);
  /* No bytes were dumped */
  list_file_out();

  while (line[list_line_position] !=
         '\0') /* Deal with any continuation lines */
  {
    list_buffer_init(line, 0, false);
    list_file_out();
  }

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Listing line dumped to appropriate file                                    */
/* Excessive use of globals (?? @@)                                           */

void list_file_out(void) {
  if (dump_code && (fList != NULL))
    fprintf(fList, "%s\n", list_buffer);
  return; /* Shouldn't reach here unless there -is- an output file */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Add the symbol table to the list file                                      */

void list_symbols(FILE* fList, sym_table* table) {
  unsigned int sym_count;
  sym_record *sorted_list, *pSym;
  int i;

  sym_count = sym_count_symbols(table, ALL);
  if (sym_count > 0)
    fprintf(fList, "\nSymbol Table: %s\n", table->name);
  {
    sorted_list = sym_sort_symbols(table, ALL, DEFINITION);
    /* Generate record list */
    pSym = sorted_list;
    while (pSym != NULL) {
      fprintf(fList, ": ");
      for (i = 0; i < SYM_NAME_MAX; i++) {
        if (i < pSym->count)
          fprintf(fList, "%c", pSym->name[i]);
        else
          fprintf(fList, " ");
      }
      if ((pSym->flags & SYM_REC_DEF_FLAG) != 0)
        fprintf(fList, "  %08X", pSym->value);
      else
        fprintf(fList, "  00000000");

      if ((pSym->flags & SYM_REC_EQU_FLAG) != 0)
        fprintf(fList, "  Value");
      else if ((pSym->flags & SYM_REC_USR_FLAG) != 0)
        fprintf(fList, "  Constant");
      else if ((pSym->flags & SYM_REC_DATA_FLAG) != 0)
        fprintf(fList, "  Offset");
      else if ((pSym->flags & SYM_REC_DEF_FLAG) == 0)
        fprintf(fList, "  Undefined");
      else {
        if ((pSym->flags & SYM_REC_EXPORT_FLAG) != 0)
          fprintf(fList, "  Global -");
        else
          fprintf(fList, "  Local --");
        if ((pSym->flags & SYM_REC_THUMB_FLAG) == 0)
          fprintf(fList, " ARM");
        else
          fprintf(fList, " Thumb");
      }
      fprintf(fList, "\n");
      pSym = pSym->pNext;
    }

    sym_delete_record_list(&sorted_list, false); /* Destroy temporary list */
  }
  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void list_buffer_init(char* line, unsigned int offset, int do_address) {
  int i;

  for (i = 0; i < LIST_BYTE_FIELD; i++)
    list_buffer[i] = ' ';
  if (do_address) {
    list_hex(list_address + offset, 8, &list_buffer[0]);
    list_buffer[8] = ':';
  }
  list_buffer[LIST_BYTE_FIELD - 2] = ';';

  for (i = 0; (i < LIST_LINE_LIST) && (line[list_line_position] != '\0');
       i++, list_line_position++) {
    if (line[list_line_position] != '\t') /* Not a TAB */
      list_buffer[LIST_BYTE_FIELD + i] = line[list_line_position];
    else { /* Expand TAB into list line (space to column # next multiple of 8)
            */
      do   /* "DO" to guarantee at least one space */
      {
        list_buffer[LIST_BYTE_FIELD + i] = ' ';
        i++;
      } while (((i % 8) != 0) && (i < LIST_LINE_LIST));
      i--; /* DO loop post-increments; so does surrounding FOR, so step back */
    }
  }

  list_buffer[LIST_BYTE_FIELD + i] = '\0'; /* Terminate buffer */

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void list_hex(unsigned int number, unsigned int length, char* destination) {
  int i, digit;

  for (i = 0; i < length; i++) {
    digit = (number >> (4 * (length - i - 1))) & 0xF;
    if (digit < 10)
      destination[i] = '0' + digit;
    else
      destination[i] = 'A' + digit - 10;
  }
  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Create a new, empty symbol table with given attributes.                    */
/* On input: name is a zero terminated ASCII string, given to the table       */
/*           flags contain the associated attributes                          */
/* Returns:  pointer (handle) for the table (NULL on failure)                 */

sym_table* sym_create_table(char* name, unsigned int flags) {
  sym_table* new_table;
  int i;

  for (i = 0; name[i] != '\0'; i++)
    ; /* Find length of string (excl. terminator) */

  new_table = (sym_table*)malloc(SYM_TABLE_SIZE); /* Allocate header */
  if (new_table != NULL) {
    new_table->name = (char*)malloc(i + 1); /* Allocate name string */
    if (new_table->name == NULL) {          /* Problem - tidy up and leave */
      free(new_table);
      new_table = NULL;
    } else {
      new_table->symbol_number = 0; /* Next unique identifier for record */
      while (i >= 0) {
        new_table->name[i] = name[i];
        i--;
      } /* Includes terminator */
      new_table->flags = flags;
      for (i = 0; i < SYM_TAB_LIST_COUNT; i++) /* Initialise linked lists */
        new_table->pList[i] = NULL;
    }
  }
  return new_table;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Delete a symbol table, including all its contents, unless records are both */
/* wanted and marked for export.                                              */
/* On input: old_table is the symbol table for destruction                    */
/*           export is a Boolean - true allows records marked for export to   */
/*             retained                                                       */
/* Returns:  Boolean - true if some of the table remains                      */

int sym_delete_table(sym_table* old_table, bool export) {
  int i;
  bool some_kept;

  some_kept = export && ((old_table->flags & SYM_TAB_EXPORT_FLAG) != 0);

  if (!some_kept) /* Not exporting whole table */
    for (i = 0; i < SYM_TAB_LIST_COUNT;
         i++) /* Chain down lists, deleting records */
      if (sym_delete_record_list(&(old_table->pList[i]), export))
        some_kept = true;

  if (!some_kept) {
    free(old_table->name);
    free(old_table);
  } /* Free, if poss. */

  return some_kept;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Define a label with the given name, value and attributes in the specified  */
/* symbol table.  Allocates memory as appropriate (linked into symbol table). */
/* On input: *name points to a string which is the label name                 */
/*           value holds the value for definition                             */
/*           flags holds the attributes                                       */
/*           table points to an existing symbol table                         */
/*           **record defines a pointer for a return value                    */
/* Returns:  enumerated type indicating the action taken                      */
/*           pointer to the appropriate record in var. specified by **record  */

defn_return sym_define_label(char* name,
                             unsigned int value,
                             unsigned int flags,
                             sym_table* table,
                             sym_record** record) {
  sym_record *ptr1, *ptr2;
  defn_return result;

  ptr1 = sym_create_record(name, value, flags | SYM_REC_DEF_FLAG, table->flags);

  if ((table == NULL) || (ptr1 == NULL))
    result = SYM_REC_ERROR; /* Oooer! */
  else {
    if ((ptr2 = sym_find_record(table, ptr1)) ==
        NULL) /* Label already exists? */
    {
      sym_add_to_table(table, ptr1); /*  No - add the new record */
      *record = ptr1;                /* Point at new record */
      result = SYM_REC_ADDED;
    } else {
      if ((ptr2->flags & SYM_REC_DEF_FLAG) == 0) /* Undefined? */
      {
        ptr2->flags |=
            SYM_REC_DEF_FLAG;      /* First definition of existing label */
        ptr2->value = ptr1->value; /* Update value */
        result = SYM_REC_DEFINED;
      } else if (ptr2->value != ptr1->value) /* Value different? */
      {
        ptr2->value = ptr1->value; /* Update value */
        result = SYM_REC_REDEFINED;
      } else
        result = SYM_REC_UNCHANGED;

      *record = ptr2;          /* Point at discovered record */
      sym_delete_record(ptr1); /* Trash temporary record */
    }
  }

  return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Locate a label with the given name and attributes in the specified symbol  */
/* table.  Creates the entry if it wasn't there.  The value is undefined.     */
/* Allocates memory as appropriate (linked into symbol table).                */
/* On input: *name points to a string which is the label name                 */
/*           flags holds the attributes                                       */
/*           table points to an existing symbol table                         */
/*           **record defines a pointer for a return value                    */
/* Returns:  true if the record was found (previously existed)                */
/*           pointer to the appropriate record in var. specified by **record  */

int sym_locate_label(char* name,
                     unsigned int flags,
                     sym_table* table,
                     sym_record** record) {
  sym_record *ptr1, *ptr2;
  bool result;
  // defn_return result;

  ptr1 = sym_create_record(name, 0, flags & ~SYM_REC_DEF_FLAG, table->flags);

  if ((ptr2 = sym_find_record(table, ptr1)) == NULL) /* Label already exists? */
  {
    *record = ptr1; /* Point at new record */
    result = false;
  } else {
    sym_delete_record(ptr1); /* Trash temporary record */
    *record = ptr2;          /* Point at discovered record */
    result = true;
  }

  return result;
}  // Errors?  (If allocation fails?)  @@@@@@

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Find a label (by name) in the designated list of tables.                   */
/* On input: name points to a string which is the label name                  */
/*           table points to an existing list of symbol tables                */
/* Returns:  pointer to record (NULL if not found)                            */

sym_record* sym_find_label_list(char* name, sym_table_item* item) {
  sym_table* table;
  sym_record* result;

  result = NULL; /* In case nothing in list */

  while ((item != NULL) && (result == NULL)) /* Terminate if EOList or found */
  {
    table = item->pTable;
    if (table != NULL)
      result = sym_find_label(name, table);
    item = item->pNext;
  }

  return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Find a label (by name) in the designated table.                            */
/* On input: name points to a string which is the label name                  */
/*           table points to an existing symbol table                         */
/* Returns:  pointer to record (NULL if not found)                            */

sym_record* sym_find_label(char* name, sym_table* table) {
  sym_record *temp, *result;

  temp = sym_create_record(name, 0, 0, table->flags); /* Hash name */
  result = sym_find_record(table, temp);              /* Search */
  sym_delete_record(temp); /* Lose temporary record */
  return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Create a new symbol record, complete with hashing etc.                     */
/* On input: name is the label name (ASCII string)                            */
/*           value is the initial value for the record                        */
/*           flags define other aspects of the record                         */
/*           global_flags define the symbol table properties                  */
/* Returns:  pointer to allocated record (NULL if this failed)                */

sym_record* sym_create_record(char* name,
                              unsigned int value,
                              unsigned int flags,
                              unsigned int global_flags) {
  sym_record* new_record;

  new_record = (sym_record*)malloc(SYM_RECORD_SIZE); /* Allocate record */

  if (new_record != NULL) {
    sym_string_copy(name, new_record, global_flags);
    new_record->pNext = NULL;
    new_record->value = value;
    new_record->flags = flags;
  }

  return new_record;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Delete a single linked list of symbol records.                             */
/* Will retain records marked for export if requested to.                     */
/* On input: ptr1 is the adddress of the start of list pointer                */
/*           export is a Boolean indicating that records may be retained      */
/* Returns:  true if any records have been kept                               */

int sym_delete_record_list(sym_record** ptr1, int export) {
  bool some_kept;
  sym_record *ptr2,
      *ptr3; /* Current and next records; ptr1 => current pointer */

  some_kept = false; /* Default to keeping nothing */

  while (*ptr1 != NULL) /* While not end of list ... */
  {
    ptr2 = *ptr1; /* Record for consideration */

    if (!export ||
        ((ptr2->flags & SYM_REC_EXPORT_FLAG) == 0)) { /* Delete record */
      ptr3 = ptr2->pNext;      /* Salvage link to next record */
      *ptr1 = ptr3;            /* Point previous link past current record */
      sym_delete_record(ptr2); /* Trash current record */
    } else {
      ptr1 = &ptr2->pNext; /* Move on ... */
      some_kept = true;    /* Noting that something was retained */
    }
  }

  return some_kept;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Delete a single symbol record                                              */

void sym_delete_record(
    sym_record* old_record) { /* Can deallocate strings etc. if such have been
                                 allocated @@ */
  free(old_record);
  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Add record to appropriate part of table (front of list)                    */

int sym_add_to_table(sym_table* table, sym_record* record) {
  unsigned int list; /* Which data substructure is used */

  if (table != NULL) {
    list = record->hash & SYM_TAB_LIST_MASK;

    record->identifier = table->symbol_number++; /* Allocate unique record No */
    record->pNext = table->pList[list];
    table->pList[list] = record;

    return SYM_NO_ERROR;
  } else
    return SYM_NO_TABLE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Search for record's twin in specified table and return a pointer.          */
/* Returns NULL if not found.                                                 */

sym_record* sym_find_record(sym_table* table, sym_record* record) {
  sym_record* ptr;
  bool found;
  int i;

  if (table != NULL) {
    ptr =
        table->pList[record->hash & SYM_TAB_LIST_MASK]; /* Correct list start */
    found = false;

    while ((ptr != NULL) && !found) {
      if ((ptr->hash == record->hash) && (ptr->count == record->count)) {
        i = 0;
        found = true;                     /* Speculation, at present */
        while ((i < ptr->count) && found) /* Scan string */
        {
          found = (ptr->name[i] == record->name[i]); /* Not found after all? */
          i++;
        }
      }
      if (!found)
        ptr = ptr->pNext; /* If not found, try again */
    }
    return ptr;
  } else
    return NULL; /* If table pointer not valid, not found */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*  Copy a string into a specified record, including case conversion,         */
/* generating hash functions, etc.                                            */

void sym_string_copy(char* string,
                     sym_record* record,
                     unsigned int table_flags) {
  unsigned int hash, count;
  int case_insensitive;
  char c;

  case_insensitive = ((table_flags & SYM_TAB_CASE_FLAG) != 0);

  count = 0;
  hash = 0;
  while ((c = string[count]) != '\0') {
    if (case_insensitive && (c >= 'a') && (c <= 'z'))
      c = c & 0xDF; /* Case conv? */
    if (count < SYM_NAME_MAX)
      record->name[count] = c;
    /* Keep characters whilst there is space */

    hash = (((hash << 5) ^ (hash >> 11)) + c); /* Crude but spreads LSBs */
    count++;
  } /* Doesn't copy the terminator */

  record->count = count;
  record->hash = hash;

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Make up an array of all the strings in the symbol table                    */
/* Used for ELF symbol table output                                           */

char* sym_strtab(sym_record* start, unsigned int count, unsigned int* length) {
  unsigned int index, i;
  char* array;
  sym_record* ptr;

  ptr = start;
  *length = 1; /* For null string at start */

  while (ptr != NULL) /* Measure space for symbol strings */
  {
    *length = *length + ptr->count + 1;
    ptr = ptr->pNext;
  }
  *length = (*length + 3) & 0xFFFFFFFC; /* Word align */

  array = (char*)malloc(*length); /* Allocate buffer space */
                                  // No error checking @@@
  index = 0;
  array[index++] = '\0'; /* "" at start */

  ptr = start;
  while (ptr != NULL) {
    for (i = 0; i < ptr->count; i++)
      array[index++] = ptr->name[i];
    array[index++] = '\0'; /* Terminator */
    ptr = ptr->pNext;
  }

  while ((index & 3) != 0)
    array[index++] = '\0'; /* Pad to word */

  return array;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Count entries of a specified type in a specified symbol table              */

unsigned int sym_count_symbols(sym_table* table, label_category what) {
  unsigned int count, i;
  sym_record* ptr;

  count = 0;

  for (i = 0; i < SYM_TAB_LIST_COUNT; i++) /* For all structures */
  {
    ptr = table->pList[i]; /* Start of list */
    while (ptr != NULL) {
      if ((what != EXPORTED) || ((ptr->flags & SYM_REC_EXPORT_FLAG) != 0))
        count++;
      ptr = ptr->pNext;
    }
  }

  return count;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Returns a newly created (allocated) linked list of symbol records copied   */
/* from the designated table and sorted as specified.                         */

sym_record* sym_sort_symbols(sym_table* table,
                             label_category what,
                             label_sort how) {
  void sym_dup_record(sym_record * old_record, sym_record * new_record) {
    unsigned int i, j;

    new_record->count = old_record->count;
    new_record->hash = old_record->hash;
    new_record->flags = old_record->flags;
    new_record->identifier = old_record->identifier;
    new_record->value = old_record->value;
    new_record->elf_section = old_record->elf_section;
    j = old_record->count;
    if (j > SYM_NAME_MAX)
      j = SYM_NAME_MAX;
    for (i = 0; i < j; i++)
      new_record->name[i] = old_record->name[i];

    return;
  }

  sym_record *temp_record, *sorted_list, *ptr1, *ptr2, **pptr;
  int i, j, min, after;
  bool found;
  unsigned int flag_mask, flag_match;

  switch (what) /* Class of records to include */
  {
    case ALL:
      flag_mask = 0;
      flag_match = 0;
      break;
    case EXPORTED:
      flag_mask = SYM_REC_EXPORT_FLAG;
      flag_match = SYM_REC_EXPORT_FLAG;
      break;
    case DEFINED:
      flag_mask = SYM_REC_DEF_FLAG;
      flag_match = SYM_REC_DEF_FLAG;
      break;
    case UNDEFINED:
      flag_mask = SYM_REC_DEF_FLAG;
      flag_match = 0;
      break;
    default:
      flag_mask = 0;
      flag_match = 0;
      break;
  }

  sorted_list = NULL;
  for (i = 0; i < SYM_TAB_LIST_COUNT; i++) {
    ptr1 = table->pList[i];
    while (ptr1 != NULL) {
      if ((ptr1->flags & flag_mask) == flag_match) /* Criteria for output */
      {
        temp_record = (sym_record*)malloc(SYM_RECORD_SIZE);
        sym_dup_record(ptr1, temp_record);

        if ((table->flags & SYM_TAB_EXPORT_FLAG) != 0)
          temp_record->flags |= SYM_REC_EXPORT_FLAG; /* Global => local flag */

        pptr = &sorted_list; /* Linked list insertion sort */
        ptr2 = sorted_list;
        found = false;
        while ((ptr2 != NULL) && !found) {
          switch (how) /* Field used for sorting */
          {
            case ALPHABETIC: /* Sort alphabetically */
              if (temp_record->count < ptr2->count)
                min = temp_record->count;
              else
                min = ptr2->count;
              if (min > SYM_NAME_MAX)
                min = SYM_NAME_MAX;
              /* Clip to field length */
              j = 0;
              while ((temp_record->name[j] == ptr2->name[j]) && (j < min))
                j++;
              after =
                  (temp_record->name[j] > ptr2->name[j]) /* After candidate? */
                  ||
                  ((j >= min) && (temp_record->count > j)); /* New string > ? */
              break;

            case VALUE: /* Sort numerically */
              after = (temp_record->value > ptr2->value) &&
                      ((temp_record->flags & SYM_REC_DEF_FLAG) != 0);
              break;

            case DEFINITION: /* In order of definition */
              after = (temp_record->identifier > ptr2->identifier);
              break;

            case FOR_ELF: /* In order of definition, locals first */
              after = ((((temp_record->flags & SYM_REC_EXPORT_FLAG) != 0) &&
                        ((ptr2->flags & SYM_REC_EXPORT_FLAG) == 0)) ||
                       (temp_record->identifier > ptr2->identifier));
              break;
          }

          if (after) {
            pptr = &(ptr2->pNext);
            ptr2 = ptr2->pNext; /* Lower, keep going */
          } else
            found = true;

          //        DUPLICATES  ???
        }
        temp_record->pNext = ptr2; /* Insert created record */
        *pptr = temp_record;
      }

      ptr1 = ptr1->pNext; /* Next source record */
    }
  }
  return sorted_list;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Print out symbols in table.  Tedious, because symbols need sorting.        */

void sym_print_table(sym_table* table,
                     label_category what,
                     label_sort how,
                     int std_out,
                     char* file) {
  sym_record *sorted_list, *ptr;
  int i;
  FILE* handle;

  if (std_out)
    handle = stdout;
  else
    handle = fopen(file, "w"); /* Open output file */

  if (file == NULL)
    fprintf(stderr, "Can't open symbol file: %s\n", file);
  else {
    sorted_list = sym_sort_symbols(table, what, how); /* Generate record list */

    fprintf(handle, "\nSymbol table: %s\n", table->name);
    fprintf(handle, "Label");
    for (i = 0; i < SYM_NAME_MAX - 2; i++)
      fprintf(handle, " ");
    //  fprintf(handle, "  ID      Length     Hash     Value    Type\n");
    fprintf(handle, "  ID      Value    Type\n");

    ptr = sorted_list;
    while (ptr != NULL) {
      for (i = 0; i < SYM_NAME_MAX; i++) {
        if (i < ptr->count)
          fprintf(handle, "%c", ptr->name[i]);
        else if (i > ptr->count + 1)
          fprintf(handle, ".");
        else
          fprintf(handle, " ");
      }
      fprintf(handle, "  %08X", ptr->identifier);
      //    fprintf(handle, "  %08X", ptr->count);
      //    fprintf(handle, "  %08X", ptr->hash);
      if ((ptr->flags & SYM_REC_DEF_FLAG) != 0)
        fprintf(handle, "  %08X", ptr->value);
      else
        fprintf(handle, " Undefined %d", ptr->value);

      if ((ptr->flags & SYM_REC_USR_FLAG) != 0)
        fprintf(handle, "  Constant   ");
      else if ((ptr->flags & SYM_REC_EQU_FLAG) != 0)
        fprintf(handle, "  Value      ");
      else if ((ptr->flags & SYM_REC_DATA_FLAG) != 0)
        fprintf(handle, "  Offset     ");
      else {
        if ((ptr->flags & SYM_REC_THUMB_FLAG) == 0)
          fprintf(handle, "  ARM label  ");
        else
          fprintf(handle, "  Thumb label");
      }

      if ((ptr->flags & SYM_REC_EXPORT_FLAG) !=
          0) /* Table flags in temp recd. */
        fprintf(handle, " (exported)");
      fprintf(handle, "\n");
      ptr = ptr->pNext;
    }

    sym_delete_record_list(&sorted_list, false); /* Destroy temporary list */

    if ((sym_print_extras & 1) != 0)
      local_label_dump(loc_lab_list, handle);
    if ((sym_print_extras & 2) != 0)
      lit_print_table(literal_list, handle);

    if (handle != stdout)
      fclose(handle);
  }

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void local_label_dump(local_label* pTable, FILE* handle) {
  if (pTable != NULL) {
    fprintf(handle, "\nLocal (labels in order of definition):\n");
    fprintf(handle, "           Local Label     Value\n");
    while (pTable != NULL) {
      fprintf(handle, "%22d:  %08X\n", pTable->label, pTable->value);
      pTable = pTable->pNext;
    }
  }

  return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void lit_print_table(literal_record* pTemp, FILE* handle) {
  bool nothing;

  nothing = true;

  while (pTemp != NULL) {
    if ((pTemp->flags & LIT_NO_DUMP) == 0) {
      if (nothing) {
        fprintf(handle, "\nLiteral pool:  Address     Value\n");
        nothing = false;
      }
      fprintf(handle, "              ");
      if ((pTemp->flags & LIT_HALF) == 0)
        fprintf(handle, "%08X:  %08X\n", pTemp->address, pTemp->value);
      else
        fprintf(handle, "%08X:      %04X\n", pTemp->address, pTemp->value);
    }
    pTemp = pTemp->pNext;
  }
  return;
}

/**
 * @brief
 * @param priority
 * @param value
 * @param math_stack
 * @param math_SP
 * @param error
 * @param string
 * @param pos
 * @param symbol_table
 */
void Eval_inner(int priority,
                int* value,
                std::array<unsigned int, MATHSTACK_SIZE>& math_stack,
                unsigned int& math_SP,
                unsigned int& error,
                std::string& string,
                unsigned int*& pos,
                sym_table*& symbol_table,
                unsigned int& first_error) {
  bool done, bracket;
  unsigned int operator, operand, unary;

  done = false;  // Termination indicator

  math_stack[math_SP] = priority;
  math_SP = math_SP + 1;  // Stack `start' marker

  while (!done) {
    error = get_variable(string, pos, &operand, &unary, &bracket, symbol_table);

    // Error not instantly fatal
    if ((error & ALL_EXCEPT_LAST_PASS) != 0) {
      // Keep note of error and pretend everything is still okay
      if (first_error == EVAL_OKAY) {
        first_error = error;
      }
      error = EVAL_OKAY;
    }

    if (error == EVAL_OKAY) {
      if (bracket) {
        Eval_inner(1, &operand, math_stack, math_SP, error, string, pos,
                   symbol_table, first_error);  // May return error
      }
      // Can now apply unary to returned value
      if (error == EVAL_OKAY) {
        switch (unary) {
          case PLUS:
            break;
          case MINUS:
            operand = -operand;
            break;
          case NOT:
            operand = ~operand;
            break;
          // Truncated log2 of operand
          case LOG: {
            unsigned int i;
            i = operand;
            operand = -1;
            while (i > 0) {
              operand++;
              i = i >> 1;
            }
          } break;
        }

        if ((error = get_operator(string, pos, &operator, & priority)) ==
            EVAL_OKAY) {
          // If priority decreasing and previous a real operator, OPERATE
          while ((priority <= math_stack[math_SP - 1]) &&
                 (math_stack[math_SP - 1] > 1)) {
            switch (math_stack[math_SP - 2]) {
              case PLUS:
                operand = math_stack[math_SP - 3] + operand;
                break;
              case MINUS:
                operand = math_stack[math_SP - 3] - operand;
                break;
              case MULTIPLY:
                operand = math_stack[math_SP - 3] * operand;
                break;
              case DIVIDE:
                if (operand != 0) {
                  operand = math_stack[math_SP - 3] / operand;
                } else {
                  operand = -1;
                  if ((error == EVAL_OKAY) && (first_error == EVAL_OKAY)) {
                    error = EVAL_DIV_BY_ZERO;
                  }
                  div_zero_this_pass = true;
                }
                break;
              case MODULUS:
                // else leave it alone
                if (operand != 0) {
                  operand = math_stack[math_SP - 3] % operand;
                }
                break;
              case LEFT_SHIFT:
                operand = math_stack[math_SP - 3] << operand;
                break;
              case RIGHT_SHIFT:
                operand = math_stack[math_SP - 3] >> operand;
                break;
              case AND:
                operand = math_stack[math_SP - 3] & operand;
                break;
              case OR:
                operand = math_stack[math_SP - 3] | operand;
                break;
              case XOR:
                operand = math_stack[math_SP - 3] ^ operand;
                break;
              case EQUALS:
                if (math_stack[math_SP - 3] == operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              case NOT_EQUAL:
                if (math_stack[math_SP - 3] != operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              case LOWER_THAN:
                if (math_stack[math_SP - 3] < operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              case LOWER_EQUAL:
                if (math_stack[math_SP - 3] <= operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              case HIGHER_THAN:
                if (math_stack[math_SP - 3] > operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              case HIGHER_EQUAL:
                if (math_stack[math_SP - 3] >= operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              case LESS_THAN:
                if ((int)math_stack[math_SP - 3] < (int)operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              case LESS_EQUAL:
                if ((int)math_stack[math_SP - 3] <= (int)operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              case GREATER_THAN:
                if ((int)math_stack[math_SP - 3] > (int)operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              case GREATER_EQUAL:
                if ((int)math_stack[math_SP - 3] >= (int)operand) {
                  operand = -1;
                } else {
                  operand = 0;
                }
                break;
              default:
                break;
            }
            math_SP = math_SP - 3;
          }
          done = (priority <= 1);  // Next operator a ")" or end

          // Priority must be increasing
          if (!done) {
            // PUSH
            if ((math_SP + 3) <= MATHSTACK_SIZE) {
              math_stack[math_SP] = operand;
              math_stack[math_SP + 1] = operator;
              math_stack[math_SP + 2] = priority;
              math_SP = math_SP + 3;
            } else {
              error = EVAL_MATHSTACK_LIMIT;  // Don't overflow stack
            }
          }
          // Now bracketed by terminators.  Matched?
          else {
            if (priority == math_stack[math_SP - 1]) {
              math_SP = math_SP - 1;
            } else if (priority == 0) {
              error = EVAL_NO_CLOSEBR;  // Errors
            } else {
              error = EVAL_NO_OPENBR;
            }
          }
        }
      }
    }
    if (error != EVAL_OKAY) {
      done = true;  // Terminate on error whatever else

      // Include position on line (if poss.)
      if (error == EVAL_NO_OPENBR) {
        error = error | (*pos - 1);  // Has stepped over extra ')'
      }
      // Arithmetic error will occur late
      else if (error != EVAL_DIV_BY_ZERO) {
        error = error | *pos;  // Include position on line
      }
    }
  }

  *value = operand;
}

/**
 * @brief Evaluate - modulo current word length
 * On entry: *string points to a pointer to the input string
 *            *pos points to an offset in the string
 *            *value points to the location for the result value
 *            *symbol_table points to the symbol table to search [@@ extend]
 * On exit:  the pointer at *pos is adjusted to the end of the expression
 *           the value at *value contains the result, assuming no error
 *           the return value is the error status
 * @param string
 * @param pos
 * @param value
 * @param symbol_table
 * @return unsigned int
 */
unsigned int evaluate(std::string string,
                      unsigned int* pos,
                      int* value,
                      sym_table* symbol_table) {
  std::array<unsigned int, MATHSTACK_SIZE> math_stack;
  unsigned int math_SP, error, first_error;

  error = EVAL_OKAY;        // "Evaluate" initialised and called from here
  first_error = EVAL_OKAY;  // Used to note if labels undefined, etc.
  math_SP = 0;
  Eval_inner(0, value, math_stack, math_SP, error, string, pos, symbol_table,
             first_error);  // Potentially recursive evaluation code

  if (error == EVAL_OKAY) {
    return first_error;  // Signal any problems held over
  } else {
    return error;
  }
}

/**
 * @brief Get a quantity from the front of the passed ASCII input string,
 * stripping the value in the process.
 * On entry: *input points to the input string
 *            *pos points to the offset in this string
 *            *value points to the location for the result value
 *            *unary points to the location for the result's unary indicator
 *            *bracket points to the location of a Boolean "found '(' signal
 *            *symbol_table points to the symbol table to search [@@ extend]
 * On exit:  the position at *pos is adjusted to the end of the variable
 *           the value at *value contains the result, assuming no error
 *           the value at *unary contains a unary code, assuming no error
 *           the value at *bracket contains a "'(' found instead" indicator
 *           the return value is the error status
 * @param input
 * @param pos
 * @param value
 * @param unary
 * @param bracket
 * @param symbol_table
 * @return int
 */
int get_variable(char* input,
                 unsigned int* pos,
                 int* value,
                 int* unary,
                 bool* bracket,
                 sym_table* symbol_table) {
  int status, radix;
  unsigned int ii;

  status = EVAL_NO_OPERAND;      // In case nothing found
  radix = -1;                    // Indicates no numeric constant spotted
  *pos = skip_spc(input, *pos);  // In case of error want this at next item
  ii = *pos;                     // String pointer within routine

  *unary = PLUS;     // Default
  *bracket = false;  // Default - no brackets
  *value = 0;

  // Deal with unary operators
  if (input[ii] == '+') {
    ii = skip_spc(input, ii + 1);
  } else if (input[ii] == '-') {
    *unary = MINUS;
    ii = skip_spc(input, ii + 1);
  } else if (input[ii] == '~') {
    *unary = NOT;
    ii = skip_spc(input, ii + 1);
  } else if (input[ii] == '|') {
    *unary = LOG;
    ii = skip_spc(input, ii + 1);
  }

  if (input[ii] == '(') {  // Open brackets instead of value
    *bracket = true;
    ii++;  // Skip bracket
    status = EVAL_OKAY;
  } else {
    int i;
    char ident[LINE_LENGTH];
    sym_record* symbol;

    if ((i = get_identifier(input, ii, ident, LINE_LENGTH)) > 0) {
      // Something taken
      if ((symbol = sym_find_label(ident, symbol_table)) != NULL) {
        // Label present and with a valid value
        if ((symbol->flags & SYM_REC_DEF_FLAG) != 0) {
          *value = symbol->value;
          status = EVAL_OKAY;
        }
        // Label found but value invalid
        else {
          status = EVAL_LABEL_UNDEF | ii;
          undefined_count++;  // Increment global variable
        }
        // Label not found
      } else {
        status = EVAL_NO_LIMIT | ii;
      }
      ii = ii + i;  // Step pointer on End of label gathering
    } else {
      if (input[ii] == '\%') {
        local_label *pStart, *pTemp;
        char c;
        int directions;  // Bit flags for search directions
        unsigned int label;

        c = input[ii + 1] & 0xDF;

        if (c == 'B') {
          directions = 1;
          ii = ii + 2;
        }
        // Backwards
        else if (c == 'F') {
          directions = 2;
          ii = ii + 2;
        }
        // Forwards
        else {
          directions = 3;
          ii = ii + 1;
        }  // Both ways

        // If searching forwards only and no local label on this line
        if ((evaluate_own_label->sort != LOCAL_LABEL) &&
            ((directions & 1) == 0)) {
          if (loc_lab_position == NULL) {
            pStart = loc_lab_list;  // Start of list
          } else {
            pStart = loc_lab_position->pNext;
          }
        }
        // If searching backwards, own label will be present already
        else {
          pStart = loc_lab_position;
        }
        if (!get_num(input, &ii, &label, 10)) {
          status = EVAL_BAD_LOC_LAB;
        } else {
          bool found;

          found = false;

          // Seach backwards
          if ((directions & 1) != 0) {
            pTemp = pStart;
            while ((pTemp != NULL) && !found) {
              if (!(found = (label == pTemp->label))) {
                pTemp = pTemp->pPrev;
              }
            }
          }

          // Seach forwards
          if (!found && ((directions & 2) != 0)) {
            pTemp = pStart;
            while ((pTemp != NULL) && !found) {
              if (!(found = (label == pTemp->label))) {
                pTemp = pTemp->pNext;
              }
            }
          }

          if (found) {
            status = EVAL_OKAY;
            *value = pTemp->value;
          } else {
            status = EVAL_NO_LIMIT;
          }
        }
      } else {
        // Character constant
        if (input[ii] == '\'') {
          ii++;

          // 'Escaped' character?
          // ;  if (true)  Insert if escapes
          // unwanted
          // No ...
          if (input[ii] != '\\') {
            if ((input[ii] != '\0') && (input[ii] != '\n') &&
                (input[ii + 1] == '\'')) {
              *value = input[ii];
              ii += 2;
              status = EVAL_OKAY;
            } else {
              status = EVAL_OPERAND_ERROR | ii;
            }
          }
          // C-style escaped characters
          else {
            ii++;  // Skip '\'
            if ((input[ii] != '\0') && (input[ii] != '\n') &&
                (input[ii + 1] == '\'')) {
              *value = c_char_esc(input[ii]);
              ii += 2;
              status = EVAL_OKAY;
            } else {
              status = EVAL_OPERAND_ERROR | ii;
            }
          }
        } else {
          if (input[ii] == '.') {
            if (assembly_pointer_defined) {
              *value = assembly_pointer + def_increment;
              status = EVAL_OKAY;
            } else {
              status = EVAL_LABEL_UNDEF | ii;
            }
            ii++;
          }
          // Try for a number
          else {
            // 'orrible 'ex prefices, etc.
            if (input[ii] == '0') {
              if ((input[ii + 1] & 0xDF) == 'X') {
                ii += 2;
                radix = 16;
              } else if ((input[ii + 1] & 0xDF) == 'B') {
                ii += 2;
                radix = 2;
              }
            }
            // Not yet identified
            if (radix < 0) {
              if ((input[ii] >= '0') && (input[ii] <= '9'))
                radix = 10;
              else if (input[ii] == '$') {
                ii++;
                radix = 16;
              } else if (input[ii] == '&') {
                ii++;
                radix = 16;
              } else if (input[ii] == ':') {
                ii++;
                radix = 2;
              } else if (input[ii] == '@') {
                ii++;
                radix = 8;
              }
            }
            if (radix > 0) {
              if (get_num(input, &ii, value, radix)) {
                status = EVAL_OKAY;
              } else {
                status = EVAL_OUT_OF_RADIX;
              }
            }
          }
        }
      }
    }
  }

  if ((status == EVAL_OKAY) || ((status & ALL_EXCEPT_LAST_PASS) != 0)) {
    *pos = ii;  // Move input pointer if successful (in some degree)
  }

  return status;  // Return error code
}

/**
 * @brief Get an operator from the front of the passed ASCII input string,
 * stripping it in the process.  Returns the token and the priority.
 * On entry: *input points to the input string
 *            *pos points to the offset in this string
 *            *operator points to the location for the operator code
 *            *priority points to the location for the priority code
 *                   0 is the lowest priority and is reserved for terminators
 *                   priority 1 is reserved for brackets
 * On exit:  the pointer at *pos is adjusted to the end of the expression
 *           the value at *operator contains the operator code
 *           the value at *priority contains the operator priority
 *           the return value is the error status
 * @param input
 * @param pos
 * @param priority
 * @return int
 */
int get_operator(char* input,
                 unsigned int* pos,
                 int*
                 operator,
                 int * priority) {
  int status;
  unsigned int ii;

  *pos = skip_spc(input, *pos);  // In case of error want this at next item
  ii = *pos;                     // String pointer within routine

  status = EVAL_NO_OPERATOR;

  // in case no operator was found, this will be the default
  switch (input[ii]) {
    case '\0':  // Terminator cases
    case ',':
    case ';':
    case ']':
    case '}':
    case '\n':
      *operator= END;
      status = EVAL_OKAY;
      break;
    case '+':
      *operator= PLUS;
      ii++;
      status = EVAL_OKAY;
      break;
    case '-':
      *operator= MINUS;
      ii++;
      status = EVAL_OKAY;
      break;
    case '*':
      *operator= MULTIPLY;
      ii++;
      status = EVAL_OKAY;
      break;
    case '/':
      *operator= DIVIDE;
      ii++;
      status = EVAL_OKAY;
      break;
    case '\\':
      *operator= MODULUS;
      ii++;
      status = EVAL_OKAY;
      break;
    case ')':
      *operator= CLOSEBR;
      ii++;
      status = EVAL_OKAY;
      break;
    case '|':
      *operator= OR;
      ii++;
      status = EVAL_OKAY;
      break;
      // case '&':  *operator = AND;       ii++;  status = EVAL_OKAY; break;
    case '^':
      *operator= XOR;
      ii++;
      status = EVAL_OKAY;
      break;
    case '=':
      *operator= EQUALS;
      ii++;
      status = EVAL_OKAY;
      break;
    case '!':
      if (input[ii + 1] == '=') {
        *operator= NOT_EQUAL;
        ii += 2;
        status = EVAL_OKAY;
      }
      break;
    case '<':
      switch (input[ii + 1]) {
        case '<':
          *operator= LEFT_SHIFT;
          ii += 2;
          break;
        case '>':
          *operator= NOT_EQUAL;
          ii += 2;
          break;
        case '=':
          *operator= LOWER_EQUAL;
          ii += 2;
          break;
        default:
          *operator= LOWER_THAN;
          ii += 1;
          break;
      }
      status = EVAL_OKAY;
      break;
    case '>':
      switch (input[ii + 1]) {
        case '>':
          *operator= RIGHT_SHIFT;
          ii += 2;
          break;
        case '=':
          *operator= HIGHER_EQUAL;
          ii += 2;
          break;
        default:
          *operator= HIGHER_THAN;
          ii += 1;
          break;
      }
      status = EVAL_OKAY;
      break;

    // Have a go at symbolically defined operators
    default: {
      int i;
      char buffer[SYM_NAME_MAX];
      sym_record* ptr;

      // Something taken
      if ((i = get_identifier(input, ii, buffer, SYM_NAME_MAX)) > 0) {
        // Symbol recognised
        if ((ptr = sym_find_label(buffer, operator_table)) != NULL) {
          *operator= ptr->value;
          ii += i;
          status = EVAL_OKAY;
        }
      }
    }
  }

  // Priority "look up". The first two priorities are fixed
  switch (*operator) {
    case END:
      *priority = 0;
      break;
    case CLOSEBR:
      *priority = 1;
      break;
    case PLUS:
      *priority = 3;
      break;
    case MINUS:
      *priority = 3;
      break;
    case MULTIPLY:
      *priority = 4;
      break;
    case DIVIDE:
      *priority = 4;
      break;
    case MODULUS:
      *priority = 4;
      break;
    case LEFT_SHIFT:
      *priority = 7;
      break;
    case RIGHT_SHIFT:
      *priority = 7;
      break;
    case AND:
      *priority = 6;
      break;
    case OR:
      *priority = 5;
      break;
    case XOR:
      *priority = 5;
      break;
    case EQUALS:
      *priority = 2;
      break;
    case NOT_EQUAL:
      *priority = 2;
      break;
    case LOWER_THAN:
      *priority = 2;
      break;
    case LOWER_EQUAL:
      *priority = 2;
      break;
    case HIGHER_THAN:
      *priority = 2;
      break;
    case HIGHER_EQUAL:
      *priority = 2;
      break;
    case LESS_THAN:
      *priority = 2;
      break;
    case LESS_EQUAL:
      *priority = 2;
      break;
    case GREATER_THAN:
      *priority = 2;
      break;
    case GREATER_EQUAL:
      *priority = 2;
      break;
  }

  if (status == EVAL_OKAY) {
    *pos = ii;  // Move input pointer if successful
  }

  return status;  // Return error code
}

/**
 * @brief
 * @param line
 * @param position
 * @return int
 */
int skip_spc(char* line, int position) {
  while ((line[position] == ' ') || (line[position] == '\t')) {
    position++;
  }

  return position;
}

/**
 * @brief Makes a copy of "filename" with the last element stripped back to
 * '/' Allocates space for new string
 * @param filename
 * @return char* null string if no '/' present
 */
char* file_path(char* filename) {
  char* pPath;
  int position;

  position = strlen(filename);  //  Find back-end of original string
  while ((position > 0) && (filename[position - 1] != '/')) {
    position--;
  }

  pPath = (char*)malloc(position + 1);  //  Allocate space for path + '\0'

  pPath[position] = '\0';  //  Insert terminator, step back

  // Copy rest of string
  while (position > 0) {
    position = position - 1;
    pPath[position] = filename[position];
  }

  return pPath;
}

/**
 * @brief Concatenates source strings and returns newly allocated string
 * @param name1
 * @param name2
 * @return char*
 */
char* pathname(char* name1, char* name2) {
  char* pResult;

  pResult = (char*)malloc(strlen(name1) + strlen(name2) + 1);  // Make room
  strcpy(pResult, name1);  // Copy in first string
  strcat(pResult, name2);  // Append second string

  return pResult;
}

/**
 * @brief If character is found it is stripped
 * @param line
 * @param pPos
 * @param offset
 * @param character
 * @return bool true if `character' is found
 */
bool cmp_next_non_space(char* line, int* pPos, int offset, char character) {
  bool result;

  *pPos = skip_spc(line, *pPos + offset);  // Strip, possibly skipping first
  result = (line[*pPos] == character);
  if (result) {
    (*pPos)++;  // Strip test character
  }
  return result;
}

/**
 * @brief
 * @param character
 * @return bool true if `character' is valid end of statement
 */
bool test_eol(char character) {
  return (character == '\0') || (character == ';') || (character == '\n');
}

/**
 * @brief Get the identifier object
 * @param line
 * @param position
 * @param buffer
 * @param max_length
 * @return unsigned int
 */
unsigned int get_identifier(char* line,
                            unsigned int position,
                            char* buffer,
                            unsigned int max_length) {
  unsigned int i;

  i = 0;
  if (alphabetic(line[position])) {
    while ((alpha_numeric(line[position])) && (i < max_length - 1)) {
      buffer[i++] = line[position++];  // Truncates if too long for buffer
    }
  }

  buffer[i] = '\0';
  return i;  // Length of symbol (sans terminator)
}

/**
 * @brief
 * @param c
 * @return bool
 */
bool alpha_numeric(char c) {
  return (((c >= '0') && (c <= '9')) || alphabetic(c));
}

/**
 * @brief
 * @param c
 * @return bool
 */
bool alphabetic(char c) {
  return ((c == '_') || ((c >= 'A') && (c <= 'Z')) ||
          ((c >= 'a') && (c <= 'z')));
}

/**
 * @brief If recognised, translate a C-style 'escaped' character code. E.g.
 * 'n' will become '\n'
 * @param c
 * @return unsigned char
 */
unsigned char c_char_esc(unsigned char c) {
  switch (c) {
    case '0':
      c = '\0';
      break;
    case '\"':
      c = '\"';
      break;
    case '\'':
      c = '\'';
      break;
    case '\?':
      c = '\?';
      break;
    case '\\':
      c = '\\';
      break;
    case 'a':
      c = '\a';
      break;
    case 'b':
      c = '\b';
      break;
    case 'f':
      c = '\f';
      break;
    case 'n':
      c = '\n';
      break;
    case 'r':
      c = '\r';
      break;
    case 't':
      c = '\t';
      break;
    case 'v':
      c = '\v';
      break;
    default:
      break;
  }

  return c;
}

/**
 * @brief Return value(c) if in radix else return -1
 * @param line
 * @param pos
 * @param radix
 * @return int
 */
int num_char(char* line, int* pos, unsigned int radix) {
  char c;

  while ((c = line[*pos]) == '_') {
    (*pos)++;  // Allow & ignore  '_'
  }
  if (c < '0') {
    return -1;
  }
  if (c >= 'a') {
    c = c & 0xDF;  // Upper case convert
  }
  if (c <= '9') {
    if (c < '0' + radix) {
      return c - '0';  // Number < radix
    } else {
      return -1;
    }
  }  // Number > radix
  else {
    if (c < 'A') {
      return -1;  // Not letter
    } else if (c < 'A' + radix - 10) {
      return c - 'A' + 10;
    } else {
      return -1;
    }
  }
}

/**
 * @brief Read number of specified radix into variable indicated by *value
 * @param line
 * @param position
 * @param value
 * @param radix
 * @return int flag to say number read (value at pointer).
 */
int get_num(char* line, int* position, int* value, unsigned int radix) {
  int i, new_digit;
  bool found;

  i = skip_spc(line, *position);
  *value = 0;
  found = false;

  while ((new_digit = num_char(line, &i, radix)) >= 0) {
    *value = (*value * radix) + new_digit;
    found = true;
    i++;
  }

  if (found) {
    *position = i;  // Move pointer
  }
  return found;
}

/**
 * @brief Test abstracted for convenience of use
 * @param error_code
 * @param first_pass
 * @param last_pass
 * @return int
 */
int allow_error(unsigned int error_code, bool first_pass, bool last_pass) {
  return (!last_pass && ((error_code & ALLOW_ON_INTER_PASS) != 0)) ||
         (first_pass && ((error_code & ALLOW_ON_FIRST_PASS) != 0));
}
