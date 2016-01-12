all: hadoop cpp

hadoop: WordCount.java
	javac -classpath /home/ec2-user/hadoop/hadoop-core-1.2.1.jar -d WordCount/ WordCount.java
	jar -cvf WordCount.jar -C WordCount/ .

cpp: WordCount.cpp
	 g++ -std=c++11 -pthread -o sharedwordcount wordcount.cpp
