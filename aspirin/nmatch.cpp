/**** Back-propagation simulation for nmatch.aspirin ****/
#include <fcntl.h>

#include "aspirin_bp.h"
#include "host.h"
#include "callcpp.h"
#include "emalloc.h"
#include "vanilla.h"
#include "bpsupport.h"
#include "freelist.h"

#ifdef __UNIX__
#include <fcntl.h>
#include <unistd.h>
#else
#include <io.h>
#endif

/*-------------------------------------------------------------------------
          Global Variables
--------------------------------------------------------------------------*/
static float init_range = 0.1;   // feedforward weights
static long init_seed = 123;
char error_string[E_STRING_SIZE];// describes current error
static LB comms_buffer;

/*-------------------------------------------------------------------------
          Network Data
--------------------------------------------------------------------------*/
#ifndef PADDING
# define PADDING 0
#endif
static float network_data[1027210 + PADDING];

                                 // # times bkwd
static unsigned int b0bcounter = 0;

// nmatch:Hidden_Layer
                                 // values
#define b0_l1_v (network_data + 0)
                                 // thresholds
#define b0_l1_t (network_data + 628)

// nmatch:Output_Layer
                                 // values
#define b0_l0_v (network_data + 440)
                                 // thresholds
#define b0_l0_t (network_data + 1288)

                                 // input vector
static float *b0_input_vector = (float *) NULL;

                                 // input vector
#define b0_input_vector0 b0_input_vector

// nmatch:Hidden_Layer input connections
                                 // weights from $INPUTS
#define b0_Hidden_Layer2 (network_data + 1570)

// nmatch:Output_Layer input connections
                                 // weights from nmatch:Hidden_Layer
#define b0_Output_Layer1 (network_data + 965170)

/*--------------------------------------------------------------------------
          Function Code
----------------------------------------------------------------------------*/
//** nmatch_propagate_forward *************************************************
void nmatch_propagate_forward() { 
  // Connection to nmatch:Hidden_Layer from $INPUTS
  BPlvdot (b0_Hidden_Layer2, b0_input_vector + 0, 1460, b0_l1_v, 1460, 220);
  // calculate the transfer function for nmatch:Hidden_Layer
  BPsig1 (b0_l1_v, b0_l1_t, 220);

  // Connection to nmatch:Output_Layer from nmatch:Hidden_Layer
  BPlvdot (b0_Output_Layer1, b0_l1_v + 0, 220, b0_l0_v, 220, 94);
  // calculate the transfer function for nmatch:Output_Layer
  BPsig1 (b0_l0_v, b0_l0_t, 94);

  // b0fcounter++;        // increment fwd counter
}                                /* nmatch_ propagate_forward */


//------------------INPUT CONTROL-----------------------------------------------
/** nmatch_set_input ************************************************************/
void nmatch_set_input(float *vector) { 
  b0_input_vector = vector;
}                                /* nmatch_set_input */


//------------------OUTPUT CONTROL----------------------------------------
/** nmatch_get_output ************************************************************/
float *nmatch_get_output() { 
  return (b0_l0_v);
}                                /* nmatch_get_output */


//------------------NETWORK FUNCTIONS-------------------------------------------
/** nmatch_error_string *******************************************************/
char *nmatch_error_string() { 
  return (error_string);
}                                /* nmatch_error_string */


/** read_header **************************************************************/
TOC_PTR read_header(int fd) { 
  int header_size = 272;
  int number;
  char extra_bytes[256];
  int counter;
  int n_bbs;                     // number of black boxes
  TOC_PTR table_of_contents;
  off_t *address_table;          // used to index bb in a file

                                 // read header
  read (fd, &number, sizeof (int));
  if (__NATIVE__ == INTEL)
    reverse32(&number); 
  if (number != 100)
    sprintf (error_string,
      "\nWarning: This dump file is from another compiler.\n");

                                 // major version
  read (fd, &number, sizeof (int));
  if (__NATIVE__ == INTEL)
    reverse32(&number); 
  if (number != 6)
    sprintf (error_string,
      "\nWarning: Dump file created with another version (v.%d) of compiler.\n",
      number);

                                 // minor version
  read (fd, &number, sizeof (int));
  if (__NATIVE__ == INTEL)
    reverse32(&number); 
  if (number != 0)
    sprintf (error_string,
      "\nWarning: Dump file created with another version of compiler.\n");

  // extra bytes for future use
  read (fd, extra_bytes, 256);
  read (fd, &n_bbs, sizeof (int));
  if (__NATIVE__ == INTEL)
    reverse32(&n_bbs); 

  // create a table of black boxes => location in file
  address_table = (off_t *) memalloc (n_bbs * sizeof (int));
                                 // add the address_table size
  header_size += n_bbs * sizeof (int);
  for (counter = 0; counter < n_bbs; counter++) {
    address_table[counter] = header_size;
    read (fd, &number, sizeof (int));
    if (__NATIVE__ == INTEL)
      reverse32(&number); 
    header_size += number;
  }
  table_of_contents = (TOC_PTR) memalloc (sizeof (TOC_STRUCT));
  table_of_contents->size = n_bbs;
  table_of_contents->address_table = address_table;
  return (table_of_contents);
}                                /* read_header */


/** load_black_box ************************************************************/
int load_black_box(int fd, const char *name, const char *key, TOC_PTR toc) { 
  int table_size = toc->size;
  off_t *address_table = toc->address_table;
  off_t *end_of_table;
  int n_layers, n_connections, size, counter, error_code, not_found;
                                 // for reading names
  char name_string[100], name_string2[100];
  int nread;

                                 // record end of address table
  end_of_table = address_table + table_size;
  do {                           // find key name in the file
    if (address_table == end_of_table) {
      sprintf (error_string, "\nUnable to find %s as %s.\n", name, key);
      return (DFERROR);
    }
    lseek (fd, *address_table++, 0);
    BPread_string(fd, name_string); 
  }
  while (strcmp (key, name_string) != 0);

  if (strcmp ("nmatch", name) == 0) {
                                 // iteration counter
    nread = read (fd, &b0bcounter, sizeof (int));
    if (__NATIVE__ == INTEL)
      reverse32(&b0bcounter); 
                                 // n layers of thresholds
    nread = read (fd, &n_layers, sizeof (int));
    if (__NATIVE__ == INTEL)
      reverse32(&n_layers); 
    if (n_layers != 2) {
      sprintf (error_string, "\nError in reading nmatch\n");
      return (DFERROR);
    }
    // load all thresholds and feedback weights
    for (counter = 0; counter < n_layers; counter++) {
                                 // read the layer name and data size
      BPread_string(fd, name_string); 
      nread = read (fd, &size, sizeof (int));
      if (__NATIVE__ == INTEL)
        reverse32(&size); 
      not_found = 1;             // reset flag (0 if read the thresholds)
      if ((error_code =
        BPread_thresholds (fd, name_string, "Hidden_Layer", size, 220,
        b0_l1_t, &not_found)) != 0)
        return (error_code);
      if ((error_code =
        BPread_thresholds (fd, name_string, "Output_Layer", size, 94,
        b0_l0_t, &not_found)) != 0)
        return (error_code);
      if (not_found) {
        sprintf (error_string, "\nUnknown layer name in file: %s\n",
          name_string);
        return (DFERROR);
      }
    }
    read (fd, &n_connections, sizeof (int));
    if (__NATIVE__ == INTEL)
      reverse32(&n_connections); 
    for (counter = 0; counter < n_connections; counter++) {
                                 // to layer
      BPread_string(fd, name_string); 
                                 // from layer
      BPread_string(fd, name_string2); 
      read (fd, &size, sizeof (int));
      if (__NATIVE__ == INTEL)
        reverse32(&size); 
      not_found = 1;             // reset flag (0 if read the weights)
      if ((error_code =
        BPread_weights (fd, name_string, "Hidden_Layer", name_string2,
        "Hidden_Layer2", size, 321200,
        b0_Hidden_Layer2, &not_found)) != 0)
        return (error_code);
      if ((error_code =
        BPread_weights (fd, name_string, "Output_Layer", name_string2,
        "Output_Layer1", size, 20680, b0_Output_Layer1,
        &not_found)) != 0)
        return (error_code);
      if (not_found) {
        sprintf (error_string, "\nUnknown connection array in file\n");
        return (DFERROR);
      }
    }
    return (0);
  }
  return (0);
}                                /* load_black_box */


/** nmatch_load_network *********************************************************/
int nmatch_load_network(char *filename) { 
  int fd, error_code;
  TOC_PTR toc;                   // table of contents

  #ifdef __UNIX__
  fd = open (filename, 0);
  #else
  fd = open (filename, _O_BINARY);
  #endif
  if (fd == -1) {
    sprintf (error_string, "\nUnable to open %s.\n", filename);
    return (FERROR);
  }
  toc = read_header (fd);        // read header
  if (toc == (TOC_PTR) NULL)
    return (DFERROR);
  if ((error_code = load_black_box (fd, "nmatch", "nmatch", toc)) != 0)
    return (error_code);
  close(fd); 
  return (0);
}                                /* nmatch_load_network */


/** nmatch_network_forward ******************************************************/
void nmatch_network_forward() { 
  nmatch_propagate_forward(); 
}                                /* nmatch_network_forward */


/** nmatch_init_netword *********************************************************/
int nmatch_init_network() { 
  int error_code = 0;
  int counter1;

  BPfrandom_init(init_seed);  // init random number generator
                                 // clear all data
  bzero ((char *) network_data, 1027210 * sizeof (float));
  BPinit_sigmoid_table();  // init table lookup for sigmoid [-1,1]
  error_string[0] = '\0';        // empty string

  comms_buffer.network_info.n_connections = 342194;
  comms_buffer.network_info.error_string = nmatch_error_string;

  for (counter1 = 0; counter1 < 94; counter1++)
    b0_l0_t[counter1] = BPfrandom (2.0 * init_range) - init_range;
  for (counter1 = 0; counter1 < 20680; counter1++)
    b0_Output_Layer1[counter1] = BPfrandom (2.0 * init_range) - init_range;
  for (counter1 = 0; counter1 < 220; counter1++)
    b0_l1_t[counter1] = BPfrandom (2.0 * init_range) - init_range;
  for (counter1 = 0; counter1 < 321200; counter1++)
    b0_Hidden_Layer2[counter1] = BPfrandom (2.0 * init_range) - init_range;
  return (error_code);
}                                /* nmatch_init_network */
