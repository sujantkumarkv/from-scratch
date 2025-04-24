[![progress-banner](https://backend.codecrafters.io/progress/http-server/3a13d3c7-490a-4e48-afa4-f9a657aab814)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

This is a starting point for C solutions to the
["Build Your Own HTTP server" Challenge](https://app.codecrafters.io/courses/http-server/overview).

[HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol) is the
protocol that powers the web. In this challenge, you'll build a HTTP/1.1 server
that is capable of serving multiple clients.

Along the way you'll learn about TCP servers,
[HTTP request syntax](https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html),
and more.

**Note**: If you're viewing this repo on GitHub, head over to
[codecrafters.io](https://codecrafters.io) to try the challenge.

# Passing the first stage

The entry point for your HTTP server implementation is in `app/server.c`. Study
and uncomment the relevant code, and push your changes to pass the first stage:

```sh
git commit -am "pass 1st stage" # any msg
git push origin master
```

Time to move on to the next stage!

# Stage 2 & beyond

Note: This section is for stages 2 and beyond.

1. Ensure you have `gcc` installed locally
1. Run `./your_program.sh` to run your program, which is implemented in
   `app/server.c`.
1. Commit your changes and run `git push origin main` to submit your solution
   to CodeCrafters. Test output will be streamed to your terminal.

## Bind to a port
In this stage, you'll create a TCP server that listens on port 4221.

TCP is the underlying protocol used by HTTP servers.

Tests
The tester will execute your program like this:

$ ./your_program.sh
Then, the tester will try to connect to your server on port 4221. The connection must succeed for you to pass this stage.

## Extract URL path
In this stage, your server will extract the URL path from an HTTP request, and respond with either a 200 or 404, depending on the path.
The tester will then send two HTTP requests to your server.

First, the tester will send a GET request, with a random string as the path:

`$ curl -v http://localhost:4221/abcdefg`

Your server must respond to this request with a 404 response:
`HTTP/1.1 404 Not Found\r\n\r\n`

Then, the tester will send a GET request, with the path /:
`$ curl -v http://localhost:4221`

Your server must respond to this request with a 200 response:
`HTTP/1.1 200 OK\r\n\r\n`

AI told me:
1. The Server Sees the Raw HTTP Request
When you run `curl http://localhost:4221/index.html`, the server receives the exact raw HTTP request string you provided:

```text
GET /index.html HTTP/1.1\r\n
Host: localhost:4221\r\n
User-Agent: curl/7.64.1\r\n
Accept: */*\r\n
\r\n
```
The \r\n (CRLF) sequences are part of the HTTP protocol and mark line endings.

The server parses this raw text to determine the HTTP method, path, headers, etc.

2. Who Converts the curl Command to This Format?
curl itself is responsible for generating the HTTP request. Here's how it works:

What curl does:
Parses the URL: Splits http://localhost:4221/index.html into:
```
Host: localhost:4221

Path: /index.html

Protocol: HTTP/1.1 (default unless you specify HTTP/2 with --http2).
```