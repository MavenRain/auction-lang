/* The evaluator, the conversion test, quote and the printer (see
   front/core.h). SHARED by the tcc kits. */
#include "front/core.h"

#include <string.h>

#define PRINT_DEPTH_MAX 200u
#define PRINT_NAMES_MAX 64u
#define VARIES 0xffu

static const char *const OP_NAMES[] = {
  [OP_NAT] = "Nat", [OP_FLAG] = "Flag", [OP_UNIT] = "Unit", [OP_PROD] = "Prod",
  [OP_SUM] = "Sum", [OP_OPTION] = "Option", [OP_LIST] = "List", [OP_EQ] = "Eq",
  [OP_FAMILY] = "?", [OP_UNIT_VAL] = "unit", [OP_FLAG_YES] = "flagYes",
  [OP_FLAG_NO] = "flagNo", [OP_PAIR] = "pair", [OP_INL] = "inl", [OP_INR] = "inr",
  [OP_NONE] = "none", [OP_SOME] = "some", [OP_NIL] = "nil", [OP_CONS] = "cons",
  [OP_PACK] = "pack", [OP_REFL] = "refl", [OP_CTOR] = "?", [OP_FIRST] = "first",
  [OP_SECOND] = "second", [OP_EITHER] = "either", [OP_OPTION_ELIM] = "option",
  [OP_PURE] = "pure", [OP_MAP] = "map", [OP_BIND] = "bind", [OP_FOLD_NAT] = "fold",
  [OP_FOLD_LIST] = "fold", [OP_FOLD_FAMILY] = "fold", [OP_UNFOLD] = "unfold",
  [OP_FILTER] = "filter", [OP_WITNESS] = "witness", [OP_PAYLOAD] = "payload",
  [OP_SYMM] = "symm", [OP_TRANS] = "trans", [OP_TRANSPORT] = "transport",
  [OP_CONG] = "cong", [OP_NAT_ADD] = "natAdd", [OP_NAT_SUB] = "natSub",
  [OP_NAT_MUL] = "natMul", [OP_NAT_EQ] = "natEq", [OP_NAT_LE] = "natLe",
  [OP_FLAG_IF] = "flagIf", [OP_NAT_DIV] = "natDiv", [OP_NAT_MOD] = "natMod",
  [OP_NAT_MAX] = "natMax", [OP_NAT_LT] = "natLt", [OP_FLAG_AND] = "flagAnd",
  [OP_FLAG_NOT] = "flagNot", [OP_FIN] = "Fin", [OP_ALL_FIN] = "allFin",
  [OP_RAT] = "Rat", [OP_RAT_ADD] = "ratAdd", [OP_RAT_SUB] = "ratSub",
  [OP_RAT_MUL] = "ratMul", [OP_RAT_DIV] = "ratDiv", [OP_RAT_OF_NAT] = "ratOfNat",
  [OP_RAT_EQ] = "ratEq", [OP_RAT_LE] = "ratLe", [OP_RAT_LT] = "ratLt",
  [OP_SUM_RAT] = "sumRat", [OP_MATRIX] = "Matrix",
  [OP_MAT_TABULATE] = "matTabulate", [OP_MAT_OF_FN] = "matOfFn",
  [OP_MAT_ENTRY] = "matEntry", [OP_MAT_COMP] = "matComp", [OP_MAT_KRON] = "matKron",
  [OP_MAT_EQ] = "matEq", [OP_PROJ] = "?"
};

/* The number of arguments of each operation. VARIES: it comes from the
   family tables. */
static const unsigned char OP_ARITY[] = {
  [OP_NAT] = 0, [OP_FLAG] = 0, [OP_UNIT] = 0, [OP_PROD] = 2, [OP_SUM] = 2,
  [OP_OPTION] = 1, [OP_LIST] = 1, [OP_EQ] = 3, [OP_FAMILY] = VARIES,
  [OP_UNIT_VAL] = 0, [OP_FLAG_YES] = 0, [OP_FLAG_NO] = 0, [OP_PAIR] = 2,
  [OP_INL] = 1, [OP_INR] = 1, [OP_NONE] = 0, [OP_SOME] = 1, [OP_NIL] = 0,
  [OP_CONS] = 2, [OP_PACK] = 2, [OP_REFL] = 0, [OP_CTOR] = VARIES, [OP_FIRST] = 1,
  [OP_SECOND] = 1, [OP_EITHER] = 3, [OP_OPTION_ELIM] = 3, [OP_PURE] = 1,
  [OP_MAP] = 2, [OP_BIND] = 2, [OP_FOLD_NAT] = 3, [OP_FOLD_LIST] = 3,
  [OP_FOLD_FAMILY] = VARIES, [OP_UNFOLD] = 3, [OP_FILTER] = 2, [OP_WITNESS] = 1,
  [OP_PAYLOAD] = 1, [OP_SYMM] = 1, [OP_TRANS] = 2, [OP_TRANSPORT] = 3,
  [OP_CONG] = 2, [OP_NAT_ADD] = 2, [OP_NAT_SUB] = 2, [OP_NAT_MUL] = 2,
  [OP_NAT_EQ] = 2, [OP_NAT_LE] = 2, [OP_FLAG_IF] = 3, [OP_NAT_DIV] = 2,
  [OP_NAT_MOD] = 2, [OP_NAT_MAX] = 2, [OP_NAT_LT] = 2, [OP_FLAG_AND] = 2,
  [OP_FLAG_NOT] = 1, [OP_FIN] = 1, [OP_ALL_FIN] = 2, [OP_RAT] = 0,
  [OP_RAT_ADD] = 2, [OP_RAT_SUB] = 2, [OP_RAT_MUL] = 2, [OP_RAT_DIV] = 2,
  [OP_RAT_OF_NAT] = 1, [OP_RAT_EQ] = 2, [OP_RAT_LE] = 2, [OP_RAT_LT] = 2,
  [OP_SUM_RAT] = 2, [OP_MATRIX] = 2, [OP_MAT_TABULATE] = 3,
  [OP_MAT_OF_FN] = 3, [OP_MAT_ENTRY] = 3, [OP_MAT_COMP] = 5, [OP_MAT_KRON] = 6,
  [OP_MAT_EQ] = 2, [OP_PROJ] = 1
};

typedef enum {
  SCRUT_VALUE,
  SCRUT_TRAP,
  SCRUT_NEUTRAL
} Scrut;

typedef struct {
  const Value **items;
  uint32_t count;
  uint32_t cap;
} ValBuf;

/* A list value: its cons cells, then the end (nil, a trap or a neutral). */
typedef struct {
  const Value *const *cells;
  uint32_t count;
  const Value *tail;
} Spine;

static const Value *oom(Machine *m) {
  diag_fail(m->diag, "OOM", m->def, "the arena limit is reached");
  return NULL;
}

static const Value *internal(Machine *m, const char *what) {
  diag_fail(m->diag, "INTERNAL", m->def, "the evaluator found %s", what);
  return NULL;
}

static int spend(Machine *m, uint64_t steps) {
  if (m->fuel >= steps) {
    m->fuel -= steps;
    return 1;
  }
  m->fuel = 0;
  return diag_fail(m->diag, "EVAL_FUEL", m->def, "the evaluation takes more than %u steps", EVAL_FUEL_STEPS);
}

static int enter(Machine *m) {
  if (m->depth < EVAL_DEPTH_LIMIT) {
    m->depth++;
    return 1;
  }
  return diag_fail(m->diag, "EVAL_DEPTH", m->def, "the evaluation nests deeper than %u levels", EVAL_DEPTH_LIMIT);
}

static Value *new_value(Machine *m, ValKind kind) {
  Value *v = arena_alloc(m->arena, sizeof *v);
  if (v == NULL) {
    oom(m);
    return NULL;
  }
  v->kind = kind;
  return v;
}

const Value *val_nat(Machine *m, uint64_t n) {
  Value *v = new_value(m, VAL_NAT);
  if (v != NULL)
    v->nat = n;
  return v;
}

static const Value *val_rat(Machine *m, int64_t num, uint64_t den) {
  Value *v = new_value(m, VAL_RAT);
  if (v != NULL) {
    v->num = num;
    v->nat = den;
  }
  return v;
}

static const Value *val_matrix(Machine *m, uint32_t rows, uint32_t cols, const Cell *cells) {
  Value *v = new_value(m, VAL_MATRIX);
  if (v != NULL) {
    v->rows = rows;
    v->cols = cols;
    v->cells = cells;
  }
  return v;
}

static const Value *val_trap(Machine *m, uint64_t reason) {
  Value *v = new_value(m, VAL_TRAP);
  if (v != NULL)
    v->nat = reason;
  return v;
}

static const Value *overflow(Machine *m) {
  m->overflowed = 1;
  return val_trap(m, TRAP_OVERFLOW);
}

static const Value *div_zero(Machine *m) {
  return val_trap(m, TRAP_DIV_ZERO);
}

const char *trap_code(const Value *v) {
  switch ((TrapReason)v->nat) {
  case TRAP_OVERFLOW:
    return "EVAL_OVERFLOW";
  case TRAP_DIV_ZERO:
    return "EVAL_DIV_ZERO";
  case TRAP_STOCHASTIC:
    return "EVAL_STOCHASTIC";
  case TRAP_MATRIX_SIZE:
    return "EVAL_MATRIX_SIZE";
  }
  return "EVAL_OVERFLOW";
}

const char *trap_text(const Value *v) {
  switch ((TrapReason)v->nat) {
  case TRAP_OVERFLOW:
    break;
  case TRAP_DIV_ZERO:
    return "a division or modulo by zero";
  case TRAP_STOCHASTIC:
    return "a matrix row is not stochastic (an entry below 0 or a row sum other than 1)";
  case TRAP_MATRIX_SIZE:
    return "a matrix has more than 2^24 rows, columns or cells";
  }
  return "an operation overflowed (a Nat past 2^64-1, a Rat numerator past 2^63-1 or denominator past 2^64-1)";
}

const Value *val_var(Machine *m, uint32_t level) {
  Value *v = new_value(m, VAL_VAR);
  if (v != NULL)
    v->nat = level;
  return v;
}

const Value *val_univ(Machine *m, uint64_t level) {
  Value *v = new_value(m, VAL_UNIV);
  if (v != NULL)
    v->nat = level;
  return v;
}

const Value *val_op(Machine *m, ValKind kind, Op op, uint32_t inst, uint32_t field, const Value *const *args, uint32_t argc) {
  const Value **copy;
  Value *v;
  uint32_t i;
  for (i = 0; i < argc; i++)
    if (args[i] == NULL)
      return NULL;
  copy = arena_alloc(m->arena, ((size_t)argc + 1u) * sizeof *copy);
  if (copy == NULL)
    return oom(m);
  v = new_value(m, kind);
  if (v == NULL)
    return NULL;
  for (i = 0; i < argc; i++)
    copy[i] = args[i];
  v->op = op;
  v->inst = inst;
  v->field = field;
  v->args = copy;
  v->argc = argc;
  return v;
}

static const Value *make(Machine *m, ValKind kind, Op op, uint32_t inst, uint32_t argc, const Value *a, const Value *b, const Value *c) {
  const Value *args[3];
  args[0] = a;
  args[1] = b;
  args[2] = c;
  return val_op(m, kind, op, inst, 0, args, argc);
}

const Value *val_make(Machine *m, Op op, uint32_t inst, uint32_t argc, const Value *a, const Value *b, const Value *c) {
  return make(m, VAL_OP, op, inst, argc, a, b, c);
}

static const Value *constant(Machine *m, Op op) {
  return make(m, VAL_OP, op, 0, 0, NULL, NULL, NULL);
}

static const Value *flag(Machine *m, int yes) {
  return constant(m, yes ? OP_FLAG_YES : OP_FLAG_NO);
}

const Value *val_arrow(Machine *m, const Value *dom, const Value *cod) {
  Value *v;
  if (dom == NULL || cod == NULL)
    return NULL;
  v = new_value(m, VAL_PI);
  if (v == NULL)
    return NULL;
  v->dom = dom;
  v->arg = cod;
  return v;
}

const Value *val_closure(Machine *m, ValKind kind, const char *name, const Value *dom, const Env *env, const Core *body) {
  Value *v;
  if (body == NULL || (dom == NULL && kind != VAL_LAM))
    return NULL;
  v = new_value(m, kind);
  if (v == NULL)
    return NULL;
  v->name = name;
  v->dom = dom;
  v->env = env;
  v->body = body;
  return v;
}

int val_is(const Value *v, Op op) {
  return v != NULL && v->kind == VAL_OP && v->op == op;
}

const Env *env_push(Machine *m, const Env *env, const Value *value) {
  Env *e;
  if (value == NULL)
    return NULL;
  e = arena_alloc(m->arena, sizeof *e);
  if (e == NULL) {
    oom(m);
    return NULL;
  }
  e->value = value;
  e->next = env;
  return e;
}

static const Value *env_lookup(Machine *m, const Env *env, uint32_t index) {
  uint32_t i;
  for (i = 0; i < index && env != NULL; i++)
    env = env->next;
  if (env == NULL)
    return internal(m, "a variable outside its scope");
  return env->value;
}

const Value *def_value(Machine *m, uint32_t index) {
  DefInfo *d;
  if (index >= m->def_count)
    return internal(m, "an unknown definition");
  d = &m->defs[index];
  if (d->value == NULL)
    d->value = eval_core(m, NULL, d->body);
  return d->value;
}

static Scrut scrut_of(const Value *v) {
  switch (v->kind) {
  case VAL_TRAP:
    return SCRUT_TRAP;
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
    return SCRUT_NEUTRAL;
  case VAL_NAT:
  case VAL_RAT:
  case VAL_MATRIX:
  case VAL_UNIV:
  case VAL_LAM:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_OP:
    return SCRUT_VALUE;
  }
  return SCRUT_VALUE;
}

/* 1 when the scrutinee S stops the operation: a trap gives the trap, a
   neutral S gives the stuck operation. */
static int blocked(Machine *m, const Value *s, Op op, uint32_t inst, uint32_t field, const Value *const *a, uint32_t n, const Value **out) {
  switch (scrut_of(s)) {
  case SCRUT_TRAP:
    *out = s;
    return 1;
  case SCRUT_NEUTRAL:
    *out = val_op(m, VAL_STUCK, op, inst, field, a, n);
    return 1;
  case SCRUT_VALUE:
    return 0;
  }
  return 0;
}

static void buf_init(ValBuf *buf) {
  buf->items = NULL;
  buf->count = 0;
  buf->cap = 0;
}

static int buf_push(Machine *m, ValBuf *buf, const Value *v) {
  const Value **grown;
  uint32_t cap;
  uint32_t i;
  if (v == NULL)
    return 0;
  if (buf->count < buf->cap) {
    buf->items[buf->count++] = v;
    return 1;
  }
  cap = buf->cap == 0 ? 16u : buf->cap * 2u;
  grown = arena_alloc(m->arena, (size_t)cap * sizeof *grown);
  if (grown == NULL) {
    oom(m);
    return 0;
  }
  for (i = 0; i < buf->count; i++)
    grown[i] = buf->items[i];
  buf->items = grown;
  buf->cap = cap;
  buf->items[buf->count++] = v;
  return 1;
}

static int spine_of(Machine *m, const Value *xs, Spine *out) {
  ValBuf cells;
  buf_init(&cells);
  while (val_is(xs, OP_CONS)) {
    if (!buf_push(m, &cells, xs) || !spend(m, 1))
      return 0;
    xs = xs->args[1];
  }
  out->cells = cells.items;
  out->count = cells.count;
  out->tail = xs;
  return xs != NULL;
}

static const Value *build_list(Machine *m, const Value *const *items, uint32_t count, const Value *tail) {
  const Value *acc = tail;
  uint32_t i;
  for (i = count; i > 0 && acc != NULL; i--)
    acc = make(m, VAL_OP, OP_CONS, 0, 2, items[i - 1u], acc, NULL);
  return acc;
}

/* The end of a list result: nil stays nil, a trap stays a trap, a neutral
   end gives OP FIRST END. */
static const Value *list_tail(Machine *m, const Value *tail, Op op, uint32_t inst, const Value *first) {
  switch (scrut_of(tail)) {
  case SCRUT_TRAP:
    return tail;
  case SCRUT_NEUTRAL:
    return make(m, VAL_STUCK, op, inst, 2, first, tail, NULL);
  case SCRUT_VALUE:
    break;
  }
  return val_is(tail, OP_NIL) ? tail : internal(m, "a list that does not end in nil");
}

const Value *apply_value(Machine *m, const Value *fn, const Value *arg) {
  Value *v;
  if (fn == NULL || arg == NULL)
    return NULL;
  switch (fn->kind) {
  case VAL_LAM:
    return closure_apply(m, fn, arg);
  case VAL_TRAP:
    return fn;
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
    v = new_value(m, VAL_APP);
    if (v == NULL)
      return NULL;
    v->dom = fn;
    v->arg = arg;
    return v;
  case VAL_NAT:
  case VAL_RAT:
  case VAL_MATRIX:
  case VAL_UNIV:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_OP:
    break;
  }
  return internal(m, "an application of a value that is not a function");
}

const Value *closure_apply(Machine *m, const Value *binder, const Value *arg) {
  const Env *env;
  if (binder == NULL || arg == NULL)
    return NULL;
  if (binder->body == NULL)
    return binder->arg;
  env = env_push(m, binder->env, arg);
  if (env == NULL)
    return NULL;
  return eval_core(m, env, binder->body);
}

static const Value *reduce_part(Machine *m, Op op, const Value *const *a, uint32_t n, Op ctor, uint32_t index) {
  const Value *out;
  if (blocked(m, a[0], op, 0, 0, a, n, &out))
    return out;
  if (!val_is(a[0], ctor))
    return internal(m, "a projection of a value of the wrong type");
  return a[0]->args[index];
}

static const Value *reduce_either(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[2], OP_EITHER, 0, 0, a, n, &out))
    return out;
  if (val_is(a[2], OP_INL))
    return apply_value(m, a[0], a[2]->args[0]);
  if (val_is(a[2], OP_INR))
    return apply_value(m, a[1], a[2]->args[0]);
  return internal(m, "an either of a value that is not a Sum");
}

static const Value *reduce_option(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[2], OP_OPTION_ELIM, 0, 0, a, n, &out))
    return out;
  if (val_is(a[2], OP_NONE))
    return a[0];
  if (val_is(a[2], OP_SOME))
    return apply_value(m, a[1], a[2]->args[0]);
  return internal(m, "an option of a value that is not an Option");
}

static const Value *reduce_pure(Machine *m, uint32_t inst, const Value *x) {
  switch ((Carrier)inst) {
  case CARRIER_OPTION:
    return make(m, VAL_OP, OP_SOME, 0, 1, x, NULL, NULL);
  case CARRIER_LIST:
    return make(m, VAL_OP, OP_CONS, 0, 2, x, constant(m, OP_NIL), NULL);
  case CARRIER_SUM:
    return make(m, VAL_OP, OP_INR, 0, 1, x, NULL, NULL);
  }
  return internal(m, "an unknown carrier");
}

static const Value *map_list(Machine *m, const Value *f, const Value *xs) {
  Spine s;
  ValBuf out;
  uint32_t i;
  if (!spine_of(m, xs, &s))
    return NULL;
  buf_init(&out);
  for (i = 0; i < s.count; i++)
    if (!buf_push(m, &out, apply_value(m, f, s.cells[i]->args[0])))
      return NULL;
  return build_list(m, out.items, out.count, list_tail(m, s.tail, OP_MAP, CARRIER_LIST, f));
}

static const Value *reduce_map(Machine *m, uint32_t inst, const Value *const *a, uint32_t n) {
  const Value *x = a[1];
  const Value *out;
  if (blocked(m, x, OP_MAP, inst, 0, a, n, &out))
    return out;
  switch ((Carrier)inst) {
  case CARRIER_OPTION:
    return val_is(x, OP_SOME) ? make(m, VAL_OP, OP_SOME, 0, 1, apply_value(m, a[0], x->args[0]), NULL, NULL) : x;
  case CARRIER_SUM:
    return val_is(x, OP_INR) ? make(m, VAL_OP, OP_INR, 0, 1, apply_value(m, a[0], x->args[0]), NULL, NULL) : x;
  case CARRIER_LIST:
    return map_list(m, a[0], x);
  }
  return internal(m, "an unknown carrier");
}

static int buf_append(Machine *m, ValBuf *buf, const Spine *s) {
  uint32_t i;
  for (i = 0; i < s->count; i++)
    if (!buf_push(m, buf, s->cells[i]->args[0]))
      return 0;
  return 1;
}

/* A list bind concatenates the lists F X. A neutral end in any of them
   leaves the whole bind stuck. */
static const Value *bind_list(Machine *m, const Value *const *a, uint32_t n) {
  Spine s;
  Spine t;
  ValBuf out;
  uint32_t i;
  if (!spine_of(m, a[0], &s))
    return NULL;
  if (scrut_of(s.tail) == SCRUT_NEUTRAL)
    return val_op(m, VAL_STUCK, OP_BIND, CARRIER_LIST, 0, a, n);
  buf_init(&out);
  for (i = 0; i < s.count; i++) {
    if (!spine_of(m, apply_value(m, a[1], s.cells[i]->args[0]), &t) || !buf_append(m, &out, &t))
      return NULL;
    if (scrut_of(t.tail) == SCRUT_NEUTRAL)
      return val_op(m, VAL_STUCK, OP_BIND, CARRIER_LIST, 0, a, n);
    if (scrut_of(t.tail) == SCRUT_TRAP)
      return build_list(m, out.items, out.count, t.tail);
  }
  return build_list(m, out.items, out.count, list_tail(m, s.tail, OP_BIND, CARRIER_LIST, a[1]));
}

static const Value *reduce_bind(Machine *m, uint32_t inst, const Value *const *a, uint32_t n) {
  const Value *x = a[0];
  const Value *out;
  if (blocked(m, x, OP_BIND, inst, 0, a, n, &out))
    return out;
  switch ((Carrier)inst) {
  case CARRIER_OPTION:
    return val_is(x, OP_SOME) ? apply_value(m, a[1], x->args[0]) : x;
  case CARRIER_SUM:
    return val_is(x, OP_INR) ? apply_value(m, a[1], x->args[0]) : x;
  case CARRIER_LIST:
    return bind_list(m, a, n);
  }
  return internal(m, "an unknown carrier");
}

/* fold step z n: step applied n times to z. */
static const Value *reduce_fold_nat(Machine *m, const Value *const *a, uint32_t n) {
  const Value *acc = a[1];
  const Value *out;
  uint64_t k;
  if (blocked(m, a[2], OP_FOLD_NAT, 0, 0, a, n, &out))
    return out;
  if (a[2]->kind != VAL_NAT)
    return internal(m, "a fold of a value that is not a Nat");
  for (k = 0; k < a[2]->nat && acc != NULL; k++)
    acc = spend(m, 1) ? apply_value(m, a[0], acc) : NULL;
  return acc;
}

/* fold f z xs: the right fold. The end of the list is folded first. */
static const Value *reduce_fold_list(Machine *m, const Value *const *a) {
  const Value *acc = NULL;
  Spine s;
  uint32_t i;
  if (!spine_of(m, a[2], &s))
    return NULL;
  switch (scrut_of(s.tail)) {
  case SCRUT_TRAP:
    acc = s.tail;
    break;
  case SCRUT_NEUTRAL:
    acc = make(m, VAL_STUCK, OP_FOLD_LIST, 0, 3, a[0], a[1], s.tail);
    break;
  case SCRUT_VALUE:
    acc = val_is(s.tail, OP_NIL) ? a[1] : internal(m, "a list that does not end in nil");
    break;
  }
  for (i = s.count; i > 0 && acc != NULL; i--)
    acc = apply_value(m, apply_value(m, a[0], s.cells[i - 1u]->args[0]), acc);
  return acc;
}

static const Value *stuck_fold(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x) {
  const Value **args = arena_alloc(m->arena, ((size_t)ncases + 1u) * sizeof *args);
  uint32_t i;
  if (args == NULL)
    return oom(m);
  for (i = 0; i < ncases; i++)
    args[i] = cases[i];
  args[ncases] = x;
  return val_op(m, VAL_STUCK, OP_FOLD_FAMILY, family, 0, args, ncases + 1u);
}

/* fold case... x over a domain family: the case of the constructor of X,
   applied to its fields. A recursive field arrives folded. */
static const Value *fold_family(Machine *m, uint32_t family, const Value *const *cases, uint32_t ncases, const Value *x) {
  const FamilyInfo *fam;
  const CtorInfo *ctor;
  const Value *acc;
  const Value *field;
  uint32_t i;
  if (x == NULL)
    return NULL;
  switch (scrut_of(x)) {
  case SCRUT_TRAP:
    return x;
  case SCRUT_NEUTRAL:
    return stuck_fold(m, family, cases, ncases, x);
  case SCRUT_VALUE:
    break;
  }
  if (!val_is(x, OP_CTOR) || x->inst >= m->ctor_count || family >= m->family_count)
    return internal(m, "a fold of a value that is not in the family");
  fam = &m->families[family];
  ctor = &m->ctors[x->inst];
  if (x->inst < fam->first_ctor || x->inst - fam->first_ctor >= ncases || x->argc != fam->param_count + ctor->field_count)
    return internal(m, "a fold of a constructor of another family");
  if (!enter(m))
    return NULL;
  acc = cases[x->inst - fam->first_ctor];
  for (i = 0; i < ctor->field_count && acc != NULL; i++) {
    field = x->args[fam->param_count + i];
    acc = apply_value(m, acc, ctor->fields[i].recursive ? fold_family(m, family, cases, ncases, field) : field);
  }
  m->depth--;
  return acc;
}

/* 1 when the step result R ends the list. *TAIL is then the end. */
static int unfold_end(Machine *m, const Value *g, uint64_t left, const Value *seed, const Value *r, const Value **tail) {
  const Value *p = val_is(r, OP_SOME) ? r->args[0] : r;
  if (val_is(r, OP_NONE)) {
    *tail = constant(m, OP_NIL);
    return 1;
  }
  switch (scrut_of(p)) {
  case SCRUT_TRAP:
    *tail = p;
    return 1;
  case SCRUT_NEUTRAL:
    *tail = make(m, VAL_STUCK, OP_UNFOLD, 0, 3, g, val_nat(m, left), seed);
    return 1;
  case SCRUT_VALUE:
    break;
  }
  if (val_is(p, OP_PAIR))
    return 0;
  *tail = internal(m, "an unfold step that is not an Option of a pair");
  return 1;
}

/* unfold g limit seed: at most LIMIT elements. */
static const Value *reduce_unfold(Machine *m, const Value *const *a, uint32_t n) {
  const Value *seed = a[2];
  const Value *out;
  const Value *r;
  const Value *tail = NULL;
  ValBuf items;
  uint64_t i;
  if (blocked(m, a[1], OP_UNFOLD, 0, 0, a, n, &out))
    return out;
  if (a[1]->kind != VAL_NAT)
    return internal(m, "an unfold limit that is not a Nat");
  buf_init(&items);
  for (i = 0; i < a[1]->nat; i++) {
    r = spend(m, 1) ? apply_value(m, a[0], seed) : NULL;
    if (r == NULL)
      return NULL;
    if (unfold_end(m, a[0], a[1]->nat - i, seed, r, &tail))
      return build_list(m, items.items, items.count, tail);
    if (!buf_push(m, &items, r->args[0]->args[0]))
      return NULL;
    seed = r->args[0]->args[1];
  }
  return build_list(m, items.items, items.count, constant(m, OP_NIL));
}

static const Value *filter_option(Machine *m, const Value *const *a, uint32_t n) {
  const Value *test;
  const Value *out;
  if (!val_is(a[1], OP_SOME))
    return a[1];
  test = apply_value(m, a[0], a[1]->args[0]);
  if (test == NULL)
    return NULL;
  if (blocked(m, test, OP_FILTER, CARRIER_OPTION, 0, a, n, &out))
    return out;
  return val_is(test, OP_FLAG_YES) ? a[1] : constant(m, OP_NONE);
}

static const Value *filter_list(Machine *m, const Value *const *a) {
  const Value *test;
  Spine s;
  ValBuf kept;
  uint32_t i;
  if (!spine_of(m, a[1], &s))
    return NULL;
  buf_init(&kept);
  for (i = 0; i < s.count; i++) {
    test = apply_value(m, a[0], s.cells[i]->args[0]);
    if (test == NULL)
      return NULL;
    switch (scrut_of(test)) {
    case SCRUT_TRAP:
      return build_list(m, kept.items, kept.count, test);
    case SCRUT_NEUTRAL:
      return build_list(m, kept.items, kept.count, make(m, VAL_STUCK, OP_FILTER, CARRIER_LIST, 2, a[0], s.cells[i], NULL));
    case SCRUT_VALUE:
      break;
    }
    if (val_is(test, OP_FLAG_YES) && !buf_push(m, &kept, s.cells[i]->args[0]))
      return NULL;
  }
  return build_list(m, kept.items, kept.count, list_tail(m, s.tail, OP_FILTER, CARRIER_LIST, a[0]));
}

static const Value *reduce_filter(Machine *m, uint32_t inst, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[1], OP_FILTER, inst, 0, a, n, &out))
    return out;
  switch ((Carrier)inst) {
  case CARRIER_OPTION:
    return filter_option(m, a, n);
  case CARRIER_LIST:
    return filter_list(m, a);
  case CARRIER_SUM:
    break;
  }
  return internal(m, "a filter with no instance");
}

/* symm, trans, cong and transport compute when each proof argument is refl. */
static const Value *reduce_proof(Machine *m, Op op, const Value *const *a, uint32_t n, uint32_t first, uint32_t count, const Value *result) {
  const Value *out;
  uint32_t i;
  for (i = first; i < first + count; i++)
    if (blocked(m, a[i], op, 0, 0, a, n, &out))
      return out;
  return result;
}

/* 1 when both arguments are numbers. Else *OUT is the trap or the stuck
   operation. */
static int nat_args(Machine *m, Op op, const Value *const *a, uint32_t n, uint64_t *x, uint64_t *y, const Value **out) {
  *out = a[0]->kind == VAL_TRAP ? a[0] : a[1];
  if (a[0]->kind == VAL_TRAP || a[1]->kind == VAL_TRAP)
    return 0;
  if (blocked(m, a[0], op, 0, 0, a, n, out) || blocked(m, a[1], op, 0, 0, a, n, out))
    return 0;
  if (a[0]->kind != VAL_NAT || a[1]->kind != VAL_NAT) {
    *out = internal(m, "a Nat operation on a value that is not a Nat");
    return 0;
  }
  *x = a[0]->nat;
  *y = a[1]->nat;
  return 1;
}

static const Value *nat_add(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, OP_NAT_ADD, a, n, &x, &y, &out))
    return out;
  return x + y < x ? overflow(m) : val_nat(m, x + y);
}

static const Value *nat_sub(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, OP_NAT_SUB, a, n, &x, &y, &out))
    return out;
  return val_nat(m, x < y ? 0 : x - y);
}

static const Value *nat_mul(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, OP_NAT_MUL, a, n, &x, &y, &out))
    return out;
  return y != 0 && x > UINT64_MAX / y ? overflow(m) : val_nat(m, x * y);
}

/* natDiv, natMod. A division by zero traps (Lean gives 0). */
static const Value *nat_div(Machine *m, Op op, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, op, a, n, &x, &y, &out))
    return out;
  if (y == 0)
    return div_zero(m);
  return val_nat(m, op == OP_NAT_DIV ? x / y : x % y);
}

static const Value *nat_max(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, OP_NAT_MAX, a, n, &x, &y, &out))
    return out;
  return val_nat(m, x < y ? y : x);
}

static const Value *nat_test(Machine *m, Op op, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t x = 0;
  uint64_t y = 0;
  if (!nat_args(m, op, a, n, &x, &y, &out))
    return out;
  return flag(m, op == OP_NAT_EQ ? x == y : op == OP_NAT_LT ? x < y : x <= y);
}

/* Rat arithmetic. A Rat value is num (int64) over nat (uint64) in lowest
   terms: nat > 0, gcd(|num|, nat) = 1, zero is 0/1, and num is never
   INT64_MIN, so a negation never overflows. Products are exact in two
   uint64 halves (no __int128 under tcc). The gcds come out before the
   products, so only a result that does not fit traps (EVAL_OVERFLOW). */
typedef struct {
  uint64_t hi;
  uint64_t lo;
} Wide;

uint64_t gcd_u64(uint64_t a, uint64_t b) {
  while (b != 0) {
    uint64_t r = a % b;
    a = b;
    b = r;
  }
  return a;
}

static uint64_t magnitude(int64_t x) {
  return x < 0 ? 0u - (uint64_t)x : (uint64_t)x;
}

static Wide wide(uint64_t x) {
  Wide w;
  w.hi = 0;
  w.lo = x;
  return w;
}

static Wide wide_mul(uint64_t a, uint64_t b) {
  uint64_t a0 = a & 0xffffffffu;
  uint64_t a1 = a >> 32;
  uint64_t b0 = b & 0xffffffffu;
  uint64_t b1 = b >> 32;
  uint64_t p00 = a0 * b0;
  uint64_t p01 = a0 * b1;
  uint64_t p10 = a1 * b0;
  uint64_t mid = (p00 >> 32) + (p01 & 0xffffffffu) + (p10 & 0xffffffffu);
  Wide w;
  w.lo = (mid << 32) | (p00 & 0xffffffffu);
  w.hi = a1 * b1 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
  return w;
}

/* A + B; the callers keep the sum below 2^128. */
static Wide wide_add(Wide a, Wide b) {
  Wide w;
  w.lo = a.lo + b.lo;
  w.hi = a.hi + b.hi + (w.lo < a.lo);
  return w;
}

/* A - B for A >= B. */
static Wide wide_sub(Wide a, Wide b) {
  Wide w;
  w.lo = a.lo - b.lo;
  w.hi = a.hi - b.hi - (a.lo < b.lo);
  return w;
}

static int wide_cmp(Wide a, Wide b) {
  if (a.hi != b.hi)
    return a.hi < b.hi ? -1 : 1;
  return (a.lo > b.lo) - (a.lo < b.lo);
}

/* A / D by shift and subtract (D > 0); *REM gets A mod D. When the top bit
   of r is set, 2r + bit is at least 2^64 > D, and the wrapped r - D is the
   true remainder. */
static Wide wide_div(Wide a, uint64_t d, uint64_t *rem) {
  Wide q = wide(0);
  uint64_t r = 0;
  int i;
  for (i = 127; i >= 0; i--) {
    uint64_t bit = i >= 64 ? (a.hi >> (i - 64)) & 1u : (a.lo >> i) & 1u;
    uint64_t top = r >> 63;
    r = (r << 1) | bit;
    if (top != 0 || r >= d) {
      r -= d;
      q.hi |= i >= 64 ? (uint64_t)1 << (i - 64) : 0u;
      q.lo |= i >= 64 ? 0u : (uint64_t)1 << i;
    }
  }
  *rem = r;
  return q;
}

/* -MAG/DEN when NEG, else MAG/DEN, already in lowest terms, into OUT. Zero
   is 0/1. A numerator above INT64_MAX or a denominator above UINT64_MAX
   gives 0 (an overflow). These raw Rat helpers work on Cells, so matComp and
   matKron make no Value per step. */
static int cell_make(int neg, Wide mag, Wide den, Cell *out) {
  if (mag.hi == 0 && mag.lo == 0) {
    out->num = 0;
    out->den = 1;
    return 1;
  }
  if (mag.hi != 0 || mag.lo > (uint64_t)INT64_MAX || den.hi != 0)
    return 0;
  out->num = neg ? -(int64_t)mag.lo : (int64_t)mag.lo;
  out->den = den.lo;
  return 1;
}

/* X + Y, or X - Y when NEGATE. Knuth 4.5.1: d1 = gcd(b, d), t = a (d / d1)
   + c (b / d1), d2 = gcd(t mod d1, d1), result (t / d2) / ((b / d1) (d / d2)). */
static int cell_add(Cell x, Cell y, int negate, Cell *out) {
  uint64_t b = x.den;
  uint64_t d = y.den;
  uint64_t d1 = gcd_u64(b, d);
  int nx = x.num < 0;
  int ny = (y.num < 0) != (negate != 0);
  Wide p = wide_mul(magnitude(x.num), d / d1);
  Wide q = wide_mul(magnitude(y.num), b / d1);
  int swap = nx != ny && wide_cmp(p, q) < 0;
  Wide t = nx == ny ? wide_add(p, q) : swap ? wide_sub(q, p) : wide_sub(p, q);
  uint64_t r = 0;
  uint64_t d2;
  if (t.hi == 0 && t.lo == 0)
    return cell_make(0, wide(0), wide(1), out);
  wide_div(t, d1, &r);
  d2 = gcd_u64(r, d1);
  return cell_make(swap ? ny : nx, wide_div(t, d2, &r), wide_mul(b / d1, d / d2), out);
}

/* X * Y: (a / g1) (c / g2) over (b / g2) (d / g1), g1 = gcd(|a|, d) and
   g2 = gcd(|c|, b). */
static int cell_mul(Cell x, Cell y, Cell *out) {
  uint64_t a = magnitude(x.num);
  uint64_t c = magnitude(y.num);
  uint64_t g1 = gcd_u64(a, y.den);
  uint64_t g2 = gcd_u64(c, x.den);
  return cell_make((x.num < 0) != (y.num < 0), wide_mul(a / g1, c / g2), wide_mul(x.den / g2, y.den / g1), out);
}

static Cell cell_of(const Value *v) {
  Cell c;
  c.num = v->num;
  c.den = v->nat;
  return c;
}

/* The Rat value of a raw result; OK = 0 traps (EVAL_OVERFLOW). */
static const Value *val_cell(Machine *m, int ok, Cell c) {
  return ok ? val_rat(m, c.num, c.den) : overflow(m);
}

static const Value *rat_make(Machine *m, int neg, Wide mag, Wide den) {
  Cell r = {0, 1};
  int ok = cell_make(neg, mag, den, &r);
  return val_cell(m, ok, r);
}

static const Value *rat_add(Machine *m, const Value *x, const Value *y, int negate) {
  Cell r = {0, 1};
  int ok = cell_add(cell_of(x), cell_of(y), negate, &r);
  return val_cell(m, ok, r);
}

static const Value *rat_mul(Machine *m, const Value *x, const Value *y) {
  Cell r = {0, 1};
  int ok = cell_mul(cell_of(x), cell_of(y), &r);
  return val_cell(m, ok, r);
}

/* X / Y on the magnitudes, with no reciprocal: (a / g1) (d / g2) over
   (b / g2) (c / g1), g1 = gcd(|a|, |c|) and g2 = gcd(b, d). Y = 0 traps
   (EVAL_DIV_ZERO). */
static const Value *rat_div(Machine *m, const Value *x, const Value *y) {
  uint64_t a = magnitude(x->num);
  uint64_t c = magnitude(y->num);
  uint64_t g2 = gcd_u64(x->nat, y->nat);
  uint64_t g1;
  if (c == 0)
    return div_zero(m);
  g1 = gcd_u64(a, c);
  return rat_make(m, (x->num < 0) != (y->num < 0), wide_mul(a / g1, y->nat / g2), wide_mul(x->nat / g2, c / g1));
}

/* -1, 0 or 1 as X <, = or > Y: by the signs, then |a| d against |c| b. */
static int rat_cmp(const Value *x, const Value *y) {
  int nx = x->num < 0;
  int ny = y->num < 0;
  int mag = wide_cmp(wide_mul(magnitude(x->num), y->nat), wide_mul(magnitude(y->num), x->nat));
  if (nx != ny)
    return nx ? -1 : 1;
  return nx ? -mag : mag;
}

/* Strict in both arguments, as nat_args. */
static int rat_args(Machine *m, Op op, const Value *const *a, uint32_t n, const Value **out) {
  *out = a[0]->kind == VAL_TRAP ? a[0] : a[1];
  if (a[0]->kind == VAL_TRAP || a[1]->kind == VAL_TRAP)
    return 0;
  if (blocked(m, a[0], op, 0, 0, a, n, out) || blocked(m, a[1], op, 0, 0, a, n, out))
    return 0;
  if (a[0]->kind != VAL_RAT || a[1]->kind != VAL_RAT) {
    *out = internal(m, "a Rat operation on a value that is not a Rat");
    return 0;
  }
  return 1;
}

/* ratAdd, ratSub, ratMul, ratDiv. */
static const Value *rat_arith(Machine *m, Op op, const Value *const *a, uint32_t n) {
  const Value *out;
  if (!rat_args(m, op, a, n, &out))
    return out;
  if (op == OP_RAT_MUL)
    return rat_mul(m, a[0], a[1]);
  if (op == OP_RAT_DIV)
    return rat_div(m, a[0], a[1]);
  return rat_add(m, a[0], a[1], op == OP_RAT_SUB);
}

/* ratEq, ratLe, ratLt. */
static const Value *rat_test(Machine *m, Op op, const Value *const *a, uint32_t n) {
  const Value *out;
  int order;
  if (!rat_args(m, op, a, n, &out))
    return out;
  order = rat_cmp(a[0], a[1]);
  return flag(m, op == OP_RAT_EQ ? order == 0 : op == OP_RAT_LT ? order < 0 : order <= 0);
}

/* ratOfNat x = x / 1; x above INT64_MAX traps (EVAL_OVERFLOW). */
static const Value *rat_of_nat(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[0], OP_RAT_OF_NAT, 0, 0, a, n, &out))
    return out;
  if (a[0]->kind != VAL_NAT)
    return internal(m, "a ratOfNat of a value that is not a Nat");
  return a[0]->nat > (uint64_t)INT64_MAX ? overflow(m) : val_rat(m, (int64_t)a[0]->nat, 1);
}

/* sumRat n f = f 0 + (f 1 + (... + (f (n - 1) + 0))) as FinStoch.lean:122-124,
   in a C loop from i = n - 1 down to 0. Exact, so only the trap point
   depends on the order. The first trap stops it; a neutral f i gives the
   stuck sumRat. sumRat 0 f = 0/1. */
static const Value *reduce_sum_rat(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  const Value *acc;
  uint64_t i;
  if (blocked(m, a[0], OP_SUM_RAT, 0, 0, a, n, &out))
    return out;
  if (a[0]->kind != VAL_NAT)
    return internal(m, "a sumRat with a size that is not a Nat");
  acc = val_rat(m, 0, 1);
  for (i = a[0]->nat; i > 0 && acc != NULL && acc->kind == VAL_RAT; i--) {
    const Value *r = apply_value(m, a[1], val_nat(m, i - 1u));
    if (r == NULL || r->kind == VAL_TRAP)
      return r;
    if (r->kind != VAL_RAT)
      return scrut_of(r) == SCRUT_NEUTRAL ? val_op(m, VAL_STUCK, OP_SUM_RAT, 0, 0, a, n) : internal(m, "a sumRat term that is not a Rat");
    acc = rat_add(m, r, acc, 0);
  }
  return acc;
}

#define MATRIX_SIDE_MAX ((uint64_t)1 << 24)
#define MATRIX_CELLS_MAX ((uint64_t)1 << 24)

/* Strict in the first COUNT arguments, in order: a trap gives the trap, then
   a neutral gives the stuck OP in *OUT. 1 when all are values. */
static int mat_args(Machine *m, Op op, const Value *const *a, uint32_t n, uint32_t count, const Value **out) {
  uint32_t i;
  for (i = 0; i < count; i++) {
    if (a[i]->kind == VAL_TRAP) {
      *out = a[i];
      return 0;
    }
  }
  for (i = 0; i < count; i++)
    if (blocked(m, a[i], op, 0, 0, a, n, out))
      return 0;
  return 1;
}

/* The cells of a ROWS by COLS matrix, all 0/1. Past 2^24 rows, columns or
   cells it gives NULL and *TRAP gets EVAL_MATRIX_SIZE (D42); out of memory
   it gives NULL with *TRAP NULL. */
static Cell *mat_cells(Machine *m, uint64_t rows, uint64_t cols, const Value **trap) {
  Cell *cells;
  uint64_t i;
  *trap = NULL;
  if (rows > MATRIX_SIDE_MAX || cols > MATRIX_SIDE_MAX || rows * cols > MATRIX_CELLS_MAX) {
    *trap = val_trap(m, TRAP_MATRIX_SIZE);
    return NULL;
  }
  cells = arena_alloc(m->arena, (size_t)(rows * cols + 1u) * sizeof *cells);
  if (cells == NULL) {
    oom(m);
    return NULL;
  }
  for (i = 0; i < rows * cols; i++) {
    cells[i].num = 0;
    cells[i].den = 1;
  }
  return cells;
}

/* 1 when X and Y have the same size and the same cells (in lowest terms,
   so equal Rats have equal parts). */
static int cells_equal(const Value *x, const Value *y) {
  uint64_t count = (uint64_t)x->rows * x->cols;
  uint64_t i;
  if (x->rows != y->rows || x->cols != y->cols)
    return 0;
  for (i = 0; i < count; i++)
    if (x->cells[i].num != y->cells[i].num || x->cells[i].den != y->cells[i].den)
      return 0;
  return 1;
}

/* matTabulate m n f: f i j for each cell, row by row, in a C loop. The first
   trap stops it; a neutral cell gives the stuck matTabulate. Each row must
   be stochastic (D44): an entry below 0/1 or a row sum other than 1/1 traps
   (EVAL_STOCHASTIC). */
static const Value *reduce_mat_tabulate(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  Cell *cells;
  uint64_t i;
  uint64_t j;
  if (!mat_args(m, OP_MAT_TABULATE, a, n, 2, &out))
    return out;
  if (a[0]->kind != VAL_NAT || a[1]->kind != VAL_NAT)
    return internal(m, "a matTabulate with a size that is not a Nat");
  cells = mat_cells(m, a[0]->nat, a[1]->nat, &out);
  if (cells == NULL)
    return out;
  for (i = 0; i < a[0]->nat; i++) {
    const Value *row = apply_value(m, a[2], val_nat(m, i));
    Cell sum = {0, 1};
    if (row == NULL)
      return NULL;
    for (j = 0; j < a[1]->nat; j++) {
      const Value *r = apply_value(m, row, val_nat(m, j));
      Cell *cell = &cells[i * a[1]->nat + j];
      if (r == NULL || r->kind == VAL_TRAP)
        return r;
      if (r->kind != VAL_RAT)
        return scrut_of(r) == SCRUT_NEUTRAL ? val_op(m, VAL_STUCK, OP_MAT_TABULATE, 0, 0, a, n) : internal(m, "a matTabulate cell that is not a Rat");
      *cell = cell_of(r);
      if (cell->num < 0)
        return val_trap(m, TRAP_STOCHASTIC);
      if (!cell_add(sum, *cell, 0, &sum))
        return overflow(m);
    }
    if (sum.num != 1 || sum.den != 1)
      return val_trap(m, TRAP_STOCHASTIC);
  }
  return val_matrix(m, (uint32_t)a[0]->nat, (uint32_t)a[1]->nat, cells);
}

/* matOfFn m n f (detMatrix): row i is 1/1 at column f i and 0/1 elsewhere;
   m applications of f, not m * n. No stochastic check: each row has one 1/1. */
static const Value *reduce_mat_of_fn(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  Cell *cells;
  uint64_t i;
  if (!mat_args(m, OP_MAT_OF_FN, a, n, 2, &out))
    return out;
  if (a[0]->kind != VAL_NAT || a[1]->kind != VAL_NAT)
    return internal(m, "a matOfFn with a size that is not a Nat");
  cells = mat_cells(m, a[0]->nat, a[1]->nat, &out);
  if (cells == NULL)
    return out;
  for (i = 0; i < a[0]->nat; i++) {
    const Value *r = apply_value(m, a[2], val_nat(m, i));
    if (r == NULL || r->kind == VAL_TRAP)
      return r;
    if (r->kind != VAL_NAT)
      return scrut_of(r) == SCRUT_NEUTRAL ? val_op(m, VAL_STUCK, OP_MAT_OF_FN, 0, 0, a, n) : internal(m, "a matOfFn column that is not a Fin");
    if (r->nat >= a[1]->nat)
      return internal(m, "a matOfFn column past the size");
    cells[i * a[1]->nat + r->nat].num = 1;
  }
  return val_matrix(m, (uint32_t)a[0]->nat, (uint32_t)a[1]->nat, cells);
}

/* matEntry M i j: the cell (i, j). */
static const Value *reduce_mat_entry(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  const Cell *cell;
  if (!mat_args(m, OP_MAT_ENTRY, a, n, 3, &out))
    return out;
  if (a[0]->kind != VAL_MATRIX || a[1]->kind != VAL_NAT || a[2]->kind != VAL_NAT || a[1]->nat >= a[0]->rows || a[2]->nat >= a[0]->cols)
    return internal(m, "a matEntry outside the matrix");
  cell = &a[0]->cells[a[1]->nat * a[0]->cols + a[2]->nat];
  return val_rat(m, cell->num, cell->den);
}

/* matComp m k n M N: cell (i, j) is the sum over l of M (i, l) * N (l, j),
   on the stored cells, O(m k n), exact. The first overflow traps
   (EVAL_OVERFLOW). */
static const Value *reduce_mat_comp(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  const Value *x = a[3];
  const Value *y = a[4];
  Cell *cells;
  uint64_t i;
  uint64_t j;
  uint64_t l;
  if (!mat_args(m, OP_MAT_COMP, a, n, 5, &out))
    return out;
  if (x->kind != VAL_MATRIX || y->kind != VAL_MATRIX || x->cols != y->rows)
    return internal(m, "a matComp of matrices that do not compose");
  cells = mat_cells(m, x->rows, y->cols, &out);
  if (cells == NULL)
    return out;
  for (i = 0; i < x->rows; i++) {
    for (j = 0; j < y->cols; j++) {
      Cell sum = {0, 1};
      for (l = 0; l < x->cols; l++) {
        Cell p = {0, 1};
        if (!cell_mul(x->cells[i * x->cols + l], y->cells[l * y->cols + j], &p))
          return overflow(m);
        if (p.num != 0 && !cell_add(sum, p, 0, &sum))
          return overflow(m);
      }
      cells[i * y->cols + j] = sum;
    }
  }
  return val_matrix(m, x->rows, y->cols, cells);
}

/* matKron m k m' k' M N: cell (x, y) is M (x mod m, y mod k) * N (x div m,
   y div k); the first component is the low digit (D11, D12). */
static const Value *reduce_mat_kron(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  const Value *x = a[4];
  const Value *y = a[5];
  Cell *cells;
  uint64_t rows;
  uint64_t cols;
  uint64_t r;
  uint64_t c;
  if (!mat_args(m, OP_MAT_KRON, a, n, 6, &out))
    return out;
  if (x->kind != VAL_MATRIX || y->kind != VAL_MATRIX)
    return internal(m, "a matKron of a value that is not a Matrix");
  rows = (uint64_t)x->rows * y->rows;
  cols = (uint64_t)x->cols * y->cols;
  cells = mat_cells(m, rows, cols, &out);
  if (cells == NULL)
    return out;
  for (r = 0; r < rows; r++)
    for (c = 0; c < cols; c++)
      if (!cell_mul(x->cells[(r % x->rows) * x->cols + c % x->cols], y->cells[(r / x->rows) * y->cols + c / x->cols], &cells[r * cols + c]))
        return overflow(m);
  return val_matrix(m, (uint32_t)rows, (uint32_t)cols, cells);
}

/* matEq M N: flagYes when the sizes and all cells are equal. */
static const Value *reduce_mat_eq(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (!mat_args(m, OP_MAT_EQ, a, n, 2, &out))
    return out;
  if (a[0]->kind != VAL_MATRIX || a[1]->kind != VAL_MATRIX)
    return internal(m, "a matEq of a value that is not a Matrix");
  return flag(m, cells_equal(a[0], a[1]));
}

static const Value *reduce_flag_if(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[0], OP_FLAG_IF, 0, 0, a, n, &out))
    return out;
  if (val_is(a[0], OP_FLAG_YES))
    return a[1];
  if (val_is(a[0], OP_FLAG_NO))
    return a[2];
  return internal(m, "a flagIf of a value that is not a Flag");
}

/* flagAnd x y = flagIf x y flagNo: only x is strict, as Lean `&&`. */
static const Value *reduce_flag_and(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[0], OP_FLAG_AND, 0, 0, a, n, &out))
    return out;
  if (val_is(a[0], OP_FLAG_YES))
    return a[1];
  if (val_is(a[0], OP_FLAG_NO))
    return a[0];
  return internal(m, "a flagAnd of a value that is not a Flag");
}

static const Value *reduce_flag_not(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  if (blocked(m, a[0], OP_FLAG_NOT, 0, 0, a, n, &out))
    return out;
  if (val_is(a[0], OP_FLAG_YES) || val_is(a[0], OP_FLAG_NO))
    return flag(m, val_is(a[0], OP_FLAG_NO));
  return internal(m, "a flagNot of a value that is not a Flag");
}

/* allFin n f: f at 0, 1, ..., n - 1 in a C loop. The first flagNo or trap
   stops it; a neutral result gives the stuck allFin. allFin 0 f = flagYes. */
static const Value *reduce_all_fin(Machine *m, const Value *const *a, uint32_t n) {
  const Value *out;
  uint64_t i;
  if (blocked(m, a[0], OP_ALL_FIN, 0, 0, a, n, &out))
    return out;
  if (a[0]->kind != VAL_NAT)
    return internal(m, "an allFin with a size that is not a Nat");
  for (i = 0; i < a[0]->nat; i++) {
    const Value *r = apply_value(m, a[1], val_nat(m, i));
    if (r == NULL || r->kind == VAL_TRAP || val_is(r, OP_FLAG_NO))
      return r;
    if (!val_is(r, OP_FLAG_YES))
      return scrut_of(r) == SCRUT_NEUTRAL ? val_op(m, VAL_STUCK, OP_ALL_FIN, 0, 0, a, n) : internal(m, "an allFin test that is not a Flag");
  }
  return flag(m, 1);
}

static const Value *reduce_proj(Machine *m, uint32_t inst, uint32_t field, const Value *const *a, uint32_t n) {
  const Value *x = a[0];
  const Value *out;
  uint32_t at;
  if (blocked(m, x, OP_PROJ, inst, field, a, n, &out))
    return out;
  if (!val_is(x, OP_CTOR) || x->inst != inst || inst >= m->ctor_count || m->ctors[inst].family >= m->family_count)
    return internal(m, "a projection of a value of the wrong family");
  at = m->families[m->ctors[inst].family].param_count + field;
  return at < x->argc ? x->args[at] : internal(m, "a projection of a missing field");
}

static uint32_t varied_arity(const Machine *m, Op op, uint32_t inst) {
  const CtorInfo *c = inst < m->ctor_count ? &m->ctors[inst] : NULL;
  const FamilyInfo *f = inst < m->family_count ? &m->families[inst] : NULL;
  if (op == OP_CTOR && c != NULL && c->family < m->family_count)
    return m->families[c->family].param_count + c->field_count;
  if (op == OP_FAMILY && f != NULL)
    return f->param_count;
  if (op == OP_FOLD_FAMILY && f != NULL)
    return f->ctor_count + 1u;
  return UINT32_MAX;
}

static int arity_ok(const Machine *m, Op op, uint32_t inst, uint32_t n) {
  if ((size_t)op >= sizeof OP_ARITY)
    return 0;
  if (OP_ARITY[op] != VARIES)
    return n == (uint32_t)OP_ARITY[op];
  return n == varied_arity(m, op, inst);
}

static const Value *reduce_op(Machine *m, Op op, uint32_t inst, uint32_t field, const Value *const *a, uint32_t n) {
  if (!arity_ok(m, op, inst, n))
    return internal(m, "an operation with the wrong number of arguments");
  switch (op) {
  case OP_NAT:
  case OP_FLAG:
  case OP_UNIT:
  case OP_PROD:
  case OP_SUM:
  case OP_OPTION:
  case OP_LIST:
  case OP_EQ:
  case OP_FAMILY:
  case OP_UNIT_VAL:
  case OP_FLAG_YES:
  case OP_FLAG_NO:
  case OP_PAIR:
  case OP_INL:
  case OP_INR:
  case OP_NONE:
  case OP_SOME:
  case OP_NIL:
  case OP_CONS:
  case OP_PACK:
  case OP_REFL:
  case OP_CTOR:
  case OP_FIN:
  case OP_RAT:
  case OP_MATRIX:
    return val_op(m, VAL_OP, op, inst, field, a, n);
  case OP_FIRST:
    return reduce_part(m, op, a, n, OP_PAIR, 0);
  case OP_SECOND:
    return reduce_part(m, op, a, n, OP_PAIR, 1);
  case OP_WITNESS:
    return reduce_part(m, op, a, n, OP_PACK, 0);
  case OP_PAYLOAD:
    return reduce_part(m, op, a, n, OP_PACK, 1);
  case OP_EITHER:
    return reduce_either(m, a, n);
  case OP_OPTION_ELIM:
    return reduce_option(m, a, n);
  case OP_PURE:
    return reduce_pure(m, inst, a[0]);
  case OP_MAP:
    return reduce_map(m, inst, a, n);
  case OP_BIND:
    return reduce_bind(m, inst, a, n);
  case OP_FOLD_NAT:
    return reduce_fold_nat(m, a, n);
  case OP_FOLD_LIST:
    return reduce_fold_list(m, a);
  case OP_FOLD_FAMILY:
    return fold_family(m, inst, a, n - 1u, a[n - 1u]);
  case OP_UNFOLD:
    return reduce_unfold(m, a, n);
  case OP_FILTER:
    return reduce_filter(m, inst, a, n);
  case OP_SYMM:
    return reduce_proof(m, op, a, n, 0, 1, constant(m, OP_REFL));
  case OP_TRANS:
    return reduce_proof(m, op, a, n, 0, 2, constant(m, OP_REFL));
  case OP_CONG:
    return reduce_proof(m, op, a, n, 1, 1, constant(m, OP_REFL));
  case OP_TRANSPORT:
    return reduce_proof(m, op, a, n, 1, 1, a[2]);
  case OP_NAT_ADD:
    return nat_add(m, a, n);
  case OP_NAT_SUB:
    return nat_sub(m, a, n);
  case OP_NAT_MUL:
    return nat_mul(m, a, n);
  case OP_NAT_EQ:
  case OP_NAT_LE:
  case OP_NAT_LT:
    return nat_test(m, op, a, n);
  case OP_FLAG_IF:
    return reduce_flag_if(m, a, n);
  case OP_NAT_DIV:
  case OP_NAT_MOD:
    return nat_div(m, op, a, n);
  case OP_NAT_MAX:
    return nat_max(m, a, n);
  case OP_FLAG_AND:
    return reduce_flag_and(m, a, n);
  case OP_FLAG_NOT:
    return reduce_flag_not(m, a, n);
  case OP_ALL_FIN:
    return reduce_all_fin(m, a, n);
  case OP_RAT_ADD:
  case OP_RAT_SUB:
  case OP_RAT_MUL:
  case OP_RAT_DIV:
    return rat_arith(m, op, a, n);
  case OP_RAT_OF_NAT:
    return rat_of_nat(m, a, n);
  case OP_RAT_EQ:
  case OP_RAT_LE:
  case OP_RAT_LT:
    return rat_test(m, op, a, n);
  case OP_SUM_RAT:
    return reduce_sum_rat(m, a, n);
  case OP_MAT_TABULATE:
    return reduce_mat_tabulate(m, a, n);
  case OP_MAT_OF_FN:
    return reduce_mat_of_fn(m, a, n);
  case OP_MAT_ENTRY:
    return reduce_mat_entry(m, a, n);
  case OP_MAT_COMP:
    return reduce_mat_comp(m, a, n);
  case OP_MAT_KRON:
    return reduce_mat_kron(m, a, n);
  case OP_MAT_EQ:
    return reduce_mat_eq(m, a, n);
  case OP_PROJ:
    return reduce_proj(m, inst, field, a, n);
  }
  return internal(m, "an unknown operation");
}

static const Value *eval_op(Machine *m, const Env *env, const Core *c) {
  const Value **args = arena_alloc(m->arena, ((size_t)c->argc + 1u) * sizeof *args);
  uint32_t i;
  if (args == NULL)
    return oom(m);
  for (i = 0; i < c->argc; i++) {
    args[i] = eval_core(m, env, c->args[i]);
    if (args[i] == NULL)
      return NULL;
  }
  return reduce_op(m, c->op, c->inst, c->field, args, c->argc);
}

static const Value *eval_inner(Machine *m, const Env *env, const Core *c) {
  switch (c->kind) {
  case CORE_VAR:
    return env_lookup(m, env, c->index);
  case CORE_GLOBAL:
    return def_value(m, c->index);
  case CORE_NAT:
    return val_nat(m, c->nat);
  case CORE_RAT:
    return val_rat(m, c->num, c->nat);
  case CORE_MATRIX:
    return val_matrix(m, c->rows, c->cols, c->cells);
  case CORE_TRAP:
    return val_trap(m, c->nat);
  case CORE_UNIV:
    return val_univ(m, c->nat);
  case CORE_LAM:
    return val_closure(m, VAL_LAM, c->name, NULL, env, c->b);
  case CORE_PI:
    return val_closure(m, VAL_PI, c->name, eval_core(m, env, c->a), env, c->b);
  case CORE_SIGMA:
    return val_closure(m, VAL_SIGMA, c->name, eval_core(m, env, c->a), env, c->b);
  case CORE_APP:
    return apply_value(m, eval_core(m, env, c->a), eval_core(m, env, c->b));
  case CORE_OP:
    return eval_op(m, env, c);
  }
  return internal(m, "an unknown core term");
}

const Value *eval_core(Machine *m, const Env *env, const Core *c) {
  const Value *v;
  if (c == NULL || !spend(m, 1) || !enter(m))
    return NULL;
  v = eval_inner(m, env, c);
  m->depth--;
  return v;
}

/* Eta: a function is equal to G when the two agree on a fresh variable. */
static int conv_fun(Machine *m, uint32_t level, const Value *a, const Value *b) {
  const Value *x = val_var(m, level);
  return conv_values(m, level + 1u, apply_value(m, a, x), apply_value(m, b, x));
}

static int conv_binder(Machine *m, uint32_t level, const Value *a, const Value *b) {
  const Value *x = val_var(m, level);
  return conv_values(m, level, a->dom, b->dom) && conv_values(m, level + 1u, closure_apply(m, a, x), closure_apply(m, b, x));
}

static int same_head(const Value *a, const Value *b) {
  return a->op == b->op && a->inst == b->inst && a->field == b->field && a->argc == b->argc;
}

/* The last argument of an operation is compared in the loop, so a long
   list does not nest. */
static int conv_loop(Machine *m, uint32_t level, const Value *a, const Value *b) {
  uint32_t i;
  while (a != NULL && b != NULL && spend(m, 1)) {
    if (a->kind == VAL_LAM || b->kind == VAL_LAM)
      return conv_fun(m, level, a, b);
    if (a->kind != b->kind)
      return 0;
    switch (a->kind) {
    case VAL_NAT:
    case VAL_UNIV:
    case VAL_VAR:
      return a->nat == b->nat;
    case VAL_RAT:
      return a->num == b->num && a->nat == b->nat;
    case VAL_MATRIX:
      return cells_equal(a, b);
    case VAL_TRAP:
      return 1;
    case VAL_LAM:
      return 0;
    case VAL_PI:
    case VAL_SIGMA:
      return conv_binder(m, level, a, b);
    case VAL_APP:
      if (!conv_values(m, level, a->dom, b->dom))
        return 0;
      a = a->arg;
      b = b->arg;
      break;
    case VAL_OP:
    case VAL_STUCK:
      if (!same_head(a, b))
        return 0;
      if (a->argc == 0)
        return 1;
      for (i = 0; i + 1u < a->argc; i++)
        if (!conv_values(m, level, a->args[i], b->args[i]))
          return 0;
      a = a->args[a->argc - 1u];
      b = b->args[b->argc - 1u];
      break;
    }
  }
  return 0;
}

int conv_values(Machine *m, uint32_t level, const Value *a, const Value *b) {
  int same;
  if (a == NULL || b == NULL || !enter(m))
    return 0;
  same = conv_loop(m, level, a, b);
  m->depth--;
  return same;
}

static Core *new_core(Machine *m, CoreKind kind) {
  Core *c = arena_alloc(m->arena, sizeof *c);
  if (c == NULL) {
    oom(m);
    return NULL;
  }
  c->kind = kind;
  return c;
}

static const Core *core_leaf(Machine *m, CoreKind kind, uint64_t nat) {
  Core *c = new_core(m, kind);
  if (c != NULL)
    c->nat = nat;
  return c;
}

static const Core *core_rat(Machine *m, int64_t num, uint64_t den) {
  Core *c = new_core(m, CORE_RAT);
  if (c != NULL) {
    c->num = num;
    c->nat = den;
  }
  return c;
}

static const Core *core_matrix(Machine *m, const Value *v) {
  Core *c = new_core(m, CORE_MATRIX);
  if (c != NULL) {
    c->rows = v->rows;
    c->cols = v->cols;
    c->cells = v->cells;
  }
  return c;
}

static const Core *quote_var(Machine *m, uint32_t level, const Value *v) {
  Core *c;
  if (v->nat >= level) {
    internal(m, "a variable above its level");
    return NULL;
  }
  c = new_core(m, CORE_VAR);
  if (c != NULL)
    c->index = level - 1u - (uint32_t)v->nat;
  return c;
}

static const Core *quote_app(Machine *m, uint32_t level, const Value *v) {
  const Core *fn = quote_value(m, level, v->dom);
  const Core *arg = quote_value(m, level, v->arg);
  Core *c;
  if (fn == NULL || arg == NULL)
    return NULL;
  c = new_core(m, CORE_APP);
  if (c == NULL)
    return NULL;
  c->a = fn;
  c->b = arg;
  return c;
}

static const Core *quote_binder(Machine *m, uint32_t level, const Value *v, CoreKind kind) {
  const Core *dom = kind == CORE_LAM ? NULL : quote_value(m, level, v->dom);
  const Core *body = quote_value(m, level + 1u, closure_apply(m, v, val_var(m, level)));
  Core *c;
  if (body == NULL || (dom == NULL && kind != CORE_LAM))
    return NULL;
  c = new_core(m, kind);
  if (c == NULL)
    return NULL;
  c->name = v->name;
  c->a = dom;
  c->b = body;
  return c;
}

static const Core *quote_op(Machine *m, uint32_t level, const Value *v) {
  const Core **args = arena_alloc(m->arena, ((size_t)v->argc + 1u) * sizeof *args);
  Core *c;
  uint32_t i;
  if (args == NULL) {
    oom(m);
    return NULL;
  }
  for (i = 0; i < v->argc; i++) {
    args[i] = quote_value(m, level, v->args[i]);
    if (args[i] == NULL)
      return NULL;
  }
  c = new_core(m, CORE_OP);
  if (c == NULL)
    return NULL;
  c->op = v->op;
  c->inst = v->inst;
  c->field = v->field;
  c->args = args;
  c->argc = v->argc;
  return c;
}

static const Core *quote_inner(Machine *m, uint32_t level, const Value *v) {
  switch (v->kind) {
  case VAL_NAT:
    return core_leaf(m, CORE_NAT, v->nat);
  case VAL_RAT:
    return core_rat(m, v->num, v->nat);
  case VAL_MATRIX:
    return core_matrix(m, v);
  case VAL_TRAP:
    return core_leaf(m, CORE_TRAP, v->nat);
  case VAL_UNIV:
    return core_leaf(m, CORE_UNIV, v->nat);
  case VAL_VAR:
    return quote_var(m, level, v);
  case VAL_APP:
    return quote_app(m, level, v);
  case VAL_LAM:
    return quote_binder(m, level, v, CORE_LAM);
  case VAL_PI:
    return quote_binder(m, level, v, CORE_PI);
  case VAL_SIGMA:
    return quote_binder(m, level, v, CORE_SIGMA);
  case VAL_OP:
  case VAL_STUCK:
    return quote_op(m, level, v);
  }
  internal(m, "an unknown value");
  return NULL;
}

const Core *quote_value(Machine *m, uint32_t level, const Value *v) {
  const Core *c;
  if (v == NULL || !enter(m))
    return NULL;
  c = quote_inner(m, level, v);
  m->depth--;
  return c;
}

const char *op_name(const Machine *m, Op op, uint32_t inst, uint32_t field) {
  if (op == OP_FAMILY)
    return inst < m->family_count ? m->families[inst].name : "?";
  if (op == OP_CTOR)
    return inst < m->ctor_count ? m->ctors[inst].name : "?";
  if (op == OP_PROJ)
    return inst < m->ctor_count && field < m->ctors[inst].field_count ? m->ctors[inst].fields[field].name : "?";
  return (size_t)op < sizeof OP_NAMES / sizeof OP_NAMES[0] ? OP_NAMES[op] : "?";
}

/* The printer keeps len + 4 <= cap, so "..." always fits. */
typedef struct {
  Machine *m;
  char *buf;
  size_t cap;
  size_t len;
  int full;
  const char *const *names;
  uint32_t name_count;
  const char *extra[PRINT_NAMES_MAX];
  uint32_t extra_count;
  unsigned depth;
} Printer;

static void put(Printer *p, const char *s) {
  size_t n = strlen(s);
  if (p->full)
    return;
  if (p->len + n + 4u <= p->cap) {
    memcpy(p->buf + p->len, s, n + 1u);
    p->len += n;
    return;
  }
  memcpy(p->buf + p->len, "...", 4u);
  p->len += 3u;
  p->full = 1;
}

static const char *var_name(const Printer *p, uint64_t level) {
  if (level < p->name_count && p->names[level] != NULL)
    return p->names[level];
  if (level >= p->name_count && level - p->name_count < p->extra_count)
    return p->extra[level - p->name_count];
  return "_";
}

static void print_value(Printer *p, const Value *v, int atom);

static int is_chain(const Value *v) {
  return v != NULL && (v->kind == VAL_OP || v->kind == VAL_STUCK) && v->argc > 0;
}

static void print_binder(Printer *p, const Value *v) {
  uint32_t level = p->name_count + p->extra_count;
  const char *name = v->name != NULL ? v->name : "x";
  const Value *body = closure_apply(p->m, v, val_var(p->m, level));
  int binder_dom = v->dom != NULL && (v->dom->kind == VAL_PI || v->dom->kind == VAL_SIGMA || v->dom->kind == VAL_LAM || v->dom->kind == VAL_UNIV);
  switch (v->kind) {
  case VAL_LAM:
    put(p, "fun ");
    put(p, name);
    put(p, " => ");
    break;
  case VAL_PI:
    if (v->name == NULL) {
      print_value(p, v->dom, binder_dom);
      put(p, " -> ");
      break;
    }
    put(p, "(");
    put(p, name);
    put(p, " : ");
    print_value(p, v->dom, 0);
    put(p, ") -> ");
    break;
  case VAL_SIGMA:
    put(p, "Sigma (");
    put(p, name);
    put(p, " : ");
    print_value(p, v->dom, 0);
    put(p, ") ");
    break;
  case VAL_NAT:
  case VAL_RAT:
  case VAL_MATRIX:
  case VAL_TRAP:
  case VAL_UNIV:
  case VAL_OP:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
    break;
  }
  if (p->extra_count < PRINT_NAMES_MAX)
    p->extra[p->extra_count] = name;
  p->extra_count++;
  print_value(p, body, v->kind == VAL_SIGMA);
  p->extra_count--;
}

/* [[1/1, 0/1], [0/1, 1/1]] (D46); 0 rows is []. */
static void print_matrix(Printer *p, const Value *v) {
  char num[64];
  uint64_t i;
  uint64_t j;
  put(p, "[");
  for (i = 0; i < v->rows && !p->full; i++) {
    put(p, i == 0 ? "[" : ", [");
    for (j = 0; j < v->cols && !p->full; j++) {
      const Cell *cell = &v->cells[i * v->cols + j];
      snprintf(num, sizeof num, "%s%lld/%llu", j == 0 ? "" : ", ", (long long)cell->num, (unsigned long long)cell->den);
      put(p, num);
    }
    put(p, "]");
  }
  put(p, "]");
}

static void print_other(Printer *p, const Value *v, int atom) {
  char num[48];
  if (v == NULL) {
    put(p, "?");
    return;
  }
  switch (v->kind) {
  case VAL_NAT:
    snprintf(num, sizeof num, "%llu", (unsigned long long)v->nat);
    put(p, num);
    return;
  case VAL_RAT:
    snprintf(num, sizeof num, "%lld/%llu", (long long)v->num, (unsigned long long)v->nat);
    put(p, num);
    return;
  case VAL_MATRIX:
    print_matrix(p, v);
    return;
  case VAL_TRAP:
    put(p, "trap");
    return;
  case VAL_VAR:
    put(p, var_name(p, v->nat));
    return;
  case VAL_OP:
  case VAL_STUCK:
    put(p, op_name(p->m, v->op, v->inst, v->field));
    return;
  case VAL_UNIV:
  case VAL_APP:
  case VAL_LAM:
  case VAL_PI:
  case VAL_SIGMA:
    break;
  }
  if (atom)
    put(p, "(");
  switch (v->kind) {
  case VAL_UNIV:
    snprintf(num, sizeof num, "Type %llu", (unsigned long long)v->nat);
    put(p, num);
    break;
  case VAL_APP:
    print_value(p, v->dom, v->dom->kind != VAL_APP && v->dom->kind != VAL_VAR);
    put(p, " ");
    print_value(p, v->arg, 1);
    break;
  case VAL_LAM:
  case VAL_PI:
  case VAL_SIGMA:
    print_binder(p, v);
    break;
  case VAL_NAT:
  case VAL_RAT:
  case VAL_MATRIX:
  case VAL_TRAP:
  case VAL_VAR:
  case VAL_OP:
  case VAL_STUCK:
    break;
  }
  if (atom)
    put(p, ")");
}

/* An operation prints as its name and its arguments. The last argument is
   printed in the loop, so a long list does not nest. */
static void print_value(Printer *p, const Value *v, int atom) {
  uint32_t open = 0;
  uint32_t i;
  if (p->depth >= PRINT_DEPTH_MAX) {
    put(p, "...");
    return;
  }
  p->depth++;
  while (is_chain(v) && !p->full) {
    if (atom) {
      put(p, "(");
      open++;
    }
    put(p, op_name(p->m, v->op, v->inst, v->field));
    for (i = 0; i + 1u < v->argc; i++) {
      put(p, " ");
      print_value(p, v->args[i], 1);
    }
    put(p, " ");
    v = v->args[v->argc - 1u];
    atom = 1;
  }
  print_other(p, v, atom);
  for (i = 0; i < open; i++)
    put(p, ")");
  p->depth--;
}

void value_print(Machine *m, const char *const *names, uint32_t name_count, const Value *v, char *buf, size_t cap) {
  Printer p;
  if (cap == 0)
    return;
  buf[0] = '\0';
  if (cap < 4u)
    return;
  memset(&p, 0, sizeof p);
  p.m = m;
  p.buf = buf;
  p.cap = cap;
  p.names = names;
  p.name_count = name_count;
  print_value(&p, v, 0);
}
