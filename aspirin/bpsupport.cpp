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
#include "aspirin_bp.h"
#include "host.h"
#include "callcpp.h"
#ifdef __UNIX__
#include <fcntl.h>
#include <unistd.h>
#else
#include <io.h>
#endif

/*** SUPPORT FUNCTIONS ***/

void BPfrandom_init(long seed) { 
  AM_SEED_RANDOM(seed);  /* init random number generator */
}


float BPfrandom(float val) { 
  val *= AM_RANDOM ();
  return (val);
}                                /* end BPfrandom */


void BPread_string(int fd, char *string) { 
  do {
    read (fd, string, 1);
  }
  while (*string++ != '\0');
}                                /* end BPread_string */


int BPread_thresholds(int fd,
                      const char *name,
                      const char *key,
                      int size,
                      int target_size,
                      float *data,
                      int *flag) {
  extern char *error_string;
  int i;

  if (strcmp (name, key) == 0) {
    if (size != target_size) {
      sprintf (error_string, "\nBad layer size for %s\n", key);
      return (DFERROR);
    }                            /* end if */
    read (fd, data, size * sizeof (float));
    if (__NATIVE__ == INTEL)
      for (i = 0; i < size; i++)
        reverse32 (&data[i]);
    *flag = 0;                   /* set flag to 0 means found the thresholds! */
  }                              /* end if */
  return (0);
}                                /* end BPread_thresholds */


int BPread_weights(int fd,
                   const char *name1,
                   const char *key1,
                   const char *name2,
                   const char *key2,
                   int size,
                   int target_size,
                   float *data,
                   int *flag) {
  extern char *error_string;
  int i;

  if (strcmp (name1, key1) == 0 && strcmp (name2, key2) == 0) {
    if (size != target_size) {
      sprintf (error_string, "\nBad layer size for %s\n", key1);
      return (DFERROR);
    }
    read (fd, data, size * sizeof (float));
    if (__NATIVE__ == INTEL)
      for (i = 0; i < size; i++)
        reverse32 (&data[i]);
    *flag = 0;                   // set flag to 0 means found the thresholds!
  }
  return (0);
}                                /* end BPread_weights */
