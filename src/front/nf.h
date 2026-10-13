/* The normal form of the checked rules ring, ringNat, field, cert and certLt
   (M5 K3b, m5-design.md 3.5). SHARED by the tcc kits. */
#ifndef LANG_FRONT_NF_H
#define LANG_FRONT_NF_H

#include "front/core.h"

typedef enum {
  NF_RING,     /* ring a b H h : Eq Rat a b; H gives Ne Rat d 0/1 */
  NF_RING_NAT, /* ringNat a b : Eq Nat a b */
  NF_FIELD,    /* field a b H h : Eq Rat a b; H gives Ne Rat d 0/1 */
  NF_CERT,     /* cert a b H h : RatLe a b; H gives RatLe, RatLt and Ne facts */
  NF_CERT_LT   /* certLt a b H h : RatLt a b */
} NfRule;

typedef enum {
  NF_OK,
  NF_DIFFER,    /* the two normal forms are not equal */
  NF_NOT_CONST, /* cert: the remainder is not a constant */
  NF_NEGATIVE,  /* cert: the remainder is negative (certLt: not positive) */
  NF_DIVISOR,   /* a divisor that is not a literal has no Ne fact */
  NF_DIV_ZERO,  /* a division by the literal 0 */
  NF_HYP,       /* a hypothesis that does not have the expected form */
  NF_TRAP,      /* a term traps */
  NF_LARGE,     /* the normal form is too large, or a coefficient overflows */
  NF_OOM
} NfVerdict;

/* Checks RULE on the values A and B at LEVEL, with the hypotheses HYPS (a
   Prop value; NULL for ringNat). *AT gets the term of a refusal, or
   NULL. */
NfVerdict nf_check(Machine *m, uint32_t level, NfRule rule, const Value *a, const Value *b, const Value *hyps, const Value **at);
const char *nf_text(NfVerdict verdict);

#endif
