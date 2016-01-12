#!/usr/bin/env python
import asyncore, asynchat, socket

data = {}

class chat(asynchat.async_chat):
  def __init__(self, server, sock, addr):
    asynchat.async_chat.__init__(self, sock=sock)
    self.ibuffer = []
    self.obuffer = ""
    self.addr = addr[0]
    data[self.addr] = []
    self.set_terminator("\r\n")

  def inbuf(self, str):
    if len(self.ibuffer[0]) <= 5:
      return False
    return self.ibuffer[0].startswith(str+" ")

  def get(self):
    return self.ibuffer[0][5:]

  def collect_incoming_data(self, data):
    self.ibuffer.append(data)

  def found_terminator(self):
    if self.inbuf("ADDF"):
      data[self.addr].append(self.get())
    elif self.inbuf("FIND"):
      s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      s.connect( (self.addr, 12346) )
      s.sendall("LIST "+self.get()+"\r\n")
      for client, files in data.iteritems():
        if self.get() in files:
          s.sendall("PEER "+client+"\r\n")
      s.sendall("DONE\r\n")
      s.close()
    self.ibuffer = []
    print repr(self.addr)+": "+repr(data[self.addr])

class ptpserv(asyncore.dispatcher):
  def __init__(self, addr, port):
    asyncore.dispatcher.__init__(self)
    self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
    self.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # kill TIME_WAIT
    self.bind((addr, port))
    self.listen(5)

  def handle_close(self):
    self.close()

  def handle_accept(self):
    conn, addr = self.accept()
    chat(self, conn, addr)

#print "Dest: "
#send = raw_input()
client = ptpserv("", 12345)
asyncore.loop()
