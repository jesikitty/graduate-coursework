#!/usr/bin/env python

import asyncore, asynchat, socket, sys, time, readline, os, base64
from threading import Thread, Lock
readline.parse_and_bind("tab: complete")

lock = Lock()
messageHistoryID = range(50)# The Message IDs
messageHistorySocket = range(50)    # The Message Sockets
messageCurrent = 0  # Message to be overwritten next
sockets = [] #list of sockets for neighbors

class chat(asynchat.async_chat): #class for handling most of the communications asynchronously
  def __init__(self, server, sock, addr):
    asynchat.async_chat.__init__(self, sock=sock) #initializations
    self.ibuffer = "" #initialize local buffer
    self.set_terminator("\r\n") #the terminating sequence of characters for all messages
    self.filename = "" #initialize local filename
    self.peers = [] #list of peers
    self.addr = addr[0] #local address

  def inbuf(self, str):
    shortcmds = ["DONE", "SENT"]
    if str in shortcmds and self.ibuffer.startswith(str):
      return True #allow buffer to be short for DONE and SENT commands
    if len(self.ibuffer) <= 5:
      return False #otherwise, if the buffer is two shor, cancel communications
    return self.ibuffer.startswith(str+" ") #if buffer is full enough, return it.

  def get(self): #get the buffer
    return self.ibuffer[5:]

  def collect_incoming_data(self, data): #collect the tincoming data, put it into local buffer
    self.ibuffer += data

  def found_terminator(self): #if the message is read all the way up to the terminator, do the following
    global messageCurrent
    global messageHistoryID
    global messageHistorySocket
    global sockets

    # client <-> client protocol
    if self.inbuf("GETF"): #request to send a file
      s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      s.connect( (self.addr, 12346) ) #connect to requestor
      self.filename = self.get()
      s.sendall("FILE "+self.filename+"\r\n") #send message indicating start of transfer
      with open(os.path.join("files", self.filename), "rb") as f: #open local file
        data = f.read(1024000) #read data chunks
        while data != "": #keep reading until file empty
          s.sendall("SEND "+base64.standard_b64encode(data)+"\r\n") #encode and send data
          data = f.read(1024000)
      s.sendall("SENT\r\n") #send message indicating finished
      s.close() #close that connection
    elif self.inbuf("FIND"): #request to locate file
      message = self.ibuffer.split()
      if message[1] not in messageHistoryID:
        messageHistoryID[messageCurrent]=message[1] #store unique ID
        messageHistorySocket[messageCurrent]=self.addr #store the socket message came from, formerly self.addr
        messageCurrent += 1
        messageCurrent = messageCurrent%50 #will reset count to 0 at 50 entries, limiting list
        if int(message[2]) != 0: #check if TTL is 0
          for s in sockets: #if not, continue sending message
            #print "sending on to neighbor"
            s.sendall("FIND "+message[1]+" "+str(int(message[2])-1)+" "+message[3]+"\r\n")
        if message[3] in os.listdir("files"): #see if file exits
          #print "file found, sending reply to "+self.addr
          s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
          s.connect( (self.addr, 12346) ) #if so, send it back
          localIP = (s.getsockname()[0])
          ttl = 8 #allow 8 hops, can be changed
          s.sendall("DNIF "+message[1]+" "+str(ttl)+" "+message[3]+" "+str(localIP)+"\r\n")
          s.close()
          search.messageIDCount += 1
    elif self.inbuf("DNIF"): #reply that file was found
        message = self.ibuffer.split()
        if message[1] in messageHistoryID:
          curmess = messageHistoryID.index(message[1]) #get the current message number
          if messageHistorySocket[curmess]!=0: #check to see if associated address is 0
            if int(message[2]) != 0: #send found message back along path
              #print "sending found message back to "+self.addr
              s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
              s.connect( (messageHistorySocket[curmess], 12346) ) #send message back along path
              s.sendall(message[0]+" "+message[1]+" "+str(int(message[2])-1)+" "+message[3]+" "+message[4]+"\r\n")
          else: #found message has arrived at requestor
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect( (message[4], 12346) ) #connect to owner for download
            s.sendall("GETF "+message[3]+"\r\n")
            #print "getting file from "+message[3]
            messageHistoryID[messageHistoryID.index(message[1])]=0
          s.close()
    elif self.inbuf("FILE"): #create file and prepare to recieve data
      self.filename = self.get()
      open(os.path.join("files", self.filename), 'w')
    elif self.inbuf("SEND"): #receive file data and add to file
      with open(os.path.join("files", self.filename), 'ab') as f:
        data = self.get() #the file data
        while len(data) % 4 != 0:
          data += '='
        f.write(base64.standard_b64decode(data)) #writing to file
    elif self.inbuf("SENT"): #all file data received
      print "Display file '%s'\n" % self.filename #print "diplay" message to show completion
      self.filename = ""
      #lock.release()
    self.ibuffer = "" #clear local buffer every iteration

class ptpserv(asyncore.dispatcher): #simple class for the peer-to-peer server
  def __init__(self, addr, port): #initialize ptpserv
    asyncore.dispatcher.__init__(self) #initialize a dispatcher for messages
    self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
    self.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # kill TIME_WAIT
    self.bind((addr, port)) #make socket and bind to port given
    self.listen(5) #listen for incoming connections

  def handle_close(self): #handle connections closing
    self.close() #close socket

  def handle_accept(self): #handle accepting new requests
    conn, addr = self.accept() #accept all connections by default
    chat(self, conn, addr) #establich new chat instance for connection

#Main functions and code

def listener(): #establishes peer-to-peer server listening on a specified port
  client = ptpserv("", 12346)
  asyncore.loop()

if len(sys.argv) < 2: #make sure config file is included in launching program
  print "Name of config peer file should be first argument to client"
  sys.exit(1)

#query method
def search(sockets):
  global messageCurrent
  global messageHistoryID
  global messageHistorySocket
  ttl = 8 #can be set to whatever

  #test to get host local ip
  #use existing socket for this later
  s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  s.connect(("8.8.8.8",80))
  localIP = (s.getsockname()[0])
  s.close()

  #get the name of the file to be searched for
  filename = raw_input("Filename: ")
  messageHistoryID[messageCurrent]=str(localIP)+"."+str(search.messageIDCount)
  messageHistorySocket[messageCurrent]=0 #log the message in the lists, with 0 as the address
  messageCurrent += 1 #increment iterator
  messageCurrent = messageCurrent%50 #reset to 0 if 50 reached
  for s in sockets: #for all sockets (neighbors), send the FIND request
    #command + messageID (IP & count) + TTL + filename
    s.sendall("FIND "+str(localIP)+"."+str(search.messageIDCount)+" "+str(ttl)+" "+filename+"\r\n")
  print "Searching"
  lock.release()
  time.sleep(1) #wait 1 second for file, allow other things to execute in that time
  lock.acquire()
  search.messageIDCount += 1 #iterate the unique ID for the client

search.messageIDCount = 0 #sent search ID to 0 on launch

lock.acquire() #sequential block of code to be protected
server = Thread(target=listener)
server.daemon = True
server.start() #fire up the server
#s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#s.connect( (sys.argv[1], 12345) )

#read from config file
#neighbor contains IP address & port
neighbors = []
file = open('config.txt', 'r')
for address in file:
  neighbors.append(address) #add each neighbor to list
file.close()

failedconn = [] #keep track of failed connections
for neighbor in neighbors:
  ninfo = neighbor.split()
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  try: #try to connect to neighbor
    s.connect( (ninfo[0], int(ninfo[1])) )
    sockets.append(s)
  except Exception, e: #mark as failed if unsuccessful
    print "failed to connect to "+ninfo[0]
    failedconn.append(neighbor)
neighbors = []
neighbors = failedconn #change neighbors list to only have unsuccessful connections
failedconn = []

while True: #loop infinitely
  print """
1) search for a file
2) reconnect to failed connections (if any)
""" #menu
  #print "neighbors left: "+`len(neighbors)`
  input = int(raw_input("Listening: "))
  if input == 1:
    search(sockets)
  elif input == 2: #try to reconnect to neighbors list (formerly unsuccessful connections)
    for neighbor in neighbors:
      ninfo = neighbor.split()
      s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      try:
        s.connect( (ninfo[0], int(ninfo[1])) )
        sockets.append(s)
      except Exception, e:
        print "failed to connect to "+ninfo[0]
        failedconn.append(neighbor)
    neighbors = []
    neighbors = failedconn
    failedconn = []
  else:
    print "Input error"

#s.close()
for s in sockets:
  s.close()
server.join()
