/* The type checker, the refusals R1-R4 and the eval command. SHARED by the
   tcc kits. */
#ifndef LANG_FRONT_CHECK_H
#define LANG_FRONT_CHECK_H

#include "front/core.h"

#define ENTRY_PARAMS_MAX 16u

/* An entry: a program definition of type Nat, Flag, or a function of Nat and
   Flag arguments to a Nat or Flag result. */
typedef struct {
  uint32_t param_count;
  int param_flag[ENTRY_PARAMS_MAX];
  int result_flag;
} Entry;

/* Checks the domain declarations, then the program, into M. Returns 1, or 0
   after a diagnostic. */
int check_program(Arena *arena, const DeclList *decls, Machine *m, Diag *diag);
/* An instance of a read document: its name, its checked type and its decoded
   value (slice C3). */
typedef struct {
  const char *name;
  const Value *type;
  const Value *value;
} ReadDef;
/* As check_program, with the COUNT READS bound as definitions after the domain
   and before the program. A read name that is a core or domain name, or the
   name of a program declaration, is REFUSE_NAME (rule R4). */
int check_program_reads(Arena *arena, const DeclList *decls, const ReadDef *reads, size_t count, Machine *m, Diag *diag);
/* Checks and evaluates one closed type against the declarations already in M.
   Infers its universe without adding a definition to M. */
int check_closed_type(Machine *m, const Term *term, const Value **out);
/* Returns 1 when TYPE is the type of an entry. */
int entry_of(Machine *m, const Value *type, Entry *out);
/* Evaluates the definition NAME on ARGS and prints the result. Returns 0, 1
   after a diagnostic, or 2 after an EVAL_ARGS diagnostic. */
int eval_command(Machine *m, const char *name, char *const *args, int arg_count, FILE *out, FILE *err);

#endif
