/* The JSON reader (slices C1 and C2). See read.h.

   The reader takes JSON text in ASCII: objects, arrays, strings, integers,
   true, false and null, with white space between the tokens. The writer
   writes only these (src/json.c: names and printed types are ASCII, and a
   Nat or a Rat part is an integer). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "front/check.h"
#include "front/front.h"
#include "read.h"

typedef struct {
  Arena *arena;
  Diag *diag;
  const char *text;
  size_t len;
  size_t pos;
  unsigned depth;
} Reader;

/* The escapes of RFC 8259 other than \u. */
static const struct {
  char escape;
  char byte;
} SHORT_ESCAPES[] = {
  {'"', '"'}, {'\\', '\\'}, {'/', '/'}, {'b', '\b'}, {'f', '\f'}, {'n', '\n'}, {'r', '\r'}, {'t', '\t'},
};

static int syntax(Reader *r, const char *what) {
  return diag_fail(r->diag, "READ_SYNTAX", NULL, "offset %zu: %s", r->pos, what);
}

static int oom(Diag *diag) {
  return diag_fail(diag, "OOM", NULL, "the arena limit is reached");
}

/* The byte at the position, or -1 at the end of the text. */
static int peek(const Reader *r) {
  return r->pos < r->len ? (unsigned char)r->text[r->pos] : -1;
}

static int is_digit(int ch) {
  return ch >= '0' && ch <= '9';
}

static int hex_digit(int ch) {
  if (is_digit(ch))
    return ch - '0';
  if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  return -1;
}

static void skip_space(Reader *r) {
  int ch = peek(r);
  while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
    r->pos++;
    ch = peek(r);
  }
}

static JsonNode *new_node(Reader *r) {
  JsonNode *node = arena_alloc(r->arena, sizeof *node);
  if (node != NULL)
    memset(node, 0, sizeof *node);
  return node;
}

static int parse_value(Reader *r, JsonNode *node);

static int parse_word(Reader *r, const char *word, JsonKind kind, JsonNode *node) {
  size_t n = strlen(word);
  if (r->len - r->pos < n || memcmp(r->text + r->pos, word, n) != 0)
    return syntax(r, "expected a value");
  r->pos += n;
  node->kind = kind;
  return 1;
}

/* An integer: an optional minus sign, then 0 or a digit 1 to 9 and more
   digits. The number keeps its text; slice C2 checks its range. */
static int parse_number(Reader *r, JsonNode *node) {
  size_t start = r->pos;
  int ch;
  if (peek(r) == '-')
    r->pos++;
  if (!is_digit(peek(r)))
    return syntax(r, "expected a digit");
  if (peek(r) == '0' && is_digit((r->pos + 1 < r->len) ? (unsigned char)r->text[r->pos + 1] : -1))
    return syntax(r, "a number with a leading zero");
  while (is_digit(peek(r)))
    r->pos++;
  ch = peek(r);
  if (ch == '.' || ch == 'e' || ch == 'E')
    return syntax(r, "a number with a fraction or an exponent");
  node->kind = JSON_NUMBER;
  node->text = r->text + start;
  node->len = r->pos - start;
  return 1;
}

/* The escape at the position, before the byte END of the closing quote. */
static int decode_escape(Reader *r, size_t end, char *out) {
  char escape = r->text[r->pos + 1];
  int code = 0;
  size_t i;
  if (escape != 'u') {
    for (i = 0; i < sizeof SHORT_ESCAPES / sizeof SHORT_ESCAPES[0]; i++) {
      if (SHORT_ESCAPES[i].escape == escape) {
        *out = SHORT_ESCAPES[i].byte;
        r->pos += 2;
        return 1;
      }
    }
    return syntax(r, "an escape that JSON does not define");
  }
  if (end - r->pos < 6)
    return syntax(r, "a \\u escape with fewer than 4 hex digits");
  for (i = 2; i < 6 && code >= 0; i++) {
    int digit = hex_digit((unsigned char)r->text[r->pos + i]);
    code = digit < 0 ? -1 : code * 16 + digit;
  }
  if (code < 0)
    return syntax(r, "a \\u escape with a byte that is not a hex digit");
  if (code >= 0x80)
    return syntax(r, "a \\u escape above \\u007f (the document is ASCII)");
  *out = (char)code;
  r->pos += 6;
  return 1;
}

/* A string at the position. First find the closing quote, then decode. */
static int parse_string(Reader *r, const char **out, size_t *out_len) {
  size_t end = r->pos + 1;
  size_t n = 0;
  char *bytes;
  while (end < r->len && r->text[end] != '"')
    end += r->text[end] == '\\' ? 2 : 1;
  if (end >= r->len)
    return syntax(r, "a string with no closing quote");
  bytes = arena_alloc(r->arena, end - r->pos);
  if (bytes == NULL)
    return oom(r->diag);
  r->pos++;
  while (r->pos < end) {
    unsigned char ch = (unsigned char)r->text[r->pos];
    if (ch < 0x20 || ch >= 0x80)
      return syntax(r, "a control byte or a byte that is not ASCII in a string");
    if (ch != '\\') {
      bytes[n++] = (char)ch;
      r->pos++;
    } else if (!decode_escape(r, end, bytes + n++)) {
      return 0;
    }
  }
  r->pos = end + 1;
  *out = bytes;
  *out_len = n;
  return 1;
}

/* The items of an array or the members of an object, after the open
   bracket, up to and with the CLOSE bracket. */
static int parse_items(Reader *r, JsonNode *node, char close) {
  JsonNode **tail = &node->first;
  int object = node->kind == JSON_OBJECT;
  if (r->depth >= READ_DEPTH_LIMIT)
    return diag_fail(r->diag, "READ_DEPTH", NULL, "offset %zu: the document nests deeper than %u levels", r->pos,
                     READ_DEPTH_LIMIT);
  r->depth++;
  r->pos++;
  skip_space(r);
  if (peek(r) != close) {
    for (;;) {
      JsonNode *item = new_node(r);
      if (item == NULL)
        return oom(r->diag);
      skip_space(r);
      if (object && peek(r) != '"')
        return syntax(r, "expected a key");
      if (object && !parse_string(r, &item->key, &item->key_len))
        return 0;
      skip_space(r);
      if (object && peek(r) != ':')
        return syntax(r, "expected ':'");
      if (object)
        r->pos++;
      if (!parse_value(r, item))
        return 0;
      *tail = item;
      tail = &item->next;
      skip_space(r);
      if (peek(r) == close)
        break;
      if (peek(r) != ',')
        return syntax(r, object ? "expected ',' or '}'" : "expected ',' or ']'");
      r->pos++;
    }
  }
  r->pos++;
  r->depth--;
  return 1;
}

static int parse_value(Reader *r, JsonNode *node) {
  int ch;
  skip_space(r);
  ch = peek(r);
  switch (ch) {
  case '{':
    node->kind = JSON_OBJECT;
    return parse_items(r, node, '}');
  case '[':
    node->kind = JSON_ARRAY;
    return parse_items(r, node, ']');
  case '"':
    node->kind = JSON_STRING;
    return parse_string(r, &node->text, &node->len);
  case 't':
    return parse_word(r, "true", JSON_TRUE, node);
  case 'f':
    return parse_word(r, "false", JSON_FALSE, node);
  case 'n':
    return parse_word(r, "null", JSON_NULL, node);
  default:
    return ch == '-' || is_digit(ch) ? parse_number(r, node) : syntax(r, "expected a value");
  }
}

static int key_is(const JsonNode *node, const char *key) {
  size_t n = strlen(key);
  return node != NULL && node->key_len == n && memcmp(node->key, key, n) == 0;
}

/* The first member is the language name with the format version 1. */
static int check_version(const JsonNode *doc, Diag *diag) {
  const JsonNode *head = doc->kind == JSON_OBJECT ? doc->first : NULL;
  int ok = key_is(head, json_lang_name) && head->kind == JSON_NUMBER && head->len == 1 && head->text[0] == '1';
  return ok ? 1 : diag_fail(diag, "READ_VERSION", NULL, "the first member is not \"%s\":1 (format version 1)",
                            json_lang_name);
}

/* Then "instances", an array of {"name":S,"type":S,"value":V}. The members
   are in the order of the writer, and there are no others. */
static int check_shape(const JsonNode *doc, Diag *diag) {
  const JsonNode *list = doc->first->next;
  const JsonNode *inst;
  size_t index = 1;
  if (!key_is(list, "instances") || list->kind != JSON_ARRAY || list->next != NULL)
    return diag_fail(diag, "READ_SHAPE", NULL, "the second and last member is not \"instances\", an array");
  for (inst = list->first; inst != NULL; inst = inst->next, index++) {
    const JsonNode *name = inst->kind == JSON_OBJECT ? inst->first : NULL;
    const JsonNode *type = name == NULL ? NULL : name->next;
    const JsonNode *value = type == NULL ? NULL : type->next;
    int ok = key_is(name, "name") && name->kind == JSON_STRING && key_is(type, "type") && type->kind == JSON_STRING
      && key_is(value, "value") && value->next == NULL;
    if (!ok)
      return diag_fail(diag, "READ_SHAPE", NULL, "instance %zu is not {\"name\":S,\"type\":S,\"value\":V}", index);
  }
  return 1;
}

/* The typed read (slice C2). Each `type` text is lexed and parsed alone,
   then checked as a closed type against the domain. No temporary declaration
   enters the scope of another type. Values are decoded as in src/json.c. */

/* The default for non-matrix values, as PRINT_MAX of src/front/check.c.
   Root matrices use a larger buffer because `langc eval` streams them. */
#define READ_PRINT_MAX 65536u

typedef struct {
  Machine *m;
  Diag *diag;
  const char *name; /* the instance, for diagnostics */
} Typed;

static int value_fail(Typed *t, const char *what) {
  return diag_fail(t->diag, "READ_VALUE", t->name, "the value does not match the type: %s", what);
}

static int made(const Value **out, const Value *v) {
  *out = v;
  return v != NULL;
}

/* core.h has no constructor for a Rat or a Matrix value, so the reader sets
   the fields of core.h itself. arena_alloc gives zeroed memory. */
static Value *new_value(Typed *t, ValKind kind) {
  Value *v = arena_alloc(t->m->arena, sizeof *v);
  if (v == NULL)
    diag_fail(t->diag, "OOM", t->name, "out of memory");
  else
    v->kind = kind;
  return v;
}

/* An unsigned decimal from 0 to 2^64 - 1, with no sign and no leading zero. */
static int parse_u64(const char *text, size_t len, uint64_t *out) {
  uint64_t n = 0;
  size_t i;
  if (len == 0 || (len > 1 && text[0] == '0'))
    return 0;
  for (i = 0; i < len; i++) {
    uint64_t d = (uint64_t)(text[i] - '0');
    if (!is_digit(text[i]) || n > (UINT64_MAX - d) / 10u)
      return 0;
    n = n * 10u + d;
  }
  *out = n;
  return 1;
}

static int decode(Typed *t, const Value *type, const JsonNode *node, const Value **out);

/* Nat: the full unsigned 64-bit value. Fin n: also below n. */
static int decode_nat(Typed *t, const Value *type, const JsonNode *node, const Value **out) {
  const Value *bound = type->op == OP_FIN && type->argc == 1 ? type->args[0] : NULL;
  uint64_t n = 0;
  if (node->kind != JSON_NUMBER || !parse_u64(node->text, node->len, &n))
    return value_fail(t, "a Nat is a decimal from 0 to 2^64-1");
  if (type->op == OP_FIN && (bound == NULL || bound->kind != VAL_NAT || n >= bound->nat))
    return value_fail(t, "a value of Fin n is below n");
  return made(out, val_nat(t->m, n));
}

/* {"num":N,"den":D} in lowest terms: D above 0 and N above -2^63. Zero is 0/1. */
static int decode_cell(Typed *t, const JsonNode *node, Cell *out) {
  const JsonNode *num = node->kind == JSON_OBJECT ? node->first : NULL;
  const JsonNode *den = num == NULL ? NULL : num->next;
  size_t neg = num != NULL && num->len > 0 && num->text[0] == '-' ? 1u : 0u;
  uint64_t mag = 0;
  uint64_t d = 0;
  int ok = key_is(num, "num") && num->kind == JSON_NUMBER && key_is(den, "den") && den->kind == JSON_NUMBER
    && den->next == NULL && parse_u64(num->text + neg, num->len - neg, &mag) && mag <= (uint64_t)INT64_MAX
    && (neg == 0 || mag != 0) && parse_u64(den->text, den->len, &d) && d != 0 && gcd_u64(mag, d) == 1;
  if (!ok)
    return value_fail(t, "a Rat is {\"num\":N,\"den\":D} in lowest terms, with D above 0 and N above -2^63");
  out->num = neg ? -(int64_t)mag : (int64_t)mag;
  out->den = d;
  return 1;
}

/* Matrix m n: m rows of n cells. Each row is stochastic (no cell below 0, a
   sum of 1), as matTabulate checks (src/front/eval.c, EVAL_STOCHASTIC). */
static int decode_matrix(Typed *t, const Value *type, const JsonNode *node, const Value **out) {
  const Value *rows = type->argc == 2 ? type->args[0] : NULL;
  const Value *cols = type->argc == 2 ? type->args[1] : NULL;
  const JsonNode *row;
  const JsonNode *item;
  uint64_t count = 0;
  uint64_t i = 0;
  Cell *cells;
  Value *v;
  if (rows == NULL || cols == NULL || rows->kind != VAL_NAT || cols->kind != VAL_NAT)
    return diag_fail(t->diag, "INTERNAL", t->name, "the reader found a Matrix type with a size that is not a Nat");
  if (rows->nat > UINT32_MAX || cols->nat > UINT32_MAX || node->kind != JSON_ARRAY)
    return value_fail(t, "a Matrix is an array of rows");
  for (row = node->first; row != NULL; row = row->next, count++) {
    uint64_t n = 0;
    if (row->kind != JSON_ARRAY)
      return value_fail(t, "a Matrix row is an array");
    for (item = row->first; item != NULL; item = item->next)
      n++;
    if (n != cols->nat)
      return value_fail(t, "a Matrix row does not have n cells");
  }
  if (count != rows->nat)
    return value_fail(t, "a Matrix does not have m rows");
  cells = arena_alloc(t->m->arena, sizeof *cells * (size_t)(count * cols->nat) + sizeof *cells);
  if (cells == NULL)
    return oom(t->diag);
  for (row = node->first; row != NULL; row = row->next) {
    uint64_t start = i;
    int stochastic = 0;
    for (item = row->first; item != NULL; item = item->next, i++) {
      if (!decode_cell(t, item, &cells[i]))
        return 0;
      if (cells[i].num < 0)
        return value_fail(t, "a Matrix cell is below 0 (the rows are stochastic)");
    }
    if (!cell_row_stochastic(t->m, cells + start, (uint32_t)cols->nat, &stochastic))
      return 0;
    if (!stochastic)
      return value_fail(t, "a Matrix row does not sum to 1 (the rows are stochastic)");
  }
  v = new_value(t, VAL_MATRIX);
  if (v == NULL)
    return 0;
  v->rows = (uint32_t)count;
  v->cols = (uint32_t)cols->nat;
  v->cells = cells;
  return made(out, v);
}

/* null, and only when the two sides have one normal form (the proof is refl). */
static int decode_eq(Typed *t, const Value *type, const JsonNode *node, const Value **out) {
  if (node->kind != JSON_NULL)
    return value_fail(t, "an Eq proof is null");
  if (type->argc != 3 || !conv_values(t->m, 0, type->args[1], type->args[2]))
    return value_fail(t, "the two sides of the Eq type are not equal, so it has no proof");
  return made(out, val_make(t->m, OP_REFL, 0, 0, NULL, NULL, NULL));
}

/* null is none. When the element type is an Option or an Eq, `some v` is
   {"some":v}; otherwise it is v (src/json.c option_json). */
static int decode_option(Typed *t, const Value *type, const JsonNode *node, const Value **out) {
  const Value *elem = type->args[0];
  int wrap = val_is(elem, OP_OPTION) || val_is(elem, OP_EQ);
  const JsonNode *inner = !wrap ? node : node->kind == JSON_OBJECT ? node->first : NULL;
  const Value *v = NULL;
  if (node->kind == JSON_NULL)
    return made(out, val_make(t->m, OP_NONE, 0, 0, NULL, NULL, NULL));
  if (wrap && (!key_is(inner, "some") || inner->next != NULL))
    return value_fail(t, "`some v` of an Option of an Option or an Eq is {\"some\":v}");
  return decode(t, elem, inner, &v) && made(out, val_make(t->m, OP_SOME, 0, 1, v, NULL, NULL));
}

/* An array. The list is built from the last item. */
static int decode_list(Typed *t, const Value *type, const JsonNode *node, const Value **out) {
  const JsonNode *item;
  const Value **items;
  const Value *list;
  size_t count = 0;
  size_t i = 0;
  if (node->kind != JSON_ARRAY)
    return value_fail(t, "a List is an array");
  for (item = node->first; item != NULL; item = item->next)
    count++;
  items = arena_alloc(t->m->arena, (count + 1u) * sizeof *items);
  if (items == NULL)
    return oom(t->diag);
  for (item = node->first; item != NULL; item = item->next, i++)
    if (!decode(t, type->args[0], item, &items[i]))
      return 0;
  list = val_make(t->m, OP_NIL, 0, 0, NULL, NULL, NULL);
  for (i = count; i > 0 && list != NULL; i--)
    list = val_make(t->m, OP_CONS, 0, 2, items[i - 1u], list, NULL);
  return made(out, list);
}

/* {"witness":w,"payload":p}. The payload type is the Sigma body at the witness. */
static int decode_pack(Typed *t, const Value *type, const JsonNode *node, const Value **out) {
  const JsonNode *w = node->kind == JSON_OBJECT ? node->first : NULL;
  const JsonNode *p = w == NULL ? NULL : w->next;
  const Value *wv = NULL;
  const Value *pv = NULL;
  const Value *payload;
  if (!key_is(w, "witness") || !key_is(p, "payload") || p->next != NULL)
    return value_fail(t, "a Sigma value is {\"witness\":w,\"payload\":p}");
  if (!decode(t, type->dom, w, &wv))
    return 0;
  payload = closure_apply(t->m, type, wv);
  return payload != NULL && decode(t, payload, p, &pv) && made(out, val_make(t->m, OP_PACK, 0, 2, wv, pv, NULL));
}

static int family_is_enum(const Machine *m, const FamilyInfo *f) {
  uint32_t i;
  for (i = 0; i < f->ctor_count; i++)
    if (m->ctors[f->first_ctor + i].field_count != 0)
      return 0;
  return 1;
}

/* The constructor of F whose name is the string TAG, or UINT32_MAX. */
static uint32_t find_ctor(const Machine *m, const FamilyInfo *f, const JsonNode *tag) {
  uint32_t i;
  for (i = 0; tag != NULL && tag->kind == JSON_STRING && i < f->ctor_count; i++) {
    const char *name = m->ctors[f->first_ctor + i].name;
    if (strlen(name) == tag->len && memcmp(name, tag->text, tag->len) == 0)
      return f->first_ctor + i;
  }
  return UINT32_MAX;
}

/* The 3 family rules of src/json.c family_json: only constants, the name;
   one constructor, an object of the fields; otherwise {"tag":name, ...}. The
   fields are in order. A field type is in the context of the parameters. */
static int decode_family(Typed *t, const Value *type, const JsonNode *node, const Value **out) {
  const FamilyInfo *f;
  const CtorInfo *ci;
  const JsonNode *tag;
  const JsonNode *field;
  const Env *env = NULL;
  const Value **args;
  uint32_t c;
  uint32_t i;
  int is_enum;
  if (type->inst >= t->m->family_count || type->argc != t->m->families[type->inst].param_count)
    return diag_fail(t->diag, "INTERNAL", t->name, "the reader found a family type with a bad index");
  f = &t->m->families[type->inst];
  is_enum = family_is_enum(t->m, f);
  tag = !is_enum && f->ctor_count != 1 && node->kind == JSON_OBJECT && key_is(node->first, "tag") ? node->first : NULL;
  c = is_enum ? find_ctor(t->m, f, node)
    : f->ctor_count == 1 ? (node->kind == JSON_OBJECT ? f->first_ctor : UINT32_MAX) : find_ctor(t->m, f, tag);
  field = is_enum || node->kind != JSON_OBJECT ? NULL : tag != NULL ? tag->next : node->first;
  if (c == UINT32_MAX)
    return value_fail(t, "the value is not a constructor of the family");
  ci = &t->m->ctors[c];
  args = arena_alloc(t->m->arena, (f->param_count + ci->field_count + 1u) * sizeof *args);
  if (args == NULL)
    return oom(t->diag);
  for (i = 0; i < f->param_count; i++) {
    args[i] = type->args[i];
    env = env_push(t->m, env, type->args[i]);
  }
  for (i = 0; i < ci->field_count; i++, field = field->next) {
    const FieldInfo *fi = &ci->fields[i];
    const Value *ft;
    if (!key_is(field, fi->name))
      return value_fail(t, "the members are not the fields of the constructor, in order");
    ft = fi->recursive ? type : eval_core(t->m, env, fi->type);
    if (ft == NULL || !decode(t, ft, field, &args[f->param_count + i]))
      return 0;
  }
  if (field != NULL)
    return value_fail(t, "the object has a member that is not a field of the constructor");
  return made(out, val_op(t->m, VAL_OP, OP_CTOR, c, 0, args, f->param_count + ci->field_count));
}

static int decode_op(Typed *t, const Value *type, const JsonNode *node, const Value **out) {
  const JsonNode *a = node->kind == JSON_OBJECT ? node->first : NULL;
  const JsonNode *b = a == NULL ? NULL : a->next;
  const Value *x = NULL;
  const Value *y = NULL;
  Cell cell;
  Value *v;
  int left = key_is(a, "inl");
  switch (type->op) {
  case OP_NAT:
  case OP_FIN:
    return decode_nat(t, type, node, out);
  case OP_RAT:
    if (!decode_cell(t, node, &cell))
      return 0;
    v = new_value(t, VAL_RAT);
    if (v == NULL)
      return 0;
    v->num = cell.num;
    v->nat = cell.den;
    return made(out, v);
  case OP_MATRIX:
    return decode_matrix(t, type, node, out);
  case OP_FLAG:
    if (node->kind != JSON_TRUE && node->kind != JSON_FALSE)
      return value_fail(t, "a Flag is true or false");
    return made(out, val_make(t->m, node->kind == JSON_TRUE ? OP_FLAG_YES : OP_FLAG_NO, 0, 0, NULL, NULL, NULL));
  case OP_UNIT:
    if (node->kind != JSON_OBJECT || a != NULL)
      return value_fail(t, "a Unit is {}");
    return made(out, val_make(t->m, OP_UNIT_VAL, 0, 0, NULL, NULL, NULL));
  case OP_EQ:
    return decode_eq(t, type, node, out);
  case OP_PROD:
    if (!key_is(a, "first") || !key_is(b, "second") || b->next != NULL)
      return value_fail(t, "a Prod is {\"first\":a,\"second\":b}");
    return decode(t, type->args[0], a, &x) && decode(t, type->args[1], b, &y)
      && made(out, val_make(t->m, OP_PAIR, 0, 2, x, y, NULL));
  case OP_SUM:
    if ((!left && !key_is(a, "inr")) || b != NULL)
      return value_fail(t, "a Sum is {\"inl\":a} or {\"inr\":b}");
    return decode(t, type->args[left ? 0 : 1], a, &x) && made(out, val_make(t->m, left ? OP_INL : OP_INR, 0, 1, x, NULL, NULL));
  case OP_OPTION:
    return decode_option(t, type, node, out);
  case OP_LIST:
    return decode_list(t, type, node, out);
  case OP_FAMILY:
    return decode_family(t, type, node, out);
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
  case OP_FIRST:
  case OP_SECOND:
  case OP_EITHER:
  case OP_OPTION_ELIM:
  case OP_PURE:
  case OP_MAP:
  case OP_BIND:
  case OP_FOLD_NAT:
  case OP_FOLD_LIST:
  case OP_FOLD_FAMILY:
  case OP_UNFOLD:
  case OP_FILTER:
  case OP_WITNESS:
  case OP_PAYLOAD:
  case OP_SYMM:
  case OP_TRANS:
  case OP_TRANSPORT:
  case OP_CONG:
  case OP_NAT_ADD:
  case OP_NAT_SUB:
  case OP_NAT_MUL:
  case OP_NAT_EQ:
  case OP_NAT_LE:
  case OP_FLAG_IF:
  case OP_NAT_DIV:
  case OP_NAT_MOD:
  case OP_NAT_MAX:
  case OP_NAT_LT:
  case OP_FLAG_AND:
  case OP_FLAG_NOT:
  case OP_ALL_FIN:
  case OP_RAT_ADD:
  case OP_RAT_SUB:
  case OP_RAT_MUL:
  case OP_RAT_DIV:
  case OP_RAT_OF_NAT:
  case OP_RAT_EQ:
  case OP_RAT_LE:
  case OP_RAT_LT:
  case OP_SUM_RAT:
  case OP_MAT_TABULATE:
  case OP_MAT_OF_FN:
  case OP_MAT_ENTRY:
  case OP_MAT_COMP:
  case OP_MAT_KRON:
  case OP_MAT_EQ:
  case OP_PROJ:
    return diag_fail(t->diag, "READ_TYPE", t->name, "the type is not a type former");
  }
  return diag_fail(t->diag, "INTERNAL", t->name, "the reader found an unknown operation");
}

static int decode(Typed *t, const Value *type, const JsonNode *node, const Value **out) {
  switch (type->kind) {
  case VAL_OP:
    return decode_op(t, type, node, out);
  case VAL_SIGMA:
    return decode_pack(t, type, node, out);
  case VAL_PI:
  case VAL_UNIV:
  case VAL_NAT:
  case VAL_RAT:
  case VAL_MATRIX:
  case VAL_TRAP:
  case VAL_LAM:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
    return diag_fail(t->diag, "READ_TYPE", t->name, "the type holds a function, a type or a stuck term, which has no JSON form");
  }
  return diag_fail(t->diag, "INTERNAL", t->name, "the reader found an unknown type kind");
}

/* As is_instance of src/json.c: not a function type, a universe or an equality. */
static int instance_type(const Value *type) {
  switch (type->kind) {
  case VAL_PI:
  case VAL_UNIV:
    return 0;
  case VAL_OP:
    return !val_is(type, OP_EQ);
  case VAL_SIGMA:
  case VAL_NAT:
  case VAL_RAT:
  case VAL_MATRIX:
  case VAL_TRAP:
  case VAL_LAM:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
    return 1;
  }
  return 1;
}

/* As `langc eval` (src/front/check.c eval_command and print_result): a Flag
   entry prints as 0 or 1. Each other value prints with value_print, which
   prints a Nat, a Rat (num/den) and a Matrix (rows of num/den) in the form of
   print_result. */
static int print_read_value(Machine *m, const Value *type, const Value *v, char **buf) {
  Entry entry;
  size_t capacity = READ_PRINT_MAX;
  if (v->kind == VAL_MATRIX) {
    /* Decoded cell counts are bounded by the input document. A cell has at
       most 40 decimal characters; allow separators, row brackets and NUL. */
    size_t needed = (size_t)v->rows * v->cols * 44u + (size_t)v->rows * 4u + 3u;
    if (needed > capacity) {
      capacity = needed;
      *buf = arena_alloc(m->arena, capacity);
      if (*buf == NULL)
        return oom(m->diag);
    }
  }
  if (entry_of(m, type, &entry) && entry.result_flag)
    snprintf(*buf, capacity, "%d", val_is(v, OP_FLAG_YES));
  else
    value_print(m, NULL, 0, v, *buf, capacity);
  return 1;
}

static int name_order(const void *a, const void *b) {
  const JsonNode *x = *(const JsonNode *const *)a;
  const JsonNode *y = *(const JsonNode *const *)b;
  size_t n = x->len < y->len ? x->len : y->len;
  int c = n == 0 ? 0 : memcmp(x->text, y->text, n);
  return c != 0 ? c : (x->len > y->len) - (x->len < y->len);
}

/* Parses TEXT as the body of one declaration. Its annotation is a parsing
   scaffold only; check_closed_type infers the body's universe. */
static int parse_type(Arena *arena, const JsonNode *type, size_t index, const Term **out, Diag *diag) {
  char name[32];
  char head[64];
  int n = snprintf(head, sizeof head, "def readType%zu : Type 0 := ", index);
  size_t len = (size_t)n + type->len + 1u;
  char *text = arena_alloc(arena, len + 1u);
  DeclList decls;
  TokenList toks;
  memset(&decls, 0, sizeof decls);
  if (text == NULL)
    return oom(diag);
  memcpy(text, head, (size_t)n);
  if (type->len != 0)
    memcpy(text + n, type->text, type->len);
  text[len - 1u] = '\n';
  snprintf(name, sizeof name, "readType%zu", index);
  if (!lex_source(arena, "type", text, len, &toks, diag) || !parse_source(arena, "type", ORIGIN_PROGRAM, &toks, &decls, diag))
    return 0;
  if (decls.count != 1u || strcmp(decls.items[0].name, name) != 0)
    return diag_fail(diag, "READ_TYPE", NULL, "the type text of instance %zu is more than one type", index);
  *out = decls.items[0].body;
  return 1;
}

int read_typed(Arena *arena, const JsonNode *doc, const char **text, size_t *len, Diag *diag) {
  const JsonNode *list = doc->first->next;
  const JsonNode *inst;
  const JsonNode **names;
  const char **lines;
  char *type_buf;
  char *value_buf;
  char *buf;
  size_t count = 0;
  size_t total = 0;
  size_t i;
  DeclList decls;
  Machine machine;
  Diag check;
  Typed t;
  for (inst = list->first; inst != NULL; inst = inst->next)
    count++;
  names = arena_alloc(arena, (count + 1u) * sizeof *names);
  lines = arena_alloc(arena, (count + 1u) * sizeof *lines);
  type_buf = arena_alloc(arena, READ_PRINT_MAX);
  value_buf = arena_alloc(arena, READ_PRINT_MAX);
  if (names == NULL || lines == NULL || type_buf == NULL || value_buf == NULL)
    return oom(diag);
  for (inst = list->first, i = 0; inst != NULL; inst = inst->next, i++) {
    size_t j;
    names[i] = inst->first;
    for (j = 0; j < names[i]->len; j++)
      if ((unsigned char)names[i]->text[j] < 0x20 || names[i]->text[j] == 0x7f)
        return diag_fail(diag, "READ_NAME", NULL, "instance %zu has a control byte in its name", i + 1u);
  }
  qsort(names, count, sizeof *names, name_order);
  for (i = 1; i < count; i++)
    if (name_order(&names[i - 1u], &names[i]) == 0)
      return diag_fail(diag, "READ_NAME", NULL, "2 instances have the name \"%.*s\"", (int)names[i]->len, names[i]->text);
  diag_init(&check);
  if (!front_load(arena, "type", "", 0, &decls, &check) || !check_program(arena, &decls, &machine, &check))
    return diag_fail(diag, "INTERNAL", NULL, "the domain does not load: %s: %s", check.code, check.msg);
  t.m = &machine;
  t.diag = diag;
  for (inst = list->first, i = 0; inst != NULL; inst = inst->next, i++) {
    const Term *term = NULL;
    const Value *type;
    const Value *value = NULL;
    size_t n;
    t.name = arena_strndup(arena, inst->first->text, inst->first->len);
    if (t.name == NULL)
      return oom(diag);
    machine.def = t.name;
    diag_init(&check);
    machine.diag = &check;
    if (!parse_type(arena, inst->first->next, i + 1u, &term, &check) || !check_closed_type(&machine, term, &type))
      return diag_fail(diag, strcmp(check.code, "OOM") == 0 ? "OOM" : "READ_TYPE", t.name,
                       "the type does not parse or check: %s: %s", check.code, check.msg);
    machine.diag = diag;
    if (!instance_type(type))
      return diag_fail(diag, "READ_TYPE", t.name, "the type is a function type, a universe or an equality, not an instance");
    if (!decode(&t, type, inst->first->next->next, &value))
      return 0;
    value_print(&machine, NULL, 0, type, type_buf, READ_PRINT_MAX);
    if (!print_read_value(&machine, type, value, &value_buf))
      return 0;
    n = strlen(t.name) + strlen(type_buf) + strlen(value_buf) + 7u;
    buf = arena_alloc(arena, n + 1u);
    if (buf == NULL)
      return oom(diag);
    snprintf(buf, n + 1u, "%s : %s = %s\n", t.name, type_buf, value_buf);
    lines[i] = buf;
    total += n;
  }
  buf = arena_alloc(arena, total + 1u);
  if (buf == NULL)
    return oom(diag);
  for (i = 0, total = 0; i < count; i++) {
    size_t n = strlen(lines[i]);
    memcpy(buf + total, lines[i], n);
    total += n;
  }
  *text = buf;
  *len = total;
  return 1;
}

int read_document(Arena *arena, const char *text, size_t len, const JsonNode **doc, Diag *diag) {
  Reader r;
  JsonNode *root;
  if (len > READ_MAX_BYTES)
    return diag_fail(diag, "READ_SIZE", NULL, "the document is larger than %zu bytes", READ_MAX_BYTES);
  memset(&r, 0, sizeof r);
  r.arena = arena;
  r.diag = diag;
  r.text = text;
  r.len = len;
  root = new_node(&r);
  if (root == NULL)
    return oom(diag);
  if (!parse_value(&r, root))
    return 0;
  skip_space(&r);
  if (r.pos != r.len)
    return syntax(&r, "bytes after the document");
  if (!check_version(root, diag) || !check_shape(root, diag))
    return 0;
  *doc = root;
  return 1;
}

typedef struct {
  Arena *arena;
  Diag *diag;
  char *data;
  size_t len;
  size_t cap;
} Out;

static int put(Out *o, const char *text, size_t n) {
  if (o->len + n + 1 > o->cap) {
    size_t cap = o->cap == 0 ? 4096 : o->cap;
    char *data;
    while (cap < o->len + n + 1)
      cap *= 2;
    data = arena_alloc(o->arena, cap);
    if (data == NULL)
      return oom(o->diag);
    if (o->len > 0)
      memcpy(data, o->data, o->len);
    o->data = data;
    o->cap = cap;
  }
  memcpy(o->data + o->len, text, n);
  o->len += n;
  o->data[o->len] = '\0';
  return 1;
}

/* The escapes of the writer (put_string in src/json.c). */
static int put_string(Out *o, const char *text, size_t n) {
  char esc[8];
  size_t i;
  int ok = put(o, "\"", 1);
  for (i = 0; ok && i < n; i++) {
    unsigned char ch = (unsigned char)text[i];
    int plain = ch >= 0x20 && ch != '"' && ch != '\\';
    if (plain)
      ok = put(o, text + i, 1);
    else {
      snprintf(esc, sizeof esc, "\\u%04x", ch);
      ok = put(o, esc, 6);
    }
  }
  return ok && put(o, "\"", 1);
}

static int write_node(Out *o, const JsonNode *node) {
  const JsonNode *item;
  int array = node->kind == JSON_ARRAY;
  int ok;
  switch (node->kind) {
  case JSON_NULL:
    return put(o, "null", 4);
  case JSON_FALSE:
    return put(o, "false", 5);
  case JSON_TRUE:
    return put(o, "true", 4);
  case JSON_NUMBER:
    return put(o, node->text, node->len);
  case JSON_STRING:
    return put_string(o, node->text, node->len);
  case JSON_ARRAY:
  case JSON_OBJECT:
    ok = put(o, array ? "[" : "{", 1);
    for (item = node->first; ok && item != NULL; item = item->next) {
      ok = (item == node->first || put(o, ",", 1))
        && (array || (put_string(o, item->key, item->key_len) && put(o, ":", 1))) && write_node(o, item);
    }
    return ok && put(o, array ? "]" : "}", 1);
  }
  return 0;
}

int read_write(Arena *arena, const JsonNode *doc, const char **text, size_t *len, Diag *diag) {
  Out o;
  memset(&o, 0, sizeof o);
  o.arena = arena;
  o.diag = diag;
  if (!write_node(&o, doc) || !put(&o, "\n", 1))
    return 0;
  *text = o.data;
  *len = o.len;
  return 1;
}
