/* The terminal runner (S5 Q10 (a)). Each test/term/NAME.lang is a fixture.
   NAME.code holds a number or a diagnostic code. A number gives a run
   fixture: langc builds build/term/NAME, the runner runs it with the lines
   of NAME.args as the arguments (no file: none) and NAME.in as the standard
   input (no file: /dev/null), and compares the standard output with
   NAME.out and the exit code with the number. If NAME.err exists, the
   standard error must start with its text. A code gives a refusal
   fixture: langc builds into build/term/none/NAME, a directory that does
   not exist, and must exit 1 with "langc: CODE: " at the start of the
   standard error. An fs row (S6a, S6b) first gets its own tree under
   build/fs/term/NAME from the table fs_items. The row trash-home runs with
   a copy of the environment that has HOME under build/fs/term/trash-home
   (fixture_env), never the real HOME. The runner writes only under
   build/term/ and build/fs/term/. */
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

#define MAX_FIXTURES 64
#define MAX_ARGS 16
#define MAX_ENV 512
#define NAME_BYTES 128
#define PATH_BYTES 512
#define TEXT_BYTES 65536

static char names[MAX_FIXTURES][NAME_BYTES];

/* Runs ARGV with the standard streams IN, OUT and ERR and the environment
   ENVP. Returns the exit code, or -1. */
static int run(char *const *argv, const char *in, const char *out, const char *err, char *const *envp) {
  posix_spawn_file_actions_t actions;
  pid_t pid;
  int status = 0;
  int spawned;
  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_addopen(&actions, 0, in, O_RDONLY, 0);
  posix_spawn_file_actions_addopen(&actions, 1, out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  posix_spawn_file_actions_addopen(&actions, 2, err, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  spawned = posix_spawn(&pid, argv[0], &actions, NULL, argv, envp) == 0;
  posix_spawn_file_actions_destroy(&actions);
  if (!spawned || waitpid(pid, &status, 0) != pid) return -1;
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* Reads at most CAP - 1 bytes of PATH into BUF with a NUL after them.
   Returns the length, or -1 if the file does not open. */
static long slurp(const char *path, char *buf, size_t cap) {
  FILE *file = fopen(path, "rb");
  size_t len;
  buf[0] = '\0';
  if (!file) return -1;
  len = fread(buf, 1, cap - 1, file);
  buf[len] = '\0';
  fclose(file);
  return (long)len;
}

static int exists(const char *path) {
  struct stat info;
  return stat(path, &info) == 0;
}

/* The fs rows (S6a) and their trees. Each row gets its own directory
   build/fs/term/NAME. Each item is a directory, a file with LEN bytes of
   TEXT or a link to TEXT, at PATH under that directory. A directory comes
   before its items. */
typedef enum { FS_DIR, FS_FILE, FS_LINK } FsKind;

typedef struct {
  const char *row;
  FsKind kind;
  const char *path;
  const char *text;
  size_t len;
} FsItem;

static const char *const fs_rows[] = {"copy-tree", "disk-usage", "find-glob", "list-dir", "make-dir", "move-refuse", "read-file",
  "search-text", "trash-home"};

static const FsItem fs_items[] = {
  {"list-dir", FS_FILE, "b.txt", "b\n", 2},
  {"list-dir", FS_FILE, "a.txt", "a\n", 2},
  {"list-dir", FS_DIR, "sub", NULL, 0},
  {"list-dir", FS_LINK, "l", "a.txt", 0},
  {"find-glob", FS_FILE, "a.c", "", 0},
  {"find-glob", FS_FILE, "b.h", "", 0},
  {"find-glob", FS_DIR, "sub", NULL, 0},
  {"find-glob", FS_FILE, "sub/c.c", "", 0},
  {"find-glob", FS_DIR, "sub/deep", NULL, 0},
  {"find-glob", FS_FILE, "sub/deep/d.c", "", 0},
  {"find-glob", FS_LINK, "link", "sub", 0},
  {"read-file", FS_FILE, "note.txt", "line one\nline two\n", 18},
  {"search-text", FS_FILE, "a.txt", "one apple\ntwo\n", 14},
  {"search-text", FS_FILE, "b.txt", "apple pie", 9},
  {"search-text", FS_FILE, "bin.dat", "apple\0x\n", 8},
  {"move-refuse", FS_FILE, "a.txt", "a\n", 2},
  {"move-refuse", FS_FILE, "b.txt", "b\n", 2},
  {"copy-tree", FS_DIR, "d", NULL, 0},
  {"copy-tree", FS_FILE, "d/f.txt", "f\n", 2},
  {"copy-tree", FS_DIR, "d/sub", NULL, 0},
  {"copy-tree", FS_LINK, "d/l", "f.txt", 0},
  {"trash-home", FS_FILE, "a.txt", "a\n", 2},
  {"trash-home", FS_DIR, "home", NULL, 0},
};

/* Makes build/fs/term/NAME and its items when NAME is an fs row. Returns 0
   and writes a FAIL line if a step fails, else 1. */
static int fs_tree(const char *name) {
  char path[PATH_BYTES];
  size_t i;
  int row = 0;
  for (i = 0; i < sizeof fs_rows / sizeof fs_rows[0]; i++) row = row || strcmp(fs_rows[i], name) == 0;
  if (!row) return 1;
  snprintf(path, sizeof path, "build/fs/term/%s", name);
  if (mkdir(path, 0755) != 0) {
    printf("FAIL terminal %s: cannot make %s\n", name, path);
    return 0;
  }
  for (i = 0; i < sizeof fs_items / sizeof fs_items[0]; i++) {
    const FsItem *item = &fs_items[i];
    FILE *file;
    int ok = 0;
    if (strcmp(item->row, name) != 0) continue;
    snprintf(path, sizeof path, "build/fs/term/%s/%s", name, item->path);
    switch (item->kind) {
    case FS_DIR:
      ok = mkdir(path, 0755) == 0;
      break;
    case FS_LINK:
      ok = symlink(item->text, path) == 0;
      break;
    case FS_FILE:
      file = fopen(path, "wb");
      ok = file != NULL && fwrite(item->text, 1, item->len, file) == item->len;
      ok = file != NULL && fclose(file) == 0 && ok;
      break;
    }
    if (!ok) {
      printf("FAIL terminal %s: cannot make %s\n", name, path);
      return 0;
    }
  }
  return 1;
}

static int by_name(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b);
}

/* Puts the names of the test/term/NAME.lang files in NAMES in order.
   Returns the count, or -1. */
static int fixtures(void) {
  DIR *dir = opendir("test/term");
  struct dirent *entry;
  int count = 0;
  if (!dir) return -1;
  while ((entry = readdir(dir)) != NULL) {
    size_t len = strlen(entry->d_name);
    if (len <= 5 || len - 5 >= NAME_BYTES || strcmp(entry->d_name + len - 5, ".lang") != 0) continue;
    if (count == MAX_FIXTURES) {
      closedir(dir);
      return -1;
    }
    memcpy(names[count], entry->d_name, len - 5);
    names[count][len - 5] = '\0';
    count++;
  }
  closedir(dir);
  qsort(names, (size_t)count, sizeof names[0], by_name);
  return count;
}

/* A run fixture with the exit code CODE. Returns 1 if it passes. */
/* The environment of the run fixture NAME:  environ, but trash-home gets a
   copy without XDG_DATA_HOME, with HOME=CWD/build/fs/term/trash-home/home
   (kickoff section 5: no test uses the real HOME, and the runner does not
   call setenv). Returns
   NULL when the copy cannot be made. */
static char *const *fixture_env(const char *name) {
  static char home[PATH_BYTES];
  static char *env[MAX_ENV + 2];
  char cwd[PATH_BYTES];
  size_t n = 0;
  size_t i;
  int len;
  if (strcmp(name, "trash-home") != 0) return environ;
  if (getcwd(cwd, sizeof cwd) == NULL) return NULL;
  len = snprintf(home, sizeof home, "HOME=%s/build/fs/term/trash-home/home", cwd);
  if (len < 0 || (size_t)len >= sizeof home) return NULL;
  for (i = 0; environ[i] != NULL && n < MAX_ENV; i++)
    if (strncmp(environ[i], "HOME=", 5) != 0 && strncmp(environ[i], "XDG_DATA_HOME=", 14) != 0)
      env[n++] = environ[i];
  if (environ[i] != NULL) return NULL;
  env[n++] = home;
  env[n] = NULL;
  return env;
}

static int run_fixture(const char *name, int code) {
  static char want[TEXT_BYTES];
  static char got[TEXT_BYTES];
  static char args[TEXT_BYTES];
  char source[PATH_BYTES], exe[PATH_BYTES], in[PATH_BYTES], out[PATH_BYTES], err[PATH_BYTES], path[PATH_BYTES];
  char *argv[MAX_ARGS + 2];
  char *build[6];
  char *line;
  int argc = 1;
  int status;
  long want_len;
  long got_len;
  char *const *env;
  snprintf(source, sizeof source, "test/term/%s.lang", name);
  snprintf(exe, sizeof exe, "build/term/%s", name);
  snprintf(out, sizeof out, "build/term/%s.stdout", name);
  snprintf(err, sizeof err, "build/term/%s.stderr", name);
  build[0] = "build/langc";
  build[1] = "build";
  build[2] = source;
  build[3] = "-o";
  build[4] = exe;
  build[5] = NULL;
  status = run(build, "/dev/null", out, err, environ);
  if (status != 0) {
    printf("FAIL terminal %s: langc build exit %d (%s)\n", name, status, err);
    return 0;
  }
  snprintf(path, sizeof path, "test/term/%s.args", name);
  argv[0] = exe;
  if (slurp(path, args, sizeof args) > 0)
    for (line = strtok(args, "\n"); line && argc <= MAX_ARGS; line = strtok(NULL, "\n")) argv[argc++] = line;
  argv[argc] = NULL;
  snprintf(in, sizeof in, "test/term/%s.in", name);
  if (!exists(in)) snprintf(in, sizeof in, "/dev/null");
  env = fixture_env(name);
  if (env == NULL) {
    printf("FAIL terminal %s: cannot make the environment\n", name);
    return 0;
  }
  status = run(argv, in, out, err, env);
  snprintf(path, sizeof path, "test/term/%s.out", name);
  want_len = slurp(path, want, sizeof want);
  got_len = slurp(out, got, sizeof got);
  if (want_len < 0) want_len = 0;
  if (status != code) {
    printf("FAIL terminal %s: want exit %d, got %d\n", name, code, status);
    return 0;
  }
  if (got_len != want_len || memcmp(got, want, (size_t)want_len) != 0) {
    printf("FAIL terminal %s: the output differs from %s\n", name, path);
    return 0;
  }
  snprintf(path, sizeof path, "test/term/%s.err", name);
  want_len = slurp(path, want, sizeof want);
  got_len = slurp(err, got, sizeof got);
  if (want_len > 0 && (got_len < want_len || memcmp(got, want, (size_t)want_len) != 0)) {
    printf("FAIL terminal %s: the standard error does not start with %s\n", name, path);
    return 0;
  }
  return 1;
}

/* A refusal fixture with the diagnostic code CODE. Returns 1 if it
   passes. */
static int refusal_fixture(const char *name, const char *code) {
  static char text[TEXT_BYTES];
  char source[PATH_BYTES], exe[PATH_BYTES], out[PATH_BYTES], err[PATH_BYTES], prefix[PATH_BYTES];
  char *build[6];
  int status;
  snprintf(source, sizeof source, "test/term/%s.lang", name);
  snprintf(exe, sizeof exe, "build/term/none/%s", name);
  snprintf(out, sizeof out, "build/term/%s.stdout", name);
  snprintf(err, sizeof err, "build/term/%s.stderr", name);
  snprintf(prefix, sizeof prefix, "langc: %s: ", code);
  build[0] = "build/langc";
  build[1] = "build";
  build[2] = source;
  build[3] = "-o";
  build[4] = exe;
  build[5] = NULL;
  status = run(build, "/dev/null", out, err, environ);
  slurp(err, text, sizeof text);
  if (status != 1 || strncmp(text, prefix, strlen(prefix)) != 0) {
    printf("FAIL terminal %s: want exit 1 and %s, got exit %d and %.80s\n", name, code, status, text);
    return 0;
  }
  return 1;
}

int main(void) {
  static char code[NAME_BYTES];
  char path[PATH_BYTES];
  char *clean[] = {"/bin/rm", "-rf", "build/term", NULL};
  char *clean_fs[] = {"/bin/rm", "-rf", "build/fs/term", NULL};
  /* A write to a closed socket or pipe gives EPIPE, not the signal (S7
     Q17 (a)). */
  signal(SIGPIPE, SIG_IGN);
  int count;
  int failed = 0;
  int i;
  if (run(clean, "/dev/null", "/dev/null", "/dev/null", environ) != 0 || mkdir("build/term", 0755) != 0) {
    printf("FAIL terminal: cannot make build/term\n");
    return 1;
  }
  if (run(clean_fs, "/dev/null", "/dev/null", "/dev/null", environ) != 0 || (mkdir("build/fs", 0755) != 0 && errno != EEXIST)
      || mkdir("build/fs/term", 0755) != 0) {
    printf("FAIL terminal: cannot make build/fs/term\n");
    return 1;
  }
  count = fixtures();
  if (count < 0) {
    printf("FAIL terminal: cannot read test/term\n");
    return 1;
  }
  for (i = 0; i < count; i++) {
    long len;
    snprintf(path, sizeof path, "test/term/%s.code", names[i]);
    len = slurp(path, code, sizeof code);
    if (len > 0 && code[len - 1] == '\n') code[len - 1] = '\0';
    if (len <= 0) {
      printf("FAIL terminal %s: no %s\n", names[i], path);
      failed++;
    } else if (code[0] >= '0' && code[0] <= '9') {
      failed += !fs_tree(names[i]) || !run_fixture(names[i], atoi(code));
    } else {
      failed += !refusal_fixture(names[i], code);
    }
  }
  printf("terminal: %d checked, %d failed\n", count, failed);
  return failed != 0 || count == 0;
}
