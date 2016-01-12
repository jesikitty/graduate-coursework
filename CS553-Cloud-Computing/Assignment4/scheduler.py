#!/usr/bin/env python
import sys, asyncore, asynchat, time, socket, os, boto, boto.ec2, boto.sqs
from multiprocessing import Process, Pool, Manager
from boto.sqs.message import Message

data = {}
joblist = {}
m = Manager()
workqueue = m.Queue()
resultqueue = m.Queue()

sqs = boto.sqs.connect_to_region("us-west-2")
requests = sqs.create_queue('requests')
responses = sqs.create_queue('responses')

ec2 = boto.ec2.connect_to_region("us-west-2")

def watchresult(s):
  while True:
    if not resultqueue.empty():
      #print "returning"
      msg = resultqueue.get()
      resultqueue.task_done()
      s.sendall("DONE "+msg+"\r\n")

    #remote worker code
    rs = responses.get_messages()
    if len(rs) > 0:
      m = rs[0]
      s.sendall("DONE "+m.get_body()+"\r\n")
      responses.delete_message(m)

class chat(asynchat.async_chat):
  global workqueue

  def __init__(self, server, sock, addr):
    asynchat.async_chat.__init__(self, sock=sock)
    self.ibuffer = []
    self.obuffer = ""
    self.addr = addr[0]
    data[self.addr] = []
    self.set_terminator("\r\n")
    print "client connected"
    time.sleep(0.5)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect( (self.addr, 3003) ) #should make this dynamic
    p = Process(target=watchresult, args=(s,))
    p.daemon = True
    p.start()
      
  def inbuf(self, str):
    if len(self.ibuffer[0]) <= 5:
      return False
    return self.ibuffer[0].startswith(str+" ")

  def get(self):
    return self.ibuffer[0][5:]

  def collect_incoming_data(self, data):
    self.ibuffer.append(data)

  def found_terminator(self):
    if self.inbuf("LOAD"):
      #print "Received task..."
      print self.get()
      message = self.get().split(' ',2)
      #print message[2]
      workqueue.put(message[1]+":"+message[2])

      #remote worker code
      m = Message()
      m.set_body(message[1]+":"+message[2])
      requests.write(m)

      #s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      #s.connect( (self.addr, 12346) )
      #s.sendall("LIST "+self.get()+"\r\n")
      #time.sleep(2)
      #s.sendall("SENT blah\r\n")
      #for client, files in data.iteritems():
      #  if self.get() in files:
      #    s.sendall("PEER "+client+"\r\n")
      #s.sendall("DONE\r\n")
      #s.close()
    self.ibuffer = []
    #print repr(self.addr)+": "+repr(data[self.addr])

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

def startserv(port):
  client = ptpserv("", port)
  asyncore.loop()

def localworker(workqueue, resultqueue):
  #print "worker"
  while True:
    if not workqueue.empty():
      job = workqueue.get()
      workqueue.task_done()
      fulljob = job.split(':')
      task = fulljob[1].split()
      print str(os.getpid())+" sleeping "+task[1]+" ms"
      try:
        time.sleep(int(task[1])/1000)
        result = fulljob[0]+':'+"success"
      except:
        result = fulljob[0]+':'+"failure"
      #print str(os.getpid())+" done"
      resultqueue.put(result)
  return

#note -> 2 reservations/instances by default: client and scheduler 
def trackRWorkers():
  print "Tracking workers"

  while True:
    reservations = ec2.get_all_reservations(filters={"instance-state-code" : [0, 16]})
    max_workers = 16

    #dynamic
    #print "Number of requests " + str(requests.count()) + " and number of reservations " + str(len(reservations))
    if requests.count() > 0 and len(reservations) == 2: #there are requests, but no workers
      print "Starting a worker"
      newWorker = ec2.run_instances('ami-e1bfe8d1', key_name='cs553', instance_type='t2.micro', 
        security_groups=['launch-wizard-2'])
      time.sleep(30)
    elif len(reservations) < max_workers:  
      if requests.count() % 2 == 0 and len(reservations)-2 < requests.count():  
        print "Starting a worker"
        newWorker = ec2.run_instances('ami-e1bfe8d1', key_name='cs553', instance_type='t2.micro', 
          security_groups=['launch-wizard-2'])
        time.sleep(30)
      elif requests.count() % 2 == 1 and  len(reservations)-2 < requests.count()-1:
        print "Starting a worker"
        newWorker = ec2.run_instances('ami-e1bfe8d1', key_name='cs553', instance_type='t2.micro', 
         security_groups=['launch-wizard-2'])  
        time.sleep(30)

#joblist = ["test1", "test2", "test3", "test4", "test5"]
#q = None

if __name__ == '__main__':

  server = None

  if len(sys.argv) < 4:
    print "need more args"
    sys.exit(1)

  if sys.argv[1] == '-s':
    if sys.argv[3] == '-lw':
      port = int(sys.argv[2])
      server = Process(target=startserv, args=(port,))
      server.start()

      workers = int(sys.argv[4])
      processes = []

      #pool = Pool(int(sys.argv[4]))
      #workers = pool.apply_async(localworker, (q))
      for w in xrange(workers):
        p = Process(target=localworker, args=(workqueue,resultqueue))
        p.start()
        processes.append(p)

      for p in processes:
        p.join()
      #workers.close()

      server.join()

    elif sys.argv[3] == '-rw':
      port = int(sys.argv[2])
      server = Process(target=startserv, args=(port,))
      server.start()

      #trw = Process(target=trackRWorkers, args=())
      #trw.start()

      #for testing purposes
      num_of_workers = 1 #change according to test
      for i in range(0, num_of_workers):
        print "Starting a worker"
        newWorker = ec2.run_instances('ami-9bc394ab', key_name='cs553', instance_type='t2.micro', 
          security_groups=['launch-wizard-2'])

      #trw.join()
      server.join()
      
    else:
      print "invalid arguments"
      sys.exit(1)
  else:
    print "invalid arguments"
    isys.exit(1)
