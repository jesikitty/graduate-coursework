#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile_extra.bin"

/* prototypes for test functions */
static void testMultiplePage(void);
static void testReadNonExistingPage(void);
static void testPagesAdded(void);
static void testBlockPos(void);
/* main function running all tests */
int main(void)
{
    testName = "";

    initStorageManager();

    testMultiplePage();

    testReadNonExistingPage();

    testPagesAdded();

    testBlockPos();

    return 0;
}

// Tests navigation and writing in a multiple page file.  Ensures writing and readings are occuring on the correct pages using the various read and write functions

void testMultiplePage(void)
{
    SM_FileHandle fh;
    SM_PageHandle ph;
    int i;

    testName = "Test for multiple page read and writing";

    ph = (SM_PageHandle)malloc(PAGE_SIZE);

    // create a new page file
    createPageFile(TESTPF);
    openPageFile(TESTPF, &fh);

    //Make file 3 pages long
    ensureCapacity(3,&fh);

    //check last block exist and is empty
    readLastBlock(&fh, ph);

    // the page should be empty (zero bytes)
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 0), "expected zero byte appended last  block");
    printf("last block was empty\n");

    //write to last block using writeCurrentBlock()
    for (i = 0; i < PAGE_SIZE; i++)
        ph[i] = (i % 10) + '0';
    writeCurrentBlock(&fh, ph);
    printf("wrote to current (last) block\n");

    // read back last block making sure was properly written to
    readCurrentBlock(&fh, ph);
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
    printf("read last block\n");

    // read back second block
    readPreviousBlock(&fh, ph);
    // the page should be empty (zero bytes)
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page");
    printf("second block was empty\n");
    
    // read back to first block 
    readPreviousBlock(&fh, ph);
    // the page should be empty (zero bytes)
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page");
    printf("first block was empty\n");

    //write to first block and test writeCurrentBlock
    for (i = 0; i < PAGE_SIZE; i++)
        ph[i] = (i % 5) + '0';
    writeCurrentBlock(&fh, ph);
    printf("wrote to current (first) block\n");    
    
      // read first block with readFirstBlock() and verify data was written in
    readFirstBlock(&fh, ph);
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == (i % 5) + '0'), "character in page read from disk is the one we expected.");
    printf("read first block\n");

    // read back to second block and make sure still empty
    readNextBlock(&fh, ph);
    // the page should be empty still (zero bytes)
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page");
    printf("second block was still empty\n");

    // destroy new page file
    destroyPageFile(TESTPF);

    TEST_DONE();
}

void testReadNonExistingPage(void) 
{

  SM_FileHandle fh;
  SM_PageHandle ph;

  testName = "test for proper non existant page return";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // create a new page file
  createPageFile (TESTPF);
  openPageFile (TESTPF, &fh);

  // test that can't go below 0 pages
  ASSERT_ERROR(readPreviousBlock(&fh,ph), "page did not exist below 0 and error was properly returned.");
  ASSERT_ERROR(readBlock(5,&fh,ph), "page did not exist higher than total pages  and error was properly returned.");


  destroyPageFile(TESTPF);
  TEST_DONE();

}

void testPagesAdded(void) 
{
    SM_FileHandle fh;
    SM_PageHandle ph;

    testName = "test adding pages";

    ph = (SM_PageHandle) malloc(PAGE_SIZE);

    // create a new page file
    createPageFile (TESTPF);
    openPageFile (TESTPF, &fh);

    //Increase total number of pages to 4
    int pagesAdded = 4;
    ensureCapacity(pagesAdded, &fh);

    // check to see that 4 pages were added
    ASSERT_EQUALS_INT(pagesAdded, fh.totalNumPages, "correct # pages added by ensure capacity");
    
    // append another page on
    appendEmptyBlock(&fh);
    pagesAdded++;

    // check to see increased to 5 pages
    ASSERT_EQUALS_INT(pagesAdded, fh.totalNumPages, "correct # pages added by append");

    destroyPageFile(TESTPF);
    TEST_DONE();  
}

void testBlockPos(void) 
{
    SM_FileHandle fh;
    SM_PageHandle ph;

    testName = "test block position updates";

    ph = (SM_PageHandle) malloc(PAGE_SIZE);
 
    // create a new page file
    createPageFile (TESTPF);
    openPageFile (TESTPF, &fh);
    
    //create empty pages to navigate
    ensureCapacity(5, &fh);
    
    //navigate pages by reading
    readLastBlock(&fh, ph);
    int pagePos = 4;
    ASSERT_EQUALS_INT(getBlockPos(&fh),pagePos, "The page position is correct.");

    //move the pagePos back two reads
    readPreviousBlock(&fh, ph);
    readPreviousBlock(&fh, ph);
    pagePos = pagePos-2;

    //check that pagePos was updated correctly
    ASSERT_EQUALS_INT(getBlockPos(&fh), pagePos, "The page position is correct.");

    //move the pagePos up one
    readNextBlock(&fh, ph);
    pagePos++;
    
    //check that pagePos was updated correctly by readNextPage()
    ASSERT_EQUALS_INT(getBlockPos(&fh), pagePos, "The page position is correct.");

    //destroy file and end test
    destroyPageFile(TESTPF);
    TEST_DONE();    
    
}

