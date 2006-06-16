/**********************************************************************
 * File:        edgloop.c  (Formerly edgeloop.c)
 * Description: Functions to clean up an outline before approximation.
 * Author:					Ray Smith
 * Created:					Tue Mar 26 16:56:25 GMT 1991
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
#include          "dirtab.h"
#include          "scanedg.h"
#include          "drawedg.h"
#include          "edgloop.h"

#define MINEDGELENGTH   8        //min decent length

#define EXTERN

EXTERN double_VAR (edges_threshold_greyfraction, 0.07,
"Min edge diff for grad vector");
EXTERN BOOL_VAR (edges_show_paths, FALSE, "Draw raw outlines");
EXTERN BOOL_VAR (edges_show_needles, FALSE, "Draw edge needles");
EXTERN INT_VAR (edges_maxedgelength, 16000, "Max steps in any outline");

static INT8 bpp;                 //bits per pixel
static WINDOW edge_win;          //window
static C_OUTLINE_IT *outline_it; //iterator
static int short_edges;          //no of short ones
static int long_edges;           //no of long ones

///**********************************************************************
//* OL_BUCKETS::OL_BUCKETS
//*
//* Construct an array of buckets for associating outlines into blobs.
//**********************************************************************/
//
//                                                                      OL_BUCKETS::OL_BUCKETS(                 ////constructor
//ICOORD                                                        bleft,                                                          //corners
//ICOORD                                                        tright
//) : bl(bleft), tr(tright)
//{
//
//      bxdim=(tright.x()-bleft.x())/BUCKETSIZE+1;
//      bydim=(tright.y()-bleft.y())/BUCKETSIZE+1;
//      buckets=new C_OUTLINE_LIST[bxdim*bydim];                                        //make array
//      index=0;
//}
//
///**********************************************************************
//* OL_BUCKETS::operator(
//*
//* Return a pointer to a list of C_OUTLINEs corresponding to the
//* given pixel coordinates.
//**********************************************************************/
//
//C_OUTLINE_LIST*                               OL_BUCKETS::operator()(                 //array access
//INT16                                                         x,                                                                              //image coords
//INT16                                                         y
//)
//{
//      return &buckets[(y-bl.y())/BUCKETSIZE*bxdim+(x-bl.x())/BUCKETSIZE];
//}
//
///**********************************************************************
//* OL_BUCKETS::count_children
//*
//* Find number of descendants of this outline.
//**********************************************************************/
//
//INT32                                                         OL_BUCKETS::count_children(     //recursive count
//C_OUTLINE*                                            outline,                                                                //parent outline
//INT32                                                         max_count                                                       //max output
//)
//{
//      BOOL8                                                   parent_box;                                                     //could it be boxy
//      INT16                                                   xmin,xmax;                                                      //coord limits
//      INT16                                                   ymin,ymax;
//      INT16                                                   xindex,yindex;                                          //current bucket
//      C_OUTLINE*                                      child;                                                          //current child
//      INT32                                                   child_count;                                            //no of children
//      INT32                                                   parent_area;                                            //potential box
//      INT32                                                   child_area;                                                     //current child
//      BOX                                                     olbox;
//      C_OUTLINE_IT                            child_it;                                                       //search iterator
//
//      olbox=outline->bounding_box();
//      xmin=(olbox.left()-bl.x())/BUCKETSIZE;
//      xmax=(olbox.right()-bl.x())/BUCKETSIZE;
//      ymin=(olbox.bottom()-bl.y())/BUCKETSIZE;
//      ymax=(olbox.top()-bl.y())/BUCKETSIZE;
//      child_count=0;
//      parent_area=0;
//      parent_box=TRUE;
//      for (yindex=ymin;yindex<=ymax;yindex++)
//      {
//              for (xindex=xmin;xindex<=xmax;xindex++)
//              {
//                      child_it.set_to_list(&buckets[yindex*bxdim+xindex]);
//                      if (child_it.empty())
//                              continue;
//                      for (child_it.mark_cycle_pt();!child_it.cycled_list();child_it.forward())
//                      {
//                              child=child_it.data();
//                              if (child!=outline
//                              && *child<*outline)
//                              {
//                                      child_count++;
//                                      if (child_count<=max_count)
//                                              child_count+=count_children(child,
//                                                      (max_count-child_count)/edges_children_per_grandchild)
//                                                      *edges_children_per_grandchild;
//                                      if (child_count>max_count)
//                                              return child_count;
//                                      if (parent_area==0)
//                                      {
//                                              parent_area=outline->outer_area();
//                                              if (parent_area<0)
//                                                      parent_area=-parent_area;
//                                              if (parent_area<outline->bounding_box().width()
//                                                      *outline->bounding_box().height()
//                                                      *edges_boxarea)
//                                                      parent_box=FALSE;
//                                      }
//                                      if (parent_box)
//                                      {
//                                              child_area=child->outer_area();
//                                              if (child_area<0)
//                                                      child_area=-child_area;
//                                              if (child_area<child->bounding_box().width()
//                                                      *child->bounding_box().height()*edges_childarea)
//                                                      return max_count+1;
//                                      }
//                              }
//                      }
//              }
//      }
//      return child_count;
//}
//
///**********************************************************************
//* OL_BUCKETS::extract_children
//*
//* Find number of descendants of this outline.
//**********************************************************************/
//
//void                                                          OL_BUCKETS::extract_children(   //recursive count
//C_OUTLINE*                                            outline,                                                                //parent outline
//C_OUTLINE_IT*                                 it                                                                              //destination iterator
//)
//{
//      INT16                                                   xmin,xmax;                                                      //coord limits
//      INT16                                                   ymin,ymax;
//      INT16                                                   xindex,yindex;                                          //current bucket
//      BOX                                                     olbox;
//      C_OUTLINE_IT                            child_it;                                                       //search iterator
//
//      olbox=outline->bounding_box();
//      xmin=(olbox.left()-bl.x())/BUCKETSIZE;
//      xmax=(olbox.right()-bl.x())/BUCKETSIZE;
//      ymin=(olbox.bottom()-bl.y())/BUCKETSIZE;
//      ymax=(olbox.top()-bl.y())/BUCKETSIZE;
//      for (yindex=ymin;yindex<=ymax;yindex++)
//      {
//              for (xindex=xmin;xindex<=xmax;xindex++)
//              {
//                      child_it.set_to_list(&buckets[yindex*bxdim+xindex]);
//                      for (child_it.mark_cycle_pt();!child_it.cycled_list();child_it.forward())
//                      {
//                              if (*child_it.data()<*outline)
//                              {
//                                      it->add_after_then_move(child_it.extract());
//                              }
//                      }
//              }
//      }
//}
//
///**********************************************************************
//* extract_edges
//*
//* Run the edge detector over the block and return a list of blobs.
//**********************************************************************/
//
//void                                                          extract_edges(                                          //find blobs
//WINDOW                                                        window,                                                         //window for output
//IMAGE                                                         *image,                                                         //image to scan
//IMAGE                                                         *t_image,                                               //thresholded image
//ICOORD                                                        page_tr,                                                                //corner of page
//PDBLK                                                         *block                                                          //block to scan
//)
//{
//      ICOORD                                          bleft;                                                          //block box
//      ICOORD                                          tright;
//      C_OUTLINE_LIST                          outlines;                                                       //outlines in block
//      C_OUTLINE_IT                            out_it=&outlines;                                       //iterator
//
//      short_edges=long_edges=0;
//      get_outlines(window,image,t_image,page_tr,block,&out_it);
//      block->bounding_box(bleft,tright);                                                      //block box
//      outlines_to_blobs(block,bleft,tright,&outlines);                //make blobs
//
///*  Commented out for UNLV
//      if (short_edges>0 || long_edges>0)
//              tprintf("Short edges=%d, Long edges=%d\n",
//                      short_edges,long_edges);
//*/
//}
//
///**********************************************************************
//* outlines_to_blobs
//*
//* Gather together outlines into blobs using the usual bucket sort.
//**********************************************************************/
//
//void                                                          outlines_to_blobs(                              //find blobs
//PDBLK                                                         *block,                                                         //block to scan
//ICOORD                                                        bleft,                                                          //block box
//ICOORD                                                        tright,
//C_OUTLINE_LIST*                               outlines                                                                //outlines in block
//)
//{
//      OL_BUCKETS                                      buckets(bleft,tright);                  //make buckets
//
//      fill_buckets(outlines,&buckets);
//      empty_buckets(block,&buckets);
//}
//
///**********************************************************************
//* fill_buckets
//*
//* Run the edge detector over the block and return a list of blobs.
//**********************************************************************/
//
//void                                                          fill_buckets(                                           //find blobs
//C_OUTLINE_LIST*                               outlines,                                                       //outlines in block
//OL_BUCKETS*                                           buckets                                                         //output buckets
//)
//{
//      BOX                                                     ol_box;                                                         //outline box
//      C_OUTLINE_IT                            out_it=outlines;                                        //iterator
//      C_OUTLINE_IT                            bucket_it;                                                      //iterator in bucket
//      C_OUTLINE*                                      outline;                                                                //current outline
//
//      for (out_it.mark_cycle_pt();!out_it.cycled_list();out_it.forward())
//      {
//              outline=out_it.extract();                                                                       //take off list
//              ol_box=outline->bounding_box();                                                 //get box
//              bucket_it.set_to_list((*buckets)(ol_box.left(),ol_box.bottom()));
//              bucket_it.add_to_end(outline);
//      }
//}
//
///**********************************************************************
//* empty_buckets
//*
//* Run the edge detector over the block and return a list of blobs.
//**********************************************************************/
//
//void                                                          empty_buckets(                                          //find blobs
//PDBLK                                                         *block,                                                         //block to scan
//OL_BUCKETS*                                           buckets                                                         //output buckets
//)
//{
//      BOOL8                                                   good_blob;                                                      //healthy blob
//      C_OUTLINE_LIST                          outlines;                                                       //outlines in block
//      C_OUTLINE_IT                            out_it=&outlines;                                       //iterator
//      C_OUTLINE_IT                            bucket_it=buckets->start_scan();
//      C_OUTLINE_IT                            parent_it;                                                      //parent outline
//      C_BLOB*                                         blob;                                                                   //new blob
//      C_BLOB_IT                                       good_blobs=block->blob_list();
//      C_BLOB_IT                                       junk_blobs=block->reject_blobs();
//
//      while (!bucket_it.empty())
//      {
//              out_it.set_to_list(&outlines);
//              do
//              {
//                      parent_it=bucket_it;                                                                            //find outermost
//                      do
//                              bucket_it.forward();
//                      while (!bucket_it.at_first() && !(*parent_it.data()<*bucket_it.data()));
//              }
//              while (!bucket_it.at_first());
//
//              out_it.add_after_then_move(parent_it.extract());        //move to new list
//              good_blob=capture_children(buckets,&junk_blobs,&out_it);
//              blob=new C_BLOB(&outlines);
//              if (good_blob)
//                      good_blobs.add_after_then_move(blob);
//              else
//                      junk_blobs.add_after_then_move(blob);
//
//              bucket_it.set_to_list(buckets->scan_next());
//      }
//}
//
///**********************************************************************
//* capture_children
//*
//* Find all neighbouring outlines that are children of this outline
//* and either move them to the output list or declare this outline
//* illegal and return FALSE.
//**********************************************************************/
//
//BOOL8                                                         capture_children(                                       //find children
//OL_BUCKETS*                                           buckets,                                                                //bucket sort clanss
//C_BLOB_IT*                                            reject_it,                                                      //dead grandchildren
//C_OUTLINE_IT*                                 blob_it                                                         //output outlines
//)
//{
//      BOOL8                                                   anydone;                                                                //anything canned
//      C_OUTLINE*                                              outline;                                                                //master outline
//      C_OUTLINE*                                              child;                                                          //child under test
//      C_OUTLINE_IT                            test_it;                                                                //for grandchildren
//      INT32                                                   child_count;                                            //no of children
//      C_BLOB*                                         blob;                                                                   //reject
//      C_OUTLINE_LIST                          r_list;                                                         //rejects
//      C_OUTLINE_IT                            r_it;                                                                   //iterator
//
//      outline=blob_it->data();
//      child_count=buckets->count_children(outline,edges_children_count_limit);
//      if (child_count>edges_children_count_limit)
//              return FALSE;
//      if (child_count==0)
//              return TRUE;
//      buckets->extract_children(outline,blob_it);                             //get single level
//      if (child_count==1)
//              return TRUE;
//      do
//      {
//              anydone=FALSE;
//              blob_it->move_to_first();
//              for (blob_it->mark_cycle_pt();!blob_it->cycled_list();blob_it->forward())
//              {
//                      child=blob_it->data();
//                      if (child!=outline)
//                      {
//                              for (test_it=*blob_it,test_it.mark_cycle_pt();
//                              !test_it.cycled_list();test_it.forward())
//                              {
//                                      if (test_it.data()!=child && *test_it.data()<*child)
//                                      {
//                                              r_it.set_to_list(&r_list);
//                                              r_it.add_after_then_move(test_it.extract());
//                                              blob=new C_BLOB(&r_list);                                               //turn to blob
//                                              reject_it->add_after_then_move(blob);
//                                              anydone=TRUE;
//                                      }
//                              }
//                              if (anydone)
//                                      break;                                                                                          //got to reatart
//                      }
//              }
//      }
//      while (anydone);                                                                                        //got to restart
//      return TRUE;
//}
//
/**********************************************************************
 * get_outlines
 *
 * Run the edge detector over the block and return a list of outlines.
 **********************************************************************/

DLLSYM void get_outlines(                      //edge detect
                         WINDOW window,        //window for output
                         IMAGE *image,         //image to scan
                         IMAGE *t_image,       //thresholded image
                         ICOORD page_tr,       //corner of page
                         PDBLK *block,         //block to scan
                         C_OUTLINE_IT *out_it  //output iterator
                        ) {
  INT16 grey_threshold;          //greyscale threshold

  edge_win = window;             //set statics
  outline_it = out_it;
  bpp = image->get_bpp ();
  grey_threshold = (INT16) (((1 << bpp) - 1) * edges_threshold_greyfraction);

  //      if (edges_select_regions)
  //              tprintf("Threshold=%d\n",grey_threshold);
  block_edges(image, t_image, block, page_tr, grey_threshold); 
  out_it->move_to_first ();
  if (window != NO_WINDOW)
    overlap_picture_ops(TRUE);  //update window
}


/**********************************************************************
 * complete_edge
 *
 * Complete the edge by cleaning it up andapproximating it.
 **********************************************************************/

void complete_edge(                  //clean and approximate
                   CRACKEDGE *start  //start of loop
                  ) {
  COLOUR colour;                 //colour to draw in
  INT16 looplength;              //steps in loop
  ICOORD botleft;                //bounding box
  ICOORD topright;
  C_OUTLINE *outline;            //new outline

                                 //check length etc.
  colour = check_path_legal (start);
  if (edges_show_paths) {
                                 //in red
    draw_raw_edge(edge_win, start, colour); 
  }

  if (colour == RED || colour == BLUE) {
    fill_blank_gradients(start);  //complete gradient vecotrs
                                 //turn to directions
    convert_gradients(start, bpp); 
    looplength = loop_bounding_box (start, botleft, topright);
    //get bounding box
    //              tprintf("Edge of length %d completed starting at (%d,%d)\n",
    //                      looplength,start->pos.x(),start->pos.y());
    if (edges_show_needles) {
                                 //draw gradient vectors
      draw_edge_needles(edge_win, start, MAGENTA); 
    }
    outline = new C_OUTLINE (start, botleft, topright, looplength);
                                 //add to list
    outline_it->add_after_then_move (outline);
  }
}


/**********************************************************************
 * check_path_legal
 *
 * Check that the outline is legal for length and for chaincode sum.
 * The return value is RED for a normal black-inside outline,
 * BLUE for a white-inside outline, MAGENTA if it is too short,
 * YELLOW if it is too long, and GREEN if it is illegal.
 * These colours are used to draw the raw outline.
 **********************************************************************/

COLOUR check_path_legal(                  //certify outline
                        CRACKEDGE *start  //start of loop
                       ) {
  DIR128 lastchain;              //last chain code
  INT16 chaindiff;               //chain code diff
  INT32 length;                  //length of loop
  INT32 chainsum;                //sum of chain diffs
  CRACKEDGE *edgept;             //current point
  const ERRCODE ED_ILLEGAL_SUM = "Illegal sum of chain codes";

  length = 0;
  chainsum = 0;                  //sum of chain codes
  edgept = start;
  lastchain = edgept->prev->dir; //previous chain code
  do {
    length++;
    if (edgept->dir.get_dir () != lastchain.get_dir ()) {
                                 //chain code difference
      chaindiff = edgept->dir - lastchain;
      if (chaindiff > 4)
        chaindiff -= 8;
      else if (chaindiff < -4)
        chaindiff += 8;
      chainsum += chaindiff;     //sum differences
      lastchain = edgept->dir;
    }
    edgept = edgept->next;
  }
  while (edgept != start && length < edges_maxedgelength);

  if (chainsum != 8 && chainsum != -8
  || edgept != start || length < MINEDGELENGTH) {
    if (edgept != start) {
      long_edges++;
      return YELLOW;
    }
    else if (length < MINEDGELENGTH) {
      short_edges++;
      return MAGENTA;
    }
    else {
      ED_ILLEGAL_SUM.error ("check_path_legal", LOG, "chainsum=%d",
        chainsum);
      return GREEN;
    }
  }
                                 //colour on inside
  return chainsum > 0 ? BLUE : RED;
}


/**********************************************************************
 * fill_blank_gradients
 *
 * Invent a gradient vector for zero ones by using the average
 * direction of 3 perpendiculars to neighbouring steps.
 **********************************************************************/

void fill_blank_gradients(                  //invent gradients
                          CRACKEDGE *start  //outline
                         ) {
  CRACKEDGE *edgept;             //current point

  edgept = start;
  do {
    if (edgept->gradx == 0 && edgept->grady == 0) {
                                 //simple sum
      edgept->gradx = edgept->prev->stepy + edgept->stepy + edgept->next->stepy;
                                 //simple sum
      edgept->grady = -(edgept->prev->stepx + edgept->stepx + edgept->next->stepx);
    }
    edgept = edgept->next;
  }
  while (edgept != start);
}


/**********************************************************************
 * loop_bounding_box
 *
 * Find the bounding box of the edge loop.
 **********************************************************************/

INT16 loop_bounding_box(                    //get bounding box
                        CRACKEDGE *&start,  //edge loop
                        ICOORD &botleft,    //bounding box
                        ICOORD &topright) {
  INT16 length;                  //length of loop
  INT16 leftmost;                //on top row
  CRACKEDGE *edgept;             //current point
  CRACKEDGE *realstart;          //topleft start

  edgept = start;
  realstart = start;
  botleft = topright = ICOORD (edgept->pos.x (), edgept->pos.y ());
  leftmost = edgept->pos.x ();
  length = 0;                    //coutn length
  do {
    edgept = edgept->next;
    if (edgept->pos.x () < botleft.x ())
                                 //get bounding box
      botleft.set_x (edgept->pos.x ());
    else if (edgept->pos.x () > topright.x ())
      topright.set_x (edgept->pos.x ());
    if (edgept->pos.y () < botleft.y ())
                                 //get bounding box
      botleft.set_y (edgept->pos.y ());
    else if (edgept->pos.y () > topright.y ()) {
      realstart = edgept;
      leftmost = edgept->pos.x ();
      topright.set_y (edgept->pos.y ());
    }
    else if (edgept->pos.y () == topright.y ()
    && edgept->pos.x () < leftmost) {
                                 //leftmost on line
      leftmost = edgept->pos.x ();
      realstart = edgept;
    }
    length++;                    //count elements
  }
  while (edgept != start);
  start = realstart;             //shift it to topleft
  return length;
}
