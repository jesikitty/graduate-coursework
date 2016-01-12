#include "buffer_mgr.h"

/************************************************************
*                    interface                             *
************************************************************/

/* Buffer Manager Interface Pool Handling */
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    // Initialize buffer pool
    bm->pageFile = (char *)malloc(strlen(pageFileName) + 1);
    strcpy(bm->pageFile, pageFileName);
    bm->numPages = numPages;
    bm->strategy = strategy;

    // Initialize management data for buffer pool
    BM_MgmtData *mgmtData;
    mgmtData = MAKE_MGMTDATA();
    mgmtData->ioWrites = 0;
    mgmtData->ioReads = 0;
    mgmtData->head = NULL;

    // Open page file for buffer mgmt
    openPageFile(bm->pageFile, &mgmtData->fh);
    
    // Initialize frame pool
    mgmtData->framePool = MAKE_FRAME_POOL(numPages);

    int i;
    for (i = 0; i < numPages; i++) {
        mgmtData->framePool[i].frameID = i;
        mgmtData->framePool[i].isDirty = FALSE;
        mgmtData->framePool[i].fixCount = 0;
        mgmtData->framePool[i].pageNum = NO_PAGE;
        mgmtData->framePool[i].memPage = (SM_PageHandle)malloc(PAGE_SIZE);
    }

    // For LRU and FIFO
    BM_Node *head = newNode(&mgmtData->framePool[0]);
    BM_Frame *frame = NULL;

    for(i = 1; i < bm->numPages; i++){
        frame = &mgmtData->framePool[i];
        insertAtHead(&head, frame);
    }
    mgmtData->head = head;

    bm->mgmtData = mgmtData;

    return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm) {
    // Flush the pool
    RC flushResult = forceFlushPool(bm);
    if (flushResult != RC_OK) {
        return flushResult;
    }

    // Retrieve management data
    BM_MgmtData *mgmtData = bm->mgmtData;
    BM_Frame *frame = &mgmtData->framePool[0];

    // Check for pinned pages
    int i;
    for (i = 0; i < bm->numPages; i++) {
        if (frame->fixCount != 0) {
            return RC_FRAME_PINNED_PAGE;
        }

        frame++;
    }

    // Close storage manager details
    RC pageCloseResult = closePageFile(&mgmtData->fh);
    if (pageCloseResult != RC_OK) {
        return pageCloseResult;
    }

    // Free memory and lists
    // Free frames
    for (i = 0; i < bm->numPages; i++) {
        free(mgmtData->framePool[i].memPage);
    }

    // Free frame pool
    free(mgmtData->framePool);

    // Free linked list nodes
    BM_Node *freeHead = mgmtData->head;
    while (freeHead != NULL) {
        freeHead = removeFromHead(&mgmtData->head);
        if (freeHead != NULL) {
            free(freeHead);
        }
    }

    // Free mgmt data
    free(bm->mgmtData);

    // Free buffer pool 
    free(bm->pageFile);
    
    return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm) {
    // Retrieve management data
    BM_MgmtData *mgmtData = bm->mgmtData;
    BM_Frame *frame = NULL;
    // Write dirty pages
    int i;
    for (i = 0; i < bm->numPages; i++) {
        frame = &mgmtData->framePool[i];
        // Check to see if current page is dirty
        if (frame->isDirty && frame->fixCount == 0) {
            RC writeBlockResult = writeBlock(frame->pageNum, &mgmtData->fh, frame->memPage);

            if (writeBlockResult != RC_OK) {
                return writeBlockResult;
            }

            mgmtData->ioWrites++;
            frame->isDirty = FALSE;
        }
    }

    return RC_OK;
}

/* Buffer Manager Interface Access Pages */
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
    // Retrieve management data
    BM_MgmtData *mgmtData = bm->mgmtData;
    // Check if a frame is already assigned to the page
    BM_Frame *frame = NULL;
    int i;
    for (i = 0; i < bm->numPages; i++) {
        frame = &mgmtData->framePool[i];
        if (page->pageNum == frame->pageNum) {
            // Page exists within the buffer
            
            frame->isDirty = TRUE;
            return RC_OK;
        }
    }

    // Page does not exist within the buffer
    return RC_PAGE_NOT_PINNED;
}

RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    // Retrieve management data
    BM_MgmtData *mgmtData = bm->mgmtData;

    // Check if a frame is already assigned to the page
    BM_Frame *frame = NULL;
    int i;

    for (i = 0; i < bm->numPages; i++) {
        frame = &mgmtData->framePool[i];
        if (page->pageNum == frame->pageNum) {
            // Page exists within the buffer
            frame->fixCount--;

            return RC_OK;
        }
    }
    // Page does not exist within the buffer
    return RC_PAGE_NOT_PINNED;
}

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    // Retrieve management data
	BM_MgmtData *mgmtData= bm->mgmtData;

	// Check if a frame is already assigned to the page
	BM_Frame *frame= &mgmtData->framePool[0];
    RC result = RC_PAGE_NOT_PINNED;
	int i;
    for (i = 0; i < bm->numPages; i++) {
        if (page->pageNum == frame->pageNum) {
		    // Page exists within the buffer
            if (frame->isDirty && frame->fixCount == 0) {
			    // Check to see if current page is dirty
                RC result = writeBlock(frame->pageNum, &mgmtData->fh, frame->memPage);
				
                if(result==RC_OK){
                    frame->isDirty = FALSE;
					mgmtData->ioWrites++;
                    return result;
				}
				return result;
			}
		}
        frame++;
	}

	return result;
}

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
    // Retrieve management data
    BM_MgmtData *mgmtData = bm->mgmtData;

    BM_Frame *frame = NULL;

    // Check for existing frame for this page
    int i;
    for (i = 0; i < bm->numPages; i++) {
        if (pageNum == (&mgmtData->framePool[i])->pageNum) {
            frame = &mgmtData->framePool[i];
            break;
        }
    }
    //printf("Passed check for existing frame");
    //printf("%s", frame);

    // Did we find a frame for this page?
    // If so, update the replacement strategy
    if (frame != NULL) {
        // Update LRU
        if (bm->strategy == RS_LRU) {
            movetoHead(&mgmtData->head, frame);
        }

        frame->fixCount++;
        page->pageNum = pageNum;
        page->data = &frame->memPage[0];
        return RC_OK;
    }
    // If we made it this far, we need to find a free frame
    
    //printf("About to go into find free frame");
    frame = findFreeFrame(bm);

    //printf("Passed find free frame");
    //printf("%s", frame);

    // Check to see if the buffer is full
    if (frame == NULL) {
        //printf("Fail for # %d ", pageNum);
        return RC_BUFFER_FRAME_POOL_FULL;
    }

    //printf("passed buffer frame pool full");

    // Add page to buffer
    if (pageNum >= mgmtData->fh.totalNumPages) {
        //printf("we had to check for capacity");
        RC capacityCheck = ensureCapacity(pageNum + 1, &mgmtData->fh);

        if (capacityCheck != RC_OK) {
            return capacityCheck;
        }
    }

    //printf("passed buffer frame capacity check");
    
    RC resultReadBlock = readBlock(pageNum, &mgmtData->fh, &frame->memPage[0]);
    if (resultReadBlock != RC_OK) {
        return resultReadBlock;
    }

    mgmtData->ioReads++;

    // Mark frame as used
    frame->fixCount++;
    frame->pageNum = pageNum;
    page->pageNum = pageNum;
    page->data = &frame->memPage[0];

    if(bm->strategy == RS_LRU || bm->strategy == RS_FIFO){
        insertAtHead(&mgmtData->head, frame);
    }

    return RC_OK;
}

// Find free frame for pinning
BM_Frame* findFreeFrame(BM_BufferPool *const bm)
{
    BM_Frame *frame = NULL;
    RC findResult = RC_OK;

    switch (bm->strategy) {
    case RS_FIFO:
        frame = findFIFO(bm);
        findResult = RC_OK;
        break;
    case RS_LRU:
        frame = findLRU(bm);
        findResult = RC_OK;
        break;
    }

    return frame;
}

BM_Frame *findFIFO(BM_BufferPool *const bm) {
    // Retrieve management data
    BM_MgmtData *mgmtData = bm->mgmtData;
    RC fifoResult = RC_OK;
    BM_Frame *frame = NULL;

    BM_Node *head = mgmtData->head;
    
    // Do we have a head or do we need to create a list?
    if(head == NULL){
        head = newNode(&mgmtData->framePool[0]);
        int i;
        for (i = 0; i < bm->numPages; i++) {
            BM_Frame *checkFrame = &mgmtData->framePool[i];
            frame = &mgmtData->framePool[i];
            insertAtHead(&head, frame);
        }
        mgmtData->head = head;
    }

    //in use
    frame = removeFromTail(head)->frame;
    while(frame->fixCount == 1){
        insertAtHead(&head, frame);
        frame = removeFromTail(head)->frame;
    }

    //must write data before using
    if (frame->isDirty && frame->fixCount == 0) {
        RC writeBlockResult = writeBlock(frame->pageNum, &mgmtData->fh, frame->memPage);

        mgmtData->ioWrites++;
        frame->isDirty = FALSE;
    }

    return frame;
}

BM_Frame *findLRU(BM_BufferPool *const bm) {
    // Retrieve management data
    BM_MgmtData *mgmtData = bm->mgmtData;
    RC lruResult = RC_OK;
    BM_Frame *frame;

    BM_Node *head = mgmtData->head;
    frame = removeFromTail(head)->frame;

    return frame;
}

/* Statistics Interface */
/* Jesi */

/* Returns an array of PageNumbers (of size numPages) where the ith element is the # of the page stored in the ith page frame.
An empty page frame is represented using the constant NO_PAGE */
PageNumber *getFrameContents(BM_BufferPool *const bm) {
    BM_MgmtData *mgmtData = bm->mgmtData;
    BM_Frame *frame = &mgmtData->framePool[0];

    PageNumber *contents = malloc(bm->numPages);
    int i;
    for (i = 0; i < bm->numPages; i++){
        contents[i] = frame->pageNum;
        frame++;
    }

    return contents;
}

/* Returns an array of booleans (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty.
Empty pages frames are considered clean. */
bool *getDirtyFlags(BM_BufferPool *const bm) {
    BM_MgmtData *mgmtData = bm->mgmtData;
    BM_Frame *frame = &mgmtData->framePool[0];

    bool *dirtyPages = malloc(bm->numPages);
    int i;
    for (i = 0; i < bm->numPages; i++){
        dirtyPages[i] = frame->isDirty;
        frame++;
    }

    return dirtyPages;
}

/* Returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame.
Return 0 for empty page frames */
int *getFixCounts(BM_BufferPool *const bm) {
    BM_MgmtData *mgmtData = bm->mgmtData;
    BM_Frame *frame = &mgmtData->framePool[0];

    int *fixCounts = malloc(bm->numPages);
    int i;
    for (i = 0; i < bm->numPages; i++){
        fixCounts[i] = frame->fixCount;
        frame++;
    }

    return fixCounts;
}

/* Returns the # of pages that have been read from disk since a buffer pool has been initialized.
Your code is responsible for initializing this statistic at pool creating time
and update whenever a page is read from the page file into a page frame. */
int getNumReadIO(BM_BufferPool *const bm) {
    BM_MgmtData *mgmtData = bm->mgmtData;
    return *(&mgmtData->ioReads);
}

/* Returns the # of pages written to the page file since the buffer pool has been initialized */
int getNumWriteIO(BM_BufferPool *const bm) {
    BM_MgmtData *mgmtData = bm->mgmtData;
    return *(&mgmtData->ioWrites);
}

/**************************************
    Helper Funcitons
***************************************/

BM_Node* newNode(BM_Frame *frame) {
    BM_Node *node = (BM_Node*)malloc(sizeof(BM_Node));
    node->frame = frame;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

void insertAtHead(BM_Node **head, BM_Frame *frame) {
    BM_Node *node = newNode(frame);
    if (head == NULL) {
        *head = node;
        return;
    }

    (*head)->prev = node;
    node->next = *head;
    *head = node;
}

void insertAtTail(BM_Node *head, BM_Frame *frame) {
    BM_Node *node = newNode(frame);
    if (head == NULL) {
        head = node;
        return;
    }
    BM_Node *temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = node;
    node->prev = temp;
}

BM_Node* removeFromHead(BM_Node **head) {
    if (*head == NULL) {
        return NULL;
    }

    BM_Node *newHead = (*head)->next;

    // Is this the only item in the list?
    if (newHead == NULL) {
        newHead = *head;
        //memcpy(newHead, *head, sizeof(BM_Node));
        *head = NULL;
        return newHead;
    }


    BM_Node *returnNode = *head;
    *head = (*head)->next;
    (*head)->prev = NULL;
    
    return returnNode;
}

BM_Node* removeFromTail(BM_Node *head) {
    if (head == NULL) {
        return NULL;
    }

    BM_Node *temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    
    temp->prev->next = NULL;
    return temp;
}

void movetoHead(BM_Node **head, BM_Frame *frame){
    BM_Node *temp = *head;

    while (temp->next != NULL) {
        if (temp->next->frame->pageNum == frame->pageNum)
            break;
        temp = temp->next;
    }

    if(temp->next == NULL)
        return;

    //found frame in list, currently in temp
    BM_Node *node = temp->next;
    if(node->prev != NULL)
        node->prev->next = node->next;
    if(temp->next != NULL)  
        node->next->prev = node->prev;

    insertAtHead(head, frame);
}