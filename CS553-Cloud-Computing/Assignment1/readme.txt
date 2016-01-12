CS553-01, Fall 2014
Benchmark Assignment
Jesi Merrick & Ron Pyka

To compile all of the benchmarks, use the command:
	make all
This will compile all the experiments for each benchmark and run CPU, Memory, and Disk (varying the number of threads, block sizes, etc). The network code is separate to avoid issues from creating and closing sockets on the same port repeatedly, and the gpu is separate since it was submitted on jarvis and not run locally.

To run the network code, the instructions will be displayed, but you will need to include 4 parameters: buffer size, iterations, port number, and number of threads. For the buffer size, it is the number of bytes. The number of iterations can be enterred by the user as well to vary the duration of the test. We recommend about 500,000 iterations for 1 Byte buffer, and 50,000 iterations for 1KB and 64KB buffer. The port is open to be changed to account for firewalls or anything else. We ran our tests on port 13000 with no issue. Finally, the threads input supports either 1 or 2 threads, and will divide the work up based on that. Any run will test both TCP and UDP.  

To run the GPU code on jarvis, once it is compiled with "make gpu" you can edit the gpu.sh script to reflect the absolute location (/home/ronpyka/gpu for example) instead of the relative location, and submit the job using "qsub -q 650.q gpu.sh" which will give you a job number. Output can be viewed by using "less gpu.sh.o[job number]". To run the gpu code locally, simply run "./gpu".

To run the CPU, Memory, and Disk benchmarks independently, the following commands can be used, respectively:
	make cpu
	make mem
	make disk
