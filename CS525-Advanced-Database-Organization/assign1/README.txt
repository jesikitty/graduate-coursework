CS 525, Spring 2015
William Bafia
Jesi Merrick
Khush Deoja
Thomas Mathews

-- Project Implementation Details --

Using the file skeleton and data structures provided, we were able to implement
a simple storage manager using basic FILE operations such as fopen, fread, fwrite,
and remove.  Each function dedicated to opening and closing pages and files uses
careful verification to ensure a file and page have been successfully created or
destroyed using a variety of the RC error handling codes and additional codes we
added for specific cases.

For managing information about a file, we added a simple data structure called
SM_FileMgmtInfo to keep track of the open file being used by the file handle structure.

To handle reading of a block, we consolidated our work to the readBlock method and had
all other methods for reading utilize the basic readBlock method.  The readBlock method
checks for a valid page within the current file and then reads that page into the memory
location provided by the SM_PageHandle structure.

To handle writing of a block, we consolidated our work to the writeBlock method and
had all other write functions utilize the basic writeBlock method.  The method checks
for a valid page within the current file and writes the block to that page number
within the file.  Should we run out of room to write information to the current page,
we append an empty page filled with '\0' data to the file and continue writing.

-- Running the Program --

To compile, simply use the command
	make all
which will compile both the original test_assign1_1 file, and the additional tests added by us.

To run the original test,
	./test_assign1

To run the additional tests (used to check the additional methods and having multiple pages),
	./test_assign1_extra

At the end
	make clean
can be used to remove the executables.
