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

/* This file contains the support and numerical functions for
   the backprop (BP) simulations.

   By using conditional compilation
   this file can be customized to run very efficiently on
   computers with vector processors by replacing key routines
   (e.g. BPvdot) with library calls to the corresponding vector
   routines.
*/

/* NOTE: IF YOU ADD SOMETHING TO VANILLA.C THEN YOU SHOULD UPDATE
         ALL OF THE OTHER MACHINE SPECIFIC FILES!!!`
*/

/* default is C code */
//#define VANILLA

/* BLAS Fortran vector libraries
#ifdef BLAS
# include "Blas.c"
# undef VANILLA
#endif*/

/* Mercury i860 vector library
#ifdef SAL
# include "Sal.c"
# undef VANILLA
#endif */

/* Vanilla C */
//#ifdef VANILLA
# include "vanilla.h"
//#endif
