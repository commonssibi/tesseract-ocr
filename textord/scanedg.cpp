/**********************************************************************
 * File:        scanedg.c  (Formerly scanedge.c)
 * Description: Raster scanning crack based edge extractor.
 * Author:					Ray Smith
 * Created:					Fri Mar 22 16:11:50 GMT 1991
 *
 * (C) Copyright 1991, Hewlett-Packard Ltd.
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
 **********************************************************************/

#include "mfcpch.h"
#include          "edgloop.h"
//#include                                      "dirtab.h"
#include          "scanedg.h"

#define WHITE_PIX     1          /*thresholded colours */
#define BLACK_PIX     0
                                 /*W->B->W */
#define FLIP_COLOUR(pix)  (1-(pix))

#define EWSIZE        4          /*edge operator size */
#define UPPER       0            /*line above above */
#define ABOVE       1            /*line above h_edge */
#define BELOW       2            /*line below h_edge */
#define LOWER       3            /*line below BELOW */

#define XMARGIN       2          //margin needed
#define YMARGIN       3          //by edge detector

                                 /*local freelist */
static CRACKEDGE *free_cracks = NULL;

/**********************************************************************
 * block_edges
 *
 * Extract edges from a PDBLK.
 **********************************************************************/

DLLSYM void block_edges(                      //get edges in a block
                        IMAGE *image,         //image to scan
                        IMAGE *t_image,       //threshold image
                        PDBLK *block,         //block in image
                        ICOORD page_tr,       //corner of page
                        INT16 grey_threshold  //difference threshold
                       ) {
  UINT8 margin;                  //margin colour
  UINT8 uppercolour;             //1st pix on prev line
  INT16 x;                       //line coords
  INT16 y;                       //current line
  INT16 xext;                    //line width
  INT16 lindex;                  //index to imlines
  ICOORD bleft;                  //bounding box
  ICOORD tright;
  ICOORD block_bleft;            //bounding box
  ICOORD block_tright;
  int xindex;                    //index to pixel
  UINT8 *ptrs[EWSIZE];           //rotating buffer
  BLOCK_LINE_IT line_it = block; //line iterator
  IMAGELINE bwline;              //thresholded line
  IMAGELINE imlines[EWSIZE];     //line in image
                                 //lines in progress
  CRACKEDGE *ptrline[MAXIMAGEWIDTH];

                                 //block box
  block->bounding_box (bleft, tright);
  block_bleft = bleft;
  block_tright = tright;
  if (tright.x () - bleft.x () <= EWSIZE
    || tright.y () - bleft.y () <= EWSIZE)
    return;                      //pointlessly small

  if (bleft.x () - XMARGIN > 0)
    bleft.set_x (bleft.x () - XMARGIN);
  else
    bleft.set_x (0);
  if (bleft.y () - YMARGIN > 0)
    bleft.set_y (bleft.y () - YMARGIN);
  else
    bleft.set_y (0);
  if (tright.x () + XMARGIN < page_tr.x ())
    tright.set_x (tright.x () + XMARGIN);
  else
    tright.set_x (page_tr.x ());
  if (tright.y () + YMARGIN < page_tr.y ())
    tright.set_y (tright.y () + YMARGIN);
  else
    tright.set_y (page_tr.y ());
  for (x = tright.x () - bleft.x () - 1; x >= 0; x--)
    ptrline[x] = NULL;           //no lines in progress

  for (lindex = 0; lindex < EWSIZE - 1; lindex++) {
    imlines[lindex].init ();
                                 //get image line
    image->fast_get_line (bleft.x (), tright.y () - 1 - lindex, tright.x () - bleft.x (), &imlines[lindex]);
                                 //ptrs to lines
    ptrs[lindex] = imlines[lindex].pixels;
  }
  imlines[lindex].init ();
  bwline.init ();

  if (tright.y () - EWSIZE / 2 >= block_tright.y ())
    x = block_bleft.x ();
  else
                                 //line above first
    x = line_it.get_line (tright.y () - EWSIZE / 2, xext);
  t_image->fast_get_line (bleft.x (), tright.y () - EWSIZE / 2,
    x - bleft.x () + 1 >= 3 ? x - bleft.x () + 1 : 3,
    &bwline);
  uppercolour = bwline.pixels[2];
  margin = bwline.pixels[x - bleft.x ()];

  for (y = tright.y () - EWSIZE / 2 - 1;
  y >= bleft.y () + (EWSIZE - 1) / 2; y--) {
    if (y >= block_bleft.y () && y < block_tright.y ()) {
      t_image->get_line (bleft.x (), y, tright.x () - bleft.x (), &bwline,
        0);
      make_margins (block, &line_it, bwline.pixels, margin, bleft.x (),
        tright.x (), y);
    }
    else {
      x = tright.x () - bleft.x ();
      for (xindex = 0; xindex < x; xindex++)
        bwline.pixels[xindex] = margin;
    }
    //              tprintf("At line %d, xstart=%d, xext=%d\n",
    //                      y,x,xext);
                                 //get image line
    image->fast_get_line (bleft.x (), y - (EWSIZE - 1) / 2, tright.x () - bleft.x (), &imlines[lindex]);
                                 //bottom line
    ptrs[EWSIZE - 1] = imlines[lindex].pixels;
    lindex++;
    if (lindex >= EWSIZE)
      lindex = 0;                //rotating buffer

    if (ptrline[2] != NULL)
                                 //fix the margin
      uppercolour = FLIP_COLOUR (uppercolour);
                                 //scan the line
    line_edges (bleft.x (), y, tright.x () - bleft.x (),
      margin, ptrs, bwline.pixels, ptrline, grey_threshold);

                                 //1st pix on prev line
    uppercolour = bwline.pixels[2];

    for (xindex = 0; xindex < EWSIZE - 1; xindex++)
                                 //rotate ptrs
      ptrs[xindex] = ptrs[xindex + 1];
  }

  free_crackedges(free_cracks);  //really free them
  free_cracks = NULL;
}


/**********************************************************************
 * make_margins
 *
 * Get an image line and set to margin non-text pixels.
 **********************************************************************/

void make_margins(                         //get a line
                  PDBLK *block,            //block in image
                  BLOCK_LINE_IT *line_it,  //for old style
                  UINT8 *pixels,           //pixels to strip
                  UINT8 margin,            //white-out pixel
                  INT16 left,              //block edges
                  INT16 right,
                  INT16 y                  //line coord
                 ) {
  PB_LINE_IT *lines;
  ICOORDELT_LIST *segments;      //bits of a line
  ICOORDELT_IT seg_it;
  INT32 start;                   //of segment
  INT16 xext;                    //of segment
  int xindex;                    //index to pixel

  if (block->poly_block () != NULL) {
    lines = new PB_LINE_IT (block->poly_block ());
    segments = lines->get_line (y);
    if (!segments->empty ()) {
      seg_it.set_to_list (segments);
      seg_it.mark_cycle_pt ();
      start = seg_it.data ()->x ();
      xext = seg_it.data ()->y ();
      for (xindex = left; xindex < right; xindex++) {
        if (xindex >= start && !seg_it.cycled_list ()) {
          xindex = start + xext - 1;
          seg_it.forward ();
          start = seg_it.data ()->x ();
          xext = seg_it.data ()->y ();
        }
        else
          pixels[xindex - left] = margin;
      }
    }
    else {
      for (xindex = left; xindex < right; xindex++)
        pixels[xindex - left] = margin;
    }
    delete segments;
    delete lines;
  }
  else {
    start = line_it->get_line (y, xext);
    for (xindex = left; xindex < start; xindex++)
      pixels[xindex - left] = margin;
    for (xindex = start + xext; xindex < right; xindex++)
      pixels[xindex - left] = margin;
  }
}


/**********************************************************************
 * whiteout_block
 *
 * Extract edges from a PDBLK.
 **********************************************************************/

void whiteout_block(                 //clean it
                    IMAGE *t_image,  //threshold image
                    PDBLK *block     //block in image
                   ) {
  INT16 x;                       //line coords
  INT16 y;                       //current line
  INT16 xext;                    //line width
  int xindex;                    //index to pixel
  UINT8 *dest;                   //destination pixel
  BOX block_box;                 //bounding box
  BLOCK_LINE_IT line_it = block; //line iterator
  IMAGELINE bwline;              //thresholded line

  block_box = block->bounding_box ();
  for (y = block_box.bottom (); y < block_box.top (); y++) {
                                 //find line limits
    x = line_it.get_line (y, xext);
    t_image->get_line (x, y, xext, &bwline, 0);
    dest = bwline.pixels;        //destination pixel
    for (xindex = 0; xindex < xext; xindex++)
      *dest++ = 1;
    t_image->put_line (x, y, xext, &bwline, 0);
  }
}


/**********************************************************************
 * line_edges
 *
 * Scan a line for edges and update the edges in progress.
 * When edges close into loops, send them for approximation.
 **********************************************************************/

void
line_edges (                     //scan for edges
INT16 x,                         //coord of line start
INT16 y,                         //coord of line
INT16 xext,                      //width of line
UINT8 uppercolour,               //start of prev line
UINT8 * ptrs[],                  //image lines
UINT8 * bwpos,                   //thresholded line
CRACKEDGE ** prevline,           //edges in progress
INT16 t                          //difference threshold
) {
  int xpos;                      //current x coord
  int xmax;                      //max x coord
  int colour;                    //of current pixel
  int prevcolour;                //of previous pixel
  CRACKEDGE *current;            //current h edge
  CRACKEDGE *newcurrent;         //new h edge

  xmax = x + xext - 2;           //max allowable coord
  bwpos += 2;
  prevcolour = uppercolour;      //forced plain margin
  prevline += 2;                 //skip margin
  current = NULL;                //nothing yet

                                 //do each pixel
  for (xpos = x + 2; xpos < xmax; xpos++, prevline++) {
    colour = *bwpos++;           //current pixel
    if (*prevline != NULL) {
                                 //changed above
                                 //change colour
      uppercolour = FLIP_COLOUR (uppercolour);
      if (colour == prevcolour) {
        if (colour == uppercolour) {
                                 //finish a line
          join_edges(current, *prevline); 
          current = NULL;        //no edge now
        }
        else
                                 //new horiz edge
          current = h_edge (xpos, y, xpos - x, uppercolour - colour, ptrs, *prevline, t);
        *prevline = NULL;        //no change this time
      }
      else {
        if (colour == uppercolour)
          *prevline = v_edge (xpos, y, xpos - x, colour - prevcolour,
            ptrs, *prevline, t);
                                 //8 vs 4 connection
        else if (colour == WHITE_PIX) {
          join_edges(current, *prevline); 
          current = h_edge (xpos, y, xpos - x, uppercolour - colour,
            ptrs, NULL, t);
          *prevline = v_edge (xpos, y, xpos - x, colour - prevcolour,
            ptrs, current, t);
        }
        else {
          newcurrent =
            h_edge (xpos, y, xpos - x, uppercolour - colour, ptrs,
            *prevline, t);
          *prevline =
            v_edge (xpos, y, xpos - x, colour - prevcolour, ptrs,
            current, t);
          current = newcurrent;  //right going h edge
        }
        prevcolour = colour;     //remember new colour
      }
    }
    else {
      if (colour != prevcolour) {
        *prevline = current =
          v_edge (xpos, y, xpos - x, colour - prevcolour, ptrs, current,
          t);
        prevcolour = colour;
      }
      if (colour != uppercolour)
        current = h_edge (xpos, y, xpos - x, uppercolour - colour,
          ptrs, current, t);
      else
        current = NULL;          //no edge now
    }
  }
  if (current != NULL) {
                                 //out of block
    if (*prevline != NULL) {     //got one to join to?
      join_edges(current, *prevline); 
      *prevline = NULL;          //tidy now
    }
    else {
                                 //fake vertical
      *prevline = v_edge (xpos, y, xpos - x, FLIP_COLOUR (prevcolour) - prevcolour, ptrs, current, t);
    }
  }
  else if (*prevline != NULL)
                                 //continue fake
    *prevline = v_edge (xpos, y, xpos - x, FLIP_COLOUR (prevcolour) - prevcolour, ptrs, *prevline, t);
}


/**********************************************************************
 * h_edge
 *
 * Create a new horizontal CRACKEDGE and join it to the given edge.
 **********************************************************************/

CRACKEDGE *
h_edge (                         //horizontal edge
INT16 x,                         //xposition
INT16 y,                         //y position
INT16 xindex,                    //index to lines
INT8 sign,                       //sign of edge
UINT8 * ptrs[],                  //image lines
CRACKEDGE * join,                //edge to join to
INT16 t                          //diff threshold
) {
  int diff;                      //greyscale difference
  int diff2;                     //2nd difference
  CRACKEDGE *newpt;              //return value
  UINT8 *above;                  //line above edge
  UINT8 *below;                  //line below edge

  //      check_mem("h_edge",JUSTCHECKS);
  if (free_cracks != NULL) {
    newpt = free_cracks;
    free_cracks = newpt->next;   //get one fast
  }
  else {
    newpt = new CRACKEDGE;
  }
  newpt->pos.set_y (y + 1);      //coords of pt
  newpt->stepy = 0;              //edge is horizontal
  above = ptrs[ABOVE] + xindex;  //pixel above edge
  below = ptrs[BELOW] + xindex;  //pixel below edge
  diff = ptrs[UPPER][xindex];    //top pixel
  diff -= *below;                //vertical diff

  if (sign > 0) {
    newpt->pos.set_x (x + 1);    //start location
    newpt->stepx = -1;
    newpt->dir = 4;              //dir code
    newpt->stepdir = 0;
    if (diff > t) {
                                 //must be of right sign
      newpt->grady = diff;
      diff2 = *(above + 1);
      diff2 -= *(above - 1);
      newpt->gradx = diff2;      //save gradient vector
    }
    else {
                                 //no vector yet
      newpt->gradx = newpt->grady = 0;
    }
    diff2 = *(above);
    diff2 -= ptrs[LOWER][xindex];//lower difference
    if (diff2 > diff && diff2 > t) {
      newpt->grady = diff2;      //max vectors
      diff2 = *(below + 1);
      diff2 -= *(below - 1);
      newpt->gradx = diff2;
    }
    else if (diff2 == diff && diff2 > t) {
      newpt->grady += diff2;     //sum vectors
      diff2 = *(below + 1);
      diff2 -= *(below - 1);
      newpt->gradx += diff2;
    }
  }
  else {
    newpt->pos.set_x (x);        //start location
    newpt->stepx = 1;
    newpt->dir = 0;              //dir code
    newpt->stepdir = 2;
    if (diff < -t) {
                                 //must be of right sign
      newpt->grady = diff;
      diff2 = *(above + 1);
      diff2 -= *(above - 1);
      newpt->gradx = diff2;      //save gradient vector
    }
    else {
                                 //no vector yet
      newpt->gradx = newpt->grady = 0;
    }
    diff2 = *(above);
    diff2 -= ptrs[LOWER][xindex];//lower difference
    if (diff2 < diff && diff2 < -t) {
      newpt->grady = diff2;      //sum vectors
      diff2 = *(below + 1);
      diff2 -= *(below - 1);
      newpt->gradx = diff2;
    }
    else if (diff2 == diff && diff2 < -t) {
      newpt->grady += diff2;     //sum vectors
      diff2 = *(below + 1);
      diff2 -= *(below - 1);
      newpt->gradx += diff2;
    }
  }

  if (join == NULL) {
    newpt->next = newpt;         //ptrs to other ends
    newpt->prev = newpt;
  }
  else {
    if (newpt->pos.x () + newpt->stepx == join->pos.x ()
    && newpt->pos.y () == join->pos.y ()) {
      newpt->prev = join->prev;  //update other ends
      newpt->prev->next = newpt;
      newpt->next = join;        //join up
      join->prev = newpt;
    }
    else {
      newpt->next = join->next;  //update other ends
      newpt->next->prev = newpt;
      newpt->prev = join;        //join up
      join->next = newpt;
    }
  }
  return newpt;
}


/**********************************************************************
 * v_edge
 *
 * Create a new vertical CRACKEDGE and join it to the given edge.
 **********************************************************************/

CRACKEDGE *
v_edge (                         //vertical edge
INT16 x,                         //xposition
INT16 y,                         //y position
INT16 xindex,                    //index to lines
INT8 sign,                       //sign of edge
UINT8 * ptrs[],                  //image lines
CRACKEDGE * join,                //edge to join to
INT16 t                          //diff threshold
) {
  int diff;                      //greyscale difference
  int diff2;                     //2nd difference
  UINT8 *above;                  //line above edge
  UINT8 *current;                //line on edge
  UINT8 *below;                  //line below edge
  CRACKEDGE *newpt;              //return value

  if (free_cracks != NULL) {
    newpt = free_cracks;
    free_cracks = newpt->next;   //get one fast
  }
  else {
    newpt = new CRACKEDGE;
  }
  newpt->pos.set_x (x);          //coords of pt
  newpt->stepx = 0;              //edge is vertical
  above = ptrs[ABOVE] + xindex;  //line above edge
  current = ptrs[BELOW] + xindex;//line containing edge
  below = ptrs[LOWER] + xindex;  //line below edge
  diff = *current;
  diff -= *(current - 2);

  if (sign > 0) {
    newpt->pos.set_y (y);        //start location
    newpt->stepy = 1;
    newpt->dir = 6;              //dir code
    newpt->stepdir = 3;
    if (diff > t) {
                                 //must be of right sign
      newpt->gradx = diff;       //save gradient vector
      diff2 = *(above - 1);
      diff2 -= *(below - 1);
      newpt->grady = diff2;
    }
    else {
                                 //no vector yet
      newpt->gradx = newpt->grady = 0;
    }
    diff2 = *(current + 1);
    diff2 -= *(current - 1);
    if (diff2 > diff && diff2 > t) {
      newpt->gradx = diff2;
      diff2 = *(above);
      diff2 -= *(below);
      newpt->grady = diff2;      //sum vectors
    }
    else if (diff2 == diff && diff2 > t) {
      newpt->gradx += diff2;
      diff2 = *(above);
      diff2 -= *(below);
      newpt->grady += diff2;     //sum vectors
    }
  }
  else {
    newpt->pos.set_y (y + 1);    //start location
    newpt->stepy = -1;
    newpt->dir = 2;              //dir code
    newpt->stepdir = 1;
    if (diff < -t) {
                                 //must be of right sign
      newpt->gradx = diff;       //save gradient vector
      diff2 = *(above - 1);
      diff2 -= *(below - 1);
      newpt->grady = diff2;
    }
    else {
                                 //no vector yet
      newpt->gradx = newpt->grady = 0;
    }
    diff2 = *(current + 1);
    diff2 -= *(current - 1);
    if (diff2 < diff && diff2 < -t) {
      newpt->gradx = diff2;
      diff2 = *(above);
      diff2 -= *(below);
      newpt->grady = diff2;      //sum vectors
    }
    else if (diff2 == diff && diff2 < -t) {
      newpt->gradx += diff2;
      diff2 = *(above);
      diff2 -= *(below);
      newpt->grady += diff2;     //sum vectors
    }
  }

  if (join == NULL) {
    newpt->next = newpt;         //ptrs to other ends
    newpt->prev = newpt;
  }
  else {
    if (newpt->pos.x () == join->pos.x ()
    && newpt->pos.y () + newpt->stepy == join->pos.y ()) {
      newpt->prev = join->prev;  //update other ends
      newpt->prev->next = newpt;
      newpt->next = join;        //join up
      join->prev = newpt;
    }
    else {
      newpt->next = join->next;  //update other ends
      newpt->next->prev = newpt;
      newpt->prev = join;        //join up
      join->next = newpt;
    }
  }
  return newpt;
}


/**********************************************************************
 * join_edges
 *
 * Join 2 edges together. Send the outline for approximation when a
 * closed loop is formed.
 **********************************************************************/

void join_edges(                   //join edge fragments
                CRACKEDGE *edge1,  //edges to join
                CRACKEDGE *edge2   //no specific order
               ) {
  CRACKEDGE *tempedge;           //for exchanging

  if (edge1->pos.x () + edge1->stepx != edge2->pos.x ()
  || edge1->pos.y () + edge1->stepy != edge2->pos.y ()) {
    tempedge = edge1;
    edge1 = edge2;               //swap araound
    edge2 = tempedge;
  }

  //      tprintf("Joining %x=(%d,%d)+(%d,%d)->%x<-%x ",
  //              edge1,edge1->pos.x(),edge1->pos.y(),edge1->stepx,edge1->stepy,
  //              edge1->next,edge1->prev);
  //      tprintf("to %x=(%d,%d)+(%d,%d)->%x<-%x\n",
  //              edge2,edge2->pos.x(),edge2->pos.y(),edge2->stepx,edge2->stepy,
  //              edge2->next,edge2->prev);
  if (edge1->next == edge2) {
                                 //already closed
    complete_edge(edge1);  //approximate it
                                 //attach freelist to end
    edge1->prev->next = free_cracks;
    free_cracks = edge1;         //and free list
  }
  else {
                                 //update opposite ends
    edge2->prev->next = edge1->next;
    edge1->next->prev = edge2->prev;
    edge1->next = edge2;         //make joins
    edge2->prev = edge1;
  }
}


/**********************************************************************
 * free_crackedges
 *
 * Really free the CRACKEDGEs by giving them back to delete.
 **********************************************************************/

void free_crackedges(                  //really free them
                     CRACKEDGE *start  //start of loop
                    ) {
  CRACKEDGE *current;            //current edge to free
  CRACKEDGE *next;               //next one to free

  for (current = start; current != NULL; current = next) {
    next = current->next;
    delete current;              //delete them all
  }
}
