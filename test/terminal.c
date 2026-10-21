/* The terminal runner (S5 Q10 (a)). Each test/term/NAME.lang is a fixture.
   NAME.code holds a number or a diagnostic code. A number gives a run
   fixture: langc builds build/term/NAME, the runner runs it with the lines
   of NAME.args as the arguments (no file: none) and NAME.in as the standard
   input (no file: /dev/null), and compares the standard output with
   NAME.out and the exit code with the number. A code gives a refusal
   fixture: langc builds into build/term/none/NAME, a directory that does
   not exist, and must exit 1 with "langc: CODE: " at the start of the
   standard error. The runner writes only under build/term/. */
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

extern char **environ;

#define MAX_FIXTURES 64
#define MAX_ARGS 16
#define NAME_BYTES 128
#define PATH_BYTES 512
#define TEXT_BYTES 65536

static char names[MAX_FIXTURES][NAME_BYTES];

/* Runs ARGV with the standard streams IN, OUT and ERR. Returns the exit
   code, or -1. */
static int run(char *const *argv, const char *in, const char *out, const char *err) {
  posix_spawn_file_actions_t actions;
  pid_t pid;
  int status = 0;
  int spawned;
  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_addopen(&actions, 0, in, O_RDONLY, 0);
  posix_spawn_file_actions_addopen(&actions, 1, out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  posix_spawn_file_actions_addopen(&actions, 2, err, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  spawned = posix_spawn(&pid, argv[0], &actions, NULL, argv, environ) == 0;
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
  status = run(build, "/dev/null", out, err);
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
  status = run(argv, in, out, err);
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
  status = run(build, "/dev/null", out, err);
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
  int count;
  int failed = 0;
  int i;
  if (run(clean, "/dev/null", "/dev/null", "/dev/null") != 0 || mkdir("build/term", 0755) != 0) {
    printf("FAIL terminal: cannot make build/term\n");
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
      failed += !run_fixture(names[i], atoi(code));
    } else {
      failed += !refusal_fixture(names[i], code);
    }
  }
  printf("terminal: %d checked, %d failed\n", count, failed);
  return failed != 0 || count == 0;
}
