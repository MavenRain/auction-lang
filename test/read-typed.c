/* Integration regressions for documents emitted by the compiler and malformed
   typed reads. These exercise the JSON writer, reader and domain checker. */
#include <stdio.h>
#include <string.h>

#include "front/check.h"
#include "front/front.h"
#include "json.h"
#include "read.h"

static int failures;
static unsigned checked;

static void result(const char *name, int ok, const Diag *diag) {
  checked++;
  if (!ok) {
    fprintf(stderr, "FAIL typed read %s: %s: %s\n", name,
            diag->set ? diag->code : "unexpected output or acceptance", diag->msg);
    failures++;
  }
}

static void built_read(const char *name, const char *program, const char *expected) {
  Arena arena;
  Diag diag;
  DeclList decls;
  Machine machine;
  const char *json = NULL;
  const char *text = NULL;
  const JsonNode *doc = NULL;
  size_t json_len = 0;
  size_t text_len = 0;
  int ok;
  arena_init(&arena, (size_t)1 << 30);
  diag_init(&diag);
  ok = front_load(&arena, name, program, strlen(program), &decls, &diag)
    && check_program(&arena, &decls, &machine, &diag)
    && json_document(&machine, &json, &json_len)
    && read_document(&arena, json, json_len, &doc, &diag)
    && read_typed(&arena, doc, &text, &text_len, &diag)
    && text_len == strlen(expected) && memcmp(text, expected, text_len) == 0;
  result(name, ok, &diag);
  arena_release(&arena);
}

static void refused_read(const char *name, const char *instances, const char *code) {
  Arena arena;
  Diag diag;
  const JsonNode *doc = NULL;
  const char *text = NULL;
  size_t len = 0;
  char json[1024];
  int ok;
  snprintf(json, sizeof json, "{\"auction-lang\":1,\"instances\":[%s]}", instances);
  arena_init(&arena, (size_t)1 << 30);
  diag_init(&diag);
  ok = read_document(&arena, json, strlen(json), &doc, &diag)
    && !read_typed(&arena, doc, &text, &len, &diag) && strcmp(diag.code, code) == 0;
  result(name, ok, &diag);
  arena_release(&arena);
}

static void large_matrix_read(void) {
  static char expected[90100];
  const char *head = "fullMatrix : Matrix 1 10000 = [[";
  size_t used = strlen(head);
  unsigned i;
  memcpy(expected, head, used);
  for (i = 0; i < 10000; i++) {
    const char *cell = i == 0 ? "1/10000" : ", 1/10000";
    size_t len = strlen(cell);
    memcpy(expected + used, cell, len);
    used += len;
  }
  memcpy(expected + used, "]]\n", 4);
  built_read("full matrix output",
    "def fullMatrix : Matrix 1 10000 := matTabulate 1 10000\n"
    " (fun (i : Fin 1) (j : Fin 10000) => 1/10000)\n", expected);
}

int main(void) {
  large_matrix_read();
  built_read("higher universes",
    "def optionUniverse : Option (Type 0) := none\n"
    "def listUniverse : List (Type 0) := nil\n"
    "def sumUniverse : Sum (Type 0) Nat := inr 7\n"
    "def higherUniverse : Option (Type 2) := none\n",
    "optionUniverse : Option (Type 0) = none\n"
    "listUniverse : List (Type 0) = nil\n"
    "sumUniverse : Sum (Type 0) Nat = inr 7\n"
    "higherUniverse : Option (Type 2) = none\n");
  built_read("wide matrix row",
    "def wideRow : Matrix 1 4 := matTabulate 1 4\n"
    " (fun (i : Fin 1) (j : Fin 4) =>\n"
    " flagIf (natEq (finVal j) 0) 1/8000000002\n"
    " (flagIf (natEq (finVal j) 1) 1/8000000006\n"
    " (flagIf (natEq (finVal j) 2) 2000000000/4000000001\n"
    " 2000000001/4000000003)))\n",
    "wideRow : Matrix 1 4 = [[1/8000000002, 1/8000000006, 2000000000/4000000001, 2000000001/4000000003]]\n");
  built_read("unsigned matrix partial sum",
    "def kronWide : Matrix 1 4 := matKron 1 2 1 2\n"
    " (matTabulate 1 2 (fun (i : Fin 1) (j : Fin 2) =>\n"
    " flagIf (natEq (finVal j) 0) 2147483645/4294967291 2147483646/4294967291))\n"
    " (matTabulate 1 2 (fun (i : Fin 1) (j : Fin 2) =>\n"
    " flagIf (natEq (finVal j) 0) 2147483639/4294967279 2147483640/4294967279))\n",
    "kronWide : Matrix 1 4 = [[4611685992657584155/18446743979220271189, "
    "4611685994805067794/18446743979220271189, "
    "4611685994805067800/18446743979220271189, "
    "4611685996952551440/18446743979220271189]]\n");
  built_read("large common denominator",
    "def bigRow : Matrix 1 4 := matComp 1 2 4\n"
    " (matTabulate 1 2 (fun (i : Fin 1) (j : Fin 2) => 1/2))\n"
    " (matTabulate 2 4 (fun (i : Fin 2) (j : Fin 4) =>\n"
    " flagIf (natEq (finVal i) 0)\n"
    " (flagIf (natEq (finVal j) 0) 1/9223372036854775807\n"
    " (flagIf (natEq (finVal j) 2) 9223372036854775806/9223372036854775807 0/1))\n"
    " (flagIf (natEq (finVal j) 1) 1/9223372036854775805\n"
    " (flagIf (natEq (finVal j) 3) 9223372036854775804/9223372036854775805 0/1))))\n",
    "bigRow : Matrix 1 4 = [[1/18446744073709551614, 1/18446744073709551610, "
    "4611686018427387903/9223372036854775807, 4611686018427387902/9223372036854775805]]\n");
  built_read("common denominator above 128 bits",
    "def splitRow : (p : Nat) -> (i : Fin 4) -> (j : Fin 8) -> Rat :=\n"
    " fun (p : Nat) (i : Fin 4) (j : Fin 8) =>\n"
    " flagIf (natEq (finVal j) (finVal i)) (ratDiv 1/1 (ratOfNat p))\n"
    " (flagIf (natEq (finVal j) (natAdd (finVal i) 4))\n"
    " (ratDiv (ratOfNat (natSub p 1)) (ratOfNat p)) 0/1)\n"
    "def multiWide : Matrix 1 8 := matComp 1 4 8\n"
    " (matTabulate 1 4 (fun (i : Fin 1) (j : Fin 4) => 1/4))\n"
    " (matTabulate 4 8 (fun (i : Fin 4) (j : Fin 8) =>\n"
    " splitRow (flagIf (natEq (finVal i) 0) 8000000001\n"
    " (flagIf (natEq (finVal i) 1) 8000000009\n"
    " (flagIf (natEq (finVal i) 2) 8000000033 8000000057))) i j))\n",
    "multiWide : Matrix 1 8 = [[1/32000000004, 1/32000000036, 1/32000000132, 1/32000000228, "
    "2000000000/8000000001, 2000000002/8000000009, 2000000008/8000000033, 2000000014/8000000057]]\n");
  refused_read("large row below one",
    "{\"name\":\"m\",\"type\":\"Matrix 1 4\",\"value\":[["
    "{\"num\":1,\"den\":18446744073709551614},{\"num\":1,\"den\":18446744073709551610},"
    "{\"num\":4611686018427387903,\"den\":9223372036854775807},"
    "{\"num\":4611686018427387901,\"den\":9223372036854775805}]]}", "READ_VALUE");
  refused_read("large row above one",
    "{\"name\":\"m\",\"type\":\"Matrix 1 4\",\"value\":[["
    "{\"num\":1,\"den\":18446744073709551614},{\"num\":1,\"den\":18446744073709551610},"
    "{\"num\":4611686018427387903,\"den\":9223372036854775807},"
    "{\"num\":4611686018427387903,\"den\":9223372036854775805}]]}", "READ_VALUE");
  refused_read("temporary definition",
    "{\"name\":\"a\",\"type\":\"Nat\",\"value\":1},"
    "{\"name\":\"b\",\"type\":\"readType1\",\"value\":2}", "READ_TYPE");
  refused_read("NUL in name",
    "{\"name\":\"a\\u0000b\",\"type\":\"Nat\",\"value\":1}", "READ_NAME");
  refused_read("newline in name",
    "{\"name\":\"a\\nb\",\"type\":\"Nat\",\"value\":1}", "READ_NAME");
  refused_read("value as a type",
    "{\"name\":\"a\",\"type\":\"1\",\"value\":1}", "READ_TYPE");
  refused_read("multiple declarations",
    "{\"name\":\"a\",\"type\":\"Nat def other : Nat := 0\",\"value\":1}", "READ_TYPE");
  printf("typed reader regressions: %u checked, %d failed\n", checked, failures);
  return failures != 0;
}
