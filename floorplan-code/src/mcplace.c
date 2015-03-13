/* file: mcplace.c                                              */

/* this file has code for min-cut placement                     */

/******************************************************************************
Copyright (c) 1991-2007, Owen Kaser

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name(s) of the copyright holders nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

/* Written by Owen Kaser, July 30, 1991                         */

#include "floorp.h"
#include "graph.h"  /* also gets slice.h */
#include "mincut.h"
#include "html.h"

slice_tree place2(graph g, char vert_slice)
/*
 -- if there are pulls on the vertices, there will be extra vertices
 -- created.  However, they all have zero weights.  We cannot
 -- permit interchange of the values, though, since the system
 -- would have to pass thru the situation that one part had all of
 -- the weight (the anchor vertices have none), and we don't want
 -- to EVER allow an empty partition.  Thus, without three vertices
 -- with weights, this method will not work.
*/
{
   slice_tree left_leaf,right_leaf,parent;
   maxgain_type pullA,pullB,cutsz;
   int harder_puller,lchild;  /* recall left and top are equiv */
   
   if (vert_slice)
   {  pullA = g->vertices[0].horiz_pull;
      pullB = g->vertices[1].horiz_pull;
   }
   else
   {  pullA = g->vertices[0].vert_pull;
      pullB = g->vertices[1].vert_pull;
   }

   harder_puller = ( labs(pullB) > labs(pullA)); /* its given preference */

   if (pullA > 40000L || pullB > 40000L) fail("oops for pullA or pullB");

   lchild = (harder_puller) ? ( pullB > 0) : (pullA < 0);

   left_leaf = make_slice_leaf( g->vertices[ lchild].original_name);
   right_leaf= make_slice_leaf( g->vertices[!lchild].original_name);   
   if (g->adjacencies[0] == NULL)
     cutsz = 0;
   else
   {
     cutsz = g->adjacencies[0]->w;  /* edge from 0 to 1 */
   }


   parent = make_slice_node( vert_slice, left_leaf, right_leaf,cutsz);

   return parent;
}



extern float width_scalefactor;  /* defined+adjusted in floorp.c */

slice_tree rec_placement(graph g,
                         float vert_dim, 
                         float horiz_dim,
                         int depth,
                         slice_tree par,
                         int ischild1,
                         int extL, int extR, int extT, int extB)
{
   slice_tree t1,t2,tt;
   graph G1,G2;
   float vert1,vert2,horiz1,horiz2;  /* children's est. dimensions */
   char vert_slice = (horiz_dim > vert_dim);
   maxgain_type cutsize;

   int first_is_leftmost, first_is_rightmost, first_is_topmost, first_is_bottommost;
   int second_is_leftmost, second_is_rightmost, second_is_topmost, second_is_bottommost;

  /*  Hack for 2007, tag clouds
   *  if the width is fixed (as in tag-clouds), our available
   *  horizontal area is estimated FIXEDWIDTH*horiz_dim.
   *  If any of the tags we're placing is bigger than this, we're hosed
   *  and would have to make up the problem elsewhere, or we'd go outside our
   *  supposedly fixed width.
   *
   *  vtx field original_name_string is tag, including size prefix
   *
   * 
   *  quick estimate of what the final aspect ration
   *  is likely to be, and forbid vertical cuts
   */
  float available_width = FIXEDWIDTH * horiz_dim;
  /* If I only just fit, I should not subdivide with vertical slice*/
  /* 2 would account for absolute worst case, where 2 max-length   */
  /* tags end up on either side of the cut. Seems too conservative */
  /* messed around emperically, 1.7 also was slightly too big      */
  /* If we do tag sizing afterward, we can get this back...        */

  if (vert_slice) {
    int i;

    /* scan tags looking for any that are too wide */
    for (i=0; i < g->n; ++i) {

      /* use the middle shape */
      int mid = g->vertices[i].size_start_index + (g->vertices[i].size_end_index -  g->vertices[i].size_start_index)/2;

      /* take what ought to be the "normal" (neither compressed nor expanded) one, at +1 */
      int tagw = basic_shapes[mid].x;  
      int tagh = basic_shapes[mid].y;  
      if (! extL) tagw += tagh/2;  /* half space on my left */
      if (! extR) tagw += tagh/2;  /* and right */

      if ( tagw * width_scalefactor >= (int) available_width) {
        vert_slice = 0;
#ifdef BLAB
        printf("fixedwidth control forbids vertical slice because %s is too wide for my %d\n",
               g->vertices[i].original_name_string, (int) available_width);
#endif
        break;
      }
    }
#ifdef BLAB
      else printf("tag width %d appears ok for my estimated width %d\n",
                  (int) get_tag_width(g->vertices[i].original_name_string, extL, extR),
                  (int) available_width);
#endif
  }

  if (g->n == 1)
    {
      /* make a leaf of slicing tree with the single node's orig name */
      return make_slice_leaf( g->vertices[0].original_name);
    }
  
  if (g->n == 2)
    {  /* second trivial case; just assign "exhaustively" */
      return place2(g,vert_slice);
    }
  
  G1 = GPH my_malloc( sizeof(*G1));
  G2 = GPH my_malloc( sizeof(*G2));
  cutsize = mincost_partition(g,vert_slice, G1,G2);
  
  if (g->n > 3)
    {
#if 0
       printf("cutting %d\n",(int)g->first_dummy);
       printf("depth %d, Cut size is %d\n",depth,(int)cutsize);
       printf("Balance factor is %e\n",((float)WA)/(WA+WB));
#endif
    }
   
   /* decide on slice direction based on cutting lgst dimension */
   /* (of course, this is estimated)                            */
   vert1 = vert2 = vert_dim;
   horiz1 = horiz2 = horiz_dim;
   if (vert_slice) {
     first_is_leftmost = extL;
     second_is_leftmost = first_is_rightmost = 0;
     second_is_rightmost = extR;
     first_is_topmost = second_is_topmost = extT;
     first_is_bottommost = second_is_bottommost = extB;
     
     horiz1 = horiz_dim * ((float)WA/(WA+WB));
     /* estimation based on area ratios */
     horiz2 = horiz_dim - horiz1;
   }
   else {
     first_is_topmost = extT;
     second_is_topmost = first_is_bottommost = 0;
     second_is_bottommost = extB;
     first_is_leftmost = second_is_leftmost = extL;
     first_is_rightmost = second_is_rightmost = extB;

     vert1 = vert_dim * ((float)WA/(WA+WB));
     vert2 = vert_dim - vert1;
   }

   tt = make_slice_node( vert_slice, STT NULL, STT NULL, cutsize);
   /* left or top */
   t1 = rec_placement(G1,vert1, horiz1,depth+1,tt,1, first_is_leftmost, first_is_rightmost, first_is_topmost, first_is_bottommost);
   /* right or bottom */
   t2 = rec_placement(G2,vert2, horiz2,depth+1,tt,0, second_is_leftmost, second_is_rightmost, second_is_topmost, second_is_bottommost);
   if (t1) dispose_graph(G1);  
   if (t2) dispose_graph(G2);
   tt->child1 = t1; tt->child2 = t2;
   return tt;
}

