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

/* This defines MACHTYPE/OS specific headers, constants, math stuff, etc. */

#ifndef AM_OS_HEADER

#define AM_OS_HEADER

#define AM_VERSION 6.0

#ifndef AM_OS_INCLUDES
/* ANSI C Headers */
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <errno.h>
#include <math.h>
#endif

#ifndef AM_EXP
# define AM_EXP(x) (exp((double)(x)))
#endif

#ifndef AM_FABS
# define AM_FABS(x) (fabs((double)(x)))
#endif

#ifndef AM_RANDOM_RANGE
# ifdef RAND_MAX
#  define AM_RANDOM_RANGE RAND_MAX
# else
                                 /* a guess, (2^31)-1 */
#  define AM_RANDOM_RANGE 2147483647.0
# endif
#endif

#ifndef AM_RANDOM
# define AM_RANDOM() ( ((float)rand()) / (AM_RANDOM_RANGE + 1.0) )
#endif

#ifndef AM_SEED_RANDOM
# define AM_SEED_RANDOM(x) ((void)srand((unsigned int)(x)))
#endif

#ifndef bzero
# define bzero(s,n) memset(s,0,n)
#endif
#endif                           /* end AM_OS_HEADER */
