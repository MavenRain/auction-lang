/* The normal form of the checked rules ring, ringNat, field, cert and certLt
   (M5 K3b, m5-design.md 3.5). SHARED by the tcc kits.

   A term is a rational function N / D of sparse polynomials with Rat
   coefficients. An atom is a maximal subterm that is not +, -, *, /, a
   literal or ratOfNat. conv_values identifies two atoms. ratOfNat goes
   through natAdd and natMul. natSub, natDiv, natMod, natMax and natMin are
   not ring operations, so they stay atoms. A division by a term that is not
   a literal is an atom, except in field: there the hypotheses must give
   Ne Rat d 0/1 for the divisor d. Equal normal forms give a commutative ring
   identity (a semiring identity for Nat), so an accepted rule is sound.
   Each failure returns 0 and keeps the first verdict. */
#include <stdlib.h>
#include <string.h>

#include "front/nf.h"

#define NF_ATOMS_MAX 64u
#define NF_TERMS_MAX 1024u
#define NF_DEGREE_MAX 32u
#define NF_HYPS_MAX 32u
/* Each numerator and denominator stays at or below 2^61, so the sum of two
   never overflows int64_t. */
#define NF_LIMIT ((uint64_t)1 << 61)

typedef struct {
  int64_t num;
  int64_t den; /* > 0, in lowest terms */
} Q;

typedef struct {
  uint32_t atom;
  uint32_t exp;
} Factor;

/* coef times the product of the factors, in atom order. */
typedef struct {
  Q coef;
  uint32_t count;
  const Factor *f;
} Mono;

/* A sum of monomials in mono_cmp order, with no zero coefficient. */
typedef struct {
  uint32_t count;
  Mono *t;
} Poly;

typedef struct {
  Poly n;
  Poly d;
} Frac;

typedef enum {
  SORT_RAT,
  SORT_NAT
} Sort;

typedef enum {
  K_ADD,
  K_SUB,
  K_MUL,
  K_DIV,
  K_CAST,
  K_LIT,
  K_ATOM,
  K_TRAP
} Kind;

typedef struct {
  Machine *m;
  uint32_t level;
  NfRule rule;
  const Value *atom[NF_ATOMS_MAX];
  Sort atom_sort[NF_ATOMS_MAX];
  uint32_t atoms;
  const Value *nonzero[NF_HYPS_MAX]; /* field: the divisors with a proof */
  uint32_t nonzeros;
  NfVerdict fail;
  const Value *at;
} Nf;

static int nf_fail(Nf *n, NfVerdict verdict, const Value *at) {
  if (n->fail == NF_OK) {
    n->fail = verdict;
    n->at = at;
  }
  return 0;
}

static uint64_t mag(int64_t x) {
  return x < 0 ? (uint64_t)0 - (uint64_t)x : (uint64_t)x;
}

static int i_mul(Nf *n, int64_t a, int64_t b, int64_t *out) {
  if (mag(a) != 0u && mag(b) > NF_LIMIT / mag(a))
    return nf_fail(n, NF_LARGE, NULL);
  *out = a * b;
  return 1;
}

static int i_add(Nf *n, int64_t a, int64_t b, int64_t *out) {
  if (mag(a + b) > NF_LIMIT)
    return nf_fail(n, NF_LARGE, NULL);
  *out = a + b;
  return 1;
}

static Q q_make(int64_t num, int64_t den) {
  Q q;
  uint64_t g = gcd_u64(mag(num), mag(den));
  int64_t s = den < 0 ? -1 : 1;
  q.num = num == 0 || g == 0u ? 0 : s * num / (int64_t)g;
  q.den = num == 0 || g == 0u ? 1 : s * den / (int64_t)g;
  return q;
}

static int q_add(Nf *n, Q a, Q b, Q *out) {
  int64_t x;
  int64_t y;
  int64_t s;
  int64_t d;
  if (!i_mul(n, a.num, b.den, &x) || !i_mul(n, b.num, a.den, &y) || !i_add(n, x, y, &s) || !i_mul(n, a.den, b.den, &d))
    return 0;
  *out = q_make(s, d);
  return 1;
}

static int q_mul(Nf *n, Q a, Q b, Q *out) {
  int64_t x;
  int64_t d;
  if (!i_mul(n, a.num, b.num, &x) || !i_mul(n, a.den, b.den, &d))
    return 0;
  *out = q_make(x, d);
  return 1;
}

static int mono_cmp(const Mono *a, const Mono *b) {
  uint32_t i;
  if (a->count != b->count)
    return a->count < b->count ? -1 : 1;
  for (i = 0; i < a->count; i++) {
    if (a->f[i].atom != b->f[i].atom)
      return a->f[i].atom < b->f[i].atom ? -1 : 1;
    if (a->f[i].exp != b->f[i].exp)
      return a->f[i].exp < b->f[i].exp ? -1 : 1;
  }
  return 0;
}

static int mono_order(const void *a, const void *b) {
  return mono_cmp(a, b);
}

static Mono *terms(Nf *n, uint64_t count) {
  Mono *t = count > NF_TERMS_MAX * 4u ? NULL : arena_alloc(n->m->arena, ((size_t)count + 1u) * sizeof *t);
  if (t == NULL)
    nf_fail(n, count > NF_TERMS_MAX * 4u ? NF_LARGE : NF_OOM, NULL);
  return t;
}

/* Sorts T, adds the coefficients of equal monomials and drops each zero. */
static int poly_norm(Nf *n, Mono *t, uint32_t count, Poly *out) {
  uint32_t i;
  uint32_t k = 0;
  uint32_t j = 0;
  if (count > 1u)
    qsort(t, count, sizeof *t, mono_order);
  for (i = 0; i < count; i++) {
    if (k > 0u && mono_cmp(&t[k - 1u], &t[i]) == 0) {
      if (!q_add(n, t[k - 1u].coef, t[i].coef, &t[k - 1u].coef))
        return 0;
    } else {
      t[k++] = t[i];
    }
  }
  for (i = 0; i < k; i++) {
    if (t[i].coef.num != 0)
      t[j++] = t[i];
  }
  if (j > NF_TERMS_MAX)
    return nf_fail(n, NF_LARGE, NULL);
  out->count = j;
  out->t = t;
  return 1;
}

static int poly_const(Nf *n, Q q, Poly *out) {
  Mono *t = terms(n, 1u);
  if (t == NULL)
    return 0;
  t[0].coef = q;
  t[0].count = 0;
  t[0].f = NULL;
  return poly_norm(n, t, 1u, out);
}

static int poly_atom(Nf *n, uint32_t atom, Poly *out) {
  Mono *t = terms(n, 1u);
  Factor *f = arena_alloc(n->m->arena, sizeof *f);
  if (t == NULL || f == NULL)
    return t == NULL ? 0 : nf_fail(n, NF_OOM, NULL);
  f->atom = atom;
  f->exp = 1;
  t[0].coef = q_make(1, 1);
  t[0].count = 1;
  t[0].f = f;
  out->count = 1;
  out->t = t;
  return 1;
}

/* A + B, or A - B when NEG. */
static int poly_add(Nf *n, Poly a, Poly b, int neg, Poly *out) {
  Mono *t = terms(n, (uint64_t)a.count + b.count);
  uint32_t i;
  if (t == NULL)
    return 0;
  for (i = 0; i < a.count; i++)
    t[i] = a.t[i];
  for (i = 0; i < b.count; i++) {
    t[a.count + i] = b.t[i];
    t[a.count + i].coef.num = neg ? -b.t[i].coef.num : b.t[i].coef.num;
  }
  return poly_norm(n, t, a.count + b.count, out);
}

static int mono_mul(Nf *n, const Mono *a, const Mono *b, Mono *out) {
  Factor *f = arena_alloc(n->m->arena, ((size_t)a->count + b->count + 1u) * sizeof *f);
  uint32_t i = 0;
  uint32_t j = 0;
  uint32_t k = 0;
  if (f == NULL)
    return nf_fail(n, NF_OOM, NULL);
  while (i < a->count || j < b->count) {
    if (j == b->count || (i < a->count && a->f[i].atom < b->f[j].atom)) {
      f[k++] = a->f[i++];
    } else if (i == a->count || b->f[j].atom < a->f[i].atom) {
      f[k++] = b->f[j++];
    } else {
      f[k] = a->f[i++];
      f[k++].exp += b->f[j++].exp;
    }
  }
  if (k > NF_DEGREE_MAX)
    return nf_fail(n, NF_LARGE, NULL);
  out->count = k;
  out->f = f;
  return q_mul(n, a->coef, b->coef, &out->coef);
}

static int poly_mul(Nf *n, Poly a, Poly b, Poly *out) {
  Mono *t = terms(n, (uint64_t)a.count * b.count);
  uint32_t i;
  uint32_t j;
  if (t == NULL)
    return 0;
  for (i = 0; i < a.count; i++) {
    for (j = 0; j < b.count; j++) {
      if (!mono_mul(n, &a.t[i], &b.t[j], &t[i * b.count + j]))
        return 0;
    }
  }
  return poly_norm(n, t, a.count * b.count, out);
}

static int poly_eq(Poly a, Poly b) {
  uint32_t i;
  if (a.count != b.count)
    return 0;
  for (i = 0; i < a.count; i++) {
    if (mono_cmp(&a.t[i], &b.t[i]) != 0 || a.t[i].coef.num != b.t[i].coef.num || a.t[i].coef.den != b.t[i].coef.den)
      return 0;
  }
  return 1;
}

/* 1 when P is a constant; *Q gets it. */
static int poly_value(Poly p, Q *q) {
  q->num = 0;
  q->den = 1;
  if (p.count == 1u && p.t[0].count == 0u)
    *q = p.t[0].coef;
  return p.count == 0u || (p.count == 1u && p.t[0].count == 0u);
}

static int frac_const(Nf *n, Q q, Frac *out) {
  return poly_const(n, q, &out->n) && poly_const(n, q_make(1, 1), &out->d);
}

/* X + Y, or X - Y when NEG. */
static int frac_add(Nf *n, Frac x, Frac y, int neg, Frac *out) {
  Poly a;
  Poly b;
  return poly_mul(n, x.n, y.d, &a) && poly_mul(n, y.n, x.d, &b) && poly_add(n, a, b, neg, &out->n) && poly_mul(n, x.d, y.d, &out->d);
}

/* X * Y, or X / Y when DIV. */
static int frac_mul(Nf *n, Frac x, Frac y, int div, Frac *out) {
  return poly_mul(n, x.n, div ? y.d : y.n, &out->n) && poly_mul(n, x.d, div ? y.n : y.d, &out->d);
}

static int atom_frac(Nf *n, const Value *v, Sort sort, Frac *out) {
  uint32_t i;
  for (i = 0; i < n->atoms; i++) {
    if (n->atom_sort[i] == sort && conv_values(n->m, n->level, n->atom[i], v))
      break;
  }
  if (i == NF_ATOMS_MAX)
    return nf_fail(n, NF_LARGE, v);
  if (i == n->atoms) {
    n->atom[i] = v;
    n->atom_sort[i] = sort;
    n->atoms++;
  }
  return poly_atom(n, i, &out->n) && poly_const(n, q_make(1, 1), &out->d);
}

static int nonzero_has(Nf *n, const Value *d) {
  uint32_t i;
  for (i = 0; i < n->nonzeros; i++) {
    if (conv_values(n->m, n->level, n->nonzero[i], d))
      return 1;
  }
  return 0;
}

/* natSub, natDiv, natMod and natMax are not ring operations: atoms. */
static Kind nat_kind(Op op) {
  return op == OP_NAT_ADD ? K_ADD : op == OP_NAT_MUL ? K_MUL : K_ATOM;
}

static Kind rat_kind(Op op) {
  return op == OP_RAT_ADD ? K_ADD : op == OP_RAT_SUB ? K_SUB : op == OP_RAT_MUL ? K_MUL : op == OP_RAT_DIV ? K_DIV : K_ATOM;
}

static Kind kind_of(const Value *v, Sort sort) {
  if (v->kind == VAL_TRAP)
    return K_TRAP;
  if (v->kind == (sort == SORT_NAT ? VAL_NAT : VAL_RAT))
    return K_LIT;
  if (v->kind != VAL_STUCK)
    return K_ATOM;
  if (sort == SORT_RAT && v->op == OP_RAT_OF_NAT && v->argc == 1u)
    return K_CAST;
  if (v->argc != 2u)
    return K_ATOM;
  return sort == SORT_NAT ? nat_kind(v->op) : rat_kind(v->op);
}

static int nf_term(Nf *n, const Value *v, Sort sort, Frac *out);

/* X / Y. A literal divisor must not be 0. Another divisor makes an atom,
   except in field, where a hypothesis must give Ne Rat Y 0/1. */
static int nf_div(Nf *n, const Value *v, Frac *out) {
  const Value *y = v->args[1];
  Frac a;
  Frac b;
  if (y->kind == VAL_RAT && y->num == 0)
    return nf_fail(n, NF_DIV_ZERO, v);
  if (y->kind != VAL_RAT && n->rule != NF_FIELD)
    return atom_frac(n, v, SORT_RAT, out);
  if (y->kind != VAL_RAT && !nonzero_has(n, y))
    return nf_fail(n, NF_DIVISOR, y);
  return nf_term(n, v->args[0], SORT_RAT, &a) && nf_term(n, y, SORT_RAT, &b) && frac_mul(n, a, b, 1, out);
}

static int nf_term(Nf *n, const Value *v, Sort sort, Frac *out) {
  Frac x;
  Frac y;
  Kind kind = v == NULL ? K_TRAP : kind_of(v, sort);
  if (v == NULL)
    return nf_fail(n, NF_OOM, NULL);
  switch (kind) {
  case K_TRAP:
    return nf_fail(n, NF_TRAP, v);
  case K_LIT:
    if (v->nat > NF_LIMIT || (sort == SORT_RAT && mag(v->num) > NF_LIMIT))
      return nf_fail(n, NF_LARGE, v);
    return frac_const(n, sort == SORT_NAT ? q_make((int64_t)v->nat, 1) : q_make(v->num, (int64_t)v->nat), out);
  case K_ADD:
  case K_SUB:
    return nf_term(n, v->args[0], sort, &x) && nf_term(n, v->args[1], sort, &y) && frac_add(n, x, y, kind == K_SUB, out);
  case K_MUL:
    return nf_term(n, v->args[0], sort, &x) && nf_term(n, v->args[1], sort, &y) && frac_mul(n, x, y, 0, out);
  case K_DIV:
    return nf_div(n, v, out);
  case K_CAST:
    return nf_term(n, v->args[0], SORT_NAT, out);
  case K_ATOM:
    return atom_frac(n, v, sort, out);
  }
  return nf_fail(n, NF_OOM, v);
}

/* 1 when V is the kernel rule NAME applied to ARGC arguments; ARGS gets them. */
static int rule_app(Machine *m, const Value *v, const char *name, uint32_t argc, const Value **args) {
  uint32_t i = argc;
  while (i > 0u && v != NULL && v->kind == VAL_APP) {
    args[--i] = v->arg;
    v = v->dom;
  }
  return i == 0u && v != NULL && v->kind == VAL_STUCK && v->op == OP_RULE && v->argc == 0u && v->inst < m->def_count && strcmp(m->defs[v->inst].name, name) == 0;
}

/* 1 when V is Eq S a b, with S the type former SORT. */
static int is_eq(const Value *v, Op sort) {
  return v != NULL && v->kind == VAL_OP && v->op == OP_EQ && v->argc == 3u && val_is(v->args[0], sort);
}

/* One hypothesis H. field: Ne Rat d 0/1 records d. cert: RatLe x y or
   RatLt x y adds y - x to SUM; RatLt sets *STRICT. */
static int nf_fact(Nf *n, const Value *h, Frac *sum, int *strict) {
  const Value *eq = h->kind == VAL_PI ? h->dom : NULL;
  const Value *cmp = is_eq(h, OP_FLAG) && val_is(h->args[2], OP_FLAG_YES) ? h->args[1] : NULL;
  Frac x;
  Frac y;
  Frac e;
  if (n->rule == NF_FIELD) {
    if (!is_eq(eq, OP_RAT) || eq->args[2]->kind != VAL_RAT || eq->args[2]->num != 0 || !rule_app(n->m, closure_apply(n->m, h, val_var(n->m, n->level)), "False", 0u, NULL) || n->nonzeros == NF_HYPS_MAX)
      return nf_fail(n, NF_HYP, h);
    n->nonzero[n->nonzeros++] = eq->args[1];
    return 1;
  }
  /* Evaluation erases the endpoints of a closed true comparison. Treat
     Eq Flag flagYes flagYes as a redundant fact, without strictness: it
     could have come from either RatLe or RatLt. The witness is still
     checked by apply_rest. */
  if (cmp != NULL && val_is(cmp, OP_FLAG_YES))
    return 1;
  if (cmp == NULL || cmp->kind != VAL_STUCK || (cmp->op != OP_RAT_LE && cmp->op != OP_RAT_LT) || cmp->argc != 2u)
    return nf_fail(n, NF_HYP, h);
  *strict = *strict || cmp->op == OP_RAT_LT;
  return nf_term(n, cmp->args[0], SORT_RAT, &x) && nf_term(n, cmp->args[1], SORT_RAT, &y) && frac_add(n, y, x, 1, &e) && frac_add(n, *sum, e, 0, sum);
}

/* The hypotheses H: True, one fact, or And P Q. */
static int nf_hyps(Nf *n, const Value *h, Frac *sum, int *strict) {
  const Value *pq[2];
  if (h == NULL)
    return nf_fail(n, NF_HYP, NULL);
  if (rule_app(n->m, h, "True", 0u, NULL))
    return 1;
  if (rule_app(n->m, h, "And", 2u, pq))
    return nf_hyps(n, pq[0], sum, strict) && nf_hyps(n, pq[1], sum, strict);
  return nf_fact(n, h, sum, strict);
}

/* N_a * D_b = N_b * D_a. Each D is a product of nonzero divisors. */
static NfVerdict nf_equal(Nf *n, const Value *a, const Value *b, Sort sort) {
  Frac x;
  Frac y;
  Poly l;
  Poly r;
  if (!nf_term(n, a, sort, &x) || !nf_term(n, b, sort, &y) || !poly_mul(n, x.n, y.d, &l) || !poly_mul(n, y.n, x.d, &r))
    return n->fail;
  return poly_eq(l, r) ? NF_OK : NF_DIFFER;
}

/* b - a - SUM must be a constant k: k >= 0 for cert; for certLt, k > 0, or
   k = 0 with a strict hypothesis. */
static NfVerdict nf_cert(Nf *n, const Value *a, const Value *b, Frac sum, int strict) {
  Frac x;
  Frac y;
  Frac r;
  Q kn;
  Q kd;
  int s;
  if (!nf_term(n, a, SORT_RAT, &x) || !nf_term(n, b, SORT_RAT, &y) || !frac_add(n, y, x, 1, &r) || !frac_add(n, r, sum, 1, &r))
    return n->fail;
  if (!poly_value(r.n, &kn) || !poly_value(r.d, &kd) || kd.num == 0) {
    nf_fail(n, NF_NOT_CONST, NULL);
    return n->fail;
  }
  s = (kn.num > 0) - (kn.num < 0);
  s = kd.num < 0 ? -s : s;
  if (n->rule == NF_CERT)
    return s >= 0 ? NF_OK : NF_NEGATIVE;
  return s > 0 || (s == 0 && strict) ? NF_OK : NF_NEGATIVE;
}

NfVerdict nf_check(Machine *m, uint32_t level, NfRule rule, const Value *a, const Value *b, const Value *hyps, const Value **at) {
  Nf n;
  Frac sum;
  int strict = 0;
  NfVerdict verdict = NF_DIFFER;
  memset(&n, 0, sizeof n);
  n.m = m;
  n.level = level;
  n.rule = rule;
  n.fail = NF_OK;
  switch (rule) {
  case NF_RING:
    verdict = nf_equal(&n, a, b, SORT_RAT);
    break;
  case NF_RING_NAT:
    verdict = nf_equal(&n, a, b, SORT_NAT);
    break;
  case NF_FIELD:
    verdict = frac_const(&n, q_make(0, 1), &sum) && nf_hyps(&n, hyps, &sum, &strict) ? nf_equal(&n, a, b, SORT_RAT) : n.fail;
    break;
  case NF_CERT:
  case NF_CERT_LT:
    verdict = frac_const(&n, q_make(0, 1), &sum) && nf_hyps(&n, hyps, &sum, &strict) ? nf_cert(&n, a, b, sum, strict) : n.fail;
    break;
  }
  *at = n.at;
  return verdict;
}

const char *nf_text(NfVerdict verdict) {
  switch (verdict) {
  case NF_OK:
    return "the normal forms agree";
  case NF_DIFFER:
    return "the two sides have different normal forms";
  case NF_NOT_CONST:
    return "b - a minus the hypotheses is not a constant";
  case NF_NEGATIVE:
    return "b - a minus the hypotheses is negative (certLt: not positive, and no strict hypothesis)";
  case NF_DIVISOR:
    return "the hypotheses give no Ne Rat d 0/1 for the divisor";
  case NF_DIV_ZERO:
    return "a division by 0";
  case NF_HYP:
    return "a hypothesis does not have the expected form (field: Ne Rat d 0/1; cert: RatLe x y or RatLt x y; joined with And, or True)";
  case NF_TRAP:
    return "a term traps";
  case NF_LARGE:
    return "the normal form is too large, or a coefficient is out of range";
  case NF_OOM:
    return "out of memory";
  }
  return "?";
}
