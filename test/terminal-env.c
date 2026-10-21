/* Checks that the trash fixture cannot inherit an external trash root. */
#define main terminal_main
#include "terminal.c"
#undef main

static int checked = 0;
static int failures = 0;

static void expect(int ok, const char *name) {
  checked++;
  if (!ok) {
    printf("FAIL terminal environment: %s\n", name);
    failures++;
  }
}

int main(void) {
  char parent_home[] = "HOME=/external/home";
  char parent_data[] = "XDG_DATA_HOME=/external/data";
  char parent_keep[] = "TERMINAL_ENV_KEEP=yes";
  char other_home[] = "HOME=/external/other";
  char *parent[] = {parent_home, parent_data, parent_keep, other_home, NULL};
  char **saved_env = environ;
  char *const *env;
  char cwd[4096];
  char expected[4096];
  size_t i;
  int homes = 0;
  int kept = 0;
  int external = 0;
  int length;
  if (getcwd(cwd, sizeof cwd) == NULL) return 1;
  length = snprintf(expected, sizeof expected, "HOME=%s/build/fs/term/trash-home/home", cwd);
  if (length < 0 || (size_t)length >= sizeof expected) return 1;
  environ = parent;
  expect(fixture_env("make-dir") == parent, "other fixtures keep the parent environment");
  env = fixture_env("trash-home");
  if (env != NULL) {
    for (i = 0; env[i] != NULL; i++) {
      if (strncmp(env[i], "HOME=", 5) == 0) {
        homes++;
        external += strcmp(env[i], expected) != 0;
      }
      if (strncmp(env[i], "XDG_DATA_HOME=", 14) == 0) external++;
      if (strcmp(env[i], parent_keep) == 0) kept++;
    }
  }
  expect(env != NULL && homes == 1 && kept == 1 && external == 0,
      "HOME is local and XDG_DATA_HOME cannot override it");
  expect(environ == parent && parent[0] == parent_home && parent[1] == parent_data
      && parent[3] == other_home, "the parent environment is unchanged");
  environ = saved_env;

  /* getcwd still fits PATH_BYTES, but adding the HOME suffix does not. */
  if ((mkdir("build", 0755) != 0 && errno != EEXIST)
      || (mkdir("build/fs", 0755) != 0 && errno != EEXIST)
      || (mkdir("build/fs/terminal-env", 0755) != 0 && errno != EEXIST)
      || chdir("build/fs/terminal-env") != 0) return 1;
  for (;;) {
    char here[4096];
    if (getcwd(here, sizeof here) == NULL) return 1;
    if (strlen(here) >= PATH_BYTES - 32) break;
    if ((mkdir("long-home-component", 0755) != 0 && errno != EEXIST)
        || chdir("long-home-component") != 0) return 1;
  }
  expect(fixture_env("trash-home") == NULL, "an overlong HOME fails before the guest runs");
  if (chdir(cwd) != 0) return 1;
  printf("terminal environment: %d checked, %d failed\n", checked, failures);
  return failures != 0;
}
