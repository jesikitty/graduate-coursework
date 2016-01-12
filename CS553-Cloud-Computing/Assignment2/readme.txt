
This readme provides instructions for the shared memory, hadoop, and swift versions of wordcount.

/----- Shared Memory -----/

This is fairly simple, first ensure that you have gcc 4.8 installed (if not, can get acquired through apt-get or yum, depending on distro.) Then simply run "make cpp" and the makefile will take care of the necessary commands ane make a sharedwordcount file. To run this, run "./sharedwordcount [num threads] [input file]" where [num threads] is the number of threads to use and [input file] is the file to run wordcount on. This will print the counts to standard out, but can be piped into an output file using ">".

/----- Hadoop -----/

/--- Set Up ---/

My configuration was based mostly on the setup found on the following tutorial and its follow-up: http://java.dzone.com/articles/how-set-multi-node-hadoop

I made sevaral changes, such as using Amazon Linux instead of Ubuntu and made adjustments as needed (username changes from ubuntu to ec2-user, use yum instead of apt-get, and a few other minor changes.) I also combined stebs as often as possible, so when installing files, making folders, or doing basic commands, I wrote these all into a script which I copied to all other nodes to speed the setup process up.

/----- Running -----/

Once the setup is completed, and the master and slaves are configured, run "hadoop namenode -format" ont he master to get the filesystem ready, then run /bin/start-all.sh to launch all the nodes. After this is done, make a directory on the filesystem using "hadoop fs -mkdir [directory]" and then copy the file to be counted intot he deirectory using "hadoop fs -put"

Once the file is in HDFS, compile the jar using the makefile by running  "make hadoop". (The javac command does point to the hadoop-core.jar file, so be sure that path is correct before running this.) Once the jar is made, the following command will run the wordcount application:

hadoop jar WordCount.jar us.ssmy.WordCount [input file] [output directory] [num maps] [num reducers]

where [input file] is the file to be counted, such as /users/hadoop/wiki10gb, [output directory] is the directory in HDFS where the results will go, such as /users/hadoop/out, and [num maps] and [num reducers] specify the number of mapping and reducing nodes to use.

Hadoop will print out the progress as this runs, and once it finishes, you can use "hadoop fs -get" to pull the results out of HDFS.

/--- Shut Down ---/

To wrap up Hadoop, simply run [hadoop folder]/bin/stop-all.sh to stop all the nodes, then feel free to stop them all on Amazon after all necessary files are saved.

/------ Swift -----/

/--- Set Up ---/

For the configuration of head and workers nodes, see the following tutorial:
https://github.com/yadudoc/cloud-tutorials

After creating an IAM user, it is additionally necessary it create an IAM group and add the user to that group.
In the IAM services, click on "Groups". Next, click on "Create New Group". On the first page, name the group (such as 'cs553'). On the second page of the process, select the "Full Administration" option.
Now, return to "Users", select the user created in the tutorial, and use "User Actions" -> "Add User to Groups" to add the user to the newly created group.

Continue on with the tutorial until right before "Supported Operations". At this point, the launcher node should be fully configured, and you should have connected to the head node. 

In order to handle the larger data set, it is necessary to add an additional volume to the head node. 
On AWS, under the EC2 services, click on "Volumes" under "Elastic Block Store".
Click on "Create Volume". In the creation window, change the Size to 22 and ensure that the Availability Zone matches that of the head node. Click create. Now, right click the new volume and click Attach Volume. Enter in the name of head node to attach the volume to it.
The volume must still be mounted in order to be used.

In the terminal (you should be connected to the head node) use the following command:
	lsblk
The newly added volume will be the one without a mountpoint. Assuming this volume is called "xvdf":
	sudo mkfs -t ext4 /dev/xvdf
	sudo mkdir /mnt/my-data
	sudo mount /dev/xvdf /mnt/my-data
Additionally, to ensure that you have access to the newly mounted volume:
	chown -R ubuntu /mnt
	chmod -R u+rX /mnt

Now, copy the read only swift-cloud-tutorial file:
	"cp -r cloud-tutorials/swift-cloud-tutorial ./"
Within the new folder, use the following command to set up Java and Swift:
	source setup.sh

/--- Run the Word Count Program ---/

Exit this folder. Now, use scp or a program such as Filezilla to copy over the program files (these are all the files in the swift folder submitted). 
Create a folder called "data" within the headnode folder.
Additionally, copy in any data to /mnt/my-data. 
Note: the scripts, as is, are expecting a file with the name "wiki10gb". This can be changed in the run.sh file.

To run the wordcount program, use the following command from the headnode folder:
	bash run.sh

run.sh will split out the dataset, feed it into the wdcount.swift file, which in turn uses the countWords.py file. After the workers (and Swift) are done, run.sh calls merge.py, which consolidates all the counts into one output file.

/--- Shut Down ---/

When done, return to the launch instance from the head node:
	exit
From the launch instance, use the following command to terminate the head node and all worker nodes:
	dissolve
As always, all instances (including the launch instance) can be terminated from the AWS-EC2 dashboard.

