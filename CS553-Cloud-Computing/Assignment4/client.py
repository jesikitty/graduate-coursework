#!/usr/bin/env python

#################### CODE CREDIT ####################
# This code is based on code written for CS 550.    #
# That code was based on a project for CS 451.      #
# Both the original 451 code and 550 code were      #
# worked on by Ron Pyka, and the 550 code was added #
# to by Jesi Merrick, the members of this group.    #
#####################################################

import asyncore, asynchat, socket, sys, time, readline, os, base64
from threading import Thread, Lock
readline.parse_and_bind("tab: complete")

lock = Lock()
schedsocket = None #socket for scheduler
taskstatus = {} #keep track of tasks

class chat(asynchat.async_chat): #class for handling most of the communications asynchronously
  def __init__(self, server, sock, addr):
    asynchat.async_chat.__init__(self, sock=sock) #initializations
    self.ibuffer = "" #initialize local buffer
    self.set_terminator("\r\n") #the terminating sequence of characters for all messages
    self.filename = "" #initialize local filename
    self.peers = [] #list of peers
    self.addr = addr[0] #local address
    print "starting clock"
    global start_time
    start_time = time.time()
    global count
    count = 1

  def inbuf(self, str):
    shortcmds = ["SENT"]
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
    global sockets
    global count
    global start_time

    if self.inbuf("DONE"):
      message = self.ibuffer.split()
      results = message[1].split(':')
      print "Task "+results[0]+" finished with result: "+results[1]
      count = count + 1
      if count == 80:
        print "Finish time : "+str(time.time() - start_time)
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

  def handle_write(self):
    sent = self.send(self.buffer.encode())
    self.buffer = self.buffer[sent:]

#Main functions and code

def listener(): #establishes peer-to-peer server listening on a specified port
  client = ptpserv("", 3003)
  asyncore.loop()

if len(sys.argv) < 3: #make sure config file is included in launching program
  print """Scheduler address and workload file should be included in the format:
    client.py -s [address:port] -w [file]"""
  sys.exit(1)

#query method
def loadall(schedsocket):
  global file
  global taskstatus
  ttl = 8 #can be set to whatever

  #test to get host local ip
  #use existing socket for this later
  s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  s.connect(("8.8.8.8",80))
  localIP = (s.getsockname()[0])
  s.close()

  tasknum = 0
  #get the name of the file to be searched for
  #filename = raw_input("Filename: ")
  for command in file: #search 200 times
    #for s in sockets: #for all sockets (neighbors), send the FIND request
      #command + messageID (IP & count) + TTL + filename
    taskstatus[tasknum] = None
    schedsocket.sendall("LOAD "+str(localIP)+" "+str(tasknum)+" "+str(command)+"\r\n")
    #print "Loading task..."
    lock.release()
    #time.sleep(1) #wait 1 second for file, allow other things to execute in that time
    lock.acquire()
    tasknum += 1 #iterate the unique ID for the client

file = None #defining file

lock.acquire() #sequential block of code to be protected
server = Thread(target=listener)
server.daemon = True
server.start() #fire up the server
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
if (sys.argv[1] == '-s'):
  if (sys.argv[3] == '-w'):
    print "Attempting to connect to scheduler..."
    while (True):
      try:
        s.connect( (sys.argv[2].split(':')[0], int(sys.argv[2].split(':')[1])) )
        schedsocket=s
        print """
Connected to the scheduler!
"""
        break
      except:
        time.sleep(2)
        print "retrying..."
    file = open(sys.argv[4], 'r')
  else:
    print "Invalid input. Must be in format: client.py -s [address:port] -w [file]"
    sys.exit()
else:
  print "Invalid input. Must be in format: client.py -s [address:port] -w [file]"
  sys.exit()

loadall(schedsocket)

#receive replies here?
lock.release()
while(True):
  time.sleep(1)
lock.acquire()

schedsocket.close()
server.join()
