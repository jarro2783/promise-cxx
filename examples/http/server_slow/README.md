This is an example of an HTTP server that uses a promise to read every line in
the header of a request. This is very slow because it needs to go through the
event loop at least once (probably twice) for every header.
