CS 525, Spring 2015
William Bafia
Jesi Merrick
Khush Deoja
Thomas Mathews

-- Project Implementation Details --

Using the provided interface and data structures, we implemented a buffer system 
that allows for quick retrieval of pages by storing them in page frames in memory.

In addition to the provided data structures, we implemented a few of our own 
too: BM_MgmtData, BM_Frame, and BM_Node.  BM_MgmtData is a structure that holds 
information useful in the general implementation of the buffer manager, such as the
storage manager file handle, the pool of frames itself, the head of our linked 
list implementation, as well as, io read and write tracking variables.  

BM_Frame represented the important information associated with our frames.  
This information consisted of the number of clients accessing a frame, whether 
or not the frame was dirty, and the actual frame data, as well as, the page 
number of the page the frame references.

Finally, the BM_Node structure was used to implement our linked list data structure.  
This structure was important in our implementation of FIFO and LRU, allowing us
to keep track of which frame should be replaced next.

Using these structures, we implemented the buffer by primarily using the given interface.  
Some additional functons we added consisted of creating the linked list, inserting and 
removing from the linked list, and our strategy functions.  findFreeFrame returns 
the next frame available for replacement.  It uses a simple switch statement to 
move between desired strategies for replacement.  The functions findFIFO and findLRU 
implement their respective strategies for frame replacement.

-- Areas for Improvement --

Currently, we have issues implementing the FIFO and LRU strategies effectively.  Future 
efforts would be put into ensuring that those strategies are implemented not only correctly 
but efficiently.  

-- Running the Program --

To compile, simply use the command
	make all
which will compile the original test_assign2_1 file.

To run the original test,
	./test_assign2

At the end
	make clean
can be used to remove the executables.
