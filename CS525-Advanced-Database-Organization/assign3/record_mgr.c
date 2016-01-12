#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "tables.h"

#define NAME_SIZE 16
#define TYPE_SIZE 4
#define LENGTH_SIZE 4
#define KEY_SIZE 4

/************************************************************
*                    interface                             *
************************************************************/

RC initRecordManager(void *mgmtData) {
	return RC_OK;
}

RC shutdownRecordManager() {
	return RC_OK;
}

RC createTable(char *name, Schema *schema) {
	SM_FileHandle fh;
	char data[PAGE_SIZE];
	char *offset = data;

	// Prepare schema for writing to block header
	memset(offset, 0, PAGE_SIZE);
	*(int*)offset = 0;
	offset = offset + sizeof(int);
	*(int*)offset = 0;
	offset = offset + sizeof(int);
	*(int*)offset = schema->numAttr;
	offset = offset + sizeof(int);
	*(int*)offset = schema->keySize;
	offset = offset + sizeof(int);

	// Copy everything from schema to be written to block
	int i;
	for (i = 0; i < schema->numAttr; i++) {
		strncpy(offset, schema->attrNames[i], NAME_SIZE);
		offset = offset + NAME_SIZE;

		*(int*)offset = (int)schema->dataTypes[i];
		offset = offset + TYPE_SIZE;

		*(int*)offset = (int)schema->typeLength[i];
		offset = offset + LENGTH_SIZE;

		if (i < schema->keySize) {
			*(int*)offset = (int)schema->keyAttrs[i];
		}
		offset = offset + KEY_SIZE;
	}

	// Write the table and schema information to file
	RC createPageFileResult;
	if ((createPageFileResult = createPageFile(name)) != RC_OK) {
		return createPageFileResult;
	}
	RC openPageFileResult;
	if ((openPageFileResult = openPageFile(name, &fh)) != RC_OK) {
		return openPageFileResult;
	}
	RC writeBlockResult;
	if ((writeBlockResult = writeBlock(0, &fh, data)) != RC_OK) {
		return writeBlockResult;
	}
	RC closePageFileResult;
	if ((closePageFileResult = closePageFile(&fh)) != RC_OK) {
		return closePageFileResult;
	}

	return RC_OK;
}

RC openTable(RM_TableData *rel, char *name) {
	// Allocate table resources
	RM_TableMgmtData *tableMgmtData = (RM_TableMgmtData*)malloc(sizeof(RM_TableMgmtData));
	rel->mgmtData = tableMgmtData;
	rel->name = (char *)malloc(strlen(name) + 1);
	strcpy(rel->name, name);

	// Setup buffer pool for table 
	initBufferPool(&tableMgmtData->bufferPool, rel->name, 200, RS_LRU, NULL);

	// Read table from page
	RC pinPageResult;
	if ((pinPageResult = pinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle, 0)) != RC_OK) {
		return pinPageResult;
	}

	// Setup schema for table
	int numAttrs;
	int keySize;
	char *offset;
	offset = (char*)tableMgmtData->pageHandle.data;

	tableMgmtData->numTuples = *(int*)offset;
	offset += sizeof(int);
	tableMgmtData->nextFreeSlot = *(int*)offset;
	offset += sizeof(int);
	numAttrs = *(int*)offset;
	offset += sizeof(int);
	keySize = *(int*)offset;
	offset += sizeof(int);

	rel->schema = MAKE_SCHEMA();
	rel->schema->numAttr = numAttrs;
	rel->schema->keySize = keySize;
	rel->schema->attrNames = MAKE_ATTRNAMES(numAttrs);
	rel->schema->dataTypes = MAKE_DATATYPES(numAttrs);
	rel->schema->typeLength = MAKE_TYPELENGTH(numAttrs);
	rel->schema->keyAttrs = MAKE_KEYATTRS(keySize);

	int i;
	for (i = 0; i < numAttrs; i++) {
		rel->schema->attrNames[i] = (char*)malloc(NAME_SIZE);
	}

	// Read and store schema in memory from page
	for (i = 0; i < rel->schema->numAttr; i++) {
		strncpy(rel->schema->attrNames[i], offset, NAME_SIZE);
		offset = offset + NAME_SIZE;

		rel->schema->dataTypes[i] = *(int*)offset;
		offset = offset + 4;

		rel->schema->typeLength[i] = *(int*)offset;
		offset = offset + 4;

		if (i < keySize)
			rel->schema->keyAttrs[i] = *(int*)offset;
		offset = offset + 4;
	}

	// Finished reading and dispose of page
	RC unpinPageResult;
	if (unpinPageResult = unpinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return unpinPageResult;
	}

	return RC_OK;
}

RC closeTable(RM_TableData *rel) {
	RM_TableMgmtData *tableMgmtData;
	tableMgmtData = rel->mgmtData;

	// Read page and prepare schema
	RC pinPageResult;
	if ((pinPageResult = pinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle, 0)) != RC_OK) {
		return pinPageResult;
	}

	char *offset;
	offset = (char*)tableMgmtData->pageHandle.data;

	RC markDirtyResult;
	if (markDirtyResult = markDirty(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return markDirtyResult;
	}

	// Update table data
	*(int*)offset = tableMgmtData->numTuples;

	RC unpinPageResult;
	if (unpinPageResult = unpinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return unpinPageResult;
	}

	// Free buffer pool
	shutdownBufferPool(&tableMgmtData->bufferPool);

	// Free schema memory
	free(rel->name);
	free(rel->schema->attrNames);
	free(rel->schema->dataTypes);
	free(rel->schema->typeLength);
	free(rel->schema->keyAttrs);
	free(rel->schema);
	rel->schema = NULL;

	// Free mgmt info
	free(rel->mgmtData);
	rel->mgmtData = NULL;

	return RC_OK;
}

RC deleteTable(char *name) {
	// delete the associated page file
	return destroyPageFile(name);
}

int getNumTuples(RM_TableData *rel) {
	// return the numTuples in the TableMgmtData struct
	return ((RM_TableMgmtData*)(rel->mgmtData))->numTuples;
}

// Bulk of work will be here 
// handling records in a table
RC insertRecord(RM_TableData *rel, Record *record) {
	RM_TableMgmtData *tableMgmtData = rel->mgmtData;
	BM_MgmtData *bufferMgmtData = tableMgmtData->bufferPool.mgmtData;
	RID *rid = &record->id;
	RM_Page *page;
	char *slotOffset;

	//  Do we have a free slot?
	if (tableMgmtData->nextFreeSlot > 0) {
		// We have a free slot for the record
		rid->page = tableMgmtData->nextFreeSlot;
		// Pin page to insert record
		RC pinPageResult;
		if (pinPageResult = pinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle, rid->page) != RC_OK) {
			return pinPageResult;
		}

		page = (RM_Page*)tableMgmtData->pageHandle.data;

		// Find the free slot for the record and save it
		rid->slot = findFreeSlot(page, rel->schema);

		if (rid->slot == NO_SLOT) { // Append new page, out of space
			RC unpinPageResult;
			if (unpinPageResult = unpinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
				return unpinPageResult;
			}

			if (appendEmptyBlock(&bufferMgmtData->fh) != RC_OK) {
				return RC_RM_INSERT_RECORD_FAIL;
			}

			rid->page = bufferMgmtData->fh.totalNumPages - 1;

			RC pinPageResult;
			if (pinPageResult = pinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle, rid->page) != RC_OK) {
				return pinPageResult;
			}
			
			//new page, set slot to 0 (first slot)
			page = (RM_Page*)tableMgmtData->pageHandle.data;
			rid->slot = 0;
		}
	}
	else {
		// No free slot so make new page for record
		RC appendEmptyBlockResult;
		if (appendEmptyBlock(&bufferMgmtData->fh) != RC_OK) {
			return RC_RM_INSERT_RECORD_FAIL;
		}

		rid->page = bufferMgmtData->fh.totalNumPages - 1;

		RC pinPageResult;
		if (pinPageResult = pinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle, rid->page) != RC_OK) {
			return pinPageResult;
		}
		page = (RM_Page*)tableMgmtData->pageHandle.data;

		rid->slot = 0;
	
	}

	// Finish writing the record now that we have slot information
	RC markDirtyResult;
	if (markDirtyResult = markDirty(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return markDirtyResult;
	}
	
	// Add header offset to slot position for slot offset
	slotOffset = ((char*)&page->data) + (rid->slot * getRecordSize(rel->schema));

	int recordSize = getRecordSize(rel->schema);
	memcpy(slotOffset + 1, record->data, recordSize);

	// Mark first byte of slot with tombstone information info
	*(char*)slotOffset = 1;

	// Update free slot information
	if (findFreeSlot(page, rel->schema) != NO_SLOT) {
		int pageNum = rid->page;
		addFreeSlot(tableMgmtData, page, rid->page);
	}

	RC unpinPageResult;
	if (unpinPageResult = unpinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return unpinPageResult;
	}
	tableMgmtData->numTuples++;

	return RC_OK;
}

int findFreeSlot(RM_Page *page, Schema *schema) {
	int totalSlots = ((int)(PAGE_SIZE - ((&((RM_Page*)0)->data) - ((char*)0)))) / getRecordSize(schema);
	char *slotOffset = (char*)&page->data;
	int recordSize = getRecordSize(schema);

	int slot;
	for (slot = 0; slot < totalSlots; slot++) {
		// Do we have a tombstone?
		if ((*(char*)slotOffset) <= 0) {
			return slot;
		}
		slotOffset = slotOffset + recordSize;
	}
	return NO_SLOT;
}

// Simply marks tombstones, doesn't physically delete or allow for records to overwrite
RC deleteRecord(RM_TableData *rel, RID id) {
	RM_TableMgmtData *tableMgmtData = rel->mgmtData;
	RM_Page *page;

	RC pinPageResult;
	if (pinPageResult = pinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle, id.page) != RC_OK) {
		return pinPageResult;
	}

	page = (RM_Page*)tableMgmtData->pageHandle.data;
	char *slotOffset = ((char*)&page->data) + (id.slot*getRecordSize(rel->schema));

	// Update tombstone
	*(char*)slotOffset = -1;

	RC markDirtyResult;
	if (markDirtyResult = markDirty(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return markDirtyResult;
	}

	RC unpinPageResult;
	if (unpinPageResult = unpinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return unpinPageResult;
	}

	tableMgmtData->numTuples--;
	return RC_OK;
}

RC updateRecord(RM_TableData *rel, Record *record) {
	RID *rid = &record->id;
	RM_TableMgmtData *tableMgmtData = rel->mgmtData;
	RM_Page *page;

	//  Check to see if feasible record
	if (rid->page == NO_PAGE || rid->slot == NO_SLOT) {
		return RC_RM_NO_RECORD;
	}

	RC pinPageResult;
	if (pinPageResult = pinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle, rid->page) != RC_OK) {
		return pinPageResult;
	}
	page = (RM_Page*)tableMgmtData->pageHandle.data;

	char *slotOffset = ((char*)&page->data) + (rid->slot * getRecordSize(rel->schema));

	int recordSize = getRecordSize(rel->schema);
	memcpy(slotOffset + 1, record->data, recordSize);

	RC markDirtyResult;
	if (markDirtyResult = markDirty(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return markDirtyResult;
	}

	RC unpinPageResult;
	if (unpinPageResult = unpinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return unpinPageResult;
	}

	return RC_OK;
}

RC getRecord(RM_TableData *rel, RID id, Record *record) {
	RM_TableMgmtData *tableMgmtData = rel->mgmtData;
	RM_Page *page;

	if (id.page == NO_PAGE || id.slot == NO_SLOT) {
		return RC_RM_NO_RECORD;
	}

	RC pinPageResult;
	if (pinPageResult = pinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle, id.page) != RC_OK) {
		return pinPageResult;
	}

	page = (RM_Page *)tableMgmtData->pageHandle.data;
	char *slotOffset = ((char*)&page->data) + (id.slot * getRecordSize(rel->schema));
	memcpy(record->data, slotOffset + 1, getRecordSize(rel->schema) - 1);

	RC unpinPageResult;
	if (unpinPageResult = unpinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle) != RC_OK) {
		return unpinPageResult;
	}

	return RC_OK;
}


RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
	RM_ScanHandleMgmtData *shMgmtData = (RM_ScanHandleMgmtData*)malloc(sizeof(RM_ScanHandleMgmtData));

	// Update Scan Handle Management Data
	scan->mgmtData = shMgmtData;

	shMgmtData->rid.page = NO_PAGE;
	shMgmtData->rid.slot = NO_SLOT;
	shMgmtData->numScans = 0;
	shMgmtData->cond = cond;

	scan->rel = rel;

	return RC_OK;
}


RC next(RM_ScanHandle *scan, Record *record) {
	// Setup mgmt data
	RM_ScanHandleMgmtData *shMgmtData = scan->mgmtData;
	RM_TableMgmtData *tableMgmtData = scan->rel->mgmtData;
	Value *result = (Value *)malloc(sizeof(Value));

	// Do we have any tuples in the table to scan?
	if (tableMgmtData->numTuples == 0) {
		return RC_RM_NO_MORE_TUPLES;
	}

	do {
		// Have we already started a scan?
		if (shMgmtData->numScans == 0) {
			shMgmtData->rid.page = 1;
			shMgmtData->rid.slot = 0;

			RC pinPageResult;
			if (pinPageResult = pinPage(&tableMgmtData->bufferPool, &shMgmtData->pageHandle, shMgmtData->rid.page) != RC_OK) {
				return pinPageResult;
			}

			shMgmtData->page = (RM_Page*)shMgmtData->pageHandle.data;
		}
		// Scan until reach last tuple
		else if (shMgmtData->numScans < tableMgmtData->numTuples) {
			
			int pageSize = (int)(PAGE_SIZE - ((&((RM_Page*)0)->data) - ((char*)0)));
			int totalSlots = pageSize / getRecordSize(scan->rel->schema);
			shMgmtData->rid.slot++;

			if (shMgmtData->rid.slot == totalSlots)
			{
				shMgmtData->rid.page++;
				shMgmtData->rid.slot = 0;

				RC pinPageResult;
				if (pinPageResult = pinPage(&tableMgmtData->bufferPool, &shMgmtData->pageHandle, shMgmtData->rid.page) != RC_OK) {
					return pinPageResult;
				}

				shMgmtData->page = (RM_Page*)shMgmtData->pageHandle.data;
			}
			
		}
		// Scan over and nothing found
		else {

			return RC_RM_NO_MORE_TUPLES;
		}

		char *slotOffset = ((char*)&(shMgmtData->page)->data) + shMgmtData->rid.slot * getRecordSize(scan->rel->schema);

		int recordSize = getRecordSize(scan->rel->schema) - 1;
		memcpy(record->data, slotOffset + 1, recordSize);
		record->id.page = shMgmtData->rid.page;
		record->id.slot = shMgmtData->rid.slot;
		shMgmtData->numScans++;

		if (shMgmtData->cond != NULL) {
			evalExpr(record, (scan->rel)->schema, shMgmtData->cond, &result);
		} 

	} while (!result->v.boolV);

	return RC_OK;
}

RC closeScan(RM_ScanHandle *scan) {
	RM_ScanHandleMgmtData *shMgmtData = scan->mgmtData;
	RM_TableMgmtData *tableMgmtData = scan->rel->mgmtData;	

	free(scan->mgmtData);
	scan->mgmtData = NULL;

	return RC_OK;
}

int getRecordSize(Schema *schema) {
	int recordSize = 0;

	int attr;
	for (attr = 0; attr < schema->numAttr; attr++) {
		recordSize = recordSize + schema->typeLength[attr];
	}

	// Add byte for boolean to indicate whether or no "deleted"
	recordSize++;

	return recordSize;
}

Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) {
	Schema *schema = MAKE_SCHEMA();
	schema->numAttr = numAttr;
	schema->keySize = keySize;
	schema->attrNames = MAKE_ATTRNAMES(numAttr);
	schema->dataTypes = MAKE_DATATYPES(numAttr);
	schema->typeLength = MAKE_TYPELENGTH(numAttr);
	schema->keyAttrs = MAKE_KEYATTRS(keySize);

	int i;
	for (i = 0; i < numAttr; i++) {
		schema->attrNames[i] = MAKE_ATTRNAMESLENGTH();
	}

	// Read schema data and save it to struct
	for (i = 0; i < schema->numAttr; i++) {
		strncpy(schema->attrNames[i], attrNames[i], NAME_SIZE);
		schema->dataTypes[i] = dataTypes[i];

		// Set schema type
		switch (schema->dataTypes[i])
		{
		case DT_STRING:
			schema->typeLength[i] = typeLength[i];
			break;
		case DT_INT:
			schema->typeLength[i] = sizeof(int);
			break;
		case DT_FLOAT:
			schema->typeLength[i] = sizeof(float);
			break;
		case DT_BOOL:
			schema->typeLength[i] = sizeof(bool);
			break;
		}
		
		// Only set keyAttr for num of keys passed
		if (i < keySize) {
			schema->keyAttrs[i] = keys[i];
		}
	}

	return schema;
}

RC freeSchema(Schema *schema) {
	int i;
	for (i = 0; i < schema->numAttr; i++) {
		free(schema->attrNames[i]);
	}

	free(schema->attrNames);
	free(schema->dataTypes);
	free(schema->typeLength);
	free(schema->keyAttrs);
	free(schema);

	return RC_OK;
}

// Khush started working on this
// dealing with records and attribute values
RC createRecord(Record **record, Schema *schema) {
	int recordSize;
	Record *newRecord;

	// Assign default record values
	newRecord = MAKE_RECORD();
	recordSize = getRecordSize(schema);
	newRecord->data = MAKE_RECDATA(recordSize);
	memset(newRecord->data, 0, recordSize);
	newRecord->id.page = NO_PAGE;
	newRecord->id.slot = NO_SLOT;
	
	// Save record to pointer
	*record = newRecord;

	return RC_OK;
}

RC freeRecord(Record *record) {
	free(record->data);
	free(record);

	return RC_OK;
}

RC getAttr(Record *record, Schema *schema, int attrNum, Value **value) {
	void *offset = record->data;
	Value *newVal = (Value*)malloc(sizeof(Value));

	int i;
	for (i = 0; i < schema->numAttr; i++) {
		if (i == attrNum) {
			newVal->dt = schema->dataTypes[attrNum];
			switch (schema->dataTypes[attrNum])
			{
			case DT_INT:
				memcpy(&newVal->v.intV, offset, schema->typeLength[i]);
				break;
			case DT_STRING:
				newVal->v.stringV = (char*)malloc(schema->typeLength[i] + 1);
				memcpy(newVal->v.stringV, offset, schema->typeLength[i]);
				newVal->v.stringV[schema->typeLength[i]] = '\0';
				break;
			case DT_FLOAT:
				memcpy(&newVal->v.floatV, offset, schema->typeLength[i]);
				break;
			case DT_BOOL:
				memcpy(&newVal->v.boolV, offset, schema->typeLength[i]);
				break;
			default: 
				return RC_RM_UNKNOWN_DATATYPE;
			}
			
		}
		offset = offset + schema->typeLength[i];
	}
	*value = newVal;

	return RC_OK;
}

RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
	void *offset = record->data;

	int i;
	for (i = 0; i < schema->numAttr; i++) {
		if (i == attrNum) {
			switch (schema->dataTypes[attrNum])
			{
			case DT_INT:
				memcpy(offset, &value->v.intV, schema->typeLength[i]);
				break;
			case DT_STRING:
				memcpy(offset, value->v.stringV, schema->typeLength[i]);
				break;
			case DT_FLOAT:
				memcpy(offset, &value->v.floatV, schema->typeLength[i]);
				break;
			case DT_BOOL:
				memcpy(offset, &value->v.boolV, schema->typeLength[i]);
				break;
			default: 
				return RC_RM_UNKNOWN_DATATYPE;
			
			}
		}
		offset = offset + schema->typeLength[i];
	}

	return RC_OK;
}

void addFreeSlot(RM_TableMgmtData *tableMgmtData, RM_Page *page, int pageNum) {
	// Am I the free slot?
	if (tableMgmtData->nextFreeSlot < 1) {
		page->next = page->prev = pageNum;
		tableMgmtData->nextFreeSlot = pageNum;
	}
	// Add the slot to the list
	else {
		BM_PageHandle pageHandle;
		RM_Page *firstPage;

		RC pinPageResult;
		if (pinPageResult = pinPage(&tableMgmtData->bufferPool, &pageHandle, tableMgmtData->nextFreeSlot) != RC_OK) {
			return;
		}
		firstPage = (RM_Page*)pageHandle.data;

		RC markDirtyResult;
		if (markDirtyResult = markDirty(&tableMgmtData->bufferPool, &pageHandle) != RC_OK) {
			return;
		}
		firstPage->prev = pageNum;

		RC unpinPageResult;
		if (unpinPageResult = unpinPage(&tableMgmtData->bufferPool, &pageHandle) != RC_OK) {
			return;
		}

		page->next = tableMgmtData->nextFreeSlot;
		tableMgmtData->nextFreeSlot = pageNum;
		page->prev = 0;
	}
}
