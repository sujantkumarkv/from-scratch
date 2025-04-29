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

## Respond with body
Your Task
In this stage, you'll implement the /echo/{str} endpoint, which accepts a string and returns it in the response body.

Response body
A response body is used to return content to the client. This content may be an entire web page, a file, a string, or anything else that can be represented with bytes.

Your /echo/{str} endpoint must return a 200 response, with the response body set to given string, and with a Content-Type and Content-Length header.

Here's an example of an /echo/{str} request:

`GET /echo/abc HTTP/1.1\r\nHost: localhost:4221\r\nUser-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\n`
And here's the expected response:

`HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 3\r\n\r\nabc`
Here's a breakdown of the response:
```
// Status line
HTTP/1.1 200 OK
\r\n                          // CRLF that marks the end of the status line

// Headers
Content-Type: text/plain\r\n  // Header that specifies the format of the response body
Content-Length: 3\r\n         // Header that specifies the size of the response body, in bytes
\r\n                          // CRLF that marks the end of the headers

// Response body
abc                           // The string from the request
```
The two headers are required for the client to be able to parse the response body. Note that each header ends in a CRLF, and the entire header section also ends in a CRLF.

Tests
The tester will execute your program like this:

`$ ./your_program.sh`
The tester will then send a GET request to the /echo/{str} endpoint on your server, with some random string.

`$ curl -v http://localhost:4221/echo/abc`
Your server must respond with a 200 response that contains the following parts:

Content-Type header set to text/plain.
Content-Length header set to the length of the given string.
Response body set to the given string.
`HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 3\r\n\r\nabc`

## Read Header
The User-Agent header describes the client's user agent.

Your /user-agent endpoint must read the User-Agent header, and return it in your response body. Here's an example of a /user-agent request:
```
// Request line
GET
/user-agent
HTTP/1.1
\r\n

// Headers
Host: localhost:4221\r\n
User-Agent: foobar/1.2.3\r\n  // Read this value
Accept: */*\r\n
\r\n

// Request body (empty)
Here is the expected response:

// Status line
HTTP/1.1 200 OK   // Status code must be 200
\r\n

// Headers
Content-Type: text/plain\r\n
Content-Length: 12\r\n
\r\n

// Response body
foobar/1.2.3     // The value of `User-Agent`
```
Tests
The tester will execute your program like this:

`$ ./your_program.sh`
The tester will then send a GET request to the /user-agent endpoint on your server. The request will have a User-Agent header.

`$ curl -v --header "User-Agent: foobar/1.2.3" http://localhost:4221/user-agent`
Your server must respond with a 200 response that contains the following parts:

Content-Type header set to text/plain.
Content-Length header set to the length of the User-Agent value.
Message body set to the User-Agent value.
`HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nfoobar/1.2.3`

## Concurrent Connections
In this stage, you'll add support for concurrent connections.

Tests
The tester will execute your program like this:

`$ ./your_program.sh`
Then, the tester will create multiple concurrent TCP connections to your server. (The exact number of connections is determined at random.) After that, the tester will send a single GET request through each of the connections.
```
$ (sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 &
$ (sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 &
$ (sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 &
Your server must respond to each request with the following response:
```

`HTTP/1.1 200 OK\r\n\r\n`

## Return a File
In this stage, you'll implement the /files/{filename} endpoint, which returns a requested file to the client.

Tests
The tester will execute your program with a --directory flag. The --directory flag specifies the directory where the files are stored, as an absolute path.

`$ ./your_program.sh --directory /tmp/`
The tester will then send two GET requests to the /files/{filename} endpoint on your server.

First request
The first request will ask for a file that exists in the files directory:
```
$ echo -n 'Hello, World!' > /tmp/foo
$ curl -i http://localhost:4221/files/foo
```
Your server must respond with a 200 response that contains the following parts:

`Content-Type` header set to `application/octet-stream`.

`Content-Length` header set to the size of the file, in bytes.
Response body set to the file contents.
```
HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 13\r\n\r\nHello, World!
```
Second request
The second request will ask for a file that doesn't exist in the files directory:

`$ curl -i http://localhost:4221/files/non_existant_file`
Your server must respond with a 404 response:

`HTTP/1.1 404 Not Found\r\n\r\n`

## Read Request Body
In this stage, you'll add support for the `POST` method of the `/files/{filename}` endpoint, which accepts text from the client and creates a new file with that text.

Request body
A request body is used to send data from the client to the server.

Here's an example of a POST /files/{filename} request:
```
// Request line
POST /files/number HTTP/1.1
\r\n

// Headers
Host: localhost:4221\r\n
User-Agent: curl/7.64.1\r\n
Accept: */*\r\n
Content-Type: application/octet-stream  // Header that specifies the format of the request body
Content-Length: 5\r\n                   // Header that specifies the size of the request body, in bytes
\r\n

// Request Body
12345
```
Tests
The tester will execute your program with a `--directory` flag. The `--directory` flag specifies the directory to create the file in, as an absolute path.

`$ ./your_program.sh --directory /tmp/`
The tester will then send a POST request to the `/files/{filename}` endpoint on your server, with the following parts:

`Content-Type` header set to `application/octet-stream`.

`Content-Length` header set to the size of the request body, in bytes.
Request body set to some random text.
`$ curl -v --data "12345" -H "Content-Type: application/octet-stream" http://localhost:4221/files/file_123`
Your server must return a 201 response:

`HTTP/1.1 201 Created\r\n\r\n`
Your server must also create a new file in the files directory, with the following requirements:

The filename must equal the filename parameter in the endpoint.
The file must contain the contents of the request body.