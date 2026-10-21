#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

#include "front/front.h"
#include "front/check.h"
#include "json.h"
#include "runtime/driver.h"

static int cases_run = 0;

/* Exercise append with an exact-size input, so sanitizers can detect reads
   beyond the first string even when its final contents would be correct. */
static int append_bounds(void) {
  Arena arena;
  Machine m;
  Diag diag;
  const Value *append;
  const Value *v;
  char first[2] = "a";
  const char second[] = "012345678901234567890123456789";
  DeclList decls;
  const char *source = "def append : Text -> Text -> Text := fun (a : Text) (b : Text) => textAppend a b\n";
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  diag_init(&diag);
  ok = front_load(&arena, "append-bounds", source, strlen(source), &decls, &diag)
    && check_program(&arena, &decls, &m, &diag);
  append = ok ? def_value(&m, m.def_count - 1u) : NULL;
  v = append == NULL ? NULL : apply_value(&m, apply_value(&m, append, val_text(&m, first, 1u)), val_text(&m, second, sizeof second - 1u));
  ok = v != NULL && v->kind == VAL_TEXT && v->nat == sizeof second
    && v->name[0] == 'a' && memcmp(v->name + 1, second, sizeof second) == 0;
  if (!ok) fprintf(stderr, "FAIL append reads only the input lengths\n");
  arena_release(&arena);
  return ok;
}

/* Replacement domains must have the field types that jsonParse constructs. */
static int json_domain_shape(const char *text_type, const char *number_type, const char *bool_type, const char *extra, const char *code) {
  Arena arena;
  Machine m;
  Diag diag;
  DeclList decls = {0};
  TokenList tokens;
  char domain[1024];
  const char *source = "def r : Sum Text Json := jsonParse \"\\\"x\\\"\"\n";
  int ok;
  cases_run++;
  snprintf(domain, sizeof domain,
    "family Json := jnull | jbool (jboolValue : %s) | jnum (jnumValue : %s) | jstr (jstrValue : %s) | jarr (jarrItems : JsonList) | jobj (jobjFields : JsonFields) %s\n"
    "and JsonList := jlNil | jlCons (jlHead : Json) (jlTail : JsonList)\n"
    "and JsonFields := jfNil | jfCons (jfKey : Text) (jfValue : Json) (jfTail : JsonFields)\n",
    bool_type, number_type, text_type, extra);
  arena_init(&arena, (size_t)128 << 20);
  diag_init(&diag);
  ok = lex_source(&arena, "json-domain", domain, strlen(domain), &tokens, &diag)
    && parse_source(&arena, "json-domain", ORIGIN_DOMAIN, &tokens, &decls, &diag)
    && lex_source(&arena, "json-program", source, strlen(source), &tokens, &diag)
    && parse_source(&arena, "json-program", ORIGIN_PROGRAM, &tokens, &decls, &diag)
    && check_program(&arena, &decls, &m, &diag);
  ok = code == NULL ? ok : !ok && diag.set && strcmp(diag.code, code) == 0;
  if (!ok) {
    fprintf(stderr, "FAIL Json schema: Text=%s, number=%s, Bool=%s, extra=%s\n", text_type, number_type, bool_type, extra);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* Use a replacement domain, so the embedded IO helpers cannot hide a bad
   signature or supply a helper that the replacement omitted. */
static int io_domain_case(const char *label, const char *helper, const char *source, const char *code) {
  Arena arena;
  Machine m;
  Diag diag;
  DeclList decls = {0};
  TokenList tokens;
  const char *domain =
    "family IOF (A : Type 0) (X : Type 0) := done (result : A)\n"
    "cofamily IO (A : Type 0) := IOF A\n"
    "def makeIO : (A : Type 0) -> A -> IO A := fun (A : Type 0) (a : A) => unfold (fun (s : A) => done A A s) a\n"
    "def base : IO Nat := makeIO Nat 7\n";
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  diag_init(&diag);
  ok = lex_source(&arena, label, domain, strlen(domain), &tokens, &diag)
    && parse_source(&arena, label, ORIGIN_DOMAIN, &tokens, &decls, &diag)
    && lex_source(&arena, label, helper, strlen(helper), &tokens, &diag)
    && parse_source(&arena, label, ORIGIN_DOMAIN, &tokens, &decls, &diag)
    && lex_source(&arena, label, source, strlen(source), &tokens, &diag)
    && parse_source(&arena, label, ORIGIN_PROGRAM, &tokens, &decls, &diag)
    && check_program(&arena, &decls, &m, &diag);
  ok = code == NULL ? ok : !ok && diag.set && strcmp(diag.code, code) == 0;
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int checked(Arena *arena, Machine *m, Diag *diag, const char *source) {
  DeclList decls;
  diag_init(diag);
  return front_load(arena, "path-regression", source, strlen(source), &decls, diag)
      && check_program(arena, &decls, m, diag);
}

static int io_deep_type(void) {
  Arena arena;
  Machine m;
  Diag diag;
  char source[4096] = "def T : Type 0 := ";
  unsigned i;
  int ok;
  cases_run++;
  for (i = 0; i < 240u; i++) strcat(source, "Unit -> ");
  strcat(source,
    "Nat\n"
    "def good : T -> Nat := fun (x : T) => fold (fun t => 0) (fun t => 0) (fun k => 0) (fun w n => 0) (fun k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p k => 0) (fun p q k => 0) (fun p q k => 0) (fun p k => 0) (fun p j k => 0) (out (map (fun (n : Nat) => x) (ioPure Nat 0)))\n");
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag, source);
  if (!ok) {
    fprintf(stderr, "FAIL IO: a valid inferred type can exceed the diagnostic printer depth\n");
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* The trash and EXDEV fields of the IoEnv of a run. A write row sets them,
   then sets the defaults back. The default trash root is under build/fs/pr/
   (S6 Q7 (a)). */
#define PR_TRASH_ROOT "build/fs/pr/trash"
static const char *pr_trash_root = PR_TRASH_ROOT;
static TrashForm pr_trash_form = TRASH_MACOS;
static int pr_force_exdev = 0;

/* Runs SOURCE in the IO driver: gives the status (-1 when the run did not
   start), *BYTES (malloc'd, *LEN bytes and a NUL after them; NULL when
   READ_ONLY) and *DIAG. */
static int io_driver_exec(const char *source, const char *input, int argc, char *const *argv, uint64_t budget,
                          int read_only, Diag *diag, char **bytes, size_t *len) {
  Arena arena;
  Machine m;
  uint32_t index = 0;
  int got = -1;
  FILE *in;
  FILE *stream;
  *bytes = NULL;
  *len = 0;
  diag_init(diag);
  arena_init(&arena, (size_t)128 << 20);
  /* fmemopen of size 0 can fail, thus an empty input is /dev/null. */
  in = input[0] ? fmemopen((void *)input, strlen(input), "r") : fopen("/dev/null", "r");
  stream = read_only ? fopen("/dev/null", "r") : open_memstream(bytes, len);
  if (in && stream && checked(&arena, &m, diag, source) && io_main(&m, &index)) {
    IoEnv env = {budget, in, stream, argc, argv, pr_trash_root, pr_trash_form, pr_force_exdev};
    got = io_run(&m, def_value(&m, index), &env);
  }
  if (in) fclose(in);
  if (stream) fclose(stream);
  arena_release(&arena);
  return got;
}

/* Runs main of SOURCE with the IO driver (S5 chunk B). INPUT is the
   standard input. READ_ONLY gives an output stream that refuses each
   write: /dev/null for reading (macOS fmemopen in mode "r" takes
   writes). Compares the status, the output bytes and the diagnostic code
   (NULL: no diagnostic). */
static int io_driver_run(const char *label, const char *source, const char *input, int argc, char *const *argv,
                         uint64_t budget, int read_only, const char *out, int status, const char *code) {
  Diag diag;
  char *bytes = NULL;
  size_t len = 0;
  int got = io_driver_exec(source, input, argc, argv, budget, read_only, &diag, &bytes, &len);
  int ok = got == status && (code ? diag.set && strcmp(diag.code, code) == 0 : !diag.set)
    && len == strlen(out) && (len == 0 || memcmp(bytes, out, len) == 0);
  if (!ok) {
    fprintf(stderr, "FAIL IO driver: %s: status %d, %zu bytes \"%.*s\"\n", label, got, len, (int)len, bytes ? bytes : "");
    diag_print(&diag, stderr);
  }
  free(bytes);
  return ok;
}

static int io_driver_case(const char *label, const char *source, const char *input, int argc, char *const *argv,
                          uint64_t budget, const char *out, int status, const char *code) {
  cases_run++;
  return io_driver_run(label, source, input, argc, argv, budget, 0, out, status, code);
}

/* The write gives the exact bytes, and a write error is IO_WRITE. */
static int io_write_bytes(void) {
  const char *source = "def main : IO Unit := write \"a\\tb\\n\"\n";
  int exact;
  int refused;
  cases_run++;
  exact = io_driver_run("write bytes", source, "", 0, NULL, EVAL_FUEL_STEPS, 0, "a\tb\n", 0, NULL);
  refused = io_driver_run("write error", source, "", 0, NULL, EVAL_FUEL_STEPS, 1, "", 1, "IO_WRITE");
  return exact && refused;
}

/* Reads one line and writes it with a bar, or writes none at the end of
   the input. */
static const char io_echo[] =
  "def main : IO Unit := bind readLine (fun (r : Option Text) => option (write \"none\") (fun (t : Text) => write (textAppend t \"|\")) r)\n";
/* Echoes each line until the end of the input. One round takes at most 370
   fuel at S7 (290 at S5, 330 at S6a, 362 at S6b: each IOF constructor adds
   8 to the folds of a round). */
static const char io_echo_loop[] =
  "def main : IO Unit := ioLoop Unit Unit (fun (u : Unit) => bind readLine (fun (r : Option Text) => option (ioPure (Sum Unit Unit) (inl unit)) (fun (t : Text) => ioMap Unit (Sum Unit Unit) (fun (x : Unit) => inr x) (write (textAppend t \"\\n\"))) r)) unit\n";

/* 100 lines at a budget of 1000: the budget is for each segment. */
static int io_segment_budget(void) {
  char input[301] = "";
  unsigned i;
  for (i = 0; i < 100u; i++) strcat(input, "ab\n");
  return io_driver_case("a small budget for each segment", io_echo_loop, input, 0, NULL, 1000, input, 0, NULL);
}

/* 1000 rounds of ioLoop at a budget of 380: the cost of a segment does not
   grow with the rounds (S5 chunk C). */
static int io_loop_rounds(void) {
  static char input[3001];
  unsigned i;
  input[0] = '\0';
  for (i = 0; i < 1000u; i++) strcat(input, "ab\n");
  return io_driver_case("ioLoop of 1000 rounds", io_echo_loop, input, 0, NULL, 380, input, 0, NULL);
}

/* The filesystem rows (S6a chunk B). The runner runs from the repo root,
   thus each guest path is relative, under build/fs/pr/ (S6 Q13 (a)). */
extern char **environ;

typedef enum { FS_DIR, FS_FILE, FS_LINK, FS_FIFO } FsForm;

/* An item of a tree (kickoff 1.5): PATH under the root, the FORM, the
   LENGTH bytes at BYTES of a file (a file can hold a NUL byte), the
   TARGET of a link, and the MODE (0 keeps the default). */
typedef struct {
  const char *path;
  FsForm form;
  const char *bytes;
  size_t length;
  const char *target;
  mode_t mode;
} FsItem;

/* A fresh build/fs/pr/: /bin/rm -rf (as terminal.c), then mkdir. The
   umask is 022 for each fs row (kickoff section 5: the copy mode row). */
static int fs_reset(void) {
  static char *const argv[] = {"/bin/rm", "-rf", "build/fs/pr", NULL};
  pid_t pid;
  int status = 0;
  int ok;
  umask(022);
  ok = posix_spawn(&pid, argv[0], NULL, NULL, argv, environ) == 0 && waitpid(pid, &status, 0) == pid
    && WIFEXITED(status) && WEXITSTATUS(status) == 0;
  ok = ok && (mkdir("build/fs", 0777) == 0 || errno == EEXIST) && mkdir("build/fs/pr", 0777) == 0;
  if (!ok) fprintf(stderr, "FAIL fs reset: build/fs/pr\n");
  return ok;
}

/* Makes ROOT/PATH of ITEM. */
static int fs_item(const char *root, const FsItem *item) {
  char path[512];
  FILE *file = NULL;
  int ok = snprintf(path, sizeof path, "%s/%s", root, item->path) < (int)sizeof path;
  switch (item->form) {
  case FS_DIR:
    ok = ok && mkdir(path, 0777) == 0;
    break;
  case FS_FILE:
    file = ok ? fopen(path, "wb") : NULL;
    ok = file != NULL && fwrite(item->bytes, 1, item->length, file) == item->length;
    ok = file != NULL && fclose(file) == 0 && ok;
    break;
  case FS_LINK:
    ok = ok && symlink(item->target, path) == 0;
    break;
  case FS_FIFO:
    ok = ok && mkfifo(path, 0666) == 0;
    break;
  }
  ok = ok && (item->mode == 0 || chmod(path, item->mode) == 0);
  if (!ok) fprintf(stderr, "FAIL fs tree: %s\n", path);
  return ok;
}

/* Makes the directory ROOT, then each item in the table order (a
   directory before its items). */
static int fs_tree(const char *root, const FsItem *items, size_t count) {
  int ok = mkdir(root, 0777) == 0;
  size_t i;
  if (!ok) fprintf(stderr, "FAIL fs tree: %s\n", root);
  for (i = 0; ok && i < count; i++) ok = fs_item(root, &items[i]);
  return ok;
}

/* The tree of the read rows, made once. */
#define FS_R "build/fs/pr/read"
/* The LENGTH bytes of a literal file text. */
#define FS_BYTES(text) text, sizeof text - 1u
/* late.dat: 8200 bytes x, a \n, then z NUL apple \n: the NUL byte is
   after the 8 KiB test of search. */
#define FS_LATE_X 8200u
static char fs_late[FS_LATE_X + 9u];

static const FsItem fs_read_items[] = {
  {"ls", FS_DIR, NULL, 0, NULL, 0},
  {"ls/.dot", FS_FILE, FS_BYTES(""), NULL, 0},
  {"ls/Zeta", FS_FILE, FS_BYTES(""), NULL, 0},
  {"ls/alpha", FS_DIR, NULL, 0, NULL, 0},
  {"ls/beta", FS_LINK, NULL, 0, "Zeta", 0},
  {"ls/pipe", FS_FIFO, NULL, 0, NULL, 0},
  {"empty", FS_DIR, NULL, 0, NULL, 0},
  {"walk", FS_DIR, NULL, 0, NULL, 0},
  {"walk/a.c", FS_FILE, FS_BYTES(""), NULL, 0},
  {"walk/.b.c", FS_FILE, FS_BYTES(""), NULL, 0},
  {"walk/sub-x.c", FS_FILE, FS_BYTES(""), NULL, 0},
  {"walk/sub", FS_DIR, NULL, 0, NULL, 0},
  {"walk/sub/c.c", FS_FILE, FS_BYTES(""), NULL, 0},
  {"walk/sub/d.h", FS_FILE, FS_BYTES(""), NULL, 0},
  {"walk/sub/deep", FS_DIR, NULL, 0, NULL, 0},
  {"walk/sub/deep/e.c", FS_FILE, FS_BYTES(""), NULL, 0},
  {"walk/link", FS_LINK, NULL, 0, "sub", 0},
  {"walk-link", FS_LINK, NULL, 0, "walk/sub", 0},
  {"files", FS_DIR, NULL, 0, NULL, 0},
  {"files/nul.bin", FS_FILE, FS_BYTES("a\0b\nc"), NULL, 0},
  {"files/empty.txt", FS_FILE, FS_BYTES(""), NULL, 0},
  {"text", FS_DIR, NULL, 0, NULL, 0},
  {"text/order", FS_DIR, NULL, 0, NULL, 0},
  {"text/order/a.txt", FS_FILE, FS_BYTES("apple\n"), NULL, 0},
  {"text/order/b.txt", FS_FILE, FS_BYTES("one apple\ntwo\napple pie\n"), NULL, 0},
  {"text/order/sub", FS_DIR, NULL, 0, NULL, 0},
  {"text/order/sub/c.txt", FS_FILE, FS_BYTES("x\nred apple\r\n"), NULL, 0},
  {"text/order/l.txt", FS_LINK, NULL, 0, "a.txt", 0},
  {"text-link", FS_LINK, NULL, 0, "text/order", 0},
  {"dangling-link", FS_LINK, NULL, 0, "none", 0},
  {"text/bin", FS_DIR, NULL, 0, NULL, 0},
  {"text/bin/bin.dat", FS_FILE, FS_BYTES("apple\0"), NULL, 0},
  {"text/bin/ok.txt", FS_FILE, FS_BYTES("apple"), NULL, 0},
  {"text/late", FS_DIR, NULL, 0, NULL, 0},
  {"text/late/late.dat", FS_FILE, fs_late, sizeof fs_late, NULL, 0},
};

/* The guest text of an fs row (kickoff row S6a B) into SOURCE: CALL, then
   "inl " and the text of an inl, or the jsonRender of the inr value (a
   Text value as a jstr when TEXT). Gives 0 when SOURCE is too small. */
static int fs_source(char *source, size_t cap, const char *call, int text) {
  const char *type = text ? "Text" : "Json";
  int len = snprintf(source, cap,
    "def main : IO Unit := bind (%s) (fun (r : Sum Text %s) => either (fun (e : Text) => write (textAppend \"inl \" e))"
    " (fun (j : %s) => write (jsonRender %s)) r)\n", call, type, type, text ? "(jstr j)" : "j");
  return len >= 0 && (size_t)len < cap;
}

static int fs_case(const char *label, const char *call, int text, const char *out) {
  char source[1024];
  if (!fs_source(source, sizeof source, call, text)) {
    cases_run++;
    fprintf(stderr, "FAIL fs row: %s: the source is too long\n", label);
    return 0;
  }
  return io_driver_case(label, source, "", 0, NULL, EVAL_FUEL_STEPS, out, 0, NULL);
}

/* 1 when A and B differ by at most 64 MiB (the free bytes drift between
   two calls, kickoff section 5). */
static int fs_near(uint64_t a, uint64_t b) {
  return (a > b ? a - b : b - a) <= (uint64_t)64 << 20;
}

/* diskUsage of PATH: the PLAN key order, total equal to the runner's own
   statvfs, and free and available within 64 MiB of it. */
static int fs_disk_usage(const char *label, const char *path) {
  char call[256];
  char source[1024];
  Diag diag;
  struct statvfs s;
  char *bytes = NULL;
  size_t len = 0;
  unsigned long long total = 0;
  unsigned long long spare = 0;
  unsigned long long avail = 0;
  int used = -1;
  int got = -1;
  int ok;
  cases_run++;
  diag_init(&diag);
  ok = snprintf(call, sizeof call, "diskUsage \"%s\"", path) < (int)sizeof call && fs_source(source, sizeof source, call, 0);
  got = ok ? io_driver_exec(source, "", 0, NULL, EVAL_FUEL_STEPS, 0, &diag, &bytes, &len) : -1;
  ok = got == 0 && !diag.set && bytes != NULL && statvfs(path, &s) == 0
    && sscanf(bytes, "{\"total\":%llu,\"free\":%llu,\"available\":%llu}%n", &total, &spare, &avail, &used) == 3
    && used >= 0 && (size_t)used == len && total == (uint64_t)s.f_blocks * (uint64_t)s.f_frsize
    && fs_near(spare, (uint64_t)s.f_bfree * (uint64_t)s.f_frsize)
    && fs_near(avail, (uint64_t)s.f_bavail * (uint64_t)s.f_frsize);
  if (!ok) {
    fprintf(stderr, "FAIL IO driver: %s: status %d, %zu bytes \"%.*s\"\n", label, got, len, (int)len, bytes ? bytes : "");
    diag_print(&diag, stderr);
  }
  free(bytes);
  return ok;
}

/* 3 operations, then write: the fuel is the budget again after each
   operation, thus a budget for one segment holds the run. */
static const char fs_three_ops[] =
  "def main : IO Unit := bind (diskUsage \"" FS_R "\") (fun (a : Sum Text Json) => bind (readFile \"" FS_R "/files/empty.txt\")"
  " (fun (b : Sum Text Text) => bind (listDir \"" FS_R "/empty\") (fun (c : Sum Text Json) => write \"ok\")))\n";
/* The budget of the 3 operation row: the probe minimum + 10 (STATUS D-row). */
#define FS_SEGMENT_BUDGET 305

/* The 33 read rows over one tree (kickoff row S6a B). Gives the count of
   the failed rows (and 1 more when the tree fails). */
static int fs_read_rows(void) {
  int failed = 0;
  memset(fs_late, 'x', FS_LATE_X);
  memcpy(fs_late + FS_LATE_X, "\nz\0apple\n", 9u);
  failed += !(fs_reset() && fs_tree(FS_R, fs_read_items, sizeof fs_read_items / sizeof fs_read_items[0]));
  failed += !fs_case("listDir sorted", "listDir \"" FS_R "/ls\"", 0,
    "[{\"name\":\".dot\",\"kind\":\"file\"},{\"name\":\"Zeta\",\"kind\":\"file\"},{\"name\":\"alpha\",\"kind\":\"dir\"},"
    "{\"name\":\"beta\",\"kind\":\"link\"},{\"name\":\"pipe\",\"kind\":\"other\"}]");
  failed += !fs_case("listDir empty directory", "listDir \"" FS_R "/empty\"", 0, "[]");
  failed += !fs_case("listDir missing path", "listDir \"" FS_R "/none\"", 0,
    "inl listDir: " FS_R "/none: No such file or directory");
  failed += !fs_case("listDir file", "listDir \"" FS_R "/ls/Zeta\"", 0, "inl listDir: " FS_R "/ls/Zeta: Not a directory");
  failed += !fs_case("listDir root link not walked", "listDir \"" FS_R "/walk-link\"", 0,
    "inl listDir: " FS_R "/walk-link: Not a directory");
  failed += !fs_case("listDir NUL byte", "listDir \"" FS_R "/ls\\x00\"", 0, "inl listDir: the path holds a NUL byte");
  failed += !fs_case("find nested sorted", "find \"" FS_R "/walk/sub\" \"*\"", 0,
    "[\"" FS_R "/walk/sub/c.c\",\"" FS_R "/walk/sub/d.h\",\"" FS_R "/walk/sub/deep\",\"" FS_R "/walk/sub/deep/e.c\"]");
  failed += !fs_case("find link not walked", "find \"" FS_R "/walk\" \"c.c\"", 0, "[\"" FS_R "/walk/sub/c.c\"]");
  failed += !fs_case("find root link not walked", "find \"" FS_R "/walk-link\" \"*\"", 0,
    "inl find: " FS_R "/walk-link: Not a directory");
  failed += !fs_case("find root link slash not walked", "find \"" FS_R "/walk-link/\" \"*\"", 0,
    "inl find: " FS_R "/walk-link/: Not a directory");
  failed += !fs_case("find dot name", "find \"" FS_R "/walk\" \"*.c\"", 0,
    "[\"" FS_R "/walk/.b.c\",\"" FS_R "/walk/a.c\",\"" FS_R "/walk/sub-x.c\",\"" FS_R "/walk/sub/c.c\",\""
    FS_R "/walk/sub/deep/e.c\"]");
  failed += !fs_case("find no match", "find \"" FS_R "/walk\" \"*.zip\"", 0, "[]");
  failed += !fs_case("find missing root", "find \"" FS_R "/none\" \"*\"", 0,
    "inl find: " FS_R "/none: No such file or directory");
  failed += !fs_case("readFile NUL", "readFile \"" FS_R "/files/nul.bin\"", 1, "\"a\\u0000b\\u000ac\"");
  failed += !fs_case("readFile empty file", "readFile \"" FS_R "/files/empty.txt\"", 1, "\"\"");
  failed += !fs_case("readFile follows file link", "readFile \"" FS_R "/text/order/l.txt\"", 1, "\"apple\\u000a\"");
  failed += !fs_case("readFile directory", "readFile \"" FS_R "/files\"", 1, "inl readFile: " FS_R "/files: Is a directory");
  failed += !fs_case("readFile missing file", "readFile \"" FS_R "/files/none\"", 1,
    "inl readFile: " FS_R "/files/none: No such file or directory");
  failed += !fs_case("search order", "search \"" FS_R "/text/order\" \"apple\"", 0,
    "[{\"path\":\"" FS_R "/text/order/a.txt\",\"line\":1,\"text\":\"apple\"},"
    "{\"path\":\"" FS_R "/text/order/b.txt\",\"line\":1,\"text\":\"one apple\"},"
    "{\"path\":\"" FS_R "/text/order/b.txt\",\"line\":3,\"text\":\"apple pie\"},"
    "{\"path\":\"" FS_R "/text/order/sub/c.txt\",\"line\":2,\"text\":\"red apple\\u000d\"}]");
  failed += !fs_case("search binary", "search \"" FS_R "/text/bin\" \"apple\"", 0,
    "[{\"path\":\"" FS_R "/text/bin/ok.txt\",\"line\":1,\"text\":\"apple\"}]");
  failed += !fs_case("search NUL after 8 KiB", "search \"" FS_R "/text/late\" \"apple\"", 0,
    "[{\"path\":\"" FS_R "/text/late/late.dat\",\"line\":2,\"text\":\"z\\u0000apple\"}]");
  failed += !fs_case("search empty text", "search \"" FS_R "/text/order\" \"\"", 0,
    "inl search: " FS_R "/text/order: the search text is empty");
  failed += !fs_case("search root directory link skipped", "search \"" FS_R "/text-link\" \"apple\"", 0, "[]");
  failed += !fs_case("search root directory link slash skipped", "search \"" FS_R "/text-link/\" \"apple\"", 0, "[]");
  failed += !fs_case("search root file link skipped", "search \"" FS_R "/text/order/l.txt\" \"apple\"", 0, "[]");
  failed += !fs_case("search dangling root link skipped", "search \"" FS_R "/dangling-link\" \"apple\"", 0, "[]");
  failed += !fs_case("search file root", "search \"" FS_R "/text/order/b.txt\" \"apple\"", 0,
    "[{\"path\":\"" FS_R "/text/order/b.txt\",\"line\":1,\"text\":\"one apple\"},"
    "{\"path\":\"" FS_R "/text/order/b.txt\",\"line\":3,\"text\":\"apple pie\"}]");
  failed += !fs_case("search missing root", "search \"" FS_R "/none\" \"apple\"", 0,
    "inl search: " FS_R "/none: No such file or directory");
  failed += !fs_disk_usage("diskUsage values", FS_R);
  failed += !fs_disk_usage("diskUsage file path", FS_R "/files/empty.txt");
  failed += !fs_case("diskUsage missing path", "diskUsage \"" FS_R "/none\"", 0,
    "inl diskUsage: " FS_R "/none: No such file or directory");
  failed += !io_driver_case("layer listDir then write",
    "def main : IO Unit := bind (listDir \"" FS_R "/empty\") (fun (r : Sum Text Json) => bind (write \"x\") (fun (u : Unit) =>"
    " either (fun (e : Text) => write (textAppend \"inl \" e)) (fun (j : Json) => write (jsonRender j)) r))\n",
    "", 0, NULL, EVAL_FUEL_STEPS, "x[]", 0, NULL);
  failed += !io_driver_case("layer 3 operations at one segment", fs_three_ops, "", 0, NULL, FS_SEGMENT_BUDGET, "ok", 0, NULL);
  return failed;
}

/* The guest text of a write row (kickoff row S6b B) into SOURCE: CALL,
   then "inl " and the text of an inl, or "unit" for the inr. Gives 0 when
   SOURCE is too small. */
static int fs_unit_source(char *source, size_t cap, const char *call) {
  int len = snprintf(source, cap,
    "def main : IO Unit := bind (%s) (fun (r : Sum Text Unit) => either (fun (e : Text) => write (textAppend \"inl \" e))"
    " (fun (u : Unit) => write \"unit\") r)\n", call);
  return len >= 0 && (size_t)len < cap;
}

static int fs_write_case(const char *label, const char *call, const char *out) {
  char source[1024];
  if (!fs_unit_source(source, sizeof source, call)) {
    cases_run++;
    fprintf(stderr, "FAIL fs row: %s: the source is too long\n", label);
    return 0;
  }
  return io_driver_case(label, source, "", 0, NULL, EVAL_FUEL_STEPS, out, 0, NULL);
}

/* The tree checks of the write rows. Each check prints a FAIL line and
   adds no case. */

/* PATH is there with the FORM (lstat: a link is not followed). */
static int fs_is(const char *label, const char *path, FsForm form) {
  static const char *const names[] = {"directory", "file", "link", "fifo"};
  struct stat st;
  int ok = lstat(path, &st) == 0;
  switch (form) {
  case FS_DIR:
    ok = ok && S_ISDIR(st.st_mode);
    break;
  case FS_FILE:
    ok = ok && S_ISREG(st.st_mode);
    break;
  case FS_LINK:
    ok = ok && S_ISLNK(st.st_mode);
    break;
  case FS_FIFO:
    ok = ok && S_ISFIFO(st.st_mode);
    break;
  }
  if (!ok) fprintf(stderr, "FAIL fs check: %s: %s is not a %s\n", label, path, names[form]);
  return ok;
}

/* PATH is not there (lstat gives ENOENT). */
static int fs_gone(const char *label, const char *path) {
  struct stat st;
  int ok = lstat(path, &st) != 0 && errno == ENOENT;
  if (!ok) fprintf(stderr, "FAIL fs check: %s: %s is there\n", label, path);
  return ok;
}

/* The file PATH holds the LENGTH bytes at BYTES (LENGTH < 256). */
static int fs_bytes(const char *label, const char *path, const char *bytes, size_t length) {
  char got[256];
  FILE *file = fopen(path, "rb");
  size_t len = file ? fread(got, 1, sizeof got, file) : 0;
  int ok = file != NULL && length < sizeof got && len == length && memcmp(got, bytes, length) == 0;
  if (file) fclose(file);
  if (!ok) fprintf(stderr, "FAIL fs check: %s: %s holds %zu bytes \"%.*s\"\n", label, path, len, (int)len, got);
  return ok;
}

/* The permission bits (07777) of PATH are MODE. */
static int fs_mode(const char *label, const char *path, mode_t mode) {
  struct stat st;
  int there = lstat(path, &st) == 0;
  int ok = there && (st.st_mode & 07777) == mode;
  if (!ok) fprintf(stderr, "FAIL fs check: %s: %s has the mode %04o, not %04o\n", label, path,
                   there ? (unsigned)(st.st_mode & 07777) : 0u, (unsigned)mode);
  return ok;
}

/* PATH is a link to TARGET. */
static int fs_target(const char *label, const char *path, const char *target) {
  char got[256];
  ssize_t len = readlink(path, got, sizeof got);
  int ok = len >= 0 && (size_t)len == strlen(target) && memcmp(got, target, (size_t)len) == 0;
  if (!ok) fprintf(stderr, "FAIL fs check: %s: %s is not a link to %s\n", label, path, target);
  return ok;
}

/* The info file PATH of the XDG form for the relative ITEM:
   "[Trash Info]\nPath=", the cwd and "/" ITEM, percent encoded as the
   driver does, "\nDeletionDate=", then the 19 date bytes
   YYYY-MM-DDThh:mm:ss and "\n". */
static int fs_trash_info(const char *label, const char *path, const char *item) {
  static const char hex[] = "0123456789ABCDEF";
  static const char date[] = "0000-00-00T00:00:00";
  char cwd[4096];
  char full[4352];
  char head[13200];
  char got[13300];
  FILE *file = fopen(path, "rb");
  size_t len = file ? fread(got, 1, sizeof got, file) : 0;
  int size = getcwd(cwd, sizeof cwd) == NULL ? -1 : snprintf(full, sizeof full, "%s/%s", cwd, item);
  size_t at = (size_t)snprintf(head, sizeof head, "[Trash Info]\nPath=");
  int ok = file != NULL && size > 0 && (size_t)size < sizeof full;
  for (size_t i = 0; ok && full[i] != '\0'; i++) {
    unsigned char c = (unsigned char)full[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || strchr("-._~/", c) != NULL) {
      head[at++] = (char)c;
      continue;
    }
    head[at++] = '%';
    head[at++] = hex[c >> 4];
    head[at++] = hex[c & 15u];
  }
  if (ok) at += (size_t)snprintf(head + at, sizeof head - at, "\nDeletionDate=");
  ok = ok && len == at + sizeof date && memcmp(got, head, at) == 0 && got[at + sizeof date - 1u] == '\n';
  for (size_t i = 0; ok && i + 1u < sizeof date; i++)
    ok = date[i] == '0' ? got[at + i] >= '0' && got[at + i] <= '9' : got[at + i] == date[i];
  if (file) fclose(file);
  if (!ok) fprintf(stderr, "FAIL fs check: %s: %s holds %zu bytes \"%.*s\"\n", label, path, len, (int)len, got);
  return ok;
}

/* The root of a write row: each row makes its own tree under build/fs/pr/
   (the read rows reset build/fs/pr/). */
#define FS_W(row) "build/fs/pr/" row
#define FS_COUNT(items) (sizeof items / sizeof items[0])

static const FsItem fs_one_dir[] = {{"d", FS_DIR, NULL, 0, NULL, 0}};
static const FsItem fs_one_file[] = {{"f", FS_FILE, FS_BYTES("x"), NULL, 0}};
static const FsItem fs_a_txt[] = {{"a.txt", FS_FILE, FS_BYTES("alpha"), NULL, 0}};
static const FsItem fs_two_files[] = {
  {"a.txt", FS_FILE, FS_BYTES("alpha"), NULL, 0},
  {"b.txt", FS_FILE, FS_BYTES("beta"), NULL, 0},
};
static const FsItem fs_dir_file[] = {
  {"d", FS_DIR, NULL, 0, NULL, 0},
  {"d/f.txt", FS_FILE, FS_BYTES("data"), NULL, 0},
};
static const FsItem fs_exdev_items[] = {
  {"d", FS_DIR, NULL, 0, NULL, 0},
  {"d/f.txt", FS_FILE, FS_BYTES("data"), NULL, 0640},
  {"d/l", FS_LINK, NULL, 0, "f.txt", 0},
};
static const FsItem fs_mode_items[] = {{"a.sh", FS_FILE, FS_BYTES("run"), NULL, 0777}};
static const FsItem fs_link_tree[] = {
  {"d", FS_DIR, NULL, 0, NULL, 0},
  {"d/f.txt", FS_FILE, FS_BYTES("data"), NULL, 0},
  {"d/sub", FS_DIR, NULL, 0, NULL, 0},
  {"d/sub/g.txt", FS_FILE, FS_BYTES("g"), NULL, 0},
  {"d/l", FS_LINK, NULL, 0, "f.txt", 0},
};
static const FsItem fs_trash_items[] = {
  {"a.txt", FS_FILE, FS_BYTES("alpha"), NULL, 0},
  {"home", FS_DIR, NULL, 0, NULL, 0},
};
static const FsItem fs_collision_items[] = {
  {"a.txt", FS_FILE, FS_BYTES("new"), NULL, 0},
  {"home", FS_DIR, NULL, 0, NULL, 0},
  {"home/.Trash", FS_DIR, NULL, 0, NULL, 0},
  {"home/.Trash/a.txt", FS_FILE, FS_BYTES("old"), NULL, 0},
};
static const FsItem fs_xdg_items[] = {
  {"a.txt", FS_FILE, FS_BYTES("alpha"), NULL, 0},
  {"data", FS_DIR, NULL, 0, NULL, 0},
};
static const FsItem fs_home_items[] = {{"home", FS_DIR, NULL, 0, NULL, 0}};
static const FsItem fs_layer_items[] = {{"src.txt", FS_FILE, FS_BYTES("s"), NULL, 0}};

/* The 19 kickoff write rows and four review regressions. The labels with
   a mutant name are the killer rows of kickoff section 4. Gives the
   count of failed rows. */
static int fs_write_rows(void) {
  int failed = 0;
  failed += !(fs_tree(FS_W("md-parents"), NULL, 0)
    && fs_write_case("makeDir parents", "makeDir \"" FS_W("md-parents") "/a/b/c\"", "unit")
    && fs_is("makeDir parents", FS_W("md-parents") "/a/b/c", FS_DIR));
  failed += !(fs_tree(FS_W("md-there"), fs_one_dir, FS_COUNT(fs_one_dir))
    && fs_write_case("makeDir there", "makeDir \"" FS_W("md-there") "/d\"", "unit")
    && fs_is("makeDir there", FS_W("md-there") "/d", FS_DIR));
  failed += !(fs_tree(FS_W("md-file"), fs_one_file, FS_COUNT(fs_one_file))
    && fs_write_case("makeDir file there", "makeDir \"" FS_W("md-file") "/f\"",
      "inl makeDir: " FS_W("md-file") "/f: File exists")
    && fs_bytes("makeDir file there", FS_W("md-file") "/f", FS_BYTES("x")));
  failed += !(fs_tree(FS_W("md-parent-file"), fs_one_file, FS_COUNT(fs_one_file))
    && fs_write_case("makeDir parent file", "makeDir \"" FS_W("md-parent-file") "/f/sub\"",
      "inl makeDir: " FS_W("md-parent-file") "/f/sub: Not a directory")
    && fs_bytes("makeDir parent file", FS_W("md-parent-file") "/f", FS_BYTES("x")));
  failed += !(fs_tree(FS_W("mv-file"), fs_a_txt, FS_COUNT(fs_a_txt))
    && fs_write_case("move file", "move \"" FS_W("mv-file") "/a.txt\" \"" FS_W("mv-file") "/b.txt\"", "unit")
    && fs_gone("move file", FS_W("mv-file") "/a.txt")
    && fs_bytes("move file", FS_W("mv-file") "/b.txt", FS_BYTES("alpha")));
  failed += !(fs_tree(FS_W("mv-dir"), fs_dir_file, FS_COUNT(fs_dir_file))
    && fs_write_case("move directory", "move \"" FS_W("mv-dir") "/d\" \"" FS_W("mv-dir") "/e\"", "unit")
    && fs_gone("move directory", FS_W("mv-dir") "/d") && fs_is("move directory", FS_W("mv-dir") "/e", FS_DIR)
    && fs_bytes("move directory", FS_W("mv-dir") "/e/f.txt", FS_BYTES("data")));
  failed += !(fs_tree(FS_W("mv-there"), fs_two_files, FS_COUNT(fs_two_files))
    && fs_write_case("move destination there", "move \"" FS_W("mv-there") "/a.txt\" \"" FS_W("mv-there") "/b.txt\"",
      "inl move: " FS_W("mv-there") "/b.txt: File exists")
    && fs_bytes("move destination there", FS_W("mv-there") "/a.txt", FS_BYTES("alpha"))
    && fs_bytes("move destination there", FS_W("mv-there") "/b.txt", FS_BYTES("beta")));
  failed += !(fs_tree(FS_W("mv-missing"), NULL, 0)
    && fs_write_case("move missing source", "move \"" FS_W("mv-missing") "/none\" \"" FS_W("mv-missing") "/b.txt\"",
      "inl move: " FS_W("mv-missing") "/none: No such file or directory")
    && fs_gone("move missing source", FS_W("mv-missing") "/b.txt"));
  pr_force_exdev = 1;
  failed += !(fs_tree(FS_W("mv-exdev"), fs_exdev_items, FS_COUNT(fs_exdev_items))
    && fs_write_case("move EXDEV", "move \"" FS_W("mv-exdev") "/d\" \"" FS_W("mv-exdev") "/e\"", "unit")
    && fs_gone("move EXDEV", FS_W("mv-exdev") "/d") && fs_mode("move EXDEV", FS_W("mv-exdev") "/e", 0755)
    && fs_bytes("move EXDEV", FS_W("mv-exdev") "/e/f.txt", FS_BYTES("data"))
    && fs_mode("move EXDEV", FS_W("mv-exdev") "/e/f.txt", 0640)
    && fs_target("move EXDEV", FS_W("mv-exdev") "/e/l", "f.txt"));
  pr_force_exdev = 0;
  failed += !(fs_tree(FS_W("cp-mode"), fs_mode_items, FS_COUNT(fs_mode_items))
    && fs_write_case("copy mode", "copy \"" FS_W("cp-mode") "/a.sh\" \"" FS_W("cp-mode") "/b.sh\"", "unit")
    && fs_mode("copy mode", FS_W("cp-mode") "/b.sh", 0777)
    && fs_bytes("copy mode", FS_W("cp-mode") "/b.sh", FS_BYTES("run"))
    && fs_bytes("copy mode", FS_W("cp-mode") "/a.sh", FS_BYTES("run")));
  failed += !(fs_tree(FS_W("cp-tree"), fs_link_tree, FS_COUNT(fs_link_tree))
    && fs_write_case("copy tree link", "copy \"" FS_W("cp-tree") "/d\" \"" FS_W("cp-tree") "/e\"", "unit")
    && fs_bytes("copy tree link", FS_W("cp-tree") "/e/f.txt", FS_BYTES("data"))
    && fs_is("copy tree link", FS_W("cp-tree") "/e/sub", FS_DIR)
    && fs_bytes("copy tree link", FS_W("cp-tree") "/e/sub/g.txt", FS_BYTES("g"))
    && fs_target("copy tree link", FS_W("cp-tree") "/e/l", "f.txt")
    && fs_target("copy tree link", FS_W("cp-tree") "/d/l", "f.txt")
    && fs_bytes("copy tree link", FS_W("cp-tree") "/d/f.txt", FS_BYTES("data")));
  failed += !(fs_tree(FS_W("cp-there"), fs_two_files, FS_COUNT(fs_two_files))
    && fs_write_case("copy destination there", "copy \"" FS_W("cp-there") "/a.txt\" \"" FS_W("cp-there") "/b.txt\"",
      "inl copy: " FS_W("cp-there") "/b.txt: File exists")
    && fs_bytes("copy destination there", FS_W("cp-there") "/a.txt", FS_BYTES("alpha"))
    && fs_bytes("copy destination there", FS_W("cp-there") "/b.txt", FS_BYTES("beta")));
  failed += !(fs_tree(FS_W("cp-own"), fs_dir_file, FS_COUNT(fs_dir_file))
    && fs_write_case("copy into own tree", "copy \"" FS_W("cp-own") "/d\" \"" FS_W("cp-own") "/d/sub\"",
      "inl copy: " FS_W("cp-own") "/d/sub: the destination is in the source tree")
    && fs_gone("copy into own tree", FS_W("cp-own") "/d/sub"));
  failed += !(fs_tree(FS_W("cp-missing"), NULL, 0)
    && fs_write_case("copy missing source", "copy \"" FS_W("cp-missing") "/none\" \"" FS_W("cp-missing") "/b.txt\"",
      "inl copy: " FS_W("cp-missing") "/none: No such file or directory")
    && fs_gone("copy missing source", FS_W("cp-missing") "/b.txt"));
  pr_trash_root = FS_W("tr-macos") "/home/.Trash";
  failed += !(fs_tree(FS_W("tr-macos"), fs_trash_items, FS_COUNT(fs_trash_items))
    && fs_write_case("trash macOS form", "trash \"" FS_W("tr-macos") "/a.txt\"", "unit")
    && fs_gone("trash macOS form", FS_W("tr-macos") "/a.txt")
    && fs_bytes("trash macOS form", FS_W("tr-macos") "/home/.Trash/a.txt", FS_BYTES("alpha"))
    && fs_mode("trash macOS form", FS_W("tr-macos") "/home/.Trash", 0700));
  pr_trash_root = FS_W("tr-collision") "/home/.Trash";
  failed += !(fs_tree(FS_W("tr-collision"), fs_collision_items, FS_COUNT(fs_collision_items))
    && fs_write_case("trash collision", "trash \"" FS_W("tr-collision") "/a.txt\"", "unit")
    && fs_gone("trash collision", FS_W("tr-collision") "/a.txt")
    && fs_bytes("trash collision", FS_W("tr-collision") "/home/.Trash/a 2.txt", FS_BYTES("new"))
    && fs_bytes("trash collision", FS_W("tr-collision") "/home/.Trash/a.txt", FS_BYTES("old")));
  pr_trash_root = FS_W("tr-xdg") "/data/Trash";
  pr_trash_form = TRASH_XDG;
  failed += !(fs_tree(FS_W("tr-xdg"), fs_xdg_items, FS_COUNT(fs_xdg_items))
    && fs_write_case("trash XDG form", "trash \"" FS_W("tr-xdg") "/a.txt\"", "unit")
    && fs_gone("trash XDG form", FS_W("tr-xdg") "/a.txt")
    && fs_bytes("trash XDG form", FS_W("tr-xdg") "/data/Trash/files/a.txt", FS_BYTES("alpha"))
    && fs_trash_info("trash XDG form", FS_W("tr-xdg") "/data/Trash/info/a.txt.trashinfo", FS_W("tr-xdg") "/a.txt"));
  pr_trash_root = FS_W("tr-xdg-parents") "/home/.local/share/Trash";
  failed += !(fs_tree(FS_W("tr-xdg-parents"), fs_trash_items, FS_COUNT(fs_trash_items))
    && fs_write_case("trash XDG missing parents", "trash \"" FS_W("tr-xdg-parents") "/a.txt\"", "unit")
    && fs_gone("trash XDG missing parents", FS_W("tr-xdg-parents") "/a.txt")
    && fs_bytes("trash XDG missing parents", FS_W("tr-xdg-parents") "/home/.local/share/Trash/files/a.txt", FS_BYTES("alpha"))
    && fs_trash_info("trash XDG missing parents", FS_W("tr-xdg-parents") "/home/.local/share/Trash/info/a.txt.trashinfo",
      FS_W("tr-xdg-parents") "/a.txt"));
  pr_trash_form = TRASH_MACOS;
  pr_trash_root = FS_W("tr-missing") "/home/.Trash";
  failed += !(fs_tree(FS_W("tr-missing"), fs_home_items, FS_COUNT(fs_home_items))
    && fs_write_case("trash missing path", "trash \"" FS_W("tr-missing") "/none\"",
      "inl trash: " FS_W("tr-missing") "/none: No such file or directory")
    && fs_gone("trash missing path", FS_W("tr-missing") "/home/.Trash"));
  pr_trash_root = PR_TRASH_ROOT;
  failed += !(fs_tree(FS_W("layer"), fs_layer_items, FS_COUNT(fs_layer_items))
    && io_driver_case("layer makeDir copy listDir",
      "def main : IO Unit := bind (makeDir \"" FS_W("layer") "/new/sub\") (fun (a : Sum Text Unit) => bind (copy \""
      FS_W("layer") "/src.txt\" \"" FS_W("layer") "/new/sub/dst.txt\") (fun (b : Sum Text Unit) => bind (listDir \""
      FS_W("layer") "/new/sub\") (fun (r : Sum Text Json) => either (fun (e : Text) => write (textAppend \"inl \" e))"
      " (fun (j : Json) => write (jsonRender j)) r)))\n",
      "", 0, NULL, EVAL_FUEL_STEPS, "[{\"name\":\"dst.txt\",\"kind\":\"file\"}]", 0, NULL)
    && fs_bytes("layer makeDir copy listDir", FS_W("layer") "/new/sub/dst.txt", FS_BYTES("s")));
  {
    int ready = fs_tree(FS_W("cp-umask"), fs_exdev_items, FS_COUNT(fs_exdev_items));
    mode_t saved = umask(0777);
    int copied = ready && fs_write_case("copy restrictive umask", "copy \"" FS_W("cp-umask") "/d\" \"" FS_W("cp-umask") "/e\"", "unit");
    umask(saved);
    failed += !(copied && fs_mode("copy restrictive umask", FS_W("cp-umask") "/e", 0755)
      && fs_bytes("copy restrictive umask", FS_W("cp-umask") "/e/f.txt", FS_BYTES("data"))
      && fs_mode("copy restrictive umask", FS_W("cp-umask") "/e/f.txt", 0640)
      && fs_target("copy restrictive umask", FS_W("cp-umask") "/e/l", "f.txt"));
    /* A failed copy can leave the new directory with its masked mode. */
    chmod(FS_W("cp-umask") "/e", 0700);
  }
  {
    int ready = fs_tree(FS_W("tr-umask"), fs_trash_items, FS_COUNT(fs_trash_items));
    mode_t saved = umask(0777);
    pr_trash_root = FS_W("tr-umask") "/home/.Trash";
    int trashed = ready && fs_write_case("trash restrictive umask", "trash \"" FS_W("tr-umask") "/a.txt\"", "unit");
    umask(saved);
    failed += !(trashed && fs_mode("trash restrictive umask", FS_W("tr-umask") "/home/.Trash", 0700)
      && fs_gone("trash restrictive umask", FS_W("tr-umask") "/a.txt")
      && fs_bytes("trash restrictive umask", FS_W("tr-umask") "/home/.Trash/a.txt", FS_BYTES("alpha")));
    chmod(FS_W("tr-umask") "/home/.Trash", 0700);
    pr_trash_root = PR_TRASH_ROOT;
  }
  {
    int ready = fs_tree(FS_W("tr-xdg-umask"), fs_trash_items, FS_COUNT(fs_trash_items));
    mode_t saved = umask(0777);
    pr_trash_root = FS_W("tr-xdg-umask") "/home/.local/share/Trash";
    pr_trash_form = TRASH_XDG;
    int trashed = ready && fs_write_case("trash XDG restrictive umask", "trash \"" FS_W("tr-xdg-umask") "/a.txt\"", "unit");
    umask(saved);
    failed += !(trashed && fs_mode("trash XDG restrictive umask", FS_W("tr-xdg-umask") "/home/.local/share/Trash", 0700)
      && fs_mode("trash XDG restrictive umask", FS_W("tr-xdg-umask") "/home/.local/share/Trash/files", 0700)
      && fs_mode("trash XDG restrictive umask", FS_W("tr-xdg-umask") "/home/.local/share/Trash/info", 0700)
      && fs_mode("trash XDG restrictive umask", FS_W("tr-xdg-umask") "/home/.local/share/Trash/info/a.txt.trashinfo", 0600)
      && fs_gone("trash XDG restrictive umask", FS_W("tr-xdg-umask") "/a.txt")
      && fs_bytes("trash XDG restrictive umask", FS_W("tr-xdg-umask") "/home/.local/share/Trash/files/a.txt", FS_BYTES("alpha"))
      && fs_trash_info("trash XDG restrictive umask", FS_W("tr-xdg-umask") "/home/.local/share/Trash/info/a.txt.trashinfo",
        FS_W("tr-xdg-umask") "/a.txt"));
    pr_trash_root = PR_TRASH_ROOT;
    pr_trash_form = TRASH_MACOS;
  }
  return failed;
}

static int big_fold_fuel(void) {
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag,
    "def r : Nat := fold (fun (acc : Nat) => acc) 0 18446744073709551616\n");
  if (ok) {
    m.fuel = 20u;
    ok = def_value(&m, m.def_count - 1u) == NULL && diag.set && strcmp(diag.code, "EVAL_FUEL") == 0;
  }
  if (!ok) {
    fprintf(stderr, "FAIL a big Nat fold uses the evaluation fuel\n");
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* S4 chunk B: out (unfold g s) spends 1 fuel, then g s spends its own (Q4).
   K is the cost of step at a variable: there out is stuck before the spend.
   A is the cost of g s at the seed of the tail. Thus the second out, the out
   of the tail, gives a layer with K + A + 1 fuel and traps with K + A. */
static int out_fuel(void) {
  Arena arena;
  Machine m;
  Diag diag;
  const Value *step, *layer, *tail;
  uint64_t k = 0u, a = 0u;
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag,
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def step : Stream Nat -> StreamF Nat (Stream Nat) := fun (x : Stream Nat) => out x\n");
  step = ok ? def_value(&m, m.def_count - 1u) : NULL;
  layer = step != NULL ? apply_value(&m, step, def_value(&m, m.def_count - 2u)) : NULL;
  tail = layer != NULL && val_is(layer, OP_CTOR) && layer->argc == 4u ? layer->args[3] : NULL;
  ok = tail != NULL && val_is(tail, OP_COUNFOLD) && tail->argc == 2u;
  if (ok) {
    m.fuel = EVAL_FUEL_STEPS;
    ok = apply_value(&m, step, val_var(&m, 0u)) != NULL;
    k = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = EVAL_FUEL_STEPS;
    ok = ok && apply_value(&m, tail->args[0], tail->args[1]) != NULL;
    a = EVAL_FUEL_STEPS - m.fuel;
    m.fuel = k + a + 1u;
    ok = ok && apply_value(&m, step, tail) != NULL;
  }
  if (ok) {
    m.fuel = k + a;
    ok = apply_value(&m, step, tail) == NULL && diag.set && strcmp(diag.code, "EVAL_FUEL") == 0;
  }
  if (!ok) {
    fprintf(stderr, "FAIL the second out traps when the fuel is 1 step short\n");
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int accepts(const char *label, const char *source) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag, source);
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* SOURCE after the extra domain text DOMAIN, which can declare a family (a
   program cannot: REFUSE_DATA). */
static int domain_checked(Arena *arena, Machine *m, Diag *diag, const char *domain, const char *source) {
  DeclList decls;
  diag_init(diag);
  return front_load_domain(arena, "path-regression", domain, source, strlen(source), &decls, diag)
      && check_program(arena, &decls, m, diag);
}

static int domain_accepts(const char *label, const char *domain, const char *source) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source);
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* 1 when DOMAIN and SOURCE fail and the first diagnostic has CODE. */
static int domain_refuses(const char *label, const char *domain, const char *source, const char *code) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = !domain_checked(&arena, &m, &diag, domain, source) && diag.set && strcmp(diag.code, code) == 0;
  if (!ok) {
    fprintf(stderr, "FAIL %s: expected %s\n", label, code);
    if (diag.set)
      diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int system_conversion(void) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  Value lines[2] = {{0}, {0}};
  Value negative;
  IConj conjunction;
  uint32_t negative_lit = 1;
  const Value *points[2];
  const Value *args[6];
  const Value *i;
  const Value *ni;
  const Value *original;
  const Value *other;
  int ok;
  uint32_t k;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag, "def seed : Nat := 0\n");
  if (ok) {
    for (k = 0; k < 2u; k++) {
      points[k] = val_nat(&m, k);
      lines[k].kind = VAL_PLAM;
      lines[k].tag = CLOS_CONST;
      lines[k].args = &points[k];
      lines[k].argc = 1;
    }
    i = val_ivar(&m, 0);
    negative = *i;
    conjunction.lits = &negative_lit;
    conjunction.count = 1;
    negative.conj = &conjunction;
    ni = &negative;
    args[0] = i; args[1] = &lines[0];
    args[2] = ni; args[3] = &lines[1];
    original = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 4);
    args[0] = ni; args[1] = &lines[1];
    args[2] = i; args[3] = &lines[0];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 4);
    ok = conv_values(&m, 2, original, other);
    args[1] = &lines[0]; args[3] = &lines[1];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 4);
    ok = ok && !conv_values(&m, 2, original, other);
    args[0] = i; args[1] = &lines[0];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 2);
    ok = ok && !conv_values(&m, 2, original, other);
    args[2] = ni; args[3] = &lines[1];
    args[4] = face_meet(&m, i, val_ivar(&m, 1)); args[5] = &lines[0];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 6);
    ok = ok && conv_values(&m, 2, original, other) && !diag.set;
  }
  if (!ok) {
    fprintf(stderr, "FAIL system conversion: order, values, coverage or refinement\n");
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int domain_roundtrip(const char *label, const char *domain, const char *source) {
  cases_run++;
  Arena arena;
  Machine m, replay;
  Diag diag;
  char type[8192], value[8192], program[32768];
  int ok;
  uint32_t index;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source);
  index = ok ? m.def_count - 1u : 0u;
  if (ok)
    ok = value_print(&m, NULL, 0, m.defs[index].type, type, sizeof type)
      && value_print(&m, NULL, 0, def_value(&m, index), value, sizeof value);
  if (ok) {
    snprintf(program, sizeof program, "def replay : %s := %s\n", type, value);
    ok = domain_checked(&arena, &replay, &diag, domain, program);
    if (!ok) fprintf(stderr, "%s\n", program);
  }
  if (ok)
    ok = conv_values(&m, 0, m.defs[index].type, replay.defs[replay.def_count - 1u].type)
      && conv_values(&m, 0, def_value(&m, index), def_value(&replay, replay.def_count - 1u));
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int roundtrip(const char *label, const char *source) {
  return domain_roundtrip(label, "", source);
}

static int domain_json_contains(const char *label, const char *domain, const char *source, const char *expected) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  const char *text = NULL;
  size_t len = 0;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source)
    && json_document(&m, &text, &len)
    && strstr(text, expected) != NULL;
  if (!ok) {
    fprintf(stderr, "FAIL %s: expected JSON containing %s\n", label, expected);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int json_contains(const char *label, const char *source, const char *expected) {
  return domain_json_contains(label, "", source, expected);
}

/* Same-name instances of the same type must retain distinct path data. */
static int distinct_path_json(const char *label, const char *first, const char *second) {
  Arena arena;
  Machine a, b;
  Diag diag;
  const char *left = NULL;
  const char *right = NULL;
  size_t left_len = 0, right_len = 0;
  uint32_t index;
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &a, &diag, first) && checked(&arena, &b, &diag, second);
  index = ok ? a.def_count - 1u : 0u;
  if (ok)
    ok = a.def_count == b.def_count
      && conv_values(&a, 0, a.defs[index].type, b.defs[index].type)
      && !conv_values(&a, 0, def_value(&a, index), def_value(&b, index)) && !diag.set
      && json_document(&a, &left, &left_len) && json_document(&b, &right, &right_len)
      && (left_len != right_len || strcmp(left, right) != 0);
  if (!ok) {
    fprintf(stderr, "FAIL %s: distinct paths need distinct JSON\n", label);
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* A failed limb allocation must not turn the digit span into UINT64_MAX. */
static int nat_literal_oom(void) {
  char digits[200001];
  Term type = {.kind = TERM_VAR, .name = "Nat"};
  Term body = {.kind = TERM_NAT, .name = digits, .nat = UINT64_MAX};
  Decl decl = {.kind = DECL_DEF, .origin = ORIGIN_PROGRAM,
    .name = "large", .type = &type, .body = &body};
  DeclList decls = {.items = &decl, .count = 1, .cap = 1};
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  cases_run++;
  memset(digits, '9', sizeof digits - 1u);
  digits[sizeof digits - 1u] = '\0';
  arena_init(&arena, (size_t)1 << 16);
  diag_init(&diag);
  ok = !check_program(&arena, &decls, &m, &diag)
    && diag.set && strcmp(diag.code, "OOM") == 0;
  if (!ok) {
    fprintf(stderr, "FAIL big Nat allocation failure: expected OOM, got %s\n",
      diag.set ? diag.code : "an accepted truncated literal");
  }
  arena_release(&arena);
  return ok;
}

/* jsonParse stops past 512 levels of nesting (JSON_DEPTH_MAX): 513 open
   brackets parse, and the 514th bracket, at byte 513, gives the error. */
static int json_nesting(void) {
  char source[2048];
  int at = snprintf(source, sizeof source, "def r : Path (Sum Text Json) (jsonParse \"");
  int i;
  for (i = 0; i < 514; i++)
    source[at++] = '[';
  snprintf(source + at, sizeof source - (size_t)at, "\") (inl \"byte 513: the nesting is deeper than 512 levels\") := refl\n");
  return accepts("JSON nesting stops past 512 levels", source);
}

int main(void) {
  int failed = 0;
  failed += !nat_literal_oom();
  failed += !accepts("Json signed number payload",
    "def signed : Json := jnum (inl 7)\n");
  failed += !accepts("Json decimal text payload",
    "def decimal : Json := jnum (inr \"-1.25e3\")\n");
  failed += !domain_refuses("Json rejects a bare Nat payload", "",
    "def wrong : Json := jnum 7\n", "TYPE_MISMATCH");
  failed += !json_contains("Int64 literal JSON emission",
    "def limit : Int64 := 9223372036854775807\n", "\"value\":9223372036854775807");
  failed += !accepts("Text literal composition",
    "def same : PathP (fun (j : I) => Path Text (hcomp Text j (fun (_ : I) => [ j -> \"a\\x00b\" ]) \"a\\x00b\") \"a\\x00b\") refl refl := fun (j : I) => refl\n");
  failed += !accepts("Int64 literal composition",
    "def same : PathP (fun (j : I) => Path Int64 (hcomp Int64 j (fun (_ : I) => [ j -> 7 ]) 7) 7) refl refl := fun (j : I) => refl\n");
  failed += !accepts("big Nat literal composition",
    "def same : PathP (fun (j : I) => Path Nat (hcomp Nat j (fun (_ : I) => [ j -> 18446744073709551616 ]) 18446744073709551616) 18446744073709551616) refl refl := fun (j : I) => refl\n");
  failed += !system_conversion();
  failed += !accepts("Glue composition with reordered faces",
    "def G : Path (Type 0) (Fin 3) (Fin 3) := fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g : PathP (fun (j : I) => G @ j) 0 0 := fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def same : PathP (fun (j : I) => Path (G @ j) (hcomp (G @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g @ j, j -> g @ j ]) (g @ j)) (g @ j)) refl refl := fun (j : I) => refl\n");
  failed += !accepts("Glue composition under a stronger face",
    "def G0 : Path (Type 0) (Fin 3) (Fin 3) := fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g0 : PathP (fun (j : I) => G0 @ j) 0 0 := fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def G1 : Path (Type 0) (Fin 3) (Fin 3) := fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)), ~j -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g1 : PathP (fun (j : I) => G1 @ j) 0 0 := fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def G : Path (Path (Type 0) (Fin 3) (Fin 3)) G0 G1 := fun (i : I) => fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)), ~j /\\ i -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g : PathP (fun (i : I) => PathP (fun (j : I) => G @ i @ j) 0 0) g0 g1 := fun (i : I) => fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def same0 : PathP (fun (j : I) => Path (G0 @ j) (hcomp (G0 @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g0 @ j, j -> g0 @ j ]) (g0 @ j)) (g0 @ j)) refl refl := fun (j : I) => refl\n"
    "def same1 : PathP (fun (j : I) => Path (G1 @ j) (hcomp (G1 @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g1 @ j, j -> g1 @ j ]) (g1 @ j)) (g1 @ j)) refl refl := fun (j : I) => refl\n"
    "def same : PathP (fun (i : I) => PathP (fun (j : I) => Path (G @ i @ j) (hcomp (G @ i @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g @ i @ j, j -> g @ i @ j ]) (g @ i @ j)) (g @ i @ j)) refl refl) same0 same1 := fun (i : I) => fun (j : I) => refl\n");
  failed += !roundtrip("PathP binder capture",
    "def original : Sigma (PathP : Nat) (Path Nat PathP PathP) := pack 0 refl\n");
  failed += !roundtrip("anonymous funExt argument",
    "def original : (h : (_ : Nat) -> Path Nat 0 0) -> Path (Nat -> Nat) (fun n => 0) (fun n => 0) := fun h => funExt h\n");
  failed += !roundtrip("application of a path of functions",
    "def original : (p : Path (Nat -> Nat) (fun n => n) (fun n => n)) -> Path Nat 0 0 := fun p => fun (i : I) => (p @ i) 0\n");
  failed += !roundtrip("dependent congruence",
    "def original : (p : Path (Charter Nat) (genesis Nat) (genesis Nat)) -> PathP (fun (i : I) => Ratified Nat (p @ i)) (ratify Nat (genesis Nat) 7) (ratify Nat (genesis Nat) 7) := fun p => cong (fun (c : Charter Nat) => ratify Nat c 7) p\n");
  failed += !roundtrip("dependent symmetry",
    "def original : (A : Path (Type 0) Nat Nat) -> (p : PathP (fun (j : I) => A @ j) 0 1) -> PathP (fun (i : I) => A @ ~i) 1 0 := fun A p => sym p\n");
  failed += !roundtrip("dependent function extensionality",
    "def original : (A : Path (Type 0) Nat Nat) -> (h : (n : Nat) -> PathP (fun (j : I) => A @ j) n n) -> PathP (fun (i : I) => Nat -> A @ i) (fun n => n) (fun n => n) := fun A h => funExt h\n");
  failed += !roundtrip("stuck transport fill",
    "def original : PathP (fun (i : I) => (p : Path (Type 0) Nat Nat) -> p @ i) (fun (p : Path (Type 0) Nat Nat) => 0) (fun (p : Path (Type 0) Nat Nat) => transport Nat Nat p 0) := fun (i : I) => fun (p : Path (Type 0) Nat Nat) => transp (fun (j : I) => p @ (i /\\ j)) (~i) 0\n");
  failed += !roundtrip("stuck hcomp with two keys",
    "def original : (n : Nat) -> (p : Path Nat n n) -> Path Nat n n := fun n p => fun (j : I) => hcomp Nat (~j \\/ j) (fun (i : I) => [ ~j -> n, j -> p @ i ]) (p @ j)\n");
  failed += !roundtrip("stuck transport in a Path line",
    "def original : (A : Type 0) -> (x : A) -> (y : A) -> (r : Path A x y) -> (p : Path A x x) -> Path A y y := fun A x y r p => transp (fun (i : I) => Path A (r @ i) (r @ i)) i0 p\n");
  failed += !roundtrip("dead constructor branch",
    "def original : (x : Prod Nat Nat) -> (n : Nat) -> (p : Path Nat n n) -> Path (Prod Nat Nat) (pair n 0) (pair n 0) := fun x n p => fun (j : I) => hcomp (Prod Nat Nat) (~j \\/ j) (fun (i : I) => [ ~j -> pair n 0, j -> pair (p @ i) 0, i0 -> x ]) (pair n 0)\n");
  failed += !roundtrip("dependent Sigma composition",
    "def original : (n : Nat) -> (p : Path Nat n n) -> Path (Sigma (x : Nat) (Path Nat x x)) (pack n refl) (pack n refl) := fun n p => fun (j : I) => hcomp (Sigma (x : Nat) (Path Nat x x)) (~j \\/ j) (fun (i : I) => [ ~j -> pack n refl, j -> pack (p @ i) refl ]) (pack n refl)\n");
  failed += !roundtrip("universe function composition",
    "def original : (p : Path (Nat -> Type 0) (fun n => Nat) (fun n => Nat)) -> Path (Nat -> Type 0) (fun n => Nat) (fun n => Nat) := fun p => fun (j : I) => hcomp (Nat -> Type 0) (~j \\/ j) (fun (i : I) => [ ~j -> (fun n => Nat), j -> p @ i ]) (fun n => Nat)\n");
  failed += !roundtrip("universe Sigma composition",
    "def original : (p : Path (Sigma (x : Nat) (Type 0)) (pack 0 Nat) (pack 0 Nat)) -> Path (Sigma (x : Nat) (Type 0)) (pack 0 Nat) (pack 0 Nat) := fun p => fun (j : I) => hcomp (Sigma (x : Nat) (Type 0)) (~j \\/ j) (fun (i : I) => [ ~j -> pack 0 Nat, j -> p @ i ]) (pack 0 Nat)\n");
  failed += !roundtrip("universe product composition",
    "def original : (p : Path (Prod Nat (Type 0)) (pair 0 Nat) (pair 0 Nat)) -> Path (Prod Nat (Type 0)) (pair 0 Nat) (pair 0 Nat) := fun p => fun (j : I) => hcomp (Prod Nat (Type 0)) (~j \\/ j) (fun (i : I) => [ ~j -> pair 0 Nat, j -> pair 0 Nat ]) (pair 0 Nat)\n");
  failed += !roundtrip("universe path composition",
    "def original : (p : Path (Path (Type 0) Nat Nat) refl refl) -> Path (Path (Type 0) Nat Nat) refl refl := fun p => fun (j : I) => hcomp (Path (Type 0) Nat Nat) (~j \\/ j) (fun (i : I) => [ ~j -> refl, j -> p @ i ]) refl\n");
  failed += !roundtrip("transport of a universe path",
    "def original : (p : Path (Type 0) Nat Nat) -> Path (Type 0) Nat Nat := fun p => transp (fun (_ : I) => Path (Type 0) Nat Nat) i0 p\n");
  failed += !roundtrip("face action on a local type",
    "def original : (P : Path (Type 0) Nat Nat) -> PathP (fun (j : I) => (n : P @ j) -> P @ j) (fun (n : Nat) => n) (fun (n : Nat) => n) := fun P => fun (j : I) => fun (n : P @ j) => hcomp (P @ j) (~j \\/ j) (fun (i : I) => [ ~j -> n, j -> n ]) n\n");
  failed += !roundtrip("a branch on a disjunctive face",
    "def original : (p : Path Nat 0 1) -> PathP (fun (j : I) => Path Nat (p @ j) (p @ j)) refl refl := fun p => fun (j : I) => hcomp (Path Nat (p @ j) (p @ j)) (~j \\/ j) (fun (i : I) => [ ~j \\/ j -> refl ]) refl\n");
  failed += !roundtrip("stuck Fin addition",
    "def original : (r : Fin 11) -> Fin 11 := fun (r : Fin 11) => finAdd 11 r 3\n");
  /* S5a item 2: a finAll def prints as finCase and round-trips. */
  failed += !roundtrip("finite case function printing",
    "def original : (k : Fin 2) -> Path (Fin 2) (finAdd 2 k 0) k := fun k => refl\n");
  failed += !roundtrip("finite case path printing",
    "def p : (k : Fin 2) -> Path (Fin 2) (finAdd 2 k 0) k := fun k => refl\n"
    "def original : Path (Fin 2 -> Fin 2) (fun k => finAdd 2 k 0) (fun k => k) := funExt p\n");
  failed += !roundtrip("finite case beyond the ordinary spine limit",
    "def original : (k : Fin 63) -> Path (Fin 63) (finAdd 63 k 0) k := fun k => refl\n");
  failed += !roundtrip("finite function case in an application",
    "def choose : Fin 2 -> Nat -> Nat := fun k => finCase 2 k (fun n => natAdd n 1) (fun n => natAdd n 2)\n"
    "def original : Fin 2 -> Nat -> Nat := fun k n => choose k n\n");
  failed += !accepts("finite case refines dependent local types",
    "def f : (k : Fin 2) -> (x : finCase 2 k Nat (Fin 3)) -> Nat := fun k x => finCase 2 k x (finVal 3 x)\n"
    "def zero : Path Nat (f 0 7) 7 := refl\n"
    "def one : Path Nat (f 1 2) 2 := refl\n");
  /* S5a item 5: a finCase at a closed scrutinee checks at the top level
     (the empty env is not an allocation failure). */
  failed += !accepts("finite case at a closed literal scrutinee",
    "def a : Path Nat (finCase 2 1 0 1) 1 := refl\n"
    "def b : Nat := finCase 2 (finEq 3 1 1) 0 1\n");
  failed += !roundtrip("Glue type at a neutral face",
    "def original : (A : Type 0) -> (e : Equiv A A) -> (f : Equiv A A) -> Path (Type 0) A A := fun A e f => fun (i : I) => Glue A (~i \\/ i) (fun (_ : I) => [ ~i -> pack A e, i -> pack A f ])\n");
  failed += !roundtrip("stuck unglue",
    "def original : (A : Type 0) -> (e : Equiv A A) -> (x : A) -> (y : A) -> (g : PathP (fun (i : I) => Glue A (~i \\/ i) (fun (_ : I) => [ ~i -> pack A e, i -> pack A e ])) x y) -> Path A (witness e x) (witness e y) := fun A e x y g => fun (i : I) => unglue (g @ i)\n");
  failed += !roundtrip("univalence at a Glue endpoint",
    "def G : Type 0 := Glue (Fin 3) i0 (fun (_ : I) => [ ])\n"
    "def u : G := glue (fun (_ : I) => [ ]) 0\n"
    "def original : G := transport G G (ua G G (idEquiv G)) u\n");
  failed += !roundtrip("unglue at a nested Glue face",
    "def G : Type 0 := Glue (Fin 3) i0 (fun (_ : I) => [ ])\n"
    "def u : G := glue (fun (_ : I) => [ ]) 0\n"
    "def p : PathP (fun (i : I) => Glue G (~i \\/ i) (fun (_ : I) => [ ~i -> pack G (idEquiv G), i -> pack G (idEquiv G) ])) u u := fun (i : I) => glue (fun (_ : I) => [ ~i -> u, i -> u ]) u\n"
    "def original : PathP (fun (i : I) => Path G (unglue (p @ i)) u) refl refl := fun (i : I) => refl\n");
  failed += !json_contains("transport table with function points",
    "def singletonFunPath : (h : Fin 1 -> Fin 1) -> (x : Fin 1) -> Path (Fin 1) (h 0) (h x) := fun h x => refl\n"
    "def toFun : Fin 1 -> (Fin 1 -> Fin 1) := fun x => fun (_ : Fin 1) => x\n"
    "def fromFun : (Fin 1 -> Fin 1) -> Fin 1 := fun h => h 0\n"
    "def toFromFun : (h : Fin 1 -> Fin 1) -> Path (Fin 1 -> Fin 1) (toFun (fromFun h)) h := fun h => fun (i : I) => fun x => singletonFunPath h x @ i\n"
    "def fromToFun : (x : Fin 1) -> Path (Fin 1) (fromFun (toFun x)) x := fun x => refl\n"
    "def finToFun : Path (Type 0) (Fin 1) (Fin 1 -> Fin 1) := ua (Fin 1) (Fin 1 -> Fin 1) (isoToEquiv (Fin 1) (Fin 1 -> Fin 1) toFun fromFun toFromFun fromToFun)\n",
    "\"transport\":[\"fun x => 0\"]");
  failed += !json_contains("stuck transport table uses path syntax",
    "def neutralLoop : Path (Type 0) (Fin 1) (Fin 1) := hcomp (Path (Type 0) (Fin 1) (Fin 1)) i0 (fun (_ : I) => [ ]) refl\n",
    "\"term\":\"fun i =>");
  failed += !domain_accepts("path constructor declaration",
    "family S1x := basex | loopx (i : I) [ ~i -> basex, i -> basex ]\n",
    "def p : Path S1x (loopx i0) (loopx i1) := fun (i : I) => loopx i\n");
  failed += !domain_refuses("path constructor boundary of a wrong type",
    "family S1x := basex | loopx (i : I) [ ~i -> 0, i -> basex ]\n",
    "def u : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_refuses("path constructor boundary that does not cover",
    "family S1x := basex | loopx (i : I) [ ~i -> basex ]\n",
    "def u : Nat := 0\n", "SYSTEM_COVER");
  failed += !domain_refuses("path constructor with no boundary",
    "family S1x := basex | loopx (i : I)\n",
    "def u : Nat := 0\n", "HIT_BOUNDARY");
  failed += !domain_json_contains("path tree fallback after a recursive field",
    "family Tree := leaf | branch (subtree : Tree) (label : S1) (i : I) [ ~i -> subtree, i -> subtree ]\n",
    "def p : Path Tree leaf leaf := fun (j : I) => branch leaf (loop j) j\n",
    "\"value\":{\"from\":{\"tag\":\"leaf\"},\"to\":{\"tag\":\"leaf\"},\"term\":\"fun j => branch leaf (loop j) j\"}}]}");
  failed += !domain_json_contains("path tree with a recursive field",
    "family Tree := leaf | branch (subtree : Tree) (label : S1) (i : I) [ ~i -> subtree, i -> subtree ]\n",
    "def p : Path Tree leaf leaf := fun (j : I) => branch (branch leaf base j) base j\n",
    "\"value\":{\"path\":\"branch\",\"args\":[{\"path\":\"branch\",\"args\":[{\"tag\":\"leaf\"},\"base\"]},\"base\"]}}]}");
  failed += !roundtrip("formal hcomp at a HIT",
    "def original : Path S1 base base := fun (j : I) => hcomp S1 (~j \\/ j) (fun (i : I) => [ ~j -> base, j -> loop i ]) (loop j)\n");
  failed += !distinct_path_json("formal hcomp retains its tube",
    "def p : Path S1 base base := fun (j : I) => hcomp S1 (~j \\/ j) (fun (i : I) => [ ~j -> base, j -> base ]) (loop j)\n",
    "def p : Path S1 base base := fun (j : I) => hcomp S1 (~j \\/ j) (fun (i : I) => [ ~j -> base, j -> loop i ]) (loop j)\n");
  failed += !roundtrip("path constructor at a neutral interval",
    "def original : PathP (fun (i : I) => Path S1 base (loop i)) refl (fun (j : I) => loop j) := fun (i : I) => fun (j : I) => loop (i /\\ j)\n");
  failed += !roundtrip("Glue endpoint substitution preserves interval connections",
    "def code : S1 -> S1 -> Type 0 := fun (y : S1) => fun (x : S1) =>\n"
    "  fold (Path S1 y y) (ua (Path S1 y y) (Path S1 y y) (idEquiv (Path S1 y y))) x\n"
    "def transportSelf : (y : S1) -> Path S1 y y := fun (y : S1) =>\n"
    "  transp (fun (i : I) => code y (loop i)) i0 (fun (_ : I) => y)\n"
    "def p : PathP (fun (j : I) => Path S1 (loop (j /\\ ~j)) (loop (j /\\ ~j)))\n"
    "  (transportSelf base) (transportSelf base) := fun (j : I) => transportSelf (loop (j /\\ ~j))\n");
  failed += !roundtrip("transport of a formal hcomp at Trunc1 along a parameter line",
    "def original : (A : Type 0) -> (B : Type 0) -> (p : Path (Type 0) A B) -> (a : A) -> (b : A) ->\n"
    "  Path (Trunc1 B) (transp (fun (i : I) => Trunc1 (p @ i)) i0 (trunc1 A a)) (transp (fun (i : I) => Trunc1 (p @ i)) i0 (trunc1 A a)) :=\n"
    "  fun (A : Type 0) => fun (B : Type 0) => fun (p : Path (Type 0) A B) => fun (a : A) => fun (b : A) => fun (j : I) =>\n"
    "  transp (fun (i : I) => Trunc1 (p @ i)) i0 (hcomp (Trunc1 A) (~j \\/ j)\n"
    "    (fun (i : I) => [ ~j -> trunc1 A a, j -> squash1 A (trunc1 A b) (trunc1 A a) i ]) (squash1 A (trunc1 A a) (trunc1 A b) j))\n");
  failed += !domain_roundtrip("transport preserves a parameter-dependent HIT boundary",
    "family ParamHit (a : Trunc1 (Fin 3)) := paramPoint (paramValue : Trunc1 (Fin 3)) |\n"
    "  paramLoop (i : I) [ ~i -> paramPoint a a, i -> paramPoint a a ]\n",
    "def endpoint : ParamHit (trunc1 (Fin 3) 1) :=\n"
    "  transp (fun (i : I) => ParamHit (squash1 (Fin 3) (trunc1 (Fin 3) 0) (trunc1 (Fin 3) 1) i)) i0\n"
    "    (paramPoint (trunc1 (Fin 3) 0) (trunc1 (Fin 3) 0))\n"
    "def original : Path (ParamHit (trunc1 (Fin 3) 1)) endpoint endpoint :=\n"
    "  fun (j : I) => transp (fun (i : I) => ParamHit (squash1 (Fin 3) (trunc1 (Fin 3) 0) (trunc1 (Fin 3) 1) i)) i0\n"
    "    (paramLoop (trunc1 (Fin 3) 0) j)\n");
  /* S4c item 17 (R7 (a)): the transport of a dimension-2 constructor along
     a parameter line stays stuck. Its boundary branches are path
     applications, not recursive fields, so a field rebuild needs a
     boundary correction (R7 (c)). */
  failed += !roundtrip("transport of squash0 along a parameter line stays stuck",
    "def original : (A : Type 0) -> (B : Type 0) -> (p : Path (Type 0) A B) -> (x : Trunc0 A) -> (y : Trunc0 A) ->\n"
    "  (u : Path (Trunc0 A) x y) -> (v : Path (Trunc0 A) x y) ->\n"
    "  Path (Path (Trunc0 B) (transp (fun (k : I) => Trunc0 (p @ k)) i0 x) (transp (fun (k : I) => Trunc0 (p @ k)) i0 y))\n"
    "    (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (u @ j)) (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (v @ j)) :=\n"
    "  fun (A : Type 0) => fun (B : Type 0) => fun (p : Path (Type 0) A B) => fun (x : Trunc0 A) => fun (y : Trunc0 A) =>\n"
    "  fun (u : Path (Trunc0 A) x y) => fun (v : Path (Trunc0 A) x y) =>\n"
    "  fun (i : I) => fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (squash0 A x y u v i j)\n");
  failed += !domain_refuses("transport of squash0 is not the field rebuild", "",
    "def same : (A : Type 0) -> (B : Type 0) -> (p : Path (Type 0) A B) -> (x : Trunc0 A) -> (y : Trunc0 A) ->\n"
    "  (u : Path (Trunc0 A) x y) -> (v : Path (Trunc0 A) x y) ->\n"
    "  Path (Path (Path (Trunc0 B) (transp (fun (k : I) => Trunc0 (p @ k)) i0 x) (transp (fun (k : I) => Trunc0 (p @ k)) i0 y))\n"
    "    (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (u @ j)) (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (v @ j)))\n"
    "    (fun (i : I) => fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (squash0 A x y u v i j))\n"
    "    (fun (i : I) => fun (j : I) => squash0 B (transp (fun (k : I) => Trunc0 (p @ k)) i0 x) (transp (fun (k : I) => Trunc0 (p @ k)) i0 y)\n"
    "      (fun (k : I) => transp (fun (m : I) => Trunc0 (p @ m)) i0 (u @ k)) (fun (k : I) => transp (fun (m : I) => Trunc0 (p @ m)) i0 (v @ k)) i j) :=\n"
    "  fun (A : Type 0) => fun (B : Type 0) => fun (p : Path (Type 0) A B) => fun (x : Trunc0 A) => fun (y : Trunc0 A) =>\n"
    "  fun (u : Path (Trunc0 A) x y) => fun (v : Path (Trunc0 A) x y) => refl\n", "PATH_UIP");
  failed += !domain_refuses("dimension-2 boundary that does not cover the square",
    "family Sq2 (A : Type 0) := sq2Point (sq2Of : A)\n"
    "  | sq2Square (sq2From : Sq2 A) (sq2To : Sq2 A) (sq2Left : Path (Sq2 A) sq2From sq2To) (sq2Right : Path (Sq2 A) sq2From sq2To) (i : I) (j : I)\n"
    "    [ ~i -> sq2Left @ j, i -> sq2Right @ j, ~j -> sq2From ]\n",
    "def u : Nat := 0\n", "SYSTEM_COVER");
  /* Boundary images must fold path fields just as evaluation does. */
  failed += !domain_roundtrip("fold boundary maps a point constructor path field",
    "family HH := hhBase | hhNode (hhX : HH) (hhY : HH) (hhP : Path HH hhX hhY) | hhLoop (i : I) [ ~i -> hhNode hhBase hhBase refl, i -> hhNode hhBase hhBase refl ]\n",
    "def original : Path (Fin 2) 0 0 := fun (t : I) => fold 0 (fun (x : Fin 2) (y : Fin 2) (p : Path (Fin 2) x y) => p @ t) refl (hhLoop t)\n");
  failed += !domain_roundtrip("fold square boundary preserves captured intervals in path fields",
    "family HH := hhBase | hhNode (hhX : HH) (hhY : HH) (hhP : Path HH hhX hhY)\n"
    "  | hhSquare (hhU : HH) (hhV : Path HH hhU hhU) (i : I) (j : I) [ ~i -> hhNode hhU hhU (fun (k : I) => hhV @ (j /\\ ~j /\\ k /\\ ~k)),\n"
    "      i -> hhNode hhU hhU (fun (k : I) => hhV @ (j /\\ ~j /\\ k /\\ ~k)), ~j -> hhNode hhU hhU refl, j -> hhNode hhU hhU refl ]\n",
    "def original : HH -> HH := fun (z : HH) => fold hhBase (fun (x : HH) (y : HH) (p : Path HH x y) => hhNode x y p) (fun (x : HH) (p : Path HH x x) => fun (a : I) => fun (b : I) => hhSquare x p a b) z\n");
  /* S5b item 7: the refutation of decEqFin is a closure. Its printed form
     must check again at the declared type (J15: Fin 0 and Unit swapped in
     the line fail with TYPE_MISMATCH at i0). */
  failed += !domain_roundtrip("decEqFin inr round-trips", "",
    "def d : Dec (Path (Fin 3) 0 1) := decEqFin 3 0 1\n");
  failed += !roundtrip("decEqNat inr round-trips",
    "def d : Dec (Path Nat 0 1) := decEqNat 0 1\n");
  /* S6a item 1 (R13 (a)): a glue element is neutral for a Nat operation.
     The transport back along the ua line gives finAdd on a glue element;
     the operation stays stuck and rule 6 prints the term (S5c: INTERNAL). */
  failed += !json_contains("Nat operation on a glue element stays stuck",
    "def loopRot1 : Path (Type 0) (Fin 3) (Fin 3) := ua (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) "
    "(fun (r : Fin 3) => finAdd 3 r 1) (fun (r : Fin 3) => finAdd 3 r 2) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl))\n"
    "def back : Path (Fin 3) 0 0 := fun (i : I) => transp (fun (j : I) => loopRot1 @ (i /\\ ~j)) i0 "
    "(transp (fun (j : I) => loopRot1 @ (i /\\ j)) (~i) 0)\n",
    "(finAdd 3 (finAdd 3 (glue (fun (_ : I) => [ ~i -> 0, i -> 1 ]) 1) 1) 2)");
  /* Both value operands of finAdd must stay neutral on Glue elements. */
  failed += !json_contains("finAdd right Glue operand stays stuck",
    "def loopRot1 : Path (Type 0) (Fin 3) (Fin 3) := ua (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) "
    "(fun (r : Fin 3) => finAdd 3 1 r) (fun (r : Fin 3) => finAdd 3 2 r) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl))\n"
    "def back : Path (Fin 3) 0 0 := fun (i : I) => transp (fun (j : I) => loopRot1 @ (i /\\ ~j)) i0 "
    "(transp (fun (j : I) => loopRot1 @ (i /\\ j)) (~i) 0)\n",
    "(finAdd 3 2 (finAdd 3 1 (glue");
  /* S6b item 6, rule 2: the components 1 and 2 are different, so each
     part must come from its own component path. */
  failed += !json_contains("Sigma path prints each component",
    "def p : Path ((e : Fin 2) * Fin 3) (pack 1 2) (pack 1 2) := refl\n",
    "\"value\":{\"witness\":{\"refl\":1},\"payload\":{\"refl\":2}}");
  /* S6b item 6, rule 3 at a ua domain: the pointwise path at x goes from
     f x to g (s x), so the table starts at x = 0 with 1 (asymmetric). */
  failed += !json_contains("path of functions prints a table of paths",
    "def l : Path (Type 0) (Fin 3) (Fin 3) := ua (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) (fun (r : Fin 3) => finAdd 3 r 1) (fun (r : Fin 3) => finAdd 3 r 2) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl))\n"
    "def q : (z : Fin 1) -> PathP (fun (i : I) => (l @ i) -> Fin 3) (fun (x : Fin 3) => finAdd 3 x 1) (fun (x : Fin 3) => x) := "
    "fun (z : Fin 1) => uaArrow (Fin 3) (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) (fun (r : Fin 3) => finAdd 3 r 1) (fun (r : Fin 3) => finAdd 3 r 2) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl)) "
    "(fun (x : Fin 3) => finAdd 3 x 1) (fun (x : Fin 3) => x) (fun (w : Fin 3) => refl)\n"
    "def p : Path ((A : Type 0) * (A -> Fin 3)) (pack (Fin 3) (fun (x : Fin 3) => finAdd 3 x 1)) (pack (Fin 3) (fun (x : Fin 3) => x)) := "
    "fun (i : I) => pack (l @ i) ((q 0) @ i)\n",
    "\"payload\":{\"paths\":[{\"from\":1,\"to\":1,\"term\":");
  /* S6b item 6, rule 1 at the Sigma carrier: a payload-major order gives
     [0,3,1,4,2,5]. */
  failed += !json_contains("ua table at a Sigma carrier",
    "def C : Type 0 := (e : Fin 2) * Fin 3\n"
    "def lift : (x : Fin 3) -> Fin 6 := fun (x : Fin 3) => finCase 3 x 0 1 2\n"
    "def fwd : (p : C) -> Fin 6 := fun (p : C) => finCase 2 (witness p) (lift (payload p)) (finAdd 6 (lift (payload p)) 3)\n"
    "def bwd : (k : Fin 6) -> C := fun (k : Fin 6) => finCase 6 k (pack 0 0) (pack 0 1) (pack 0 2) (pack 1 0) (pack 1 1) (pack 1 2)\n"
    "def sec : (k : Fin 6) -> Path (Fin 6) (fwd (bwd k)) k := fun (k : Fin 6) => refl\n"
    "def retC : (e : Fin 2) -> (x : Fin 3) -> Path C (bwd (fwd (pack e x))) (pack e x) := fun (e : Fin 2) => fun (x : Fin 3) => refl\n"
    "def retAll : (p : C) -> Path C (bwd (fwd p)) p := fun (p : C) => retC (witness p) (payload p)\n"
    "def u : Path (Type 0) C (Fin 6) := ua C (Fin 6) (isoToEquiv C (Fin 6) fwd bwd sec retAll)\n",
    "\"value\":{\"from\":\"Sigma (e : Fin 2) (Fin 3)\",\"to\":\"Fin 6\",\"transport\":[0,1,2,3,4,5]}");
  /* Empty literal carriers still select the table rules. */
  failed += !json_contains("empty Fin universe path prints an empty table",
    "def p : Path (Type 0) (Fin 0) (Fin 0) := refl\n",
    "\"value\":{\"from\":\"Fin 0\",\"to\":\"Fin 0\",\"transport\":[]}");
  failed += !json_contains("empty Sigma base prints an empty transport table",
    "def C : Type 0 := (e : Fin 0) * Fin 2\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"transport\":[]}");
  failed += !json_contains("empty Sigma fibers print an empty transport table",
    "def C : Type 0 := (e : Fin 2) * Fin 0\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"transport\":[]}");
  failed += !json_contains("path of functions on Fin zero prints an empty table",
    "def f : Fin 0 -> Nat := fun (x : Fin 0) => 0\n"
    "def p : Path (Fin 0 -> Nat) f f := refl\n",
    "\"value\":{\"paths\":[]}");
  failed += !json_contains("path of functions on an empty Sigma prints an empty table",
    "def C : Type 0 := (e : Fin 2) * Fin 0\n"
    "def f : C -> Nat := fun (x : C) => 0\n"
    "def p : Path (C -> Nat) f f := refl\n",
    "\"value\":{\"paths\":[]}");
  /* Every Sigma fiber must be literal before either table rule applies.
     The second fiber here is infinite, beyond the finite first fiber. */
  failed += !json_contains("mixed Sigma carrier keeps the whole universe path",
    "def C : Type 0 := (e : Fin 2) * (finCase 2 e (Fin 1) Nat)\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"value\":{\"refl\":\"Sigma ");
  failed += !json_contains("mixed Sigma function domain keeps the whole path",
    "def C : Type 0 := (e : Fin 2) * (finCase 2 e (Fin 1) Nat)\n"
    "def f : C -> Nat := fun (x : C) => 0\n"
    "def p : Path (C -> Nat) f f := refl\n",
    "\"value\":{\"refl\":\"fun x => 0\"}");
  /* Literal dependent fibers can have different sizes, including zero. */
  failed += !json_contains("dependent Sigma transport enumerates every fiber",
    "def C : Type 0 := (e : Fin 3) * (finCase 3 e (Fin 1) (Fin 0) (Fin 2))\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"transport\":[{\"witness\":0,\"payload\":0},{\"witness\":2,\"payload\":0},{\"witness\":2,\"payload\":1}]}");
  failed += !json_contains("dependent Sigma function paths enumerate every fiber",
    "def C : Type 0 := (e : Fin 3) * (finCase 3 e (Fin 1) (Fin 0) (Fin 2))\n"
    "def f : C -> Nat := fun (x : C) => finVal 3 (witness x)\n"
    "def p : Path (C -> Nat) f f := refl\n",
    "\"value\":{\"paths\":[{\"refl\":0},{\"refl\":2},{\"refl\":2}]}");
  /* S2 chunk A (orbit-lang S6 B1 to B4 and S7 B1 rows): indexed families,
     mutual blocks and the elim check. The texts come from orbit
     test/fronttool.c with Size as Nat, sz0 as 0, szS as succ, Unit as Nat
     and fresh names beside the domain names. The fold values of the L/R
     row are checked and not reduced here: the block fold eval is chunk B. */
  failed += !domain_refuses("indexed family: an end at another index",
    "family Vx : Nat -> Type 0 := vnilx : Vx 0 | vconsx (vxSize : Nat) (vxTail : Vx vxSize) : Vx (succ vxSize) "
    "| vbadx (badxSize : Nat) (badxTail : Vx badxSize) (i : I) : Vx (succ badxSize) "
    "[ ~i -> vconsx badxSize badxTail, i -> badxTail ]\n",
    "def u : Nat := 0\n", "HIT_INDEX");
  failed += !domain_refuses("indexed family: a constructor without a result",
    "family Vx : Nat -> Type 0 := vnilx | vconsx (vxSize : Nat) (vxTail : Vx vxSize) : Vx (succ vxSize)\n",
    "def u : Nat := 0\n", "HIT_INDEX");
  failed += !domain_refuses("indexed family: a result head that is not the family",
    "family Vx : Nat -> Type 0 := vnilx : Nat | vconsx (vxSize : Nat) (vxTail : Vx vxSize) : Vx (succ vxSize)\n",
    "def u : Nat := 0\n", "HIT_INDEX");
  failed += !domain_refuses("block: a header names a later sort",
    "family P0 : Q0 -> Type 0 := p0 : P0 q0 and Q0 := q0\n",
    "def u : Nat := 0\n", "BLOCK_ORDER");
  failed += !domain_accepts("block: a field names a later sort",
    "family Ch := gen | adoptx (adoptxOf : Ch) (adoptxBy : Rt adoptxOf) and Rt : Ch -> Type 0 := rat (ratOf : Ch) : Rt ratOf\n",
    "def u : Nat := 0\n");
  failed += !domain_refuses("block fold: an extra argument before the value",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def bad : Nat := fold (Even -> Nat) Nat (fun (e : Even) => 0) (fun (u : Nat) => fun (e : Even) => 1) "
    "(fun (g : Even -> Nat) => 0) 0 ez\n", "TYPE_MISMATCH");
  failed += !domain_refuses("reserved header: a fold of the sort under check",
    "family Ax := ax (axN : Nat) and Bx (x : Ax) (y : fold Nat x) := bx\n",
    "def value : Bx (ax 0) 0 := bx (ax 0) 0\n", "HIT_BOUNDARY");
  failed += !domain_refuses("block universe: at Type 0",
    "family Low := low (high : High) and High := highCtor (big : Type 0)\n",
    "def small : Type 0 := Low\n", "TYPE_MISMATCH");
  failed += !domain_accepts("block universe: at Type 1",
    "family Low := low (high : High) and High := highCtor (big : Type 0)\n",
    "def large : Type 1 := Low\n");
  failed += !domain_accepts("higher-order index telescope: 1",
    "family Fx (A : Type 0) : ((Gx : Type 0) -> Gx -> Gx) -> Type 0 := mkx (mkxArg : (T : Type 0) -> T -> T) : Fx A mkxArg and Gx := gx\n",
    "def u : Nat := 0\n");
  failed += !domain_accepts("higher-order index telescope: 2",
    "family Fx (Gx : Type 0) : Gx -> Type 0 := mkx (mkxArg : Gx) : Fx Gx mkxArg and Gx := gx\n",
    "def u : Nat := 0\n");
  failed += !domain_accepts("block fold: the same parameter telescope",
    "family Lx (A : Type 0) (a : A) := leftLx (lxValue : A) and Rx (A : Type 0) (a : A) := rightRx (rxLeft : Lx A a)\n",
    "def firstL : Nat := fold Nat Nat (fun (n : Nat) => n) (fun (n : Nat) => 0) (leftLx Nat 7 8)\n"
    "def secondR : Nat := fold Nat Nat (fun (n : Nat) => n) (fun (n : Nat) => 0) (rightRx Nat 7 (leftLx Nat 7 8))\n");
  failed += !domain_refuses("block fold: a different parameter telescope",
    "family Fx (A : Type 0) := makeFx (fxValue : A) and Gx (n : Nat) := makeGx\n",
    "def result : Nat := fold Nat Nat (fun (x : Nat) => 0) 0 (makeGx 7)\n", "TYPE_MISMATCH");
  failed += !domain_accepts("elim: a partial family eliminator",
    "family Nx := zerox | succx (predx : Nx)\n",
    "def elimId : Nx -> Nx := elim (fun (n : Nx) => Nx) zerox (fun n ih => succx ih)\n");
  failed += !domain_accepts("elim: mixed motive annotations",
    "family Nx := zerox | succx (predx : Nx)\n"
    "family Vx : Nx -> Type 0 := nilVx : Vx zerox | consVx (lenx : Nx) (tailVx : Vx lenx) : Vx (succx lenx)\n",
    "def elimMixed : (n : Nx) -> Vx n -> Nx := fun n v => elim (fun k => fun (t : Vx k) => Nx) zerox (fun k t ih => succx ih) v\n");
  /* S2 chunk B (orbit-lang S6 B3, S6 B4 and S7 B1 rows): the indexed and
     block fold eval, the fold print order and the transport round trips.
     The texts come from orbit test/fronttool.c with Size as Nat, sz0 as 0,
     szS as succ, Unit as Nat, unit as 0 and fresh names beside the domain
     names. */
  failed += !domain_accepts("indexed fold: index arguments are not parameters",
    "family Vx (A : Type 0) : Nat -> Type 0 := vnx : Vx A 0 | vcx (item : A) : Vx A (succ 0) "
    "| segx (i : I) : Vx A 0 [ ~i -> vnx A, i -> vnx A ]\n",
    "def result : Path Nat (fold 0 (fun a => a) refl (vcx Nat 7)) 7 := refl\n");
  failed += !domain_accepts("block fold: Even/Odd one",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def one : Path Nat (fold Nat Nat 0 (fun (u : Nat) => 1) (fun (n : Nat) => 0) (es (os ez))) 1 := refl\n");
  failed += !domain_accepts("block fold: Even/Odd two",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def two : Path Nat (fold Nat Nat 0 (fun (u : Nat) => natAdd u 1) (fun (n : Nat) => natAdd n 1) (es (os ez))) 2 := refl\n");
  failed += !domain_accepts("block fold: Box, a sibling recursive field at its target",
    "family Boxx := boxx (boxxLeft : Itm) (boxxRight : Itm) (boxxPath : Path Itm boxxLeft boxxRight) and Itm := itm\n",
    "def result : Path Nat 0 0 := fun (i : I) => fold Nat Nat (fun x y p => p @ i) 0 (boxx itm itm refl)\n");
  failed += !domain_accepts("block fold: L/R, the folds reduce",
    "family Lx (A : Type 0) (a : A) := leftLx (lxValue : A) and Rx (A : Type 0) (a : A) := rightRx (rxLeft : Lx A a)\n",
    "def firstL : Path Nat (fold Nat Nat (fun (n : Nat) => n) (fun (n : Nat) => 0) (leftLx Nat 7 8)) 8 := refl\n"
    "def secondR : Path Nat (fold Nat Nat (fun (n : Nat) => n) (fun (n : Nat) => 0) (rightRx Nat 7 (leftLx Nat 7 8))) 0 := refl\n");
  failed += !domain_accepts("transport: the fibre of a transport result",
    "family Vx (A : Type 0) : Nat -> Type 0 := vnx : Vx A 0 "
    "| vcx (vcxLen : Nat) (vcxHead : A) (vcxTail : Vx A vcxLen) : Vx A (succ vcxLen)\n",
    "def fibre : Vx Nat (succ 0) := transp (fun (i : I) => Vx Nat (succ 0)) i0 (vcx Nat 0 7 (vnx Nat))\n");
  failed += !domain_roundtrip("block fold: the print order of the targets, Even -> Nat",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def printEven : Even -> Nat := fun x => fold Nat Nat 0 (fun u => 1) (fun n => 0) x\n");
  failed += !domain_roundtrip("block fold: the print order of the targets, Odd -> Nat",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n",
    "def printOdd : Odd -> Nat := fun x => fold Nat Nat 0 (fun u => 1) (fun n => 0) x\n");
  failed += !domain_roundtrip("transport round trip: a constant line at Vec",
    "family Vx (A : Type 0) : Nat -> Type 0 := vnx : Vx A 0\n",
    "def line : Vx Nat 0 := transp (fun (i : I) => Vx Nat 0) i0 (vnx Nat)\n");
  failed += !domain_roundtrip("transport round trip: an index line",
    "family Nx := zx | sx (px : Nx) | edgex (i : I) [ ~i -> zx, i -> sx zx ]\n"
    "family Vx : Nx -> Type 0 := vnx : Vx zx | vcx : Vx (sx zx)\n",
    "def line : Vx (sx zx) := transp (fun (i : I) => Vx (edgex i)) i0 vnx\n");
  failed += !domain_roundtrip("transport round trip: Even/Odd",
    "family Even (A : Type 0) := ez | es (oddPrev : Odd A) and Odd (A : Type 0) := os (evenPrev : Even A)\n",
    "def line : Even Nat := transp (fun (i : I) => Even Nat) i0 (es Nat (os Nat (ez Nat)))\n");
  failed += !domain_json_contains("indexed constructors retain only parameters in their field environment", "",
    "def vector : Vec Nat 1 := vcons Nat 0 7 (vnil Nat)\n",
    "\"value\":{\"tag\":\"vcons\",\"consLen\":0,\"consHead\":7,\"consTail\":{\"tag\":\"vnil\"}}");
  failed += !domain_json_contains("indexed enum constructor JSON",
    "family Ix : Nat -> Type 0 := ix : Ix 0\n",
    "def result : Ix 0 := ix\n", "\"value\":\"ix\"");
  failed += !domain_json_contains("indexed sibling constructor JSON", "",
    "def result : Ratified Nat (genesis Nat) := ratify Nat (genesis Nat) 7\n",
    "\"value\":{\"ratifyBase\":{\"tag\":\"genesis\"},\"ratifyRule\":7}");
  failed += !domain_json_contains("HIT JSON traverses recursive fields at their sibling sort",
    "family Boxx := boxx (boxxItem : Itm) "
    "| boxxLoop (boxxAt : Itm) (i : I) [ ~i -> boxx boxxAt, i -> boxx boxxAt ] and Itm := itm\n",
    "def result : Path Boxx (boxx itm) (boxx itm) := fun (i : I) => boxxLoop itm i\n",
    "\"value\":{\"path\":\"boxxLoop\",\"args\":[\"itm\"]}");
  failed += !domain_refuses("elim cannot use unfinished constructor slots in a field type",
    "family Xx := zxx | packxx (pxx : Xx) (qxx : Path Nat (elim (fun (v : Xx) => Nat) 0 0 0 pxx) 0) "
    "| edgexx (i : I) [ ~i -> zxx, i -> zxx ]\n",
    "def p : Path Xx (packxx zxx refl) (packxx zxx refl) := fun (i : I) => packxx (edgexx i) refl\n",
    "HIT_BOUNDARY");
  failed += !domain_refuses("elim cannot use unfinished constructor slots in a boundary",
    "family Xx := zxx | edgexx (i : I) [ ~i -> zxx, i -> elim (fun (v : Xx) => Xx) zxx zxx zxx ]\n",
    "def result : Nat := 0\n", "HIT_BOUNDARY");
  failed += !domain_refuses("diagnostic printing skips unnamed constructor slots",
    "family Ax := ax (axType : Type 0) "
    "and Bx : (elim (fun (x : Ax) => Type 0) Nat (ax Nat)) -> Type 0 := bx (bxVal : Nat) : Bx bxVal\n",
    "def result : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_accepts("block header uses the settled universe of an earlier sort",
    "family Ax := ax (axType : Type 0) "
    "and Bx : ((fun (T : Type 1) => T) Ax) -> Type 0 := bx (bxVal : Ax) : Bx bxVal\n",
    "def result : Type 1 := Ax\n");
  failed += !domain_accepts("block field uses the settled universe of a later sort",
    "family Ax := ax (axField : Bx ((fun (T : Type 1) => Nat) (Bx Nat))) "
    "and Bx : Type 0 -> Type 0 := bx (bxType : Type 0) : Bx bxType\n",
    "def result : Type 1 := Ax\n");
  failed += !domain_refuses("settled block headers keep exact universe equality",
    "family Ax := ax (axType : Type 0) "
    "and Bx : ((fun (T : Type 0) => T) Ax) -> Type 0 := bx (bxVal : Ax) : Bx bxVal\n",
    "def result : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_refuses("settled block fields keep exact universe equality",
    "family Ax := ax (axField : Bx ((fun (T : Type 0) => Nat) (Bx Nat))) "
    "and Bx : Type 0 -> Type 0 := bx (bxType : Type 0) : Bx bxType\n",
    "def result : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_refuses("settled block universes are not cumulative",
    "family Ax := ax (axVal : Nat) "
    "and Bx : ((fun (T : Type 1) => T) Ax) -> Type 0 := bx (bxVal : Ax) : Bx bxVal\n",
    "def result : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_roundtrip("elim motive inference distinguishes a case from the scrutinee",
    "family Fx := fx0 | fx1 family Gx := gx\n",
    "def run : Fx -> Gx := fun (x : Fx) => elim (fun (u : Fx) => Gx) gx gx x\n");
  failed += !domain_accepts("bare partial elim supplies the scrutinee before selecting a family",
    "family Fx := fx0 | fx1 family Gx := gx\n",
    "def run : Fx -> Gx := elim (fun u => Gx) gx gx\n");
  failed += !domain_roundtrip("elim motive inference validates arguments after a function result",
    "family Fx := fx0 | fx1 family Gx := gx family Hx := hx0 | hx1 | hx2\n",
    "def run : Fx -> Hx -> Gx := fun (x : Fx) (y : Hx) => "
    "elim (fun (u : Fx) => Hx -> Gx) (fun (z : Hx) => gx) (fun (z : Hx) => gx) x y\n");
  failed += !domain_roundtrip("indexed dependent elim motive round trip", "",
    "def identityProof : (n : Nat) -> (v : Vec Nat n) -> Path (Vec Nat n) v v := fun (n : Nat) (v : Vec Nat n) => "
    "elim (fun (k : Nat) (w : Vec Nat k) => Path (Vec Nat k) w w) refl "
    "(fun l h t ih => refl) (fun l x y r ih => fun (i : I) => refl) v\n");
  /* S2 chunk C, the M7 guest test: a fold at a neutral quotEq case g
     stores its ends in the PAPP. A cong line quotes them, and the line at
     i1 reads the stored i1 end (trunc0 2, not the i0 end trunc0 a). */
  failed += !domain_accepts("cong at i1 of a fold with a neutral path case reads the i1 end", "",
    "def Eq4 : Fin 4 -> Fin 4 -> Type 0 := fun (a : Fin 4) (b : Fin 4) => Path (Fin 4) a b\n"
    "def selfPath : (x : Trunc0 (Fin 4)) -> Path (Trunc0 (Fin 4)) x x := fun (x : Trunc0 (Fin 4)) => refl\n"
    "def foldG : ((a : Fin 4) -> (b : Fin 4) -> Path (Fin 4) a b -> Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) a) (trunc0 (Fin 4) b)) -> Quot (Fin 4) Eq4 -> Trunc0 (Fin 4) := "
    "fun (g : (a : Fin 4) -> (b : Fin 4) -> Path (Fin 4) a b -> Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) a) (trunc0 (Fin 4) b)) (z : Quot (Fin 4) Eq4) => "
    "fold (fun (a : Fin 4) => trunc0 (Fin 4) a) g "
    "(fun (x : Trunc0 (Fin 4)) (y : Trunc0 (Fin 4)) (p : Path (Trunc0 (Fin 4)) x y) (q : Path (Trunc0 (Fin 4)) x y) => fun (i : I) => fun (j : I) => squash0 (Fin 4) x y p q i j) z\n"
    "def atEnd : (g : (a : Fin 4) -> (b : Fin 4) -> Path (Fin 4) a b -> Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) a) (trunc0 (Fin 4) b)) -> (a : Fin 4) -> (r : Path (Fin 4) a 2) -> "
    "Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) 2) (trunc0 (Fin 4) 2) := "
    "fun (g : (a : Fin 4) -> (b : Fin 4) -> Path (Fin 4) a b -> Path (Trunc0 (Fin 4)) (trunc0 (Fin 4) a) (trunc0 (Fin 4) b)) (a : Fin 4) (r : Path (Fin 4) a 2) => "
    "cong selfPath (fun (i : I) => foldG g (quotEq (Fin 4) Eq4 a 2 r i)) @ i1\n");
  /* S2 chunk C, the N4 killer: fold_res reads the target of the sort of a
     block fold. Odd has a path constructor, so an hcomp at Odd is formal,
     and the fold of it composes at the Odd target Fin 2, not at the Even
     target Nat. The round trip prints the hcomp type and checks it again. */
  failed += !domain_roundtrip("block fold of a formal hcomp at the second sort composes at its own target",
    "family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even) | oloop (i : I) : Odd [ ~i -> os ez, i -> os ez ]\n",
    "def oddHcomp : (o : Odd) -> Path (Fin 2) (fold Nat (Fin 2) 0 (fun (u : Fin 2) => 1) (fun (n : Nat) => 0) (fun (i : I) => 0) o) "
    "(fold Nat (Fin 2) 0 (fun (u : Fin 2) => 1) (fun (n : Nat) => 0) (fun (i : I) => 0) o) := "
    "fun (o : Odd) => fun (j : I) => fold Nat (Fin 2) 0 (fun (u : Fin 2) => 1) (fun (n : Nat) => 0) (fun (i : I) => 0) "
    "(hcomp Odd (~j \\/ j) (fun (i : I) => [ ~j -> o, j -> o ]) o)\n");
  /* S3 chunk B: a path at each base type. A Text with a zero byte prints
     with \x00 and reads back; an Int64 sum and a big Nat sum normalize;
     a JSON render then parse is the tree. */
  failed += !roundtrip("a Text path with a zero byte", "def t : Path Text \"a\\x00b\" \"a\\x00b\" := refl\n");
  failed += !roundtrip("an Int64 sum", "def q : Int64 := int64Add 2 3\n");
  failed += !roundtrip("a big Nat sum is the literal 2^64",
    "def r : Path Nat (natAdd 18446744073709551615 1) 18446744073709551616 := refl\n");
  failed += !accepts("a JSON render then parse gives the tree",
    "def j : Path (Sum Text Json) (jsonParse (jsonRender (jarr (jlCons (jnum (inl 1)) jlNil)))) "
    "(inr (jarr (jlCons (jnum (inl 1)) jlNil))) := refl\n");
  failed += !roundtrip("a negative Int64 prints as source",
    "def r : Int64 := int64Neg 5\n");
  failed += !roundtrip("the minimum Int64 prints as source",
    "def r : Int64 := int64Add 9223372036854775807 1\n");
  failed += !roundtrip("negative Int64 path endpoints print as source",
    "def r : Path Int64 (int64Neg 5) (int64Sub 0 5) := refl\n");
  failed += !roundtrip("negative Int64 constructor fields print as source",
    "def r : Option Int64 := int64Rem (int64Neg 7) 2\n");
  failed += !append_bounds();
  failed += !accepts("JSON refuses a leading zero",
    "def r : Path (Sum Text Json) (jsonParse \"01\") (inl \"byte 1: a number has a leading zero\") := refl\n");
  failed += !accepts("JSON refuses a negative leading zero",
    "def r : Path (Sum Text Json) (jsonParse \"[-01]\") (inl \"byte 3: a number has a leading zero\") := refl\n");
  failed += !accepts("JSON accepts zero before a fraction or exponent",
    "def r : Path (Sum Text Json) (jsonParse \"[0.1,-0e2]\") "
    "(inr (jarr (jlCons (jnum (inr \"0.1\")) (jlCons (jnum (inr \"-0e2\")) jlNil)))) := refl\n");
  failed += !roundtrip("decEqNat at equal big Nats",
    "def r : Sum (Path Nat 18446744073709551616 18446744073709551616) "
    "(Path Nat 18446744073709551616 18446744073709551616 -> Empty) := decEqNat 18446744073709551616 18446744073709551616\n");
  failed += !roundtrip("decEqNat at different big Nats of the same width",
    "def r : Sum (Path Nat 18446744073709551616 18446744073709551617) "
    "(Path Nat 18446744073709551616 18446744073709551617 -> Empty) := decEqNat 18446744073709551616 18446744073709551617\n");
  failed += !roundtrip("decEqNat at a big Nat and a word with the same limb count",
    "def r : Sum (Path Nat 18446744073709551616 3) "
    "(Path Nat 18446744073709551616 3 -> Empty) := decEqNat 18446744073709551616 3\n");
  failed += !roundtrip("an unfold with a big bound can terminate immediately",
    "def r : List Nat := unfold (fun (s : Nat) => none) (natAdd 18446744073709551615 1) 0\n");
  failed += !roundtrip("a neutral unfold retains its big bound",
    "def r : (Nat -> Option (Prod Nat Nat)) -> List Nat := fun (step : Nat -> Option (Prod Nat Nat)) => "
    "unfold step 18446744073709551616 0\n");
  failed += !accepts("a partially evaluated unfold decrements its big bound",
    "def r : (step : Nat -> Option (Prod Nat Nat)) -> Path (List Nat) "
    "(unfold (fun (s : Nat) => finCase 2 (natEq s 0) (step s) (some (pair 7 1))) 18446744073709551616 0) "
    "(cons 7 (unfold (fun (s : Nat) => finCase 2 (natEq s 0) (step s) (some (pair 7 1))) 18446744073709551615 1)) "
    ":= fun (step : Nat -> Option (Prod Nat Nat)) => refl\n");
  failed += !big_fold_fuel();
  failed += !accepts("big arithmetic carries and borrows through every limb",
    "def a : Path Nat (succ 79228162514264337593543950335) 79228162514264337593543950336 := refl\n"
    "def b : Path Nat (natSub 79228162514264337593543950336 1) 79228162514264337593543950335 := refl\n"
    "def c : Path Nat (natMul 18446744073709551616 18446744073709551616) 340282366920938463463374607431768211456 := refl\n"
    "def d : Path Nat (natSub 18446744073709551616 1) 18446744073709551615 := refl\n"
    "def e : Path Nat (natMul 18446744073709551616 0) 0 := refl\n");
  failed += !json_domain_shape("Text", "Sum Int64 Text", "Fin 2", "", NULL);
  failed += !json_domain_shape("Empty", "Sum Int64 Text", "Fin 2", "", "JSON_FAMILY");
  failed += !json_domain_shape("Text", "Text", "Fin 2", "", "JSON_FAMILY");
  failed += !json_domain_shape("Text", "Sum Int64 Text", "Nat", "", "JSON_FAMILY");
  failed += !json_domain_shape("Text", "Sum Int64 Text", "Fin 2", "| jextra", "JSON_FAMILY");
  /* S3 chunk C: the Json block and the Log family. elim refuses a block
     sort (elim-block.lang), so the Json case is the block fold: it counts
     the nodes of a tree with an array, an object and a string. The Log case
     is an elim with a constant motive; it agrees with logLength and gives 2. */
  failed += !accepts("the Json block fold counts the nodes",
    "def r : Path Nat (fold Nat Nat Nat 1 (fun (b : Fin 2) => 1) (fun (n : Sum Int64 Text) => 1) (fun (s : Text) => 1) "
    "(fun (items : Nat) => succ items) (fun (fields : Nat) => succ fields) 0 (fun (h : Nat) (t : Nat) => natAdd h t) "
    "0 (fun (k : Text) (v : Nat) (t : Nat) => natAdd v t) "
    "(jarr (jlCons jnull (jlCons (jobj (jfCons \"k\" (jstr \"a\") jfNil)) jlNil)))) 4 := refl\n");
  failed += !json_nesting();
  failed += !accepts("the Log elim agrees with logLength",
    "def s : Path Nat (elim (fun (l : Log) => Nat) 0 (fun (rest : Log) (ih : Nat) (entry : Json) => succ ih) "
    "(logAppend (logAppend logEmpty jnull) (jbool true))) (logLength (logAppend (logAppend logEmpty jnull) (jbool true))) := refl\n"
    "def t : Path Nat (elim (fun (l : Log) => Nat) 0 (fun (rest : Log) (ih : Nat) (entry : Json) => succ ih) "
    "(logAppend (logAppend logEmpty jnull) (jbool true))) 2 := refl\n");
  /* S4 chunk A: the cofamilies. A refused layer is extra domain text, not
     a change to domain/domain.lang. */
  failed += !domain_refuses("cofamily: the body is not a domain family",
    "cofamily Pairs (A : Type 0) := Prod A\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: an indexed layer",
    "family IxF (A : Type 0) (X : Type 0) : Nat -> Type 0 := ixz : IxF A X 0\n"
    "cofamily Ix (A : Type 0) := IxF A\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: a layer with a path constructor",
    "family CircF (A : Type 0) (X : Type 0) := cbase | cloop (i : I) [ ~i -> cbase A X, i -> cbase A X ]\n"
    "cofamily Circ (A : Type 0) := CircF A\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: X in the domain of a Pi field",
    "family BackF (A : Type 0) (X : Type 0) := back (backRead : X -> A)\n"
    "cofamily Back (A : Type 0) := BackF A\n", "", "CODATA_POSITIVE");
  failed += !domain_refuses("cofamily: X under List",
    "family RoseF (A : Type 0) (X : Type 0) := rose (roseLabel : A) (roseKids : List X)\n"
    "cofamily Rose (A : Type 0) := RoseF A\n", "", "CODATA_POSITIVE");
  failed += !domain_accepts("cofamily: a Pi chain field, the Mealy shape",
    "family PairInF (I0 : Type 0) (X : Type 0) := pairIn (pairInNext : I0 -> I0 -> X)\n"
    "cofamily PairIn (I0 : Type 0) := PairInF I0\n",
    "def sum : PairIn Nat := unfold (fun (s : Nat) => pairIn Nat Nat (fun (a : Nat) (b : Nat) => natAdd s (natAdd a b))) 0\n"
    "def counter : Mealy Nat Nat := unfold (fun (s : Nat) => mealy Nat Nat Nat s (fun (i : Nat) => natAdd s i)) 0\n");
  failed += !accepts("out gives one layer F ps (C ps)",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def layer : StreamF Nat (Stream Nat) := out nats\n"
    "def step : Stream Nat -> StreamF Nat (Stream Nat) := fun (x : Stream Nat) => out x\n");
  failed += !roundtrip("the unfold of a cofamily prints and checks again",
    "def r : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n");
  failed += !domain_refuses("cofamily: a layer above Type 0",
    "family LargeF (X : Type 0) := largeF (largeType : Type 0) (largeNext : X)\n"
    "cofamily Large := LargeF\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: a parameter shadows the layer name",
    "family ShadowF (A : Type 0) (X : Type 0) := shadowF (shadowNext : X)\n"
    "cofamily Shadow (ShadowF : Type 0) := ShadowF ShadowF\n", "", "CODATA_FORM");
  failed += !domain_refuses("cofamily: shadowed parameters are not in order",
    "family DupF (A : Type 0) (B : Type 0) (X : Type 0) := dupF (dupNext : X)\n"
    "cofamily Dup (A : Type 0) (A : Type 0) := DupF A A\n", "", "CODATA_FORM");
  failed += !roundtrip("out of a canonical unfold round trips",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def layer : StreamF Nat (Stream Nat) := out nats\n");
  failed += !roundtrip("coiteration with an Option seed round trips",
    "def seed : Option Nat := none\n"
    "def nats : Stream Nat := unfold (fun (s : Option Nat) => scons Nat (Option Nat) 0 s) seed\n");
  failed += !roundtrip("coiteration with a function seed round trips",
    "def seed : Nat -> Nat := fun (n : Nat) => n\n"
    "def nats : Stream Nat := unfold (fun (s : Nat -> Nat) => scons Nat (Nat -> Nat) (s 0) s) seed\n");
  failed += !domain_accepts("cofamily: dependent parameters keep their order",
    "family DepF (A : Type 0) (a : A) (X : Type 0) := depF (depNext : X)\n"
    "cofamily Dep (A : Type 0) (a : A) := DepF A a\n",
    "def value : Dep Nat 0 := unfold (fun (n : Nat) => depF Nat 0 Nat (succ n)) 0\n"
    "def layer : DepF Nat 0 (Dep Nat 0) := out value\n");
  failed += !roundtrip("coiteration types under binders use fresh names",
    "def make : (A : Type 0) -> Option A -> Stream Nat := "
    "fun (A : Type 0) (_case0 : Option A) => unfold "
    "(fun (s : Option A) => scons Nat (Option A) 0 s) _case0\n");
  failed += !roundtrip("coiteration in path endpoints retains its types",
    "def seed : Option Nat := none\n"
    "def nats : Stream Nat := unfold (fun (s : Option Nat) => scons Nat (Option Nat) 0 s) seed\n"
    "def same : Path (Stream Nat) nats nats := refl\n");
  failed += !roundtrip("coiteration with a cofamily seed round trips",
    "def seed : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def nats : Stream Nat := unfold (fun (s : Stream Nat) => scons Nat (Stream Nat) 0 s) seed\n");
  /* S4 chunk B: the out step. */
  failed += !accepts("out: one layer",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def one : Path (StreamF Nat (Stream Nat)) (out nats) "
    "(scons Nat (Stream Nat) 0 (unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 1)) := refl\n");
  failed += !accepts("out: two layers",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def two : Path (StreamF Nat (Stream Nat)) "
    "(out (elim (fun (l : StreamF Nat (Stream Nat)) => Stream Nat) (fun (h : Nat) (t : Stream Nat) => t) (out nats))) "
    "(scons Nat (Stream Nat) 1 (unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 2)) := refl\n");
  failed += !domain_accepts("out: a Pi chain field maps under its binders",
    "family PairInF (I0 : Type 0) (X : Type 0) := pairIn (pairInNext : I0 -> I0 -> X)\n"
    "cofamily PairIn (I0 : Type 0) := PairInF I0\n",
    "def counter : Mealy Nat Nat := unfold (fun (s : Nat) => mealy Nat Nat Nat s (fun (i : Nat) => natAdd s i)) 0\n"
    "def counterOut : Path (MealyF Nat Nat (Mealy Nat Nat)) (out counter) (mealy Nat Nat (Mealy Nat Nat) 0 "
    "(fun (i : Nat) => unfold (fun (s : Nat) => mealy Nat Nat Nat s (fun (i : Nat) => natAdd s i)) (natAdd 0 i))) := refl\n"
    "def sum : PairIn Nat := unfold (fun (s : Nat) => pairIn Nat Nat (fun (a : Nat) (b : Nat) => natAdd s (natAdd a b))) 0\n"
    "def sumOut : Path (PairInF Nat (PairIn Nat)) (out sum) (pairIn Nat (PairIn Nat) "
    "(fun (a : Nat) (b : Nat) => unfold (fun (s : Nat) => pairIn Nat Nat (fun (a : Nat) (b : Nat) => natAdd s (natAdd a b))) "
    "(natAdd 0 (natAdd a b)))) := refl\n");
  failed += !roundtrip("out of a variable is stuck and round trips",
    "def same : (x : Stream Nat) -> Path (StreamF Nat (Stream Nat)) (out x) (out x) := fun (x : Stream Nat) => refl\n"
    "def step : Stream Nat -> StreamF Nat (Stream Nat) := fun (x : Stream Nat) => out x\n");
  failed += !roundtrip("transp at a cofamily: a constant line gives the base, a line that changes is neutral",
    "def nats : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat n (succ n)) 0\n"
    "def constant : Path (Stream Nat) (transp (fun (i : I) => Stream Nat) i0 nats) nats := refl\n"
    "def moved : (A : Type 0) -> (B : Type 0) -> Path (Type 0) A B -> Stream A -> Stream B := "
    "fun (A : Type 0) (B : Type 0) (p : Path (Type 0) A B) (s : Stream A) => transp (fun (i : I) => Stream (p @ i)) i0 s\n");
  failed += !out_fuel();
  /* S4 chunk C: refl after eval. */
  failed += !accepts("unfold: refl of two unfolds after a take",
    "def zerosA : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat 0 n) 0\n"
    "def zerosB : Stream Nat := unfold (fun (n : Nat) => scons Nat Nat 0 (succ n)) 0\n"
    "def take : Nat -> Stream Nat -> List Nat := fun (n : Nat) => fold "
    "(fun (rest : Stream Nat -> List Nat) (s : Stream Nat) => fold (fun (h : Nat) (t : Stream Nat) => cons h (rest t)) (out s)) "
    "(fun (s : Stream Nat) => nil) n\n"
    "def same : Path (List Nat) (take 3 zerosA) (take 3 zerosB) := refl\n");
  failed += !domain_refuses("unfold: refl of g at 0 and g at 1 after eval", "",
    "def g : Nat -> StreamF Nat Nat := fun (n : Nat) => scons Nat Nat n (succ n)\n"
    "def bad : Path (Stream Nat) (unfold g (natSub 1 1)) (unfold g (natAdd 0 1)) := refl\n", "PATH_END");
  /* S5 chunk A: the IO block, and pure, map and bind at IO. */
  failed += !accepts("IO: the domain IO block gives a layer and a type",
    "def l : IOF Unit Nat := writeOp Unit Nat \"a\" 0\n"
    "def t : Type 0 := IO (Option Text)\n");
  failed += !accepts("IO: pure at IO A is ioPure, one ret layer",
    "def p : IO Nat := pure 7\n"
    "def same : Path (IOF Nat (IO Nat)) (out p) (ret Nat (IO Nat) 7) := refl\n");
  failed += !accepts("IO: map at IO A",
    "def m : IO Nat := map (fun (u : Unit) => 7) (write \"a\")\n");
  failed += !accepts("IO: bind at IO A",
    "def b : IO Unit := bind readLine (fun (o : Option Text) => option (write \"none\") (fun (t : Text) => write t) o)\n");
  failed += !accepts("IO: out (write \"a\") has the type IOF Unit (IO Unit)",
    "def l : IOF Unit (IO Unit) := out (write \"a\")\n");
  failed += !accepts("IO: out readLine has the type IOF (Option Text) (IO (Option Text))",
    "def l : IOF (Option Text) (IO (Option Text)) := out readLine\n");
  failed += !io_domain_case("IO: pure rejects a helper returning Nat",
    "def ioPure : (A : Type 0) -> A -> Nat := fun (A : Type 0) (a : A) => 0\n",
    "def bad : IO Nat := pure 7\n", "TYPE_MISMATCH");
  failed += !io_domain_case("IO: map rejects a helper returning Nat",
    "def ioMap : (A : Type 0) -> (B : Type 0) -> (A -> B) -> IO A -> Nat := fun (A : Type 0) (B : Type 0) (f : A -> B) (m : IO A) => 0\n",
    "def bad : IO Nat := map (fun (n : Nat) => n) base\n", "TYPE_MISMATCH");
  failed += !io_domain_case("IO: bind rejects a helper returning Nat",
    "def ioBind : (A : Type 0) -> (B : Type 0) -> IO A -> (A -> IO B) -> Nat := fun (A : Type 0) (B : Type 0) (m : IO A) (f : A -> IO B) => 0\n",
    "def bad : IO Nat := bind base (fun (n : Nat) => base)\n", "TYPE_MISMATCH");
  failed += !io_domain_case("IO: pure rejects a term parameter in place of a type",
    "def ioPure : Nat -> Nat -> IO Nat := fun (a : Nat) (b : Nat) => base\n",
    "def bad : IO Nat := pure 7\n", "TYPE_MISMATCH");
  failed += !io_domain_case("IO: map rejects a term parameter in place of its second type",
    "def ioMap : (A : Type 0) -> Nat -> (A -> Nat) -> IO A -> IO Nat := fun (A : Type 0) (b : Nat) (f : A -> Nat) (m : IO A) => base\n",
    "def bad : IO Nat := map (fun (n : Nat) => n) base\n", "TYPE_MISMATCH");
  failed += !io_domain_case("IO: pure works with a compatible replacement helper",
    "def ioPure : (A : Type 0) -> A -> IO A := fun (A : Type 0) (a : A) => makeIO A a\n",
    "def good : IO Nat := pure 7\n", NULL);
  failed += !io_domain_case("IO: map works with a compatible replacement helper",
    "def ioMap : (A : Type 0) -> (B : Type 0) -> (A -> B) -> IO A -> IO B := fun (A : Type 0) (B : Type 0) (f : A -> B) (m : IO A) => makeIO B (f (fold (fun (a : A) => a) (out m)))\n",
    "def good : IO Nat := map (fun (n : Nat) => n) base\n", NULL);
  failed += !io_domain_case("IO: bind works with a compatible replacement helper",
    "def ioBind : (A : Type 0) -> (B : Type 0) -> IO A -> (A -> IO B) -> IO B := fun (A : Type 0) (B : Type 0) (m : IO A) (f : A -> IO B) => f (fold (fun (a : A) => a) (out m))\n",
    "def good : IO Nat := bind base (fun (n : Nat) => base)\n", NULL);
  failed += !io_domain_case("IO: missing pure helper", "",
    "def bad : IO Nat := pure 7\n", "NAME_UNKNOWN");
  failed += !io_domain_case("IO: missing map helper", "",
    "def bad : IO Nat := map (fun (n : Nat) => n) base\n", "NAME_UNKNOWN");
  failed += !io_domain_case("IO: missing bind helper", "",
    "def bad : IO Nat := bind base (fun (n : Nat) => base)\n", "NAME_UNKNOWN");
  failed += !domain_refuses("IO: inferred map result must inhabit Type 0", "",
    "def bad : Nat := fold (fun t => 0) (fun t => 0) (fun k => 0) (fun w n => 0) (fun k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p k => 0) (fun p q k => 0) (fun p q k => 0) (fun p k => 0) (fun p j k => 0) (out (map (fun (n : Nat) => Nat) (ioPure Nat 0)))\n", "TYPE_MISMATCH");
  failed += !domain_refuses("IO: an inferred function type can also be too large", "",
    "def bad : Nat := fold (fun t => 0) (fun t => 0) (fun k => 0) (fun w n => 0) (fun k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p k => 0) (fun p q k => 0) (fun p q k => 0) (fun p k => 0) (fun p j k => 0) (out (map (fun (n : Nat) => fun (T : Type 0) => T) (ioPure Nat 0)))\n", "TYPE_MISMATCH");
  failed += !domain_refuses("IO: neutral map result keeps its universe", "",
    "def bad : (T : Type 1) -> T -> Nat := fun (T : Type 1) (x : T) => fold (fun t => 0) (fun t => 0) (fun k => 0) (fun w n => 0) (fun k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p k => 0) (fun p q k => 0) (fun p q k => 0) (fun p k => 0) (fun p j k => 0) (out (map (fun (n : Nat) => x) (ioPure Nat 0)))\n", "TYPE_MISMATCH");
  failed += !accepts("IO: map infers a neutral small result type",
    "def good : (F : Nat -> Type 0) -> (k : Nat) -> F k -> Nat := fun (F : Nat -> Type 0) (k : Nat) (x : F k) => fold (fun t => 0) (fun t => 0) (fun k => 0) (fun w n => 0) (fun k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p k => 0) (fun p q k => 0) (fun p q k => 0) (fun p k => 0) (fun p j k => 0) (out (map (fun (n : Nat) => x) (ioPure Nat 0)))\n");
  failed += !accepts("IO: a shadowed type binder keeps its original level",
    "def good : (A : Type 0) -> A -> (A : Type 0) -> Nat := fun (A : Type 0) (x : A) (A : Type 0) => fold (fun t => 0) (fun t => 0) (fun k => 0) (fun w n => 0) (fun k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p q k => 0) (fun p k => 0) (fun p k => 0) (fun p q k => 0) (fun p q k => 0) (fun p k => 0) (fun p j k => 0) (out (map (fun (n : Nat) => x) (ioPure Nat 0)))\n");
  failed += !accepts("IO: map and bind propagate the IO hint to pure",
    "def good : IO Nat := map (fun (n : Nat) => natAdd n 1) (pure 7)\n"
    "def bound : IO Nat := bind (pure 7) (fun (n : Nat) => pure n)\n");
  failed += !io_deep_type();
  {
    static char *const io_argv[] = {"prog", "a", "b c"};
    failed += !io_driver_case("echo", io_echo, "ab\ncd\n", 0, NULL, EVAL_FUEL_STEPS, "ab|", 0, NULL);
    failed += !io_driver_case("end of input", io_echo, "", 0, NULL, EVAL_FUEL_STEPS, "none", 0, NULL);
    failed += !io_driver_case("a last line with no newline", io_echo, "xy", 0, NULL, EVAL_FUEL_STEPS, "xy|", 0, NULL);
    failed += !io_driver_case("args", "def main : IO Unit := bind args (fun (j : Json) => write (jsonRender j))\n", "",
      3, io_argv, EVAL_FUEL_STEPS, "[\"a\",\"b c\"]", 0, NULL);
    failed += !io_write_bytes();
    failed += !io_segment_budget();
    failed += !io_driver_case("the tau loop trap", "def main : IO Unit := unfold (fun (u : Unit) => tau Unit Unit u) unit\n", "",
      0, NULL, 1000, "", 1, "EVAL_FUEL");
    failed += !io_driver_case("the ioBind effect order",
      "def main : IO Unit := bind (write \"a\") (fun (u : Unit) => write \"b\")\n", "", 0, NULL, EVAL_FUEL_STEPS, "ab", 0, NULL);
    failed += !io_loop_rounds();
    failed += fs_read_rows();
    failed += fs_write_rows();
  }
  printf("path regressions: %d checked, %d failed\n", cases_run, failed);
  return failed != 0;
}
