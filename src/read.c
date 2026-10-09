/* The JSON reader (slice C1). See read.h.

   The reader takes JSON text in ASCII: objects, arrays, strings, integers,
   true, false and null, with white space between the tokens. The writer
   writes only these (src/json.c: names and printed types are ASCII, and a
   Nat or a Rat part is an integer). */
#include <stdio.h>
#include <string.h>

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
