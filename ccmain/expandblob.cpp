/**************************************************************************

    $Log$
    Revision 1.1  2006/06/16 22:17:03  lvincent
    Initial checkin of Tesseract 1.0

    Revision 1.1.1.1  2004/02/20 19:38:53  slumos
    Import original HP distribution

* Revision 5.1  89/07/27  11:46:53  11:46:53  ray ()
* Added ratings acces methods.
* This version ready for independent development.
*

**************************************************************************/
#include "mfcpch.h"
#include "expandblob.h"
#include "tessclas.h"
#include "const.h"
#include "structures.h"
#include "freelist.h"

/***********************************************************************
free_blob(blob) frees the blob and everything it is connected to,
i.e. outlines, nodes, edgepts, bytevecs, ratings etc
*************************************************************************/
void free_blob(  /*blob to free */
               register TBLOB *blob) {
  if (blob == NULL)
    return;                      /*duff blob */
  free_tree (blob->outlines);    /*do the tree of outlines */
  oldblob(blob);  /*free the actual blob */
}


/***************************************************************************
free_tree(outline) frees the current outline
and then its sub-tree
*****************************************************************************/
void free_tree(  /*outline to draw */
               register TESSLINE *outline) {
  if (outline == NULL)
    return;                      /*duff outline */
  if (outline->next != NULL)
    free_tree (outline->next);
  if (outline->child != NULL)
    free_tree (outline->child);  /*and sub-tree */
  free_outline(outline);  /*free the outline */
}


/*******************************************************************************
free_outline(outline) frees an outline and anything connected to it
*********************************************************************************/
void free_outline(  /*outline to free */
                  register TESSLINE *outline) {
  if (outline->compactloop != NULL)
                                 /*no compact loop */
      memfree (outline->compactloop);

  if (outline->loop != NULL)
    free_loop (outline->loop);

  oldoutline(outline); 
}


/*********************************************************************************
free_loop(startpt) frees all the elements of the closed loop
starting at startpt
***********************************************************************************/
void free_loop(  /*outline to free */
               register EDGEPT *startpt) {
  register EDGEPT *edgept;       /*current point */

  if (startpt == NULL)
    return;
  edgept = startpt;
  do {
    edgept = oldedgept (edgept); /*free it and move on */
  }
  while (edgept != startpt);
}
