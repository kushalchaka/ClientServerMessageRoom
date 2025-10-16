# ClientServerMessageRoom
A lightweight chat server that supports multiple concurrent users with private messaging, broadcasts, and random message sending. Uses TCP sockets, POSIX threads, and simple command parsing to handle users asynchronously. Includes dynamic port binding and notifications when users join or leave.

To use, compile the server program and run it. It should output a port number that can be used by clients to connect.

For clients to connect, they need to enter the following command

  ncat \<hostname> \<port#>

  The hostname can be found with the hostname command, and the port number is provided by the server upon running
