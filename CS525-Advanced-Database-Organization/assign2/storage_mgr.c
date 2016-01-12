#include "storage_mgr.h"

/************************************************************
*                    interface                             *
************************************************************/

/* manipulating page files */
/* Khush */

extern void initStorageManager(void)
{
}

extern RC createPageFile(char *fileName)
{
    // Open or create the file
    FILE *file = fopen(fileName, "w+");

    // Check for errors
    if (file == NULL)
    {
        return RC_FILE_CREATE_FAILED;
    }

    // Insert one zero-filled page
    char page[PAGE_SIZE];
    memset(page, 0, PAGE_SIZE);

    size_t amountWritten = fwrite(page, PAGE_SIZE, 1, file);

    // Check for write error
    if (amountWritten != 1)
    {
        remove(fileName);
        return RC_WRITE_FAILED;
    }

    // Close and flush all data to file
    if (fclose(file) != 0)
    {
        return RC_FILE_CLOSE_FAILED;
    }

    return RC_OK;
}

extern RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
    // Open the file
    FILE *file = fopen(fileName, "r+");

    // If the file doesn't exist, throw an error
    if (file == NULL)
    {
        return RC_FILE_NOT_FOUND;
    }

    // Setup the file handle
    fHandle->fileName = (char *)malloc(strlen(fileName) + 1);
    strcpy(fHandle->fileName, fileName);

    struct stat st;
    if (stat(fileName, &st) != 0)
    {
        return RC_FILE_SIZE_UNKNOWN;
    }
    fHandle->totalNumPages = (st.st_size) / PAGE_SIZE;

    fHandle->curPagePos = 0;

    SM_FileMgmtInfo *mgmtInfo = (SM_FileMgmtInfo *)malloc(sizeof(SM_FileMgmtInfo));
    mgmtInfo->file = file;
    fHandle->mgmtInfo = mgmtInfo;

    return RC_OK;
}

extern RC closePageFile(SM_FileHandle *fHandle)
{
    // Close and flush data to the file
    FILE *file = ((SM_FileMgmtInfo *)fHandle->mgmtInfo)->file;
    if (fclose(file) != 0)
    {
        return RC_FILE_CLOSE_FAILED;
    }

    // Free memory
    free(fHandle->mgmtInfo);
    free(fHandle->fileName);

    fHandle->mgmtInfo = NULL;
    fHandle->fileName = NULL;
    fHandle = NULL;

    return RC_OK;
}

extern RC destroyPageFile(char *fileName)
{
    if (remove(fileName) != 0)
    {
        return RC_FILE_DESTROY_FAILED;
    }

    return RC_OK;
}

/* reading blocks from disc */
/* Billy & Tom */

extern RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // Check for valid page number
    if (pageNum < 0 || pageNum > fHandle->totalNumPages)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Go to requested page within file
    FILE *file = ((SM_FileMgmtInfo*)fHandle->mgmtInfo)->file;
    fseek(file, pageNum * PAGE_SIZE, SEEK_SET);

    // Read the section
    size_t amountRead = fread(memPage, PAGE_SIZE, 1, file);

    // Check for read failure
    if (amountRead < PAGE_SIZE && ferror(file))
    {
        return RC_READ_PAGE_FAILED;
    }

    // Update where we are in the file
    fHandle->curPagePos = pageNum;

    return RC_OK;
}

extern int getBlockPos(SM_FileHandle *fHandle)
{
    return fHandle->curPagePos;
}

extern RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    return readBlock(0, fHandle, memPage);
}

extern RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int prevPage = getBlockPos(fHandle) - 1;
    return readBlock(prevPage, fHandle, memPage);

}

extern RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int currBlock = getBlockPos(fHandle);
    return readBlock(currBlock, fHandle, memPage);
}

extern RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int nextBlock = getBlockPos(fHandle) + 1;
    return readBlock(nextBlock, fHandle, memPage);
}

extern RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int lastBlock = fHandle->totalNumPages - 1;
    return readBlock(lastBlock, fHandle, memPage);
}

/* writing blocks to a page file */
/* Jesi */
/* Writing full pages at a time... */

extern RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    FILE *fp = ((SM_FileMgmtInfo*)fHandle->mgmtInfo)->file;

    if (pageNum < fHandle->totalNumPages) { //check if pageNum is a legal value
        //change curPagePos to specified pageNum
        fHandle->curPagePos = pageNum;
        fseek(fp, fHandle->curPagePos*PAGE_SIZE, SEEK_SET);
        //now write memPage to file
        int input_size = strlen(memPage);

        int check = 0, i = 0;
        for (i = 0; i < input_size; i++){
            //fprintf(fp, "%s", memPage); //assumes memPage is <= PAGE_SIZE
            putc(memPage[i], fp);
            check++;

            int currentPage = fHandle->curPagePos;
            int totalPages = fHandle->totalNumPages;
            if (check == PAGE_SIZE) {
                if (currentPage == totalPages - 1) { //ran out of pages to write to
                    appendEmptyBlock(fHandle);
                }
                fHandle->curPagePos = currentPage++;
                check = 0;
            }
        }
        return RC_OK; //done writing, no errors
    }
    return RC_WRITE_FAILED; //failed because pageNum is too large
}

extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int currBlock = fHandle->curPagePos;
    return writeBlock(currBlock, fHandle, memPage); //done writing, no errors
}

extern RC appendEmptyBlock(SM_FileHandle *fHandle)
{
    //Go to end of file, using the current totalNumOfPages as aid
    //add on page of \0 (similar to createPageFile())
    FILE *fp = ((SM_FileMgmtInfo*)fHandle->mgmtInfo)->file;
    int i = 0;

    fseek(fp, (fHandle->totalNumPages*PAGE_SIZE) + 1, SEEK_SET);
    for (i = 0; i < PAGE_SIZE; i++){
        putc('\0', fp);
    }

    fHandle->totalNumPages++;
    return RC_OK;
}

extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
    FILE *fp = ((SM_FileMgmtInfo*)fHandle->mgmtInfo)->file;

    //similar to appendEmptyBlock, except may add several pages
    if (numberOfPages > fHandle->totalNumPages) {
        int pagestoAdd = numberOfPages - fHandle->totalNumPages, i = 0;

        fseek(fp, (fHandle->totalNumPages*PAGE_SIZE) + 1, SEEK_SET);
        for (i = 0; i < PAGE_SIZE*pagestoAdd; i++){
            putc('\0', fp);
        }
        fHandle->totalNumPages = numberOfPages;
        return RC_OK;
    }

    return RC_NO_PAGES_TO_ADD;
}
