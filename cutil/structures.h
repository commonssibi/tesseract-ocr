/* -*-C-*-
 ********************************************************************************
 *
 * File:        structures.h  (Formerly structures.h)
 * Description:  Allocate all the different types of structures.
 * Author:       Mark Seaman, OCR Technology
 * Created:      Wed May 30 10:12:12 1990
 * Modified:     Tue May 21 11:07:47 1991 (Mark Seaman) marks@hpgrlt
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (c) Copyright 1990, Hewlett-Packard Company.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 *********************************************************************************/
#ifndef STRUCTURES_H
#define STRUCTURES_H

/*----------------------------------------------------------------------
              I n c l u d e s
----------------------------------------------------------------------*/
#include "tessclas.h"
#include "oldlist.h"
#include "freelist.h"
#include "danerror.h"

#define NUM_DATA_TYPES 20

extern int max_data_types;
extern void_void memory_print_functions[NUM_DATA_TYPES];

/*----------------------------------------------------------------------
              M a c r o s
----------------------------------------------------------------------*/
/**********************************************************************
 * makestructure
 *
 * Allocate a chunk of memory for a particular data type.  This macro
 * defines an allocation, deallocation, and status printing function
 * for each new data type.
 **********************************************************************/

#define makestructure(new,old,print,type,nextfree,blocksize,typestring,usecount)                \
																									\
static type                *nextfree;                                        \
static int                 usecount=0;                                       \
																									\
																									\
void print()                                                                 \
{                                                                            \
	cprintf("%6d %s elements in use\n", usecount, typestring);                 \
}                                                                            \
																									\
																									\
type *new()                                                                  \
{                                                                            \
	static   int            first_time = 1;                                   \
	register int            index;                                            \
	register type           *element;                                         \
	type                    *returnelement;				/*return next ptr*/        \
	char                    *newblock;                                        \
	extern int              structblockcount;                                 \
																									\
	if (first_time)                                                           \
	memory_print_functions [max_data_types++] = print;                      \
	first_time = FALSE;                                                       \
																									\
	returnelement=nextfree;                                                   \
	if (nextfree==NULL)                                                       \
	{                                                                         \
		if ((sizeof(type) & 7)==0 && sizeof(type)>=16)                         \
		{                                                                      \
			newblock=(char *)memalloc_p(sizeof(type)*blocksize+4);              \
			if (newblock!=NULL)                                                 \
			{                                                                   \
				newblock=(char *)(((int)newblock +4) & (-8));                    \
			}                                                                   \
			element=(type *)newblock;                                           \
		}                                                                      \
		else                                                                   \
		{                                                                      \
			element=(type *)memalloc_p(sizeof(type)*blocksize);                 \
		}                                                                      \
		structblockcount++;                                                    \
		if (!element) DoError (0, "Could not allocate block of cells");        \
		returnelement=element;							/*going to return first*/  \
		for (index=0;index<blocksize-1;index++)                                \
		{  ((type**)element)[0]=element+1;				/*make links*/             \
			element++;                                                          \
		}                                                                      \
		((type**)element)[0]=NULL;						/*with end*/         \
	}                                                                         \
	nextfree=((type**)returnelement)[0];                                      \
	usecount++;                                                               \
	return (returnelement);                                                   \
}                                                                            \
																									\
																									\
																									\
void old(type* deadelement)                                                       \
{                                                                            \
	if (!deadelement) DoError (0, "Deallocated a NULL pointer");             \
	((type**)deadelement)[0]=nextfree;                                        \
	nextfree=deadelement;                                                     \
	usecount--;                                                               \
}                                                                            \


/**********************************************************************
 * newstructure
 *
 * Allocate a chunk of memory for a particular data type.
 **********************************************************************/

#define newstructure(name,type,nextfree,blocksize,errorstring,usecount)\
static type                *nextfree;					/*head of freelist*/\
static int                 usecount=0;					/*no of used objects*/\
\
type *name()											/*returns a new type*/\
{\
	register int            index;						/*index to block*/\
	register type           *element;					/*current element*/\
	type                    *returnelement;				/*return value*/\
	char                    *newblock;					/*new block of mem*/\
	extern int              structblockcount;			/*no of memallocs done*/\
\
\
	returnelement=nextfree;\
	if (nextfree==NULL)\
	{\
		if ((sizeof(type) & 7)==0 && sizeof(type)>=16)\
		{\
			newblock=(char *)memalloc_p(sizeof(type)*blocksize+4);\
			if (newblock!=NULL)\
			{\
				newblock=(char *)(((int)newblock +4) & (-8));\
			}\
			element=(type *)newblock;\
		}\
		else\
		{\
			element=(type *)memalloc_p(sizeof(type)*blocksize);\
																																																								/*get more memory*/\
		}\
		structblockcount++;\
		if (element==NULL)\
		{\
			cprintf("Error:MEMORY_OUT:%s\n",errorstring);\
			abort();\
		}\
		returnelement=element;							/*going to return first*/\
		for (index=0;index<blocksize-1;index++)\
		{  ((type**)element)[0]=element+1;				/*make links*/\
			element++;\
		}\
		((type**)element)[0]=NULL;						/*with end*/\
	}\
	nextfree=((type**)returnelement)[0];\
	usecount++;\
	return returnelement;\
}

/**********************************************************************
 * oldstructure
 *
 * Returns a structure to the freelist
 **********************************************************************/

#define oldstructure(name,type,nextfree,stringtype,usecount)\
\
type *name(type* deadelement)\
{\
	type                    *returnelement;				/*return next ptr*/\
\
	if (deadelement==NULL)\
	{\
		cprintf("No of %ss in use=%d\n",stringtype,usecount);\
														/**/\
		return (type *) 0x80000000;\
	}\
	returnelement=deadelement->next;					/*return link*/\
	((type**)deadelement)[0]=nextfree;					/*next free blob*/\
	nextfree=deadelement;\
	usecount--;\
	return returnelement;\
}

/*----------------------------------------------------------------------
              F u n c t i o n s
----------------------------------------------------------------------*/
extern TBLOB *newblob(); 
extern TBLOB *oldblob(TBLOB *); 

extern TESSLINE *newoutline(); 
extern void oldoutline(TESSLINE *); 

extern EDGEPT *newedgept(); 
extern EDGEPT *oldedgept(EDGEPT *); 

extern TWERD *newword(); 
extern void oldword(TWERD *); 

extern LIST new_cell(); 
extern void free_cell(LIST); 
#endif
