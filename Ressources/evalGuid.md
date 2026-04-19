# Webserv Evaluation Guide

> A complete reference for understanding and defending your HTTP server project.

---

## Table of Contents

1. [What is HTTP?](#what-is-http)
2. [How a Web Server Works](#how-a-web-server-works)
3. [Non-Blocking I/O & I/O Multiplexing](#non-blocking-io--io-multiplexing)
4. [epoll — How It Works Internally](#epoll--how-it-works-internally)
5. [Sockets — From Zero to Connection](#sockets--from-zero-to-connection)
6. [HTTP Request Parsing](#http-request-parsing)
7. [HTTP Response Building](#http-response-building)
8. [CGI — Common Gateway Interface](#cgi--common-gateway-interface)
9. [Configuration File](#configuration-file)
10. [Sessions & Cookies](#sessions--cookies)
11. [Key Project Rules (Forbidden / Allowed)](#key-project-rules)
12. [Evaluation Checklist](#evaluation-checklist)
13. [Common Questions & Answers](#common-questions--answers)
14. [Quick curl & telnet Tests](#quick-curl--telnet-tests)

---

## What is HTTP?

HTTP (Hypertext Transfer Protocol) is a **text-based, stateless, application-layer protocol**.

- Client sends a **request** → Server sends a **response**.
- Default port: **80** (HTTP) / **443** (HTTPS).
- Our server uses **HTTP/1.0** as reference.

### HTTP Request Format

```
METHOD URI VERSION\r\n
Header-Key: Header-Value\r\n
...\r\n
\r\n
[optional body]
```

**Example:**
```
GET /index.html HTTP/1.0\r\n
Host: localhost\r\n
\r\n
```

### HTTP Response Format

```
VERSION STATUS_CODE STATUS_MESSAGE\r\n
Header-Key: Header-Value\r\n
...\r\n
\r\n
[optional body]
```

**Example:**
```
HTTP/1.0 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 42\r\n
\r\n
<html><body>Hello!</body></html>
```

---

## How a Web Server Works

```
1. Create a socket (socket())
2. Bind it to ip:port (bind())
3. Mark it as passive/listening (listen())
4. Wait for events (epoll_wait())
5. Accept new client (accept())
6. Read client data (recv())
7. Parse HTTP request
8. Build HTTP response
9. Send response (send())
10. Close or keep connection
```

Our server does ALL of this **without blocking** — using `epoll`.

---

## Non-Blocking I/O & I/O Multiplexing

### Why Non-Blocking?

A **blocking** `recv()` stops the entire program until data arrives.
With many clients, this is a disaster — one slow client freezes the server.

**Non-blocking** means: if no data is ready, `recv()` returns `-1` with `errno = EAGAIN` instead of waiting.

> **IMPORTANT:** In this project, checking `errno` after `read`/`write` is **FORBIDDEN**. The return value itself tells you what happened (0 = disconnected, -1 = error/nothing yet, >0 = bytes read).

### What is I/O Multiplexing?

It lets ONE thread watch MANY file descriptors at once.
You give a list of fds to the kernel → kernel tells you which ones are ready.

**Options:** `select()`, `poll()`, `epoll()`, `kqueue()`
**We use:** `epoll()` (Linux only)

### The Rule: ONE epoll for everything

```
epoll watches:
  - server sockets (EPOLLIN → accept new client)
  - client sockets (EPOLLIN → recv data)
  - client sockets (EPOLLOUT → send data)
```

**Never** call `recv()`/`send()` on a socket without `epoll` confirming it is ready first.

---

## epoll — How It Works Internally

### Step 1: Create epoll instance

```c
int epollFd = epoll_create1(EPOLL_CLOEXEC);
// returns a file descriptor for the epoll object
```

### Step 2: Register file descriptors

```c
struct epoll_event ev;
ev.events  = EPOLLIN;          // watch for readable data
ev.data.fd = serverSocketFd;
epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocketFd, &ev);
```

Operations:
- `EPOLL_CTL_ADD` — start watching fd
- `EPOLL_CTL_MOD` — change what events to watch
- `EPOLL_CTL_DEL` — stop watching fd

### Step 3: Wait for events

```c
struct epoll_event events[MAX_EVENTS];
int n = epoll_wait(epollFd, events, MAX_EVENTS, timeout_ms);
// n = number of ready fds
// timeout = -1 means wait forever, 0 means don't wait
```

### Step 4: Loop over ready fds

```c
for (int i = 0; i < n; i++) {
    int fd = events[i].data.fd;

    if (isServerSocket(fd)) {
        acceptNewClient(fd);         // EPOLLIN on server socket
    } else if (events[i].events & EPOLLIN) {
        recvFromClient(fd);          // data arrived
    } else if (events[i].events & EPOLLOUT) {
        sendToClient(fd);            // socket ready to send
    }
}
```

### How we switch from EPOLLIN to EPOLLIN|EPOLLOUT

After a complete request is received, we modify the fd to also watch for EPOLLOUT:

```c
ev.events  = EPOLLIN | EPOLLOUT;
ev.data.fd = clientFd;
epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
```

This way, we ONLY send when the socket buffer is ready.

---

## Sockets — From Zero to Connection

### What is a file descriptor (fd)?

In Linux, **everything is a file**. A socket is just a fd (an integer).
`fd = 0` → stdin, `fd = 1` → stdout, `fd = 2` → stderr, `fd = 3+` → your sockets.

### Full server socket setup (`Socket::setup()`)

We use `getaddrinfo()` instead of filling `sockaddr_in` manually.
It resolves a hostname + port string into a list of `addrinfo` structs — we try each one until `bind()` succeeds.

```cpp
void Socket::setup() {
    addrinfo  hints;
    addrinfo *result = NULL, *rp = NULL;
    bool      bound = false;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;        // IPv4 only
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags    = AI_PASSIVE;     // use local IP if _ip is empty

    // Step 1: resolve host + port → linked list of addrinfo
    int status = getaddrinfo(_ip.empty() ? NULL : _ip.c_str(),
                             _port.c_str(), &hints, &result);
    if (status != 0)
        ERROR_LOG("Socket::setup::getaddrinfo", gai_strerror(status));

    // Step 2: try each result until socket() + setsockopt() + bind() succeed
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        _fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (_fd == -1)
            continue;                   // this address didn't work, try next

        int opt = 1;
        if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            ::close(_fd);
            _fd = -1;
            continue;                   // setsockopt failed, try next
        }
        // SO_REUSEADDR: avoid "Address already in use" after server restart

        if (bind(_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            bound = true;
            break;                      // bind succeeded, stop looping
        }
        ::close(_fd);
        _fd = -1;
    }
    freeaddrinfo(result);               // free the linked list

    if (!bound)
        ERROR_LOG("Socket::setup", "failed to bind on " + _ip + ":" + _port);

    // Step 3: set FD_CLOEXEC — fd closes automatically on execve() (CGI)
    if (fcntl(_fd, F_SETFD, FD_CLOEXEC) == -1)
        ERROR_LOG("Socket::setup::fcntl", "failed to set FD_CLOEXEC");

    // Step 4: set O_NONBLOCK — recv/send never block, return -1 if not ready
    int flags = fcntl(_fd, F_GETFL, 0);
    if (flags == -1)
        ERROR_LOG("Socket::setup::fcntl", "F_GETFL failed");
    if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1)
        ERROR_LOG("Socket::setup::fcntl", "F_SETFL O_NONBLOCK failed");

    // Step 5: mark socket as passive — ready to accept() incoming connections
    if (listen(_fd, SOMAXCONN) == -1)
        ERROR_LOG("Socket::setup::listen", "failed to listen");
}
```

**Why `getaddrinfo()` instead of `sockaddr_in` directly?**
It handles hostname resolution (e.g., `"localhost"` → `127.0.0.1`) and is more portable.
`SOMAXCONN` is the system maximum backlog (queue of pending connections before `accept()`).

### Accepting a client

```c
struct sockaddr_in client_addr;
socklen_t len = sizeof(client_addr);
int clientFd = accept(serverFd, (struct sockaddr*)&client_addr, &len);
// clientFd is a NEW fd for this specific client
// serverFd keeps listening for more clients
```

### Why htons / ntohs?

Network uses **Big Endian** byte order. Your CPU may use **Little Endian**.

- `htons(port)` → host to network short (for port numbers)
- `htonl(ip)`   → host to network long (for IP addresses)
- `ntohs()` / `ntohl()` → reverse direction (reading from network)

---

## HTTP Request Parsing

Our parser has 4 states:

```
PARSE_REQUEST_LINE → PARSE_HEADERS → PARSE_BODY → PARSE_COMPLETE
```

### State 1: Request Line

Parse: `METHOD URI VERSION\r\n`

- Validate method: `GET`, `POST`, `DELETE` → else 501
- Validate URI: max 8192 chars, no bad chars → else 400
- Match URI to a location block from config

### State 2: Headers

Parse each line: `Key: Value\r\n`
Empty line `\r\n` signals end of headers.

- Keys are stored lowercase (e.g., `content-type`, `content-length`)
- Duplicate `Content-Length` → 400

### State 3: Body

Only if `Content-Length` header is present.

- Read exactly `Content-Length` bytes
- If body > `client_max_body_size` → 413
- No chunked transfer encoding (we return 501 for that)

### State 4: Complete

Request is fully parsed. Build response.

---

## HTTP Response Building

### Response flow

```
Request valid?
  NO  → errorPage(statusCode)
  YES →
    Has location?
      NO  → 404
      YES →
        Has redirect?     → 301/302 + Location header
        Method allowed?   → NO → 405
        Method == GET?    → handleGet()
        Method == POST?   → handlePost()
        Method == DELETE? → handleDelete()
```

### GET

1. `stat()` the file path
2. If regular file → read and send
3. If directory → check for index file, then autoindex, else 403
4. Not found → 404

### POST

1. CGI? → run CGI script
2. `/login` route? → handle session
3. Upload configured? → save file to `upload_path`
4. Otherwise → 403

### DELETE

1. `stat()` path → not found → 404
2. File → `remove()` → 204 No Content
3. Directory → recursive delete → 204
4. No write permission → 403

### Status Codes Used

| Code | Meaning |
|------|---------|
| 200 | OK |
| 201 | Created (file uploaded) |
| 204 | No Content (DELETE success) |
| 301 | Moved Permanently (redirect / trailing slash) |
| 302 | Found (redirect) |
| 400 | Bad Request |
| 403 | Forbidden |
| 404 | Not Found |
| 405 | Method Not Allowed |
| 413 | Request Entity Too Large |
| 500 | Internal Server Error |
| 501 | Not Implemented |

---

## CGI — Common Gateway Interface

CGI allows the server to **run an external program** to generate a dynamic response.

### How CGI works (step by step)

```
1. Server forks() a child process
2. Child:
   a. Opens body file → dup2 it to STDIN
   b. Opens result file → dup2 it to STDOUT
   c. chdir() to script directory
   d. execve(interpreter, [interpreter, script], envp)
3. Parent:
   a. Does NOT block — uses waitpid() with WNOHANG in epoll loop
   b. Polls every epoll cycle to check if child is done
   c. If child exceeds 5 seconds → kill(pid, SIGKILL)
   d. When done → read result file → parse CGI headers → send response
```

### CGI Environment Variables

| Variable | Value |
|----------|-------|
| `REQUEST_METHOD` | GET / POST |
| `QUERY_STRING` | part after `?` in URI |
| `CONTENT_TYPE` | from request header |
| `CONTENT_LENGTH` | body size |
| `SCRIPT_FILENAME` | full path to script |
| `SCRIPT_NAME` | URI path to script |
| `PATH_INFO` | extra path after script name |
| `SERVER_NAME` | server host |
| `SERVER_PORT` | server port |
| `GATEWAY_INTERFACE` | CGI/1.1 |
| `REDIRECT_STATUS` | 200 |
| `HTTP_*` | all other request headers |

### CGI Output Format

```
Content-Type: text/html\n
\n
<html>...</html>
```

- Headers come FIRST, then blank line, then body.
- If no `Content-Length` → EOF marks end.
- We read from the temp file after CGI exits.

### Why FD_CLOEXEC?

When `fork()` + `execve()` happens, all server fds are inherited by the child.
A leaked socket fd in CGI = security hole (CGI can write to other clients!).
`FD_CLOEXEC` makes the fd close automatically on `execve()`.

---

## Configuration File

Inspired by NGINX's `server` block syntax.

### Server block

```nginx
server {
    listen localhost:8080;         # ip:port to listen on
    server_name mysite.local;      # name (virtual host)
    root ./www;                    # base directory
    index index.html;              # default file for directories
    client_max_body_size 1MB;      # max body (supports K, M, G)
    error_page 404 /error_pages/404.html;

    location /path {
        allow_methods GET POST DELETE;
        autoindex on;              # directory listing
        upload_path ./www/uploads/;
        cgi_pass .py /usr/bin/python3;
        cgi_pass .pl /usr/bin/perl;
        return 301 /new-url;
        root /override/root;
        index other.html;
    }
}
```

### Location matching

- Longest prefix match wins.
- `/kapouet` rooted to `/tmp/www` → `/kapouet/foo/bar` → `/tmp/www/foo/bar`
- No regex required.

### Multiple servers

- Different `ip:port` pairs → OK (different websites).
- Same `ip:port` on different host IPs → OK.
- Same `ip:port` same host → ERROR (unless virtual hosts).

---

## Sessions & Cookies

### How cookies work

```
1. Client visits /login → POST username
2. Server creates a session ID (random string)
3. Server stores: sessionId → { username, isLogged }
4. Server sends: Set-Cookie: session_id=<id>; Path=/; HttpOnly
5. Browser stores cookie
6. On next request: Cookie: session_id=<id>
7. Server looks up session → knows who the user is
```

### Cookie header formats

**Server → Client:**
```
Set-Cookie: session_id=id_123; Path=/; HttpOnly
```

**Client → Server:**
```
Cookie: session_id=id_123
```

### Session storage

Sessions are stored **in memory** (a `std::map<std::string, SessionInfo>`).
They are lost when the server restarts (no persistence).

---

## Key Project Rules

### FORBIDDEN
- `errno` check after `recv()`/`send()` → **grade 0**
- `read()`/`write()` on socket WITHOUT going through `epoll` first → **grade 0**
- More than one `epoll_wait()` call in the loop → **grade 0**
- `fork()` for anything other than CGI → **grade 0**
- `printf`, `malloc`, `free`, `realloc`, `calloc`
- STL containers (`vector`, `map`, `list`, etc.) — wait, `map` and `vector` ARE used (they are standard C++98)
- External libraries / Boost
- `using namespace std`
- `friend` keyword

### ALLOWED
- `epoll`, `select`, `poll`, `kqueue`
- `socket`, `bind`, `listen`, `accept`, `recv`, `send`
- `fork` + `execve` (CGI only)
- `fcntl` with: `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC`
- `std::map`, `std::vector`, `std::string` (C++98 STL is fine)
- `new` / `delete`

### One read/write per client per epoll cycle

Each time `epoll_wait()` returns an event for a client:
- Do **one** `recv()` call
- Do **one** `send()` call
Never loop `recv()` until EOF inside the event handler.

---

## Evaluation Checklist

### Code Review Questions

- [ ] Which function used for I/O multiplexing? → `epoll_wait()`
- [ ] Is there only ONE `epoll_wait()` in the main loop? → Yes
- [ ] Does `epoll` check read AND write at the same time? → `EPOLLIN | EPOLLOUT`
- [ ] Is there only one `recv()`/`send()` per client per event? → Yes
- [ ] Is `errno` checked after `read`/`write`? → NO (forbidden)
- [ ] Are all sockets non-blocking? → Yes (`O_NONBLOCK` via `fcntl`)

### Configuration Tests

```bash
# Test 1: Default error page
curl http://localhost:8080/nonexistent

# Test 2: Body size limit
curl -X POST -H "Content-Type: plain/text" --data "toolongbody" http://localhost:8080/uploads

# Test 3: Method not allowed
curl -X DELETE http://localhost:8080/

# Test 4: Redirect
curl -v http://localhost:8080/old

# Test 5: Autoindex
curl http://localhost:8080/images/
```

### Basic Feature Tests

```bash
# GET
curl http://localhost:8080/

# POST upload
curl -X POST -H "Content-Type: image/png" --data-binary @file.png http://localhost:8080/uploads

# DELETE
curl -X DELETE http://localhost:8080/uploads/somefile.png

# Unknown method (should not crash)
curl -X PATCH http://localhost:8080/
```

### CGI Tests

```bash
# GET to CGI
curl "http://localhost:8080/cgi-bin/test.py?name=hello"

# POST to CGI
curl -X POST -H "Content-Type: text/plain" \
     --data "username=test" http://localhost:8080/cgi-bin/test.py

# Test with error script (infinite loop → timeout after 5 sec)
curl http://localhost:8080/cgi-bin/loop.py
```

### Port Tests

```bash
# Two different servers
curl http://localhost:8080/
curl http://localhost:8081/
```

### Stress Test with Siege

```bash
# Install siege first (homebrew or apt)
siege -b -c 10 -r 100 http://localhost:8080/
# Availability should be > 99.5%
# Memory should not grow indefinitely
# No hanging connections
```

---

## Common Questions & Answers

**Q: What is select() / epoll() and how does it work?**
A: It is a system call that watches multiple file descriptors at once. You register fds with events (read/write). The kernel tells you which fds are ready. You then do ONE read or write on the ready fd.

**Q: How do you handle multiple clients without threads?**
A: With epoll. All clients share one event loop. We never block on any fd. Each client has its own state (receive buffer, send buffer, request, response).

**Q: Why is errno forbidden after read/write?**
A: The project spec says so. The return value is enough: `0` = disconnected, `-1` = error, `>0` = bytes transferred. Checking errno would mean we are relying on a side effect that may not be portable.

**Q: How does CGI avoid blocking the server?**
A: We `fork()` the CGI process. The parent does NOT `waitpid()` in a blocking way. Instead, every epoll cycle we call `waitpid(pid, &status, WNOHANG)`. If the child is not done yet, we return and continue the event loop. If it takes more than 5 seconds, we `kill()` it.

**Q: What is FD_CLOEXEC and why do you use it?**
A: It is a flag on a file descriptor. When you `execve()` a new program (CGI script), all fds marked with `FD_CLOEXEC` are automatically closed. This prevents the CGI child from inheriting server sockets and potentially reading/writing other clients' data.

**Q: How does path resolution work?**
A: URI → strip query string → normalize `..` and `.` → longest matching location prefix → prepend root → full filesystem path.

**Q: What happens with an unknown HTTP method?**
A: We return `501 Not Implemented`. The server never crashes.

**Q: How does autoindex work?**
A: We call `opendir()` on the directory path, loop `readdir()`, build an HTML list with links to each entry, and send it.

---

## Quick curl & telnet Tests

### Using telnet manually

```
telnet localhost 8080

# Then type:
GET / HTTP/1.0
Host: localhost
[press Enter twice]
```

### curl examples

```bash
# Verbose GET (shows request + response headers)
curl -v http://localhost:8080/

# POST with body
curl -X POST -d "key=value" http://localhost:8080/login

# Upload binary file
curl -X POST --data-binary @photo.jpg \
     -H "Content-Type: image/jpeg" \
     http://localhost:8080/uploads

# DELETE
curl -X DELETE http://localhost:8080/uploads/file.txt

# Follow redirects
curl -L http://localhost:8080/old

# Custom header
curl -H "Cookie: session_id=abc123" http://localhost:8080/dashboard.html
```

---

*Generated for webserv evaluation — mofouzi, mait-you — 42 curriculum*
