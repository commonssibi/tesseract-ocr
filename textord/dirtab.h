/**********************************************************************
 * File:						dirtab.h     (Formerly: dirtable.h)
 * Description: Turn gradient vectors into directions using a massive lookup table.
 * Author:					Ray Smith
 * Created:					Tue Mar 26 15:57:43 GMT 1991
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

#ifndef           DIRTAB_H
#define           DIRTAB_H

#include          "grphics.h"
#include          "crakedge.h"
#include          "mod128.h"

void convert_gradients(                   //set dir members
                       CRACKEDGE *start,  //start of outline
                       INT8 bpp           //bits per pixel
                      );
DIR128 convert_one_gradient(             //set dir members
                            ICOORD grad  //gradient to convert
                           );
void draw_edge_needles(                   //draw gradients
                       WINDOW fd,         //window to draw in
                       CRACKEDGE *start,  //start of outline
                       COLOUR colour      //colour to draw in
                      );
void draw_gradient_needles(                   //draw gradients
                           WINDOW fd,         //window to draw in
                           CRACKEDGE *start,  //start of outline
                           COLOUR colour      //colour to draw in
                          );
void dir_to_gradient(                //convert to vector
                     DIR128 dir,     //dir to convert
                     ICOORD &vector  //result
                    );
#endif
