/*

 ****************   NO WARRANTY  *****************

Since the Aspirin/MIGRAINES system is licensed free of charge,
Russell Leighton and the MITRE Corporation provide absolutley
no warranty. Should the Aspirin/MIGRAINES system prove defective,
you must assume the cost of all necessary servicing, repair or correction.
In no way will Russell Leighton or the MITRE Corporation be liable to you for
damages, including any lost profits, lost monies, or other
special, incidental or consequential damages arising out of
the use or inability to use the Aspirin/MIGRAINES system.

  *****************   COPYRIGHT  *******************

This software is the copyright of Russell Leighton and the MITRE Corporation.
It may be freely used and modified for research and development
purposes. We require a brief acknowledgement in any research
paper or other publication where this software has made a significant
contribution. If you wish to use it for commercial gain you must contact
The MITRE Corporation for conditions of use. Russell Leighton and
the MITRE Corporation provide absolutely NO WARRANTY for this software.

   August, 1992
   Russell Leighton
   The MITRE Corporation
   7525 Colshire Dr.
   McLean, Va. 22102-3481

*/

/*********   THIS INFO IS THE CURRENT HEADER INFO FOR BACKPROP SIMULATIONS **********/

#ifndef BACKPROP_HEADER

#include "header.h"

#define E_STRING_SIZE 128        // size of error string

#define FERROR -1                /* unable to open/close file */
#define DFERROR -2               /* error in dump file data */

/* table of contents structure (used when reading dump files) */
typedef struct table_of_contents
{
  int size;                      /* number of entries in address table */
  off_t *address_table;
  /* array of byte counts between black
                                     boxes in dump file. */
} TOC_STRUCT, *TOC_PTR;

/* Buffer with global info about network */
typedef struct network_buffer
{
  int n_connections;             /* total number of connections in whole network */
  char *(*error_string) (void);
} NB, *NB_PTR;

/* Communication Buffer w/layer info */
typedef struct layer_buffer
{
  NB network_info;               // info and functions global to the declared network.
} LB, *LB_PTR;
#endif
