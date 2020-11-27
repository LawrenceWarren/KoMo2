/* ':':':':':':':':':'@' '@':':':':':':':':':'@' '@':':':':':':':':':'| */
/* :':':':':':':':':':'@'@':':':':':':':':':':'@'@':':':':':':':':':':| */
/* ':':':': : :':':':':'@':':':':': : :':':':':'@':':':':': : :':':':'| */
/*  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/*		Name:		chump.h					*/
/*		Version:	1.3.0					*/
/*		Date:		7/2/2003				*/
/*  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

#ifndef CHUMP_H
#define CHUMP_H

#include <glib.h>

#define DEFINITIONSTRING "define"
#define INTEGERSTRING "int"
#define RELATIVESTRING "relative"
#define UINTEGERSTRING "uint"
#define URELATIVESTRING "urelative"

typedef enum { NONTERM, TERM, TERM2FREE, NONTERM2FREE } RuleElementType;
// Two separate flags:  TERM points at a string, NONTERM doesn't ("TERMinal
// node") 2FREE indicates a `non-permanent' record

typedef enum {
  TRANRULE,
  INTRULE,
  RELATIVERULE,
  UINTRULE,
  URELATIVERULE
} RuleType;

typedef struct DefinitionStackName {
  struct DefinitionStackName* next;
  char* string;
  GList* rules;
} DefinitionStack;

typedef struct {
  RuleElementType type;  // Defines variant record fields etc.
  char* string;          // Private is TERM*, shared if NONTERM*
  int twin;              // List position of record in another list
  GList* rules;          // Start of substructure if NONTERM*
} RuleElement;

typedef struct {
  RuleType type;
  GList* ascii;
  GList* bit;
} Rule;

typedef struct {
  GList* distext;
  GList* bitfield;
} Disasm;

typedef struct {
  GList* binary;
  char* text;
  GList* ruletext;
} Asm;

GList dis_error;

DefinitionStack* asm_parser(GList*);
DefinitionStack* asm_define(GList*, DefinitionStack*, char*);
Rule* asm_rule(GList*, DefinitionStack*);
Disasm asm_disassemble(GList* bitfield, GList* rules, int);
GList* asm_assemble(char*, GList* rules, int, int);
int asm_assemble_wrapper(char*, GList*, char*, unsigned int, int, char*, int);
Asm asm_assembleold(char*, GList* rules, int, int, int);
GList* chararr2glist(int, unsigned char*);
void asm_print(GList*);
char* asm_sprint(GList* list, char*);
void asm_freestringlist(GList*);
GList* asm_copystringlist(GList*);
char* asm_sprint_tabstring(char*, char*);
int asm_sbprint(GList* list, unsigned char** data);
#endif

/*									*/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 	*/
/*                     end of chump.h					*/
/************************************************************************/
