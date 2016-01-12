import string
import random
import sys

#CANDIDATE_CHARS = string.ascii_letters+string.digits
#filesdir = "files"

if len(sys.argv)<3:
  print "format is genfile.py [sleeptime(ms)] [iterations]"
  sys.exit(1)

sleeptime = sys.argv[1]
iter = int(sys.argv[2])
print "Making file..."

fname = 'testworkload.txt'
#print fname
f = open(fname,'w')
for i in range(iter):
  f.write('sleep '+ sleeptime + '\n')
f.close()
