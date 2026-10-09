/* The langc command line of the tcc-json kit (forked from tcc-wasm).
   Exit 0: ok. Exit 1: the program is refused. Exit 2: usage or IO. */
#include <stdio.h>
#include <string.h>

#include "front/check.h"
#include "front/front.h"
#include "json.h"
#include "read.h"

#define ARENA_LIMIT_BYTES ((size_t)1 << 30)

typedef enum {
  CMD_CHECK,
  CMD_EVAL,
  CMD_BUILD,
  CMD_READ
} Command;

typedef struct {
  Command command;
  const char *prog_path;
  const char *entry;
  char **args;
  int arg_count;
  const char *out_path; /* NULL: stdout */
  int json;             /* read --json: write the parsed tree (slice C1) */
  const char *read_path; /* eval or build --read DOC (slice C3); NULL: none */
} Options;

static const struct {
  const char *word;
  Command command;
} COMMANDS[] = {
  {"check", CMD_CHECK},
  {"eval", CMD_EVAL},
  {"build", CMD_BUILD},
  {"read", CMD_READ},
};

static int usage(FILE *err) {
  fputs("usage: langc check PROG\n"
        "       langc eval PROG [--read DOC] NAME [ARGS...]\n"
        "       langc build PROG [--read DOC] [-o OUT]\n"
        "       langc read [--json] DOC\n", err);
  return 2;
}

static int find_command(const char *word, Command *out) {
  for (size_t i = 0; i < sizeof COMMANDS / sizeof COMMANDS[0]; i++) {
    if (strcmp(COMMANDS[i].word, word) == 0) {
      *out = COMMANDS[i].command;
      return 1;
    }
  }
  return 0;
}

/* AT is the index of the first word after PROG and its --read DOC. */
static int parse_build_options(int argc, char **argv, int at, Options *opt, Diag *diag) {
  if (argc == at) return 1;
  if (argc == at + 2 && strcmp(argv[at], "-o") == 0) {
    opt->out_path = argv[at + 1];
    return 1;
  }
  return diag_fail(diag, "USAGE", NULL, "build takes PROG, an optional --read DOC and an optional -o OUT");
}

/* `--read DOC` is valid only as the 2 words right after PROG, so the ARGS of
   eval keep every later word. Returns the index of the next word. */
static int parse_read_option(int argc, char **argv, Options *opt) {
  if (argc >= 5 && strcmp(argv[3], "--read") == 0) {
    opt->read_path = argv[4];
    return 5;
  }
  return 3;
}

static int parse_options(int argc, char **argv, Options *opt, Diag *diag) {
  memset(opt, 0, sizeof *opt);
  if (argc < 3 || !find_command(argv[1], &opt->command)) {
    return diag_fail(diag, "USAGE", NULL, "expected a command and a program path");
  }
  opt->prog_path = argv[2];
  switch (opt->command) {
    case CMD_CHECK:
      return argc == 3 ? 1 : diag_fail(diag, "USAGE", NULL, "too many arguments");
    case CMD_EVAL: {
      int at = parse_read_option(argc, argv, opt);
      if (argc <= at || strcmp(argv[at], "--read") == 0) return diag_fail(diag, "USAGE", NULL, "eval needs a definition name");
      opt->entry = argv[at];
      opt->args = argv + at + 1;
      opt->arg_count = argc - at - 1;
      return 1;
    }
    case CMD_BUILD:
      return parse_build_options(argc, argv, parse_read_option(argc, argv, opt), opt, diag);
    case CMD_READ:
      if (argc == 4 && strcmp(argv[2], "--json") == 0) {
        opt->json = 1;
        opt->prog_path = argv[3];
        return 1;
      }
      return argc == 3 ? 1 : diag_fail(diag, "USAGE", NULL, "read takes an optional --json and one DOC");
  }
  return 0;
}

/* Reads at most LIMIT + 1 bytes, so *LEN > LIMIT tells that the file is larger. */
static int read_file(Arena *arena, const char *path, size_t limit, char **out, size_t *len, Diag *diag) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) return diag_fail(diag, "IO", NULL, "cannot open %s", path);
  char *buf = arena_alloc(arena, limit + 1);
  size_t count = buf == NULL ? 0 : fread(buf, 1, limit + 1, file);
  int read_error = ferror(file);
  fclose(file);
  if (buf == NULL) return diag_fail(diag, "OOM", NULL, "no memory for %s", path);
  if (read_error) return diag_fail(diag, "IO", NULL, "cannot read %s", path);
  *out = buf;
  *len = count;
  return 1;
}

static int read_source(Arena *arena, const char *path, char **out, size_t *len, Diag *diag) {
  if (!read_file(arena, path, SOURCE_MAX_BYTES, out, len, diag)) return 0;
  if (*len > SOURCE_MAX_BYTES) return diag_fail(diag, "IO_SIZE", NULL, "%s is larger than %zu bytes", path, SOURCE_MAX_BYTES);
  return 1;
}

/* Writes the document only after it is complete, so a failure leaves no file. */
static int write_output(const char *path, const char *text, size_t len, Diag *diag) {
  FILE *file = path == NULL ? stdout : fopen(path, "wb");
  if (file == NULL) return diag_fail(diag, "IO", NULL, "cannot open %s", path);
  size_t count = fwrite(text, 1, len, file);
  int closed = path == NULL ? fflush(file) : fclose(file);
  return count == len && closed == 0 ? 1 : diag_fail(diag, "IO", NULL, "cannot write %s", path == NULL ? "stdout" : path);
}

/* langc read [--json] DOC: the document is not a program. READ_SIZE is a
   refusal (exit 1). With --json, the parsed tree is written again (slice C1).
   Otherwise each instance is checked and printed as `name : type = value`. */
static int run_read(const char *path, int json, Arena *arena, Diag *diag) {
  char *text = NULL;
  size_t len = 0;
  if (!read_file(arena, path, READ_MAX_BYTES, &text, &len, diag)) return 2;
  const JsonNode *doc = NULL;
  const char *out = NULL;
  size_t out_len = 0;
  if (!read_document(arena, text, len, &doc, diag)) return 1;
  int ok = json ? read_write(arena, doc, &out, &out_len, diag) : read_typed(arena, doc, &out, &out_len, diag);
  if (!ok) return 1;
  return write_output(NULL, out, out_len, diag) ? 0 : 2;
}

/* --read DOC: decodes each instance of DOC against the domain (in DOMAIN), for
   check_program_reads. Returns the exit status: 0, 1 (refused) or 2 (IO). */
static int load_reads(const char *path, Arena *arena, Machine *domain, const ReadDef **reads, size_t *count, Diag *diag) {
  char *text = NULL;
  size_t len = 0;
  const JsonNode *doc = NULL;
  if (!read_file(arena, path, READ_MAX_BYTES, &text, &len, diag)) return 2;
  return read_document(arena, text, len, &doc, diag) && read_defs(arena, doc, domain, reads, count, diag) ? 0 : 1;
}

static int run(const Options *opt, Arena *arena, Diag *diag) {
  if (opt->command == CMD_READ) return run_read(opt->prog_path, opt->json, arena, diag);
  char *text = NULL;
  size_t len = 0;
  if (!read_source(arena, opt->prog_path, &text, &len, diag)) return 2;
  Machine domain;
  const ReadDef *reads = NULL;
  size_t read_count = 0;
  int status = opt->read_path == NULL ? 0 : load_reads(opt->read_path, arena, &domain, &reads, &read_count, diag);
  if (status != 0) return status;
  DeclList decls;
  if (!front_load(arena, opt->prog_path, text, len, &decls, diag)) return 1;
  Machine machine;
  if (!check_program_reads(arena, &decls, reads, read_count, &machine, diag)) return 1;
  switch (opt->command) {
    case CMD_CHECK:
      puts("ok");
      return 0;
    case CMD_EVAL:
      return eval_command(&machine, opt->entry, opt->args, opt->arg_count, stdout, stderr);
    case CMD_BUILD: {
      const char *doc = NULL;
      size_t doc_len = 0;
      if (!json_document(&machine, &doc, &doc_len)) return 1;
      return write_output(opt->out_path, doc, doc_len, diag) ? 0 : 2;
    }
    case CMD_READ:
      break; /* run_read, above */
  }
  return 1;
}

int main(int argc, char **argv) {
  Diag diag;
  diag_init(&diag);
  Options opt;
  if (!parse_options(argc, argv, &opt, &diag)) {
    diag_print(&diag, stderr);
    return usage(stderr);
  }
  Arena arena;
  arena_init(&arena, ARENA_LIMIT_BYTES);
  int status = run(&opt, &arena, &diag);
  arena_release(&arena);
  if (status != 0) diag_print(&diag, stderr);
  return status;
}
