/* Certificate regressions through the parser and checker. */
#include <stdio.h>
#include <string.h>

#include "front/check.h"
#include "front/front.h"

static unsigned checked;
static unsigned failures;

static void expect_check(const char *name, const char *program, const char *code) {
  Arena arena;
  Diag diag;
  DeclList decls;
  Machine machine;
  int ok;
  int pass;
  arena_init(&arena, (size_t)1 << 30);
  diag_init(&diag);
  ok = front_load(&arena, name, program, strlen(program), &decls, &diag)
    && check_program(&arena, &decls, &machine, &diag);
  pass = code == NULL ? ok : !ok && diag.set && strcmp(diag.code, code) == 0;
  checked++;
  if (!pass) {
    fprintf(stderr, "FAIL certificate %s: %s: %s\n", name,
            diag.set ? diag.code : "unexpected acceptance", diag.msg);
    failures++;
  }
  arena_release(&arena);
}

int main(void) {
  expect_check("closed non-strict fact",
    "theorem t : (x : Rat) -> RatLe x (ratAdd x 1/1) := fun x => "
    "cert x (ratAdd x 1/1) (RatLe 0/1 1/1) (decide (ratLe 0/1 1/1))\n", NULL);
  expect_check("closed strict fact",
    "theorem t : (x : Rat) -> RatLt x (ratAdd x 1/1) := fun x => "
    "certLt x (ratAdd x 1/1) (RatLt 0/1 1/1) (decide (ratLt 0/1 1/1))\n", NULL);
  expect_check("closed reflexive fact",
    "theorem t : (x : Rat) -> RatLe x x := fun x => "
    "cert x x (RatLe 0/1 0/1) (ratLeRefl 0/1)\n", NULL);
  expect_check("mixed closed and open facts",
    "theorem t : (x : Rat) -> (y : Rat) -> RatLe x y -> RatLe x (ratAdd y 1/1) := "
    "fun x => fun y => fun h => cert x (ratAdd y 1/1) "
    "(And (RatLe 0/1 1/1) (RatLe x y)) "
    "(andIntro (RatLe 0/1 1/1) (RatLe x y) (decide (ratLe 0/1 1/1)) h)\n", NULL);
  expect_check("closed strict fact cannot prove reflexivity",
    "theorem t : (x : Rat) -> RatLt x x := fun x => "
    "certLt x x (RatLt 0/1 1/1) (decide (ratLt 0/1 1/1))\n", "RULE_CERT_LT");
  expect_check("closed non-strict fact cannot prove strictness",
    "theorem t : (x : Rat) -> RatLt x x := fun x => "
    "certLt x x (RatLe 0/1 0/1) (ratLeRefl 0/1)\n", "RULE_CERT_LT");
  expect_check("closed false fact is refused",
    "theorem t : (x : Rat) -> RatLe 1/1 0/1 -> RatLe x (ratSub x 1/1) := "
    "fun x => fun h => cert x (ratSub x 1/1) (RatLe 1/1 0/1) h\n", "RULE_CERT");
  expect_check("closed true fact cannot prove a negative gap",
    "theorem t : (x : Rat) -> RatLe x (ratSub x 1/1) := fun x => "
    "cert x (ratSub x 1/1) (RatLe 0/1 1/1) (decide (ratLe 0/1 1/1))\n", "RULE_CERT");
  expect_check("closed fact still needs a witness",
    "theorem t : (x : Rat) -> RatLe x (ratAdd x 1/1) := fun x => "
    "cert x (ratAdd x 1/1) (RatLe 0/1 1/1) trivial\n", "RULE_CERT");
  printf("certificate regressions: %u checked, %u failed\n", checked, failures);
  return failures != 0;
}
