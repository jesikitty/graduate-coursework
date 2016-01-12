import glob
from collections import defaultdict

def countWords():
    words = defaultdict(int)
    for f in glob.glob('data/counts*'):
	    with open(f, 'r') as fin:
	        for line in fin:
	        	item = line.split();
	        	words[item[0]] += int(item[1])
    with open('wordcount-swift.txt', 'w') as fout:
        for word in sorted(words, key=words.__getitem__, reverse=True):
            fout.write(word + ':\t' + str(words[word]) + "\n")
 
countWords()
