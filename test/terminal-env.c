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

  /* S7: an ollama row gets the mock port, and no fixture keeps the parent
     OLLAMA_HOST (a real Ollama can listen there). */
  {
    char parent_host[] = "OLLAMA_HOST=192.0.2.1:11434";
    char *with_host[] = {parent_home, parent_keep, parent_host, NULL};
    char *const *row;
    char *const *refused;
    char *const *trash;
    int hosts = 0;
    int good = 0;
    int stray = 0;
    environ = with_host;
    mock_port = 4321u;
    row = fixture_env("ollama-stream");
    for (i = 0; row != NULL && row[i] != NULL; i++) {
      hosts += strncmp(row[i], "OLLAMA_HOST=", 12) == 0;
      good += strcmp(row[i], "OLLAMA_HOST=127.0.0.1:4321") == 0 || row[i] == parent_home || row[i] == parent_keep;
    }
    expect(row != NULL && hosts == 1 && good == 3, "an ollama row gets OLLAMA_HOST=127.0.0.1:PORT");
    refused = fixture_env("ollama-refused");
    for (i = 0; refused != NULL && refused[i] != NULL; i++)
      stray += strncmp(refused[i], "OLLAMA_HOST=", 12) == 0 && strcmp(refused[i], "OLLAMA_HOST=127.0.0.1:1") != 0;
    trash = fixture_env("trash-home");
    for (i = 0; trash != NULL && trash[i] != NULL; i++) stray += strncmp(trash[i], "OLLAMA_HOST=", 12) == 0;
    expect(refused != NULL && trash != NULL && stray == 0, "the parent OLLAMA_HOST is dropped");
    mock_port = 0;
    environ = saved_env;
  }

  /* S8 Q12 (a): an example row builds examples/NAME.lang, another row its
     test/term/NAME.lang. */
  {
    char example[PATH_BYTES];
    char other[PATH_BYTES];
    fixture_source("organize-apply", example, sizeof example);
    fixture_source("list-dir", other, sizeof other);
    expect(strcmp(example, "examples/organize.lang") == 0 && strcmp(other, "test/term/list-dir.lang") == 0,
        "an example row builds examples/NAME.lang");
  }

  {
    char text[] = "dir\n--apply\n\n\"\"\n";
    char *argv[4];
    int count = fixture_args(text, argv, 4);
    expect(count == 4 && strcmp(argv[0], "dir") == 0
        && strcmp(argv[1], "--apply") == 0
        && strcmp(argv[2], "") == 0 && strcmp(argv[3], "") == 0,
        "fixture arguments preserve an empty final argument");
  }

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
