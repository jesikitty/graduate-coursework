#ifndef TABLES_H
#define TABLES_H

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

#include "dt.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

// Data Types, Records, and Schemas
typedef enum DataType {
  DT_INT = 0,
  DT_STRING = 1,
  DT_FLOAT = 2,
  DT_BOOL = 3
} DataType;

typedef struct Value {
  DataType dt;
  union v {
    int intV;
    char *stringV;
    float floatV;
    bool boolV;
  } v;
} Value;

typedef struct RID {
  PageNumber page;
  int slot;
} RID;

typedef struct Record
{
  RID id;
  char *data;
} Record;

// information of a table schema: its attributes, datatypes,
typedef struct Schema
{
  int numAttr;
  char **attrNames;
  DataType *dataTypes;
  int *typeLength;
  int *keyAttrs;
  int keySize;
} Schema;

// TableData: Management Structure for a Record Manager to handle one relation
typedef struct RM_TableData
{
  char *name;
  Schema *schema;
  void *mgmtData;
} RM_TableData;



/********************************************
	Custom Structs
*********************************************/

// This is a header data structure for each page that indicates free space on the page, if it has a page before it with records for the table, and if it has a page after it for continued record data

#define PAGE_FULL -1
#define NO_SLOT -1


typedef struct RM_TableMgmtData 
{
  int numTuples;
  int nextFreeSlot;
  BM_BufferPool bufferPool;
  BM_PageHandle pageHandle;
} RM_TableMgmtData;

typedef struct RM_Page
{
	int next;
	int prev;
	char data;
} RM_Page;


// Macros for allocating space
#define MAKE_SCHEMA()                     \
    ((Schema *) malloc (sizeof(Schema)))

#define MAKE_ATTRNAMES(numAttr)                  \
    ((char **) malloc (sizeof(char*) * numAttr))

#define MAKE_ATTRNAMESLENGTH()          \
    ((char *) malloc (NAME_SIZE))

#define MAKE_DATATYPES(numAttr)                     \
    ((DataType *) malloc (sizeof(DataType) * numAttr))

#define MAKE_TYPELENGTH(numAttr)                  \
    ((int *) malloc (sizeof(int) * numAttr))

#define MAKE_KEYATTRS(keySize)                  \
    ((int *) malloc (sizeof(int) * keySize))

#define MAKE_RECORD()                         \
  ((Record *) malloc (sizeof(Record)))

#define MAKE_RECDATA(recordSize)         \
  ((char *) malloc (sizeof(recordSize)))


#define MAKE_STRING_VALUE(result, value)				\
  do {									\
    (result) = (Value *) malloc(sizeof(Value));				\
    (result)->dt = DT_STRING;						\
    (result)->v.stringV = (char *) malloc(strlen(value) + 1);		\
    strcpy((result)->v.stringV, value);					\
  } while(0)


#define MAKE_VALUE(result, datatype, value)				\
  do {									\
    (result) = (Value *) malloc(sizeof(Value));				\
    (result)->dt = datatype;						\
    switch(datatype)							\
      {									\
      case DT_INT:							\
	(result)->v.intV = value;					\
	break;								\
      case DT_FLOAT:							\
	(result)->v.floatV = value;					\
	break;								\
      case DT_BOOL:							\
	(result)->v.boolV = value;					\
	break;								\
      }									\
  } while(0)


// debug and read methods
extern Value *stringToValue (char *value);
extern char *serializeTableInfo(RM_TableData *rel);
extern char *serializeTableContent(RM_TableData *rel);
extern char *serializeSchema(Schema *schema);
extern char *serializeRecord(Record *record, Schema *schema);
extern char *serializeAttr(Record *record, Schema *schema, int attrNum);
extern char *serializeValue(Value *val);

#endif
