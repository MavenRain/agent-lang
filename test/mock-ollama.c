/* One loopback connection, with a response supplied by the regression. */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>

/* The review mock (one child for each response). terminal.c does not use
   it:  it defines MOCK_SCRIPT_ONLY (check-clang -Wunused-function). */
#ifndef MOCK_SCRIPT_ONLY
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

#endif

/* The scripted mock (S7 Q12 (a), kickoff section 1.5): one listen socket
   at 127.0.0.1 port 0, one forked child for all rows, and one script for
   each request path. */
typedef enum {
  MOCK_ONE,
  MOCK_STREAM,
  MOCK_LENGTH,
  MOCK_CLOSE,
  MOCK_ECHO,
  MOCK_404,
  MOCK_500,
  MOCK_MIDSTREAM,
  MOCK_BAD,
  MOCK_CUT,
  MOCK_SHORT,
  MOCK_LF,
  MOCK_SLOW,
  MOCK_NUMBERS,
  MOCK_EMPTY,
  MOCK_MANY,
  MOCK_CHAT,
  MOCK_NONE
} MockScript;

/* The request paths, in the order of MockScript. */
static const char *const mock_paths[] = {
  "/mock/one", "/mock/stream", "/mock/length", "/mock/close", "/mock/echo", "/mock/404", "/mock/500",
  "/mock/midstream", "/mock/bad", "/mock/cut", "/mock/short", "/mock/lf", "/mock/slow", "/mock/numbers",
  "/mock/empty", "/mock/many", "/mock/chat",
};

/* /mock/numbers: an Int64, a fraction and a large integer (S7 Q11 (a)). */
#define MOCK_NUMBERS_BODY "{\"total_duration\":10706818083,\"temperature\":0.8,\"context\":[1,-2,30000000000000000000]}"

static const char mock_chunked[] =
  "HTTP/1.1 200 OK\r\nContent-Type: application/x-ndjson\r\nTransfer-Encoding: chunked\r\n\r\n";

/* Writes LEN bytes of DATA to FD (a write loop, EINTR). Gives 1 when it
   writes all of them. */
static int mock_put(int fd, const char *data, size_t len) {
  size_t sent = 0;
  while (sent < len) {
    ssize_t got = write(fd, data + sent, len - sent);
    if (got < 0 && errno == EINTR) continue;
    if (got <= 0) return 0;
    sent += (size_t)got;
  }
  return 1;
}

static int mock_text(int fd, const char *text) {
  return mock_put(fd, text, strlen(text));
}

/* A response with a Content-Length. STATUS is the code and the reason. */
static int mock_sized(int fd, const char *status, const char *body, size_t len) {
  char head[160];
  int size = snprintf(head, sizeof head, "HTTP/1.1 %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n",
                      status, len);
  return size > 0 && (size_t)size < sizeof head && mock_put(fd, head, (size_t)size) && mock_put(fd, body, len);
}

static int mock_json(int fd, const char *status, const char *body) {
  return mock_sized(fd, status, body, strlen(body));
}

/* One chunk: the size in hex, EXT (a chunk extension or ""), CRLF, DATA,
   CRLF. */
static int mock_chunk(int fd, const char *ext, const char *data) {
  char head[64];
  size_t len = strlen(data);
  int size = snprintf(head, sizeof head, "%zx%s\r\n", len, ext);
  return size > 0 && (size_t)size < sizeof head && mock_put(fd, head, (size_t)size) && mock_put(fd, data, len)
    && mock_put(fd, "\r\n", 2u);
}

/* 1 when the headers of REQUEST (the bytes before END) hold TEXT. */
static int mock_header(const char *request, const char *end, const char *text) {
  const char *at = strstr(request, text);
  return at != NULL && at < end;
}

/* /mock/echo: the request body (BODY, LEN bytes) back, when the request
   has the HTTP/1.1 request line and all four headers match the body. */
static int mock_echo(int fd, const char *request, unsigned port, const char *body, size_t len) {
  static const char line[] = "POST /mock/echo HTTP/1.1\r\n";
  char host[64];
  char length[64];
  const char *end = strstr(request, "\r\n\r\n");
  snprintf(host, sizeof host, "\r\nHost: 127.0.0.1:%u\r\n", port);
  snprintf(length, sizeof length, "\r\nContent-Length: %zu\r\n", len);
  if (strncmp(request, line, sizeof line - 1u) != 0 || end == NULL || !mock_header(request, end, host)
      || !mock_header(request, end, "\r\nContent-Type: application/json\r\n")
       || !mock_header(request, end, length)
      || !mock_header(request, end, "\r\nConnection: close\r\n"))
    return mock_sized(fd, "400 Bad Request", "bad headers", 11u);
  return mock_sized(fd, "200 OK", body, len);
}

/* /mock/many: 1000 generate stream objects to the close. */
static int mock_many(int fd) {
  char line[48];
  int ok = mock_text(fd, "HTTP/1.0 200 OK\r\n\r\n");
  int i = 0;
  while (ok && i < 1000) {
    int size = snprintf(line, sizeof line, "{\"i\":%d,\"done\":false}\n", i);
    ok = size > 0 && mock_put(fd, line, (size_t)size);
    i++;
  }
  return ok;
}

/* The script of REQUEST: its start is "POST PATH ". */
static MockScript mock_script(const char *request) {
  int i = 0;
  while (strncmp(request, "POST ", 5u) == 0 && i < (int)MOCK_NONE) {
    size_t len = strlen(mock_paths[i]);
    if (strncmp(request + 5, mock_paths[i], len) == 0 && request[5u + len] == ' ') return (MockScript)i;
    i++;
  }
  return MOCK_NONE;
}

/* Reads one request from FD (the headers to CRLF CRLF and the
   Content-Length bytes of the body), then writes the response of its
   script. PORT is the listen port (the Host header of /mock/echo). */
static void mock_answer(int fd, unsigned port) {
  static char request[65536];
  size_t have = 0;
  size_t need = sizeof request - 1u;
  size_t head = 0;
  while (have < need) {
    ssize_t got = read(fd, request + have, need - have);
    if (got < 0 && errno == EINTR) continue;
    if (got <= 0) break;
    have += (size_t)got;
    request[have] = '\0';
    if (head == 0u) {
      const char *end = strstr(request, "\r\n\r\n");
      const char *length = strstr(request, "\r\nContent-Length: ");
      size_t body = length != NULL && end != NULL && length < end ? (size_t)strtoul(length + 18, NULL, 10) : 0u;
      head = end != NULL ? (size_t)(end + 4 - request) : 0u;
      need = head == 0u ? need : head + body < sizeof request - 1u ? head + body : sizeof request - 1u;
    }
  }
  request[have] = '\0';
  switch (mock_script(request)) {
    case MOCK_ONE:
      (void)mock_json(fd, "200 OK", "{\"model\":\"m\",\"response\":\"hi\",\"done\":true}");
      break;
    case MOCK_STREAM:
      (void)(mock_text(fd, mock_chunked) && mock_chunk(fd, "", "{\"response\":\"a\",\"done\":false}\n")
             && mock_chunk(fd, ";x=1", "{\"response\":\"b\",")
             && mock_chunk(fd, "", "\"done\":false}\n{\"response\":\"\",\"done\":true}\n")
             && mock_text(fd, "0\r\nX-Note: end\r\n\r\n"));
      break;
    case MOCK_LENGTH:
      (void)mock_sized(fd, "200 OK", "{\"n\":1}\n{\"n\":2}\n", 16u);
      break;
    case MOCK_CLOSE:
      (void)mock_text(fd, "HTTP/1.0 200 OK\r\n\r\n{\"n\":1}\r\n\r\n{\"n\":2}");
      break;
    case MOCK_ECHO:
      (void)mock_echo(fd, request, port, request + head, have > head ? have - head : 0u);
      break;
    case MOCK_404:
      (void)mock_json(fd, "404 Not Found", "{\"error\":\"model 'x' not found\"}");
      break;
    case MOCK_500:
      (void)mock_sized(fd, "500 Internal Server Error", "boom", 4u);
      break;
    case MOCK_MIDSTREAM:
      (void)mock_json(fd, "200 OK", "{\"response\":\"a\",\"done\":false}\n{\"error\":\"out of memory\"}\n");
      break;
    case MOCK_BAD:
      (void)mock_sized(fd, "200 OK", "{\"n\":1}\n{\"n\":}\n", 15u);
      break;
    case MOCK_CUT:
      (void)(mock_text(fd, mock_chunked) && mock_chunk(fd, "", "{\"n\":1}\n") && mock_text(fd, "20\r\n{\"n\":2"));
      break;
    case MOCK_SHORT:
      (void)mock_text(fd, "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n{\"n\":1}\n");
      break;
    case MOCK_LF:
      (void)mock_text(fd, "HTTP/1.1 200 OK\nContent-Length: 7\n\n{\"n\":1}");
      break;
    case MOCK_SLOW:
      sleep(1);
      break;
    case MOCK_NUMBERS:
      (void)mock_sized(fd, "200 OK", MOCK_NUMBERS_BODY, sizeof MOCK_NUMBERS_BODY - 1u);
      break;
    case MOCK_EMPTY:
      (void)mock_sized(fd, "200 OK", "", 0u);
      break;
    case MOCK_MANY:
      (void)mock_many(fd);
      break;
    case MOCK_CHAT:
      (void)mock_json(fd, "200 OK", "{\"model\":\"m\",\"message\":{\"role\":\"assistant\",\"content\":\"hi there\"},\"done\":true}");
      break;
    case MOCK_NONE:
      (void)mock_sized(fd, "404 Not Found", "{\"error\":\"no script\"}", 21u);
      break;
  }
}

/* A listen socket at 127.0.0.1 and a free port (bind port 0, S7 Q12 (a)):
   gives the fd and *PORT, or -1 (errno is the error). */
static int mock_listen(unsigned *port) {
  struct sockaddr_in address;
  socklen_t size = sizeof address;
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&address, 0, sizeof address);
  address.sin_family = AF_INET;
  if (fd < 0) return -1;
  if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1
      || bind(fd, (struct sockaddr *)&address, sizeof address) != 0 || listen(fd, 16) != 0
      || getsockname(fd, (struct sockaddr *)&address, &size) != 0) {
    int error = errno;
    close(fd);
    errno = error;
    return -1;
  }
  *port = (unsigned)ntohs(address.sin_port);
  return fd;
}

/* The child: answers each connection on FD until the accept times out
   (30 s: an orphan child ends) or SIGTERM ends it. */
static void mock_serve(int fd, unsigned port) {
  struct timeval wait = {30, 0};
  int serving = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &wait, sizeof wait) == 0;
  while (serving) {
    int client = accept(fd, NULL, NULL);
    if (client >= 0) {
      mock_answer(client, port);
      close(client);
    }
    serving = client >= 0 || errno == EINTR;
  }
}
