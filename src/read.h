/* The JSON reader (slice C1). The tcc-json kit only (not shared with tcc-wasm).

   `langc read DOC` reads a document of `langc build` (format version 1):
   {"LANG":1,"instances":[{"name","type","value"}]}. read_document parses it
   into a tree in the arena, and read_write writes the tree again in the
   format of src/json.c. Thus each document of the writer reads and writes
   back with no change. This slice does not check a value against its type. */
#ifndef LANG_READ_H
#define LANG_READ_H

#include <stddef.h>

#include "json.h"

/* 16 MiB. The largest JSON golden is 5,587 bytes. */
#define READ_MAX_BYTES ((size_t)1 << 24)

/* The writer nests at most JSON_DEPTH_LIMIT + 5 arrays and objects: the
   document, the instance list and the instance, 1,999 value levels, then a
   matrix (the rows, a row and a cell). The reader permits 3 more. */
#define READ_DEPTH_LIMIT (JSON_DEPTH_LIMIT + 8u)

typedef enum {
  JSON_NULL,
  JSON_FALSE,
  JSON_TRUE,
  JSON_NUMBER,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT
} JsonKind;

/* A node of the tree. A number keeps its text. A string keeps its bytes
   after the escapes are decoded. A member of an object also has a key. */
typedef struct JsonNode JsonNode;
struct JsonNode {
  JsonKind kind;
  const char *text; /* a number or a string */
  size_t len;
  const char *key; /* a member of an object */
  size_t key_len;
  JsonNode *first; /* the first item of an array or member of an object */
  JsonNode *next;  /* the next item or member */
};

/* Parses the LEN bytes of TEXT. Returns 1 and sets DOC, or 0 after a
   diagnostic: READ_SIZE, READ_SYNTAX, READ_DEPTH, READ_VERSION, READ_SHAPE
   or OOM. */
int read_document(Arena *arena, const char *text, size_t len, const JsonNode **doc, Diag *diag);

/* Writes DOC in the format of src/json.c, with a final newline. Returns 1
   and sets TEXT and LEN, or 0 after a diagnostic (OOM). */
int read_write(Arena *arena, const JsonNode *doc, const char **text, size_t *len, Diag *diag);

#endif
