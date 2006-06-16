/**********************************************************************
 * File:        edgloop.h  (Formerly edgeloop.h)
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

#ifndef           EDGLOOP_H
#define           EDGLOOP_H

#include          "grphics.h"
#include          "varable.h"
#include          "img.h"
#include          "pdblock.h"
#include          "coutln.h"
#include          "crakedge.h"

#define BUCKETSIZE      16

//class OL_BUCKETS
//{
//public:
//                                                                      OL_BUCKETS(                                                     //constructor
//      ICOORD                                          bleft,                                                          //corners
//      ICOORD                                          tright);
//
//                                                                      ~OL_BUCKETS()                                           //cleanup
//      {
//              delete [bxdim*bydim] buckets;
//      }
//      C_OUTLINE_LIST*                 operator()(                                                     //array access
//      INT16                                                   x,                                                                              //image coords
//      INT16                                                   y);
//      C_OUTLINE_LIST*                 start_scan()                                            //first non-empty bucket
//      {
//              for (index=0;buckets[index].empty() && index<bxdim*bydim-1;index++);
//              return &buckets[index];
//      }
//      C_OUTLINE_LIST*                 scan_next()                                                     //next non-empty bucket
//      {
//              for (;buckets[index].empty() && index<bxdim*bydim-1;index++);
//              return &buckets[index];
//      }
//      INT32                                                   count_children(                                 //recursive sum
//      C_OUTLINE*                                      outline,                                                        //parent outline
//      INT32                                                   max_count);                                                     //max output
//      void                                                    extract_children(                                       //single level get
//      C_OUTLINE*                                      outline,                                                                //parent outline
//      C_OUTLINE_IT*                           it);                                                                    //destination iterator
//
//private:
//      C_OUTLINE_LIST*                 buckets;                                                                //array of buckets
//      INT16                                                   bxdim;                                                          //size of array
//      INT16                                                   bydim;
//      ICOORD                                          bl;                                                                     //corners
//      ICOORD                                          tr;
//      INT32                                                   index;                                                          //for extraction scan
//};

extern double_VAR_H (edges_threshold_greyfraction, 0.07,
"Min edge diff for grad vector");
extern BOOL_VAR_H (edges_show_paths, FALSE, "Draw raw outlines");
extern BOOL_VAR_H (edges_show_needles, FALSE, "Draw edge needles");
extern INT_VAR_H (edges_children_per_grandchild, 10,
"Importance ratio for chucking outlines");
extern INT_VAR_H (edges_children_count_limit, 45,
"Max holes allowed in blob");
extern INT_VAR_H (edges_maxedgelength, 16000, "Max steps in any outline");
extern double_VAR_H (edges_childarea, 0.5,
"Max area fraction of child outline");
extern double_VAR_H (edges_boxarea, 0.8,
"Min area fraction of grandchild for box");
DLLSYM void get_outlines(                      //edge detect
                         WINDOW window,        //window for output
                         IMAGE *image,         //image to scan
                         IMAGE *t_image,       //thresholded image
                         ICOORD page_tr,       //corner of page
                         PDBLK *block,         //block to scan
                         C_OUTLINE_IT *out_it  //output iterator
                        );
void complete_edge(                  //clean and approximate
                   CRACKEDGE *start  //start of loop
                  );
COLOUR check_path_legal(                  //certify outline
                        CRACKEDGE *start  //start of loop
                       );
void fill_blank_gradients(                  //invent gradients
                          CRACKEDGE *start  //outline
                         );
INT16 loop_bounding_box(                    //get bounding box
                        CRACKEDGE *&start,  //edge loop
                        ICOORD &botleft,    //bounding box
                        ICOORD &topright);
#endif
