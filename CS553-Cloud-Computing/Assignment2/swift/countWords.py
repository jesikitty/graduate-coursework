import sys
from collections import defaultdict

def countWords(source):
    words = defaultdict(int)
    with open(source, 'r') as fin:
        for line in fin:
            for word in line.split():
                words[word] += 1
    for word in sorted(words, key=words.__getitem__, reverse=True):
                print word + ' ' + str(words[word])
 
countWords(sys.argv[1])
