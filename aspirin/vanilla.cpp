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

/************************ vector stuff ****************************/
#include "header.h"

/* BPvdot: Vector dot product */
float BPvdot(float *v1, float *v2, int n) { 
  float sum = 0.0;

  while (n--)
    sum += *v1++ * *v2++;

  return (sum);
}                                /* end BPvdot */


/* BPlvdot: Looping vector dot product (vector matrix mult) */
void BPlvdot(float *weights,
             float *from,
             int overlap,
             float *to,
             int n,
             int lc) {
  while (lc--) {
    *to++ = BPvdot (weights, from, n);
    weights += n;
    from += n - overlap;
  }
}                                /* end BPlvdot */


/************************ node stuff ************************/

#define TABLE_SIZE 1024
                                 /* table lookup sigmoid */
static float sigmoid_array[TABLE_SIZE];
static float *sigmoid_ptr;       /* pointer to center of sigmoid */

void BPinit_sigmoid_table() { 
  int counter;
  /* init table lookup for sigmoid [-1/2,1/2] */
  for (counter = 0; counter < TABLE_SIZE; counter++)
    sigmoid_array[counter]       /* domain (8,8) w/511.5 at center (for 1024 table) */
      = (1.0 /
      (1.0 +
      AM_EXP (((TABLE_SIZE - 1.0) -
      2.0 * counter) / (TABLE_SIZE / 8.0)))) - 0.5;
  sigmoid_ptr = sigmoid_array + (TABLE_SIZE / 2) - 1;
}                                /* end BPinit_sigmoid_table */


/* Sigmoid macros */
#define sigmoid(x) ( (AM_FABS((x)) >= 8.0)?(((x)>0.0)?(0.499999):(-0.499999)):(*(sigmoid_ptr + (int)((TABLE_SIZE/16.0)*(x) + 0.5))) )
#define sigmoid1(x) (sigmoid(x) + 0.5)

/* addbias: add the bias to the node value */
static void addbias(float *nodes, float *biases, int n) { 
  while (n--)
    *nodes++ += *biases++;
}                                /* end addbias */


/* BPsig1: sigmoid1 transfer function on layer */
void BPsig1(float *nodes, float *biases, int n) { 
  addbias(nodes, biases, n); 
  while (n--) {
    float val;

    val = *nodes;
    *nodes++ = sigmoid1 (val);
  }

}                                /* end BPsig1 */
