import sys, os, time, boto, boto.ec2, boto.sqs, boto.utils
from boto.sqs.message import Message
from boto.dynamodb2.fields import HashKey
from boto.dynamodb2.table import Table

sqs = boto.sqs.connect_to_region("us-west-2")
requests = sqs.get_queue('requests')
responses = sqs.get_queue('responses')

messages = Table('messages', schema=[HashKey('message')])

inst_id = boto.utils.get_instance_metadata()['instance-id']
ec2 = boto.ec2.connect_to_region("us-west-2")


idle = int(sys.argv[2])
check = 0


#should be set to kill itself after a period of time
while True:
	rs = requests.get_messages()

	#if idle != 0 and check >= idle:
		#print "Worker is shutting down"
		#ec2.terminate_instances(instance_ids=[inst_id])

	if len(rs) > 0:
	  	check = 0
	 	m = rs[0]	
	  	job = m.get_body()

		if not messages.has_item(message=job):
		  messages.put_item(data={'message': job})		
		  fulljob = job.split(':')
		  task = fulljob[1].split()
		  print str(os.getpid())+" sleeping "+task[1]+" ms"
		  try:
		    time.sleep(int(task[1])/1000)
		    result = fulljob[0]+':'+"success"
		  except:
		    result = fulljob[0]+':'+"failure"
		  #print str(os.getpid())+" done"
		  mreturn = Message()
		  mreturn.set_body(result)
		  responses.write(mreturn)
		requests.delete_message(m)  
	else:
		#print "Check " + str(check)
		time.sleep(1)
		check = check + 1
