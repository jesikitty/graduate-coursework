#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

//Necessary for SM_PageHandle and SM_FileHandle declarations
#include "storage_mgr.h"

// Include return codes and methods for logging errors
#include "dberror.h"

/* Generic includes */
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>

// Include bool DT
#include "dt.h"

// Replacement Strategies
typedef enum ReplacementStrategy {
    RS_FIFO = 0,
    RS_LRU = 1,
    RS_CLOCK = 2,
    RS_LFU = 3,
    RS_LRU_K = 4
} ReplacementStrategy;

/************************************************************
*           Data types & structures                         *
************************************************************/
typedef int PageNumber;
#define NO_PAGE -1

typedef struct BM_BufferPool {
    char *pageFile;
    int numPages;
    ReplacementStrategy strategy;
    void *mgmtData; // use this one to store the bookkeeping info your buffer manager needs for a buffer pool
} BM_BufferPool;

typedef struct BM_PageHandle {
    PageNumber pageNum;
    char *data;
} BM_PageHandle;

/************************************************************
*           Additional Helper Structs                       *
************************************************************/

typedef struct BM_Frame {
    PageNumber pageNum;
    int frameID;
    int fixCount;
    bool isDirty;
    SM_PageHandle memPage;
} BM_Frame;

typedef struct BM_Node {
    BM_Frame *frame;
    struct BM_Node *next;
    struct BM_Node *prev;
} BM_Node;

typedef struct BM_MgmtData {
    SM_FileHandle fh;

    BM_Frame *framePool;
    int ioWrites;
    int ioReads;

    BM_Node *head;
} BM_MgmtData;

/************************************************************
*               Convenience Macros                          *
************************************************************/

#define MAKE_POOL()					\
    ((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()				\
    ((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

#define MAKE_FRAME()               \
    ((BM_Frame *) malloc (sizeof(BM_Frame)))

#define MAKE_FRAME_POOL(i)               \
    ((BM_Frame *) malloc (sizeof(BM_Frame) * i))

#define MAKE_MGMTDATA()         \
    ((BM_MgmtData *) malloc (sizeof(BM_MgmtData)))

/************************************************************
*               Interface                                   *
************************************************************/

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData);
RC shutdownBufferPool(BM_BufferPool *const bm);
RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page);
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page);
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page);
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum);

BM_Frame* findFreeFrame(BM_BufferPool *const bm);
BM_Frame *findFIFO(BM_BufferPool *const bm);
BM_Frame *findLRU(BM_BufferPool *const bm);

// Statistics Interface
PageNumber *getFrameContents(BM_BufferPool *const bm);
bool *getDirtyFlags(BM_BufferPool *const bm);
int *getFixCounts(BM_BufferPool *const bm);
int getNumReadIO(BM_BufferPool *const bm);
int getNumWriteIO(BM_BufferPool *const bm);

/**************************************
Helper Funcitons
***************************************/
BM_Node* newNode(BM_Frame *frame);
void insertAtHead(BM_Node **head, BM_Frame *frame);
void insertAtTail(BM_Node *head, BM_Frame *frame);
BM_Node* removeFromHead(BM_Node **head);
BM_Node* removeFromTail(BM_Node *head);
void movetoHead(BM_Node **head, BM_Frame *frame);

#endif
