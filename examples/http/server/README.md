This is an HTTP server example that uses a promise to indicate when the socket
is ready to read, but not for anything more fine grained than that. This seems
to be about an order of magnitude more efficient than the slow version that
uses a promise to read every line.
