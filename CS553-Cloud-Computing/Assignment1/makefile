all: cpu, gpu, mem, net, disk

cpu: cpu.cpp FORCE
	g++ -O2 -std=c++0x -pthread -o cpu cpu.cpp -Wl,--no-as-needed
	./cpu

gpu: gpu.cu FORCE
	nvcc -o gpu gpu.cu

mem: mem.cpp FORCE
	g++ -std=c++0x -pthread -o mem mem.cpp -Wl,--no-as-needed
	./mem

disk: disk.cpp FORCE
	g++ -std=c++0x -pthread -o disk disk.cpp -Wl,--no-as-needed
	./disk
 
net: net.cpp FORCE
	g++ -std=c++0x -pthread -o net net.cpp -Wl,--no-as-needed
	./net

clean:
	rm -rf *.o cpu gpu mem disk net test.txt

FORCE:
