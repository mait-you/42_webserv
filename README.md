*This project has been created as part of the 42 curriculum by mofouzi, mait-you.*

# Description

Webserv is an HTTP server implemented in C++98, inspired by the NGINX web server. The goal of this project is to build a fully functional HTTP/1.0 server, handling multiple clients efficiently using non-blocking I/O and `epoll` (Linux only). The server supports static file serving, CGI execution, file uploads, and basic session management.

# Instructions

## Requirements

- Linux operating system (`epoll` is used for event handling)
- C++98 compatible compiler

## Compilation

1. Clone the repository.
2. Run `make` to build the server.

## Running the Server

```bash
./webserv [configuration file]
```

- If no configuration file is provided, the server uses the default configuration at `config/default.conf`.
- You can customize the server by editing or providing your own configuration file (see the `config/` directory for examples).

## Usage

- By default, the server listens on `localhost:8080`.
- You can change the port, IP, and other settings in the configuration file.
- The server supports `GET`, `POST`, and `DELETE` HTTP methods.
- CGI scripts are supported.
- File uploads and session management (via cookies) are implemented.

### Example Usage

1. Start the server:
```bash
./webserv config/default.conf
```
2. Open your browser at http://localhost:8080 or use `curl`:
```bash
curl http://localhost:8080
```

You can test the server using a standard web browser or command-line tools like `curl`.

# Resources

## Documentation & References

- [CGI Environment Variables](https://www.cgi101.com/book/ch3/text.html)
- [Understanding HTTP Responses](https://www.tutorialspoint.com/http/http_responses.htm)
- [What is HTTP? How the Internet Works\! (YouTube)](https://www.youtube.com/watch?v=wW2A5SZ3GkI)
- [How an HTTP Request Gets Served (YouTube)](https://www.youtube.com/watch?v=hWyBeEF3CqQ)
- [HTTP/1.0 RFC Reference](https://datatracker.ietf.org/doc/html/rfc1945#section-6.1.1)
- [MDN HTTP Overview](http://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Overview)
- [curl Tutorial](https://curl.se/docs/tutorial.html)
- [Uniform Resource Identifier (URI)](https://http.dev/uri)
- [CGI Example: Read Data](https://www.infor.uva.es/~jvegas/cursos/web/cgi-bin/fuentes/readdata.html)
- [RFC 3875: CGI](https://datatracker.ietf.org/doc/html/rfc3875#section-4.1.4)
- [HTTP Cookies Crash Course (YouTube)](https://www.youtube.com/watch?v=sovAIX4doOE&list=PLQnljOFTspQU6zO0drAYHFtkkyfNJw1IO&index=12)
- [HTTP Cookies](https://http.dev/cookies)

## Acknowledgments

Thanks to the incredibly helpful READMEs from the following repositories, which helped a lot in finding resources and understanding the project:

- [Toufa7/WebServer](https://github.com/Toufa7/WebServer)
- [zelhajou/ft\_net\_webserv](https://github.com/zelhajou/ft_net_webserv)

## AI Usage

AI was used to help understand the core concepts of HTTP servers, assist with understanding some useful utility functions, and proofread/fix the grammar when writing this README.