CS 525, Spring 2015
William Bafia
Jesi Merrick
Khush Deoja
Thomas Mathews

-- Project Implementation Details --

Similar to how we implemented the previous two assignments, we built off the provided interface and data structures to create a record manager, while also including some of our own functions and datastructures to supplement.  The functionality is described in more detail below:

* createTable() and closeTable()
To begin, we created a table using the createTable() function.  This required that we store information at the beginning of the table to describe its contents, items such as 
management data and schema information.  This information is important in the maintenance of the table.  For example, it is important in locating free space to be utilized for future record insertions.  Once the table is no longer needed, closeTable() shuts down allocated resources and frees memory being used.

* deleteTable()
We simply destroy the page file holding the table using our destroyPageFile() function.

* getNumTuples()
We stored the num of tuples in a table mgmt data data structure for easy referencing.  Within this data structure is a int "numTuples" that is returned by this funtion to indicate the current number of tuples in a table.

* insertRecord() and deleteRecord() 
Insertion follows a fairly straight forward algorithm.  First, we simple check if there are any free slots to place the record into.  If there are not any slots, a new page is added to the buffer pool. If there is no space available, we append a new page to the buffer, allowing for new records to be inserted.  Otherwise, we simply insert the record into the free slot.  Since records are not deleted completely in record managers until the database is reorganized, we used tombstones to simply mark a record as ready to be deleted.  This was simply a byte flipped from 1 to -1 if a record was deleted (boolean).

* updateRecord() 
Update record simply replaces the previous record with the new record.  

* startScan(), next(), and closeScan()
These functions all rely on using a list of pages to scan through.  In order to do this it was necessary to create a new datastructure to handle scan mgmt data.  Using this mgmt data the scanner simply interates through the records one by one.  If the expected expr is found, the record is returned, otherwise the scan continues.  Once the scan completes, closeScan() is called and releases allocated resources. 

* getAttr() and setAttr()
These functions simply iterated through the number of attributes in a schema until the chosen attrNum was found, once found they either return the value of the attribute or they set the value of the attribute to a new value.


-- Running the Program --

To compile, simply use the command
	make all
which will compile the original test_assign3_1 file.

To run the original test file use,
	./test_assign3

At the end
	make clean
can be used to remove the executables.
