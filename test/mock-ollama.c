/* One loopback connection, with a response supplied by the regression. */
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>

typedef struct {
  pid_t pid;
  unsigned port;
} MockOllama;

static int mock_ollama_reply(int listener, const char *response, size_t len) {
  char request[4096];
  size_t have = 0;
  size_t need = sizeof request;
  size_t sent = 0;
  int fd = accept(listener, NULL, NULL);
  if (fd < 0) return 0;
  while (have < need && have + 1u < sizeof request) {
    ssize_t got = read(fd, request + have, sizeof request - have - 1u);
    if (got < 0 && errno == EINTR) continue;
    if (got <= 0) break;
    have += (size_t)got;
    request[have] = '\0';
    if (need == sizeof request) {
      const char *end = strstr(request, "\r\n\r\n");
      const char *length = strstr(request, "\r\nContent-Length: ");
      if (end != NULL && length != NULL && length < end)
        need = (size_t)(end + 4 - request) + (size_t)strtoul(length + 18, NULL, 10);
    }
  }
  while (have >= need && sent < len) {
    ssize_t got = write(fd, response + sent, len - sent);
    if (got < 0 && errno == EINTR) continue;
    if (got <= 0) break;
    sent += (size_t)got;
  }
  close(fd);
  return have >= need && sent == len;
}

static int mock_ollama_start(MockOllama *server, const char *response, size_t len) {
  struct sockaddr_in address;
  socklen_t size = sizeof address;
  int listener = socket(AF_INET, SOCK_STREAM, 0);
  memset(&address, 0, sizeof address);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(0x7f000001u);
  if (listener < 0) return 0;
  if (bind(listener, (struct sockaddr *)&address, sizeof address) != 0 || listen(listener, 1) != 0
      || getsockname(listener, (struct sockaddr *)&address, &size) != 0) {
    close(listener);
    return 0;
  }
  server->port = (unsigned)ntohs(address.sin_port);
  server->pid = fork();
  if (server->pid == 0) {
    int ok;
    signal(SIGPIPE, SIG_IGN);
    alarm(5);
    ok = mock_ollama_reply(listener, response, len);
    close(listener);
    _exit(ok ? 0 : 1);
  }
  close(listener);
  return server->pid > 0;
}

static int mock_ollama_finish(const MockOllama *server) {
  int status = 0;
  pid_t got;
  do {
    got = waitpid(server->pid, &status, 0);
  } while (got < 0 && errno == EINTR);
  return got == server->pid && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
