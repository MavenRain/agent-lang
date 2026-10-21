/* The IO driver (S5 Q6 (a)): v := main; then out v (1 fuel). ret ends the
   run, tau t gives v := t, and an operation runs, then v := k r. The
   driver finds the layer constructors by name once at the start. A
   filesystem operation (S6) gives inl of the text `NAME: PATH: TEXT`
   (Q12 (a)) or inr of the value: a value, not an exit. */
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

#include "runtime/driver.h"

typedef struct {
  uint32_t io;
  uint32_t ret;
  uint32_t tau;
  uint32_t read_line;
  uint32_t write;
  uint32_t args;
  uint32_t list_dir;
  uint32_t find;
  uint32_t read_file;
  uint32_t search;
  uint32_t disk_usage;
  uint32_t make_dir;
  uint32_t move;
  uint32_t copy;
  uint32_t trash;
  uint32_t jnum;
  uint32_t jstr;
  uint32_t jarr;
  uint32_t jobj;
  uint32_t jl_nil;
  uint32_t jl_cons;
  uint32_t jf_nil;
  uint32_t jf_cons;
} IoNames;

/* The object keys, in the PLAN 6.2 order of each object (kickoff 1.3). */
typedef enum { KEY_NAME, KEY_KIND, KEY_PATH, KEY_LINE, KEY_TEXT, KEY_TOTAL, KEY_FREE, KEY_AVAILABLE, KEY_COUNT } Key;

/* The listDir kinds (S6 Q16 (a)), from lstat. */
typedef enum { KIND_FILE, KIND_DIR, KIND_LINK, KIND_OTHER, KIND_COUNT } Kind;

static const char *const key_names[KEY_COUNT] = {"name", "kind", "path", "line", "text", "total", "free", "available"};
static const char *const kind_names[KIND_COUNT] = {"file", "dir", "link", "other"};

/* The environment and the values that each response shares: one key
   text for each key and run (S6 Q14 (a)). */
typedef struct {
  const IoEnv *env;
  const Value *key[KEY_COUNT];
  const Value *kind[KIND_COUNT];
  const Value *jl_nil;
  const Value *jf_nil;
} IoRun;

typedef enum {
  LAYER_RET,
  LAYER_TAU,
  LAYER_READ_LINE,
  LAYER_WRITE,
  LAYER_ARGS,
  LAYER_LIST_DIR,
  LAYER_FIND,
  LAYER_READ_FILE,
  LAYER_SEARCH,
  LAYER_DISK_USAGE,
  LAYER_MAKE_DIR,
  LAYER_MOVE,
  LAYER_COPY,
  LAYER_TRASH,
  LAYER_OTHER
} Layer;

static int find_family(const Machine *m, const char *name, uint32_t *out) {
  for (uint32_t i = 0; i < m->family_count; i++) {
    if (strcmp(m->families[i].name, name) == 0) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

/* Finds the constructor NAME of FAMILY with FIELDS point fields. */
static int find_ctor(const Machine *m, uint32_t family, const char *name, uint32_t fields, uint32_t *out) {
  for (uint32_t i = 0; i < m->ctor_count; i++) {
    if (m->ctors[i].family == family && m->ctors[i].field_count == fields && strcmp(m->ctors[i].name, name) == 0) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

int io_family(const Machine *m, uint32_t *out) {
  return find_family(m, "IO", out) && m->families[*out].cofamily && m->families[*out].param_count == 1u;
}

int io_main(Machine *m, uint32_t *index) {
  for (uint32_t i = 0; i < m->def_count; i++) {
    if (strcmp(m->defs[i].name, "main") == 0) {
      *index = i;
      m->def = m->defs[i].name;
      return 1;
    }
  }
  return diag_fail(m->diag, "BUILD_MAIN", NULL, "the program has no definition main");
}

static int io_names(const Machine *m, IoNames *n) {
  uint32_t layer = 0;
  uint32_t json = 0;
  uint32_t list = 0;
  uint32_t fields = 0;
  int ok = io_family(m, &n->io);
  layer = ok ? m->families[n->io].layer : 0u;
  ok = ok && find_ctor(m, layer, "ret", 1u, &n->ret) && find_ctor(m, layer, "tau", 1u, &n->tau)
    && find_ctor(m, layer, "readLineOp", 1u, &n->read_line) && find_ctor(m, layer, "writeOp", 2u, &n->write)
    && find_ctor(m, layer, "argsOp", 1u, &n->args)
    && find_ctor(m, layer, "listDirOp", 2u, &n->list_dir) && find_ctor(m, layer, "findOp", 3u, &n->find)
    && find_ctor(m, layer, "readFileOp", 2u, &n->read_file) && find_ctor(m, layer, "searchOp", 3u, &n->search)
    && find_ctor(m, layer, "diskUsageOp", 2u, &n->disk_usage)
    && find_ctor(m, layer, "makeDirOp", 2u, &n->make_dir) && find_ctor(m, layer, "moveOp", 3u, &n->move)
    && find_ctor(m, layer, "copyOp", 3u, &n->copy) && find_ctor(m, layer, "trashOp", 2u, &n->trash)
    && find_family(m, "Json", &json) && find_family(m, "JsonList", &list) && find_family(m, "JsonFields", &fields)
    && find_ctor(m, json, "jnum", 1u, &n->jnum) && find_ctor(m, json, "jstr", 1u, &n->jstr)
    && find_ctor(m, json, "jarr", 1u, &n->jarr) && find_ctor(m, json, "jobj", 1u, &n->jobj)
    && find_ctor(m, list, "jlNil", 0u, &n->jl_nil) && find_ctor(m, list, "jlCons", 2u, &n->jl_cons)
    && find_ctor(m, fields, "jfNil", 0u, &n->jf_nil) && find_ctor(m, fields, "jfCons", 3u, &n->jf_cons);
  return ok ? 1 : diag_fail(m->diag, "INTERNAL", m->def, "the domain has no IO layer of the S6b shape");
}

static Layer layer_of(const IoNames *n, const Value *layer) {
  if (!val_is(layer, OP_CTOR)) return LAYER_OTHER;
  if (layer->inst == n->ret) return LAYER_RET;
  if (layer->inst == n->tau) return LAYER_TAU;
  if (layer->inst == n->read_line) return LAYER_READ_LINE;
  if (layer->inst == n->write) return LAYER_WRITE;
  if (layer->inst == n->args) return LAYER_ARGS;
  if (layer->inst == n->list_dir) return LAYER_LIST_DIR;
  if (layer->inst == n->find) return LAYER_FIND;
  if (layer->inst == n->read_file) return LAYER_READ_FILE;
  if (layer->inst == n->search) return LAYER_SEARCH;
  if (layer->inst == n->disk_usage) return LAYER_DISK_USAGE;
  if (layer->inst == n->make_dir) return LAYER_MAKE_DIR;
  if (layer->inst == n->move) return LAYER_MOVE;
  if (layer->inst == n->copy) return LAYER_COPY;
  if (layer->inst == n->trash) return LAYER_TRASH;
  return LAYER_OTHER;
}

/* readLine: some of the next line without its \n, or none at the end of
   the input or after a read error. */
static const Value *read_line(Machine *m, FILE *in) {
  char *line = NULL;
  size_t cap = 0;
  ssize_t got = getline(&line, &cap, in);
  size_t len = got > 0 ? (size_t)got : 0u;
  size_t keep = len > 0u && line[len - 1u] == '\n' ? len - 1u : len;
  char *copy = got < 0 ? NULL : arena_alloc(m->arena, keep + 1u);
  const Value *text = NULL;
  if (copy != NULL) {
    memcpy(copy, line, keep);
    copy[keep] = '\0';
    text = val_text(m, copy, keep);
  }
  free(line);
  if (got < 0) return val_make(m, OP_NONE, 0, 0, NULL, NULL, NULL);
  if (text == NULL) {
    diag_fail(m->diag, "OOM", m->def, "no memory for a line of %zu bytes", keep);
    return NULL;
  }
  return val_make(m, OP_SOME, 0, 1, text, NULL, NULL);
}

/* args: jarr of jstr of each argument after the program name. */
static const Value *args_json(Machine *m, const IoNames *n, int argc, char *const *argv) {
  const Value *list = val_ctor(m, n->jl_nil, NULL, 0u);
  for (int i = argc - 1; i >= 1 && list != NULL; i--) {
    const Value *text = val_text(m, argv[i], strlen(argv[i]));
    const Value *str = text == NULL ? NULL : val_ctor(m, n->jstr, &text, 1u);
    const Value *cell[2] = {str, list};
    list = str == NULL ? NULL : val_ctor(m, n->jl_cons, cell, 2u);
  }
  return list == NULL ? NULL : val_ctor(m, n->jarr, &list, 1u);
}

/* write: all the bytes of the text, then fflush. */
static int write_text(Machine *m, const Value *text, FILE *out) {
  if (text->kind != VAL_TEXT) return diag_fail(m->diag, "INTERNAL", m->def, "write got a value that is not a text");
  size_t count = fwrite(text->name, 1, (size_t)text->nat, out);
  int flushed = fflush(out);
  return count == text->nat && flushed == 0 ? 1 : diag_fail(m->diag, "IO_WRITE", m->def, "cannot write %llu bytes", (unsigned long long)text->nat);
}

/* A Text of the LEN bytes at BYTES, copied into the arena. */
static const Value *arena_text(Machine *m, const char *bytes, size_t len) {
  char *copy = arena_alloc(m->arena, len + 1u);
  if (copy == NULL) {
    diag_fail(m->diag, "OOM", m->def, "no memory for a text of %zu bytes", len);
    return NULL;
  }
  if (len > 0u) memcpy(copy, bytes, len);
  copy[len] = '\0';
  return val_text(m, copy, len);
}

/* The Json makers. Each gives NULL when an argument is NULL (after an OOM
   diagnostic). */
static const Value *json_str(Machine *m, const IoNames *n, const Value *text) {
  return text == NULL ? NULL : val_ctor(m, n->jstr, &text, 1u);
}

/* jnum (inl N) when N is at most INT64_MAX, else jnum (inr) of the
   decimal text (S6 Q9 (a), O10 (a)). */
static const Value *json_num(Machine *m, const IoNames *n, uint64_t value) {
  char digits[24];
  int len = snprintf(digits, sizeof digits, "%llu", (unsigned long long)value);
  int small = value <= (uint64_t)INT64_MAX;
  const Value *num = small ? val_int64(m, (int64_t)value) : arena_text(m, digits, (size_t)len);
  const Value *sum = num == NULL ? NULL : val_make(m, small ? OP_INL : OP_INR, 0, 1, num, NULL, NULL);
  return sum == NULL ? NULL : val_ctor(m, n->jnum, &sum, 1u);
}

static const Value *json_cons(Machine *m, const IoNames *n, const Value *head, const Value *tail) {
  const Value *cell[2] = {head, tail};
  return head == NULL || tail == NULL ? NULL : val_ctor(m, n->jl_cons, cell, 2u);
}

/* jfCons KEY VALUE TAIL. */
static const Value *json_field(Machine *m, const IoNames *n, const Value *key, const Value *value, const Value *tail) {
  const Value *cell[3] = {key, value, tail};
  return value == NULL || tail == NULL ? NULL : val_ctor(m, n->jf_cons, cell, 3u);
}

static const Value *json_wrap(Machine *m, uint32_t ctor, const Value *inner) {
  return inner == NULL ? NULL : val_ctor(m, ctor, &inner, 1u);
}

/* inr V: the value of an operation. */
static const Value *fs_ok(Machine *m, const Value *v) {
  return v == NULL ? NULL : val_make(m, OP_INR, 0, 1, v, NULL, NULL);
}

/* inl of `NAME: PATH: DETAIL`, or of `NAME: DETAIL` when PATH is NULL
   (S6 Q12 (a)). */
static const Value *fs_fail(Machine *m, const char *name, const char *path, const char *detail) {
  int len = path == NULL ? snprintf(NULL, 0, "%s: %s", name, detail) : snprintf(NULL, 0, "%s: %s: %s", name, path, detail);
  char *bytes = len < 0 ? NULL : arena_alloc(m->arena, (size_t)len + 1u);
  const Value *text = NULL;
  if (bytes == NULL) {
    diag_fail(m->diag, "OOM", m->def, "no memory for the error text of %s", name);
    return NULL;
  }
  if (path == NULL)
    snprintf(bytes, (size_t)len + 1u, "%s: %s", name, detail);
  else
    snprintf(bytes, (size_t)len + 1u, "%s: %s: %s", name, path, detail);
  text = val_text(m, bytes, (size_t)len);
  return text == NULL ? NULL : val_make(m, OP_INL, 0, 1, text, NULL, NULL);
}

/* The Text TEXT as a NUL ended copy in the arena (kickoff 1.2). Gives 0
   after a diagnostic. *OUT is NULL when the text holds a NUL byte: the
   operation then gives inl. */
static int fs_text(Machine *m, const Value *text, const char **out) {
  char *copy = NULL;
  *out = NULL;
  if (text->kind != VAL_TEXT) return diag_fail(m->diag, "INTERNAL", m->def, "a filesystem operation got a value that is not a text");
  if (text->nat > 0u && memchr(text->name, '\0', (size_t)text->nat) != NULL) return 1;
  copy = arena_alloc(m->arena, (size_t)text->nat + 1u);
  if (copy == NULL) return diag_fail(m->diag, "OOM", m->def, "no memory for a path of %llu bytes", (unsigned long long)text->nat);
  if (text->nat > 0u) memcpy(copy, text->name, (size_t)text->nat);
  copy[text->nat] = '\0';
  *out = copy;
  return 1;
}

/* An entry of a walk: the path (malloc'd), the offset of its base name
   and its kind from lstat. */
typedef struct {
  char *path;
  size_t base;
  Kind kind;
} FsEntry;

/* The entries of a walk, and the first error: errno at the path FAILED
   (malloc'd; NULL when no copy was possible). ERROR 0: no error. */
typedef struct {
  FsEntry *items;
  size_t count;
  size_t cap;
  char *failed;
  int error;
} FsList;

static void fs_free(FsList *l) {
  for (size_t i = 0; i < l->count; i++) free(l->items[i].path);
  free(l->items);
  free(l->failed);
}

/* Records the first error, errno ERROR at PATH. Gives 0. */
static int fs_error(FsList *l, const char *path, int error) {
  if (l->error == 0) {
    l->failed = strdup(path);
    l->error = error;
  }
  return 0;
}

/* Adds PATH (malloc'd; the list takes it) to L. Gives 0 after an error. */
static int fs_push(FsList *l, char *path, size_t base, Kind kind) {
  if (l->count == l->cap) {
    size_t cap = l->cap == 0u ? 64u : l->cap * 2u;
    FsEntry *items = cap < l->cap ? NULL : realloc(l->items, cap * sizeof *items);
    if (items == NULL) {
      fs_error(l, path, ENOMEM);
      free(path);
      return 0;
    }
    l->items = items;
    l->cap = cap;
  }
  l->items[l->count].path = path;
  l->items[l->count].base = base;
  l->items[l->count].kind = kind;
  l->count++;
  return 1;
}

static Kind kind_of(mode_t mode) {
  if (S_ISREG(mode)) return KIND_FILE;
  if (S_ISDIR(mode)) return KIND_DIR;
  if (S_ISLNK(mode)) return KIND_LINK;
  return KIND_OTHER;
}

/* Inspect the final root component without following a link, including
   when the caller appends slashes. Keep ENOTDIR for a file with a slash. */
static int fs_root_stat(const char *path, struct stat *st) {
  size_t len = strlen(path);
  char *trimmed = NULL;
  int error;
  while (len > 1u && path[len - 1u] == '/') len--;
  if (path[len] != '\0') {
    trimmed = strndup(path, len);
    if (trimmed == NULL) return ENOMEM;
  }
  error = lstat(trimmed == NULL ? path : trimmed, st) == 0 ? 0 : errno;
  if (error == 0 && trimmed != NULL && !S_ISDIR(st->st_mode) && !S_ISLNK(st->st_mode)) error = ENOTDIR;
  free(trimmed);
  return error;
}

static int fs_walk(FsList *l, const char *dir, int deep);

/* Adds DIR/NAME to L with its kind from lstat (S6 Q11 (a): no link is
   followed). DEEP walks a directory too. */
static int fs_entry(FsList *l, const char *dir, const char *name, int deep) {
  size_t a = strlen(dir);
  size_t b = strlen(name);
  char *path = malloc(a + b + 2u);
  struct stat st;
  Kind kind = KIND_OTHER;
  if (path == NULL) return fs_error(l, dir, ENOMEM);
  memcpy(path, dir, a);
  path[a] = '/';
  memcpy(path + a + 1u, name, b + 1u);
  if (lstat(path, &st) != 0) {
    fs_error(l, path, errno);
    free(path);
    return 0;
  }
  kind = kind_of(st.st_mode);
  return fs_push(l, path, a + 1u, kind) && (!deep || kind != KIND_DIR || fs_walk(l, path, deep));
}

/* Adds each entry of the directory DIR to L, without . and .. (the
   probe: readdir gives both). The path of an entry is DIR, a '/', then
   the name (S6 Q16 (a)). DEEP walks each directory from lstat (never a
   link, Q11 (a)). */
static int fs_walk(FsList *l, const char *dir, int deep) {
  struct stat st;
  int error = fs_root_stat(dir, &st);
  DIR *d;
  struct dirent *e = NULL;
  int ok = 1;
  if (error != 0) return fs_error(l, dir, error);
  if (!S_ISDIR(st.st_mode)) return fs_error(l, dir, ENOTDIR);
  d = opendir(dir);
  if (d == NULL) return fs_error(l, dir, errno);
  while (ok && (errno = 0, e = readdir(d)) != NULL)
    if (strcmp(e->d_name, ".") != 0 && strcmp(e->d_name, "..") != 0) ok = fs_entry(l, dir, e->d_name, deep);
  if (ok && errno != 0) ok = fs_error(l, dir, errno);
  closedir(d);
  return ok;
}

/* Byte order of the paths (S6 Q10 (a)). */
static int fs_order(const void *a, const void *b) {
  return strcmp(((const FsEntry *)a)->path, ((const FsEntry *)b)->path);
}

/* Walks ROOT into L, sorted by path. Gives NULL when the walk is good, or
   the inl response of NAME (NULL after an OOM diagnostic: then *FAILED
   is 1). */
static const Value *fs_scan(Machine *m, const char *name, const char *root, int deep, FsList *l, int *failed) {
  const Value *response = NULL;
  *failed = 0;
  if (fs_walk(l, root, deep)) {
    if (l->count > 1u) qsort(l->items, l->count, sizeof *l->items, fs_order);
    return NULL;
  }
  response = fs_fail(m, name, l->failed != NULL ? l->failed : root, strerror(l->error));
  *failed = 1;
  return response;
}

/* All the bytes of PATH (Q11 (a): a link is followed), malloc'd. Gives 0,
   or errno. A directory gives EISDIR. */
static int fs_read(const char *path, char **bytes, size_t *len) {
  struct stat st;
  int fd = open(path, O_RDONLY);
  int error = 0;
  char *buf = NULL;
  size_t cap = 0;
  size_t used = 0;
  ssize_t got = 1;
  *bytes = NULL;
  *len = 0;
  if (fd < 0) return errno;
  if (fstat(fd, &st) != 0)
    error = errno;
  else if (S_ISDIR(st.st_mode))
    error = EISDIR;
  /* One byte more than the size: the read at the end gives 0 with no
     realloc. */
  cap = error == 0 && st.st_size > 0 ? (size_t)st.st_size + 1u : 4096u;
  buf = error == 0 ? malloc(cap) : NULL;
  if (error == 0 && buf == NULL) error = ENOMEM;
  while (error == 0 && got > 0) {
    if (used == cap) {
      char *grown = cap * 2u < cap ? NULL : realloc(buf, cap * 2u);
      if (grown == NULL)
        error = ENOMEM;
      else {
        buf = grown;
        cap *= 2u;
      }
    }
    got = error == 0 ? read(fd, buf + used, cap - used) : 0;
    if (got < 0)
      error = errno;
    else
      used += (size_t)got;
  }
  close(fd);
  if (error != 0) {
    free(buf);
    return error;
  }
  *bytes = buf;
  *len = used;
  return 0;
}

/* listDir (S6 row 2): the entries of PATH sorted by name, each
   {"name", "kind"}. */
static const Value *list_dir(Machine *m, const IoNames *n, const IoRun *run, const Value *arg) {
  const char *path = NULL;
  FsList l = {NULL, 0u, 0u, NULL, 0};
  const Value *list = run->jl_nil;
  const Value *response = NULL;
  int failed = 0;
  if (!fs_text(m, arg, &path)) return NULL;
  if (path == NULL) return fs_fail(m, "listDir", NULL, "the path holds a NUL byte");
  response = fs_scan(m, "listDir", path, 0, &l, &failed);
  for (size_t i = l.count; !failed && i > 0u && list != NULL; i--) {
    const FsEntry *e = &l.items[i - 1u];
    const Value *name = json_str(m, n, arena_text(m, e->path + e->base, strlen(e->path + e->base)));
    const Value *fields = json_field(m, n, run->key[KEY_NAME], name, json_field(m, n, run->key[KEY_KIND], run->kind[e->kind], run->jf_nil));
    list = json_cons(m, n, json_wrap(m, n->jobj, fields), list);
  }
  fs_free(&l);
  return failed ? response : fs_ok(m, json_wrap(m, n->jarr, list));
}

/* find (S6 row 4): the paths under ROOT whose base name matches GLOB
   (fnmatch flags 0: a `*` matches a leading dot, as find -name), sorted
   by the full path. The walk does not test the root, and a root that is
   not a directory gives ENOTDIR (Q16 (a)). */
static const Value *find_paths(Machine *m, const IoNames *n, const IoRun *run, const Value *root_arg, const Value *glob_arg) {
  const char *root = NULL;
  const char *glob = NULL;
  FsList l = {NULL, 0u, 0u, NULL, 0};
  const Value *list = run->jl_nil;
  const Value *response = NULL;
  int failed = 0;
  if (!fs_text(m, root_arg, &root) || !fs_text(m, glob_arg, &glob)) return NULL;
  if (root == NULL) return fs_fail(m, "find", NULL, "the path holds a NUL byte");
  if (glob == NULL) return fs_fail(m, "find", root, "the glob holds a NUL byte");
  response = fs_scan(m, "find", root, 1, &l, &failed);
  for (size_t i = l.count; !failed && i > 0u && list != NULL; i--) {
    const FsEntry *e = &l.items[i - 1u];
    if (fnmatch(glob, e->path + e->base, 0) == 0) list = json_cons(m, n, json_str(m, n, arena_text(m, e->path, strlen(e->path))), list);
  }
  fs_free(&l);
  return failed ? response : fs_ok(m, json_wrap(m, n->jarr, list));
}

/* readFile (S6 row 6): all the bytes of PATH; a NUL byte stays. */
static const Value *read_file(Machine *m, const Value *arg) {
  const char *path = NULL;
  char *bytes = NULL;
  size_t len = 0;
  int error = 0;
  const Value *text = NULL;
  if (!fs_text(m, arg, &path)) return NULL;
  if (path == NULL) return fs_fail(m, "readFile", NULL, "the path holds a NUL byte");
  error = fs_read(path, &bytes, &len);
  if (error != 0) return fs_fail(m, "readFile", path, strerror(error));
  text = arena_text(m, bytes, len);
  free(bytes);
  return fs_ok(m, text);
}

/* A match of search: the path text, the line number and the line text. */
typedef struct {
  const Value *path;
  uint64_t line;
  const Value *text;
} Hit;

typedef struct {
  Hit *items;
  size_t count;
  size_t cap;
} HitList;

static int hit_push(Machine *m, HitList *h, const Value *path, uint64_t line, const Value *text) {
  if (text == NULL) return 0;
  if (h->count == h->cap) {
    size_t cap = h->cap == 0u ? 64u : h->cap * 2u;
    Hit *items = cap < h->cap ? NULL : realloc(h->items, cap * sizeof *items);
    if (items == NULL) return diag_fail(m->diag, "OOM", m->def, "no memory for %zu search matches", h->count);
    h->items = items;
    h->cap = cap;
  }
  h->items[h->count].path = path;
  h->items[h->count].line = line;
  h->items[h->count].text = text;
  h->count++;
  return 1;
}

/* 1 when the LEN bytes at HAY hold the NEEDLE bytes: a byte compare (a
   line can hold a NUL byte, and memmem is not POSIX). */
static int holds_bytes(const char *hay, size_t len, const char *needle, size_t size) {
  for (size_t i = 0; size <= len && i <= len - size; i++)
    if (memcmp(hay + i, needle, size) == 0) return 1;
  return 0;
}

/* Adds the lines of the file PATH that hold NEEDLE to H. A file with a
   NUL byte in its first 8 KiB is skipped. Lines split at \n, the number
   from 1, the text without the \n (a \r stays, S6 Q5 (a)). Gives 0 after
   a diagnostic, and sets *ERROR at a read error. */
static int search_file(Machine *m, HitList *h, const char *path, const Value *needle, int *error) {
  char *bytes = NULL;
  size_t len = 0;
  size_t start = 0;
  uint64_t line = 1;
  const Value *path_text = NULL;
  int ok = 1;
  *error = fs_read(path, &bytes, &len);
  if (*error != 0) return 1;
  if (memchr(bytes, '\0', len < 8192u ? len : 8192u) == NULL) {
    while (ok && start < len) {
      const char *nl = memchr(bytes + start, '\n', len - start);
      size_t end = nl == NULL ? len : (size_t)(nl - bytes);
      if (holds_bytes(bytes + start, end - start, needle->name, (size_t)needle->nat)) {
        path_text = path_text != NULL ? path_text : arena_text(m, path, strlen(path));
        ok = path_text != NULL && hit_push(m, h, path_text, line, arena_text(m, bytes + start, end - start));
      }
      start = end + 1u;
      line++;
    }
  }
  free(bytes);
  return ok;
}

/* search (S6 row 8, O14 (a)): each line that holds TEXT, in each regular
   file under ROOT (walked as find; a link to a file is skipped), or in
   the file ROOT. Sorted by the path, then the line number (Q10 (a)). Each
   match is {"path", "line", "text"}. */
static const Value *search_text(Machine *m, const IoNames *n, const IoRun *run, const Value *root_arg, const Value *needle) {
  const char *root = NULL;
  struct stat st;
  FsList l = {NULL, 0u, 0u, NULL, 0};
  HitList h = {NULL, 0u, 0u};
  const Value *list = run->jl_nil;
  const Value *response = NULL;
  int failed = 0;
  int error = 0;
  int ok = 1;
  size_t i = 0;
  if (!fs_text(m, root_arg, &root)) return NULL;
  if (needle->kind != VAL_TEXT) {
    diag_fail(m->diag, "INTERNAL", m->def, "search got a value that is not a text");
    return NULL;
  }
  if (root == NULL) return fs_fail(m, "search", NULL, "the path holds a NUL byte");
  if (needle->nat == 0u) return fs_fail(m, "search", root, "the search text is empty");
  error = fs_root_stat(root, &st);
  if (error != 0) return fs_fail(m, "search", root, strerror(error));
  if (S_ISLNK(st.st_mode)) return fs_ok(m, json_wrap(m, n->jarr, list));
  if (S_ISREG(st.st_mode))
    ok = search_file(m, &h, root, needle, &error);
  else
    response = fs_scan(m, "search", root, 1, &l, &failed);
  for (i = 0; !failed && ok && error == 0 && i < l.count; i++)
    if (l.items[i].kind == KIND_FILE) ok = search_file(m, &h, l.items[i].path, needle, &error);
  if (!failed && ok && error != 0) {
    response = fs_fail(m, "search", i == 0u ? root : l.items[i - 1u].path, strerror(error));
    failed = 1;
  }
  for (i = h.count; !failed && ok && i > 0u && list != NULL; i--) {
    const Hit *hit = &h.items[i - 1u];
    const Value *fields = json_field(m, n, run->key[KEY_PATH], json_str(m, n, hit->path),
      json_field(m, n, run->key[KEY_LINE], json_num(m, n, hit->line), json_field(m, n, run->key[KEY_TEXT], json_str(m, n, hit->text), run->jf_nil)));
    list = json_cons(m, n, json_wrap(m, n->jobj, fields), list);
  }
  fs_free(&l);
  free(h.items);
  if (!ok) return NULL;
  return failed ? response : fs_ok(m, json_wrap(m, n->jarr, list));
}

/* diskUsage (S6 row 10, O16 (a)): statvfs of the filesystem that holds
   PATH, {"total", "free", "available"} in bytes (as df). */
static const Value *disk_usage(Machine *m, const IoNames *n, const IoRun *run, const Value *arg) {
  const char *path = NULL;
  struct statvfs s;
  uint64_t total = 0;
  uint64_t spare = 0;
  uint64_t avail = 0;
  const Value *fields = NULL;
  if (!fs_text(m, arg, &path)) return NULL;
  if (path == NULL) return fs_fail(m, "diskUsage", NULL, "the path holds a NUL byte");
  if (statvfs(path, &s) != 0) return fs_fail(m, "diskUsage", path, strerror(errno));
  /* macOS fsblkcnt_t is 4 bytes: the products are in uint64_t. */
  total = (uint64_t)s.f_blocks * (uint64_t)s.f_frsize;
  spare = (uint64_t)s.f_bfree * (uint64_t)s.f_frsize;
  avail = (uint64_t)s.f_bavail * (uint64_t)s.f_frsize;
  fields = json_field(m, n, run->key[KEY_TOTAL], json_num(m, n, total),
    json_field(m, n, run->key[KEY_FREE], json_num(m, n, spare), json_field(m, n, run->key[KEY_AVAILABLE], json_num(m, n, avail), run->jf_nil)));
  return fs_ok(m, json_wrap(m, n->jobj, fields));
}

/* inr unit: the value of a write operation (S6 Q2 (a)). */
static const Value *fs_unit(Machine *m) {
  return fs_ok(m, val_make(m, OP_UNIT_VAL, 0, 0, NULL, NULL, NULL));
}

/* The first failure of a copy, a remove or a trash: the PATH (malloc'd)
   and the errno ERROR, or the fixed DETAIL (S6 Q12 (a)). */
typedef struct {
  char *path;
  int error;
  const char *detail;
} FsFault;

/* Keeps the first failure. Gives 0. */
static int fs_fault(FsFault *f, const char *path, int error, const char *detail) {
  if (f->path != NULL || f->error != 0 || f->detail != NULL) return 0;
  f->path = strdup(path);
  f->error = f->path == NULL ? ENOMEM : error;
  f->detail = f->path == NULL ? NULL : detail;
  return 0;
}

static const Value *fs_fault_value(Machine *m, const char *name, FsFault *f) {
  const Value *v = fs_fail(m, name, f->path, f->detail != NULL ? f->detail : strerror(f->error));
  free(f->path);
  return v;
}

/* DIR, a `/`, then NAME (malloc'd). */
static char *fs_join(const char *dir, const char *name) {
  size_t a = strlen(dir);
  size_t b = strlen(name);
  char *out = malloc(a + b + 2u);
  if (out == NULL) return NULL;
  memcpy(out, dir, a);
  out[a] = '/';
  memcpy(out + a + 1u, name, b + 1u);
  return out;
}

/* Writes LEN bytes to FD. Gives 0, or the errno. */
static int fs_write_all(int fd, const char *bytes, size_t len) {
  size_t done = 0;
  while (done < len) {
    ssize_t put = write(fd, bytes + done, len - done);
    if (put < 0 && errno != EINTR) return errno;
    if (put > 0) done += (size_t)put;
  }
  return 0;
}

/* makeDir (S6 row 3, Q3 (a)): mkdir of each prefix, as mkdir -p. At
   EEXIST it goes on only when stat gives a directory (a link to a
   directory is a directory, Q11 (a)). A file at a parent gives ENOTDIR,
   a file at the path EEXIST. The error names the path as given. */
static const Value *make_dir(Machine *m, const Value *arg) {
  const char *path = NULL;
  char *buf = NULL;
  size_t len = 0;
  int error = 0;
  if (!fs_text(m, arg, &path)) return NULL;
  if (path == NULL) return fs_fail(m, "makeDir", NULL, "the path holds a NUL byte");
  len = strlen(path);
  buf = strdup(path);
  error = buf == NULL ? ENOMEM : len == 0u ? ENOENT : 0;
  for (size_t i = 1u; error == 0 && i <= len; i++) {
    struct stat st;
    if (i < len && (buf[i] != '/' || buf[i - 1u] == '/')) continue;
    buf[i] = '\0';
    error = mkdir(buf, 0777) == 0 ? 0 : errno;
    if (error == EEXIST) error = stat(buf, &st) != 0 ? errno : S_ISDIR(st.st_mode) ? 0 : i < len ? ENOTDIR : EEXIST;
    buf[i] = path[i];
  }
  free(buf);
  return error == 0 ? fs_unit(m) : fs_fail(m, "makeDir", path, strerror(error));
}

static int fs_copy(FsFault *f, const char *from, const char *to);

/* A file: create TO with O_EXCL, copy the bytes, then fchmod to the mode
   of FROM (the umask clears bits at the create). */
static int fs_copy_file(FsFault *f, const char *from, const char *to, mode_t mode) {
  char buf[16384];
  int in = open(from, O_RDONLY);
  int out = in < 0 ? -1 : open(to, O_WRONLY | O_CREAT | O_EXCL, 0600);
  int ok = in >= 0 && out >= 0;
  if (in < 0) fs_fault(f, from, errno, NULL);
  if (in >= 0 && out < 0) fs_fault(f, to, errno, NULL);
  while (ok) {
    ssize_t got = read(in, buf, sizeof buf);
    int error = got < 0 ? errno : fs_write_all(out, buf, (size_t)got);
    if (got == 0) break;
    if (got < 0 && error == EINTR) continue;
    if (error != 0) ok = fs_fault(f, got < 0 ? from : to, error, NULL);
  }
  ok = ok && (fchmod(out, mode & 07777) == 0 || fs_fault(f, to, errno, NULL));
  if (in >= 0) close(in);
  if (out >= 0 && close(out) != 0) ok = ok && fs_fault(f, to, errno, NULL);
  return ok;
}

/* A directory: mkdir, the entries, then chmod at the end (a read only
   directory can then get its entries). */
static int fs_copy_dir(FsFault *f, const char *from, const char *to, mode_t mode) {
  DIR *d = NULL;
  struct dirent *e = NULL;
  int ok = mkdir(to, 0700) == 0 || fs_fault(f, to, errno, NULL);
  /* The temporary directory must stay usable even under umask 0777. */
  if (ok && chmod(to, 0700) != 0) ok = fs_fault(f, to, errno, NULL);
  d = ok ? opendir(from) : NULL;
  if (ok && d == NULL) ok = fs_fault(f, from, errno, NULL);
  while (ok && (errno = 0, e = readdir(d)) != NULL) {
    char *a = NULL;
    char *b = NULL;
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
    a = fs_join(from, e->d_name);
    b = fs_join(to, e->d_name);
    ok = a != NULL && b != NULL ? fs_copy(f, a, b) : fs_fault(f, from, ENOMEM, NULL);
    free(a);
    free(b);
  }
  if (ok && errno != 0) ok = fs_fault(f, from, errno, NULL);
  if (d != NULL) closedir(d);
  return ok && (chmod(to, mode & 07777) == 0 || fs_fault(f, to, errno, NULL));
}

/* A link: readlink, then symlink (a link is copied as a link, Q11 (a)). */
static int fs_copy_link(FsFault *f, const char *from, const char *to, mode_t mode) {
  char target[4096];
  ssize_t len = readlink(from, target, sizeof target);
  if (len < 0) return fs_fault(f, from, errno, NULL);
  if ((size_t)len == sizeof target) return fs_fault(f, from, ENAMETOOLONG, NULL);
  target[len] = '\0';
  if (symlink(target, to) != 0) return fs_fault(f, to, errno, NULL);
#ifdef __APPLE__
  /* macOS applies umask to links and checks their read permission. */
  return fchmodat(AT_FDCWD, to, mode & 07777, AT_SYMLINK_NOFOLLOW) == 0 || fs_fault(f, to, errno, NULL);
#else
  (void)mode;
  return 1;
#endif
}

static int fs_copy(FsFault *f, const char *from, const char *to) {
  struct stat st;
  if (lstat(from, &st) != 0) return fs_fault(f, from, errno, NULL);
  if (S_ISREG(st.st_mode)) return fs_copy_file(f, from, to, st.st_mode);
  if (S_ISDIR(st.st_mode)) return fs_copy_dir(f, from, to, st.st_mode);
  if (S_ISLNK(st.st_mode)) return fs_copy_link(f, from, to, st.st_mode);
  return fs_fault(f, from, 0, "copy cannot copy this kind (not a file, a directory or a link)");
}

/* 1 when TO is in the tree of the directory FROM: the real path of the
   parent of TO (with no last slash) is the real path of FROM or under it. */
static int fs_inside(const char *from, const char *to) {
  size_t len = strlen(to);
  char *last = NULL;
  char *parent = NULL;
  char *a = realpath(from, NULL);
  char *b = NULL;
  size_t n = a == NULL ? 0u : strlen(a);
  int inside = 0;
  while (len > 1u && to[len - 1u] == '/') len--;
  parent = strndup(to, len);
  last = parent == NULL ? NULL : strrchr(parent, '/');
  if (last != NULL && last == parent) last[1] = '\0';
  else if (last != NULL) *last = '\0';
  b = parent == NULL ? NULL : realpath(last == NULL ? "." : parent, NULL);
  inside = a != NULL && b != NULL && strncmp(a, b, n) == 0 && (b[n] == '\0' || b[n] == '/' || n == 1u);
  free(parent);
  free(a);
  free(b);
  return inside;
}

/* The copy of FROM to TO. A directory refuses a TO in its own tree. */
static int fs_copy_top(FsFault *f, const char *from, const char *to) {
  struct stat st;
  if (lstat(from, &st) != 0) return fs_fault(f, from, errno, NULL);
  if (S_ISDIR(st.st_mode) && fs_inside(from, to)) return fs_fault(f, to, 0, "the destination is in the source tree");
  return fs_copy(f, from, to);
}

/* Removes the tree PATH (the EXDEV leg of move): a directory after its
   entries. A link is removed, not followed. */
static int fs_remove(FsFault *f, const char *path) {
  struct stat st;
  DIR *d = NULL;
  struct dirent *e = NULL;
  int ok = lstat(path, &st) == 0 || fs_fault(f, path, errno, NULL);
  if (ok && !S_ISDIR(st.st_mode)) return unlink(path) == 0 || fs_fault(f, path, errno, NULL);
  d = ok ? opendir(path) : NULL;
  if (ok && d == NULL) ok = fs_fault(f, path, errno, NULL);
  while (ok && (errno = 0, e = readdir(d)) != NULL) {
    char *a = NULL;
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
    a = fs_join(path, e->d_name);
    ok = a != NULL ? fs_remove(f, a) : fs_fault(f, path, ENOMEM, NULL);
    free(a);
  }
  if (ok && errno != 0) ok = fs_fault(f, path, errno, NULL);
  if (d != NULL) closedir(d);
  return ok && (rmdir(path) == 0 || fs_fault(f, path, errno, NULL));
}

/* The two path arguments of move and copy. Gives 0 after a diagnostic;
   *FROM or *TO is NULL when its text holds a NUL byte. */
static int fs_two_paths(Machine *m, const Value *a, const Value *b, const char **from, const char **to) {
  return fs_text(m, a, from) && fs_text(m, b, to);
}

/* 1 when TO is there (lstat). Else 0 with *ERROR the errno, 0 for ENOENT. */
static int fs_there(const char *to, int *error) {
  struct stat st;
  int there = lstat(to, &st) == 0;
  *error = there || errno == ENOENT ? 0 : errno;
  return there;
}

/* move (S6 row 5, Q3 (a) and Q4 (a)). rename replaces a file and an empty
   directory with no error, thus lstat (TO) first. Another process can
   make TO between the lstat and the rename: a race window. At EXDEV (or
   FORCE_EXDEV): copy with the mode bits, then remove the source tree.
   When the copy fails, the source stays. */
static const Value *move_path(Machine *m, const IoRun *run, const Value *from_arg, const Value *to_arg) {
  const char *from = NULL;
  const char *to = NULL;
  FsFault f = {NULL, 0, NULL};
  int error = 0;
  if (!fs_two_paths(m, from_arg, to_arg, &from, &to)) return NULL;
  if (from == NULL || to == NULL) return fs_fail(m, "move", NULL, "the path holds a NUL byte");
  if (fs_there(to, &error)) return fs_fail(m, "move", to, strerror(EEXIST));
  if (error != 0) return fs_fail(m, "move", to, strerror(error));
  error = run->env->force_exdev ? EXDEV : rename(from, to) == 0 ? 0 : errno;
  if (error == 0) return fs_unit(m);
  if (error != EXDEV) return fs_fail(m, "move", from, strerror(error));
  return fs_copy_top(&f, from, to) && fs_remove(&f, from) ? fs_unit(m) : fs_fault_value(m, "move", &f);
}

/* copy (S6 row 7, Q3 (a)): refuse a TO that is there (the race window of
   move), then copy the tree with the mode bits. */
static const Value *copy_path(Machine *m, const Value *from_arg, const Value *to_arg) {
  const char *from = NULL;
  const char *to = NULL;
  FsFault f = {NULL, 0, NULL};
  int error = 0;
  if (!fs_two_paths(m, from_arg, to_arg, &from, &to)) return NULL;
  if (from == NULL || to == NULL) return fs_fail(m, "copy", NULL, "the path holds a NUL byte");
  if (fs_there(to, &error)) return fs_fail(m, "copy", to, strerror(EEXIST));
  if (error != 0) return fs_fail(m, "copy", to, strerror(error));
  return fs_copy_top(&f, from, to) ? fs_unit(m) : fs_fault_value(m, "copy", &f);
}

/* The trash name of the base name BASE (LEN bytes) at the try N: BASE,
   then BASE with ` N` before the last extension (Q6 (a)). A dot at the
   start of BASE is not an extension. Gives a malloc'd text. */
static char *trash_name(const char *base, size_t len, unsigned n) {
  size_t stem = len;
  int size = 0;
  char *out = NULL;
  for (size_t i = 1u; i < len; i++)
    if (base[i] == '.') stem = i;
  size = n < 2u ? (int)len : snprintf(NULL, 0, "%.*s %u%.*s", (int)stem, base, n, (int)(len - stem), base + stem);
  out = size < 0 ? NULL : malloc((size_t)size + 1u);
  if (out != NULL && n < 2u) memcpy(out, base, len);
  if (out != NULL && n < 2u) out[len] = '\0';
  if (out != NULL && n >= 2u) snprintf(out, (size_t)size + 1u, "%.*s %u%.*s", (int)stem, base, n, (int)(len - stem), base + stem);
  return out;
}

/* Makes missing parents and DIR with mode 0700, independent of umask.
   Existing directories keep their modes. Gives 0, or the errno. */
static int trash_dir(const char *dir) {
  size_t len = strlen(dir);
  char *buf = strdup(dir);
  int error = buf == NULL ? ENOMEM : len == 0u ? ENOENT : 0;
  for (size_t i = 1u; error == 0 && i <= len; i++) {
    struct stat st;
    if (i < len && (buf[i] != '/' || buf[i - 1u] == '/')) continue;
    buf[i] = '\0';
    if (mkdir(buf, 0700) == 0) error = chmod(buf, 0700) == 0 ? 0 : errno;
    else error = errno;
    if (error == EEXIST) error = stat(buf, &st) != 0 ? errno : S_ISDIR(st.st_mode) ? 0 : ENOTDIR;
    buf[i] = dir[i];
  }
  free(buf);
  return error;
}

/* The info text of the XDG trash: `[Trash Info]`, `Path=` the percent
   encoded absolute path, `DeletionDate=` the local time. Gives a
   malloc'd text, or NULL. */
static char *trash_info(const char *path) {
  static const char hex[] = "0123456789ABCDEF";
  char cwd[4096];
  char date[32];
  time_t now = time(NULL);
  struct tm local;
  char *full = path[0] == '/' ? strdup(path) : getcwd(cwd, sizeof cwd) == NULL ? NULL : fs_join(cwd, path);
  size_t len = full == NULL ? 0u : strlen(full);
  char *out = full == NULL ? NULL : malloc(3u * len + 64u);
  size_t at = 0;
  int ok = out != NULL && localtime_r(&now, &local) != NULL && strftime(date, sizeof date, "%Y-%m-%dT%H:%M:%S", &local) != 0u;
  if (ok) at = (size_t)snprintf(out, 32u, "[Trash Info]\nPath=");
  for (size_t i = 0; ok && i < len; i++) {
    unsigned char c = (unsigned char)full[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c != 0u && strchr("-._~/", c) != NULL)) {
      out[at++] = (char)c;
      continue;
    }
    out[at++] = '%';
    out[at++] = hex[c >> 4];
    out[at++] = hex[c & 15u];
  }
  if (ok) snprintf(out + at, 3u * len + 64u - at, "\nDeletionDate=%s\n", date);
  free(full);
  if (!ok) free(out);
  return ok ? out : NULL;
}

/* The macOS form: rename into ROOT (made with mode 0700), at a name
   collision the first free name (trash_name). Another process can make
   the name between the lstat and the rename: a race window. */
static int trash_macos(FsFault *f, const char *root, const char *path, const char *base, size_t len) {
  int error = trash_dir(root);
  int done = 0;
  if (error != 0) return fs_fault(f, root, error, NULL);
  for (unsigned n = 1u; !done; n++) {
    char *name = trash_name(base, len, n);
    char *dest = name == NULL ? NULL : fs_join(root, name);
    int there = dest != NULL && fs_there(dest, &error);
    if (dest == NULL) error = ENOMEM;
    if (!there && error == 0) done = rename(path, dest) == 0 ? 1 : -1;
    if (done < 0 || error != 0) fs_fault(f, path, error != 0 ? error : errno, NULL);
    free(name);
    free(dest);
    if (error != 0) return 0;
  }
  return done > 0;
}

/* The XDG form: ROOT/info/NAME.trashinfo with O_EXCL first, then rename
   into ROOT/files/NAME. When the rename fails, remove the info file. */
static int trash_xdg(FsFault *f, const char *root, const char *path, const char *base, size_t len) {
  char *files = fs_join(root, "files");
  char *info = fs_join(root, "info");
  char *text = trash_info(path);
  int error = files == NULL || info == NULL || text == NULL ? ENOMEM : trash_dir(root);
  int done = 0;
  error = error != 0 ? error : trash_dir(files);
  error = error != 0 ? error : trash_dir(info);
  if (error != 0) fs_fault(f, root, error, NULL);
  for (unsigned n = 1u; error == 0 && !done; n++) {
    char *name = trash_name(base, len, n);
    char *dest = name == NULL ? NULL : fs_join(files, name);
    size_t size = name == NULL ? 0u : strlen(info) + strlen(name) + 12u;
    char *note = size == 0u ? NULL : malloc(size);
    int fd = -1;
    const char *where = root;
    if (note != NULL) snprintf(note, size, "%s/%s.trashinfo", info, name);
    error = dest == NULL || note == NULL ? ENOMEM : 0;
    where = dest == NULL ? root : dest;
    if (error == 0 && !fs_there(dest, &error) && error == 0) {
      where = note;
      fd = open(note, O_WRONLY | O_CREAT | O_EXCL, 0600);
      if (fd >= 0) {
        error = fchmod(fd, 0600) == 0 ? 0 : errno;
        if (error == 0) error = fs_write_all(fd, text, strlen(text));
      } else error = errno == EEXIST ? 0 : errno;
      if (fd >= 0 && close(fd) != 0 && error == 0) error = errno;
      if (fd >= 0 && error == 0) done = rename(path, dest) == 0 ? 1 : -1;
      if (done < 0) error = errno;
      if (done < 0) where = path;
      /* The info file of a failed rename (the trashed item is never
         unlinked). */
      if (fd >= 0 && done <= 0) unlink(note);
    }
    if (error != 0) fs_fault(f, where, error, NULL);
    free(name);
    free(dest);
    free(note);
  }
  free(files);
  free(info);
  free(text);
  return done > 0;
}

/* trash (S6 row 9, Q6 (a)): rename into the trash, never unlink. At EXDEV
   inl (never a copy and an unlink). No trash root (no HOME) gives inl. */
static const Value *trash_path(Machine *m, const IoRun *run, const Value *arg) {
  const char *path = NULL;
  const char *base = NULL;
  struct stat st;
  FsFault f = {NULL, 0, NULL};
  size_t len = 0;
  int ok = 0;
  if (!fs_text(m, arg, &path)) return NULL;
  if (path == NULL) return fs_fail(m, "trash", NULL, "the path holds a NUL byte");
  if (lstat(path, &st) != 0) return fs_fail(m, "trash", path, strerror(errno));
  if (run->env->trash_root == NULL) return fs_fail(m, "trash", path, "there is no trash directory (no HOME)");
  len = strlen(path);
  while (len > 1u && path[len - 1u] == '/') len--;
  base = path + len;
  while (base > path && base[-1] != '/') base--;
  ok = run->env->trash_form == TRASH_MACOS ? trash_macos(&f, run->env->trash_root, path, base, (size_t)(path + len - base))
                                           : trash_xdg(&f, run->env->trash_root, path, base, (size_t)(path + len - base));
  return ok ? fs_unit(m) : fs_fault_value(m, "trash", &f);
}

/* The shared values of a run. Gives 0 after an OOM diagnostic. */
static int io_run_init(Machine *m, const IoNames *n, const IoEnv *env, IoRun *run) {
  int ok = 1;
  run->env = env;
  for (int i = 0; i < KEY_COUNT; i++) {
    run->key[i] = val_text(m, key_names[i], strlen(key_names[i]));
    ok = ok && run->key[i] != NULL;
  }
  for (int i = 0; i < KIND_COUNT; i++) {
    run->kind[i] = json_str(m, n, val_text(m, kind_names[i], strlen(kind_names[i])));
    ok = ok && run->kind[i] != NULL;
  }
  run->jl_nil = val_ctor(m, n->jl_nil, NULL, 0u);
  run->jf_nil = val_ctor(m, n->jf_nil, NULL, 0u);
  return ok && run->jl_nil != NULL && run->jf_nil != NULL;
}

/* One layer of *V. Returns -1 to go on, 0 at ret, or 1 after a
   diagnostic. */
static int io_step(Machine *m, const IoNames *n, const IoRun *run, const Value **v) {
  const Value *layer = eval_out(m, n->io, *v);
  const Value *const *field = layer == NULL ? NULL : layer->args + m->families[n->io].param_count + 1u;
  const Value *response = NULL;
  const Value *next = NULL;
  if (layer == NULL) return 1;
  switch (layer_of(n, layer)) {
  case LAYER_RET:
    return 0;
  case LAYER_TAU:
    *v = field[0];
    return -1;
  case LAYER_READ_LINE:
    response = read_line(m, run->env->in);
    next = field[0];
    break;
  case LAYER_ARGS:
    response = args_json(m, n, run->env->argc, run->env->argv);
    next = field[0];
    break;
  case LAYER_WRITE:
    if (!write_text(m, field[0], run->env->out)) return 1;
    m->fuel = run->env->budget;
    *v = field[1];
    return -1;
  case LAYER_LIST_DIR:
    response = list_dir(m, n, run, field[0]);
    next = field[1];
    break;
  case LAYER_FIND:
    response = find_paths(m, n, run, field[0], field[1]);
    next = field[2];
    break;
  case LAYER_READ_FILE:
    response = read_file(m, field[0]);
    next = field[1];
    break;
  case LAYER_SEARCH:
    response = search_text(m, n, run, field[0], field[1]);
    next = field[2];
    break;
  case LAYER_DISK_USAGE:
    response = disk_usage(m, n, run, field[0]);
    next = field[1];
    break;
  case LAYER_MAKE_DIR:
    response = make_dir(m, field[0]);
    next = field[1];
    break;
  case LAYER_MOVE:
    response = move_path(m, run, field[0], field[1]);
    next = field[2];
    break;
  case LAYER_COPY:
    response = copy_path(m, field[0], field[1]);
    next = field[2];
    break;
  case LAYER_TRASH:
    response = trash_path(m, run, field[0]);
    next = field[1];
    break;
  case LAYER_OTHER:
    diag_fail(m->diag, "INTERNAL", m->def, "the IO driver found a layer that is not an IO constructor");
    return 1;
  }
  if (response == NULL) return 1;
  m->fuel = run->env->budget;
  *v = eval_apply(m, next, response);
  return *v == NULL ? 1 : -1;
}

int io_run(Machine *m, const Value *v, const IoEnv *env) {
  IoNames n;
  IoRun run;
  int status = v == NULL || !io_names(m, &n) || !io_run_init(m, &n, env, &run) ? 1 : -1;
  m->fuel = env->budget;
  while (status < 0)
    status = io_step(m, &n, &run, &v);
  return status;
}
