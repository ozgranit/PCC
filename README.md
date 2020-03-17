# PCC
A toy client/server architecture

a printable characters counting(PCC) server. Clients connect to the server and send it a stream of bytes. The server counts how
many of the bytes are printable and returns that number to the client. The server also maintains
overall statistics on the distribution of printable characters it has received from all clients. When
the server terminates, it prints these statistics to standard output.

### to compile: gcc -O3 -D_POSIX_C_SOURCE=200809 -std=c11 pcc_server.c (or pcc_client.c)
### Client specification:
  Command line arguments:
  - argv[1]: server’s IP address (assume a valid IP address)
  - argv[2]: server’s port (assume a 16-bit unsigned integer)
  - **argv[3]**: path of the file to send
### Server specification:
  Command line arguments:
  argv[1]: server’s port (assume a 16-bit unsigned integer).
### Exit Server through SIGINT.
