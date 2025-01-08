import socket

request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("localhost", 8083))
s.sendall(request[:10].encode())
s.sendall(request[10:].encode())
