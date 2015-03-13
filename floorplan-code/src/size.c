/* file: size.c                                                    */

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



/* Sizing function computations for the fast parallel floorplanner */
/* We follow McFarland closely, keeping just the best B functions  */
/* except that we use a merge operation, rather than a brute-force */
/* pairwise computation.                                           */
/* Written by Owen Kaser,  August 8, 1991                          */


 
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "floorp.h"
#include "graph.h"  /* gets slice too */

#define B         	100       /* Max number of steps per size fcn */
#define EMPTY     	-1       /* indicates shape entry bucket empty */
#define TBLMAX    	(N*B)
#define HUGEL     	1000000000

shapeentry T[B];
shapeentry dims[TBLMAX];

void entry(int key_is_y, int e1, int e2, int xmin, float interval)
{  int x,y;
   int bucket;

   /* combine them together */
   if (key_is_y)
   {
      x = dims[e1].x + dims[e2].x;
      y = MAX( dims[e1].y, dims[e2].y);
   }
   else
   {
      y = dims[e1].y + dims[e2].y;
      x = MAX( dims[e1].x, dims[e2].x);
   }


   /* see if it's the best yet, in its bucket */

   bucket = (x-xmin)/interval;
   /* due to logic bug and/or rounding errors, bucket can go out of range by one */
   /* hackaround follows (may or may not be perfectly correct)                   */
   if (bucket == B) bucket--;

   assert(0 <= bucket && bucket < B);

#ifdef BLAB
   printf("compute entry %d %d for bucket %d\n",x,y, bucket);
#endif


   if (T[bucket].x == EMPTY || 
       (x * (float)y < T[bucket].x * (float) T[bucket].y))
   { T[bucket].x = x; T[bucket].y = y;
     T[bucket].src1 = e1; T[bucket].src2 = e2;
   }
}



void merge(int key_is_y, /* if true,  y's are in descending order; x's in ascending */
           slice_tree c1, slice_tree c2)
{
   int step;
   int stop1,stop2,cur1,cur2,prev1,prev2;
   long key1,key2;
   int xmax,xmin;
   float interval;

   if (key_is_y)     /* vertical cut */
   {  step = -1;
      cur1 = c1->shape_fcn_end;
      cur2 = c2->shape_fcn_end;
      stop1 = c1->shape_fcn_start-1;
      stop2 = c2->shape_fcn_start-1;

      xmax = dims[cur1].x + dims[cur2].x;
      xmin = dims[stop1+1].x + dims[stop2+1].x;
   }
   else
   {  step = 1;
      cur1 = c1->shape_fcn_start;
      cur2 = c2->shape_fcn_start;
      stop1 = c1->shape_fcn_end+1;
      stop2 = c2->shape_fcn_end+1;

      xmax = MAX( dims[stop1-1].x, dims[stop2-1].x);
      xmin = MAX( dims[cur1].x, dims[cur2].x);
   }

   interval = ((float)xmax - xmin)/B;
   if (interval < 0.0001)
     interval = 1.0;

   prev1 = prev2 = -1;

   while (cur1 != stop1 || cur2 != stop2)  /* go until both empty */
   {
      key1 = (key_is_y) ? dims[cur1].y : dims[cur1].x;
      key2 = (key_is_y) ? dims[cur2].y : dims[cur2].x;

      if (cur1 == stop1) key1 = HUGEL;
      if (cur2 == stop2) key2 = HUGEL;

      if (key1 < key2)
      {
         /* cannot do if #2 has no previous entry */
         if (prev2 != -1)
         {
           entry( key_is_y, cur1, prev2, xmin, interval);
         }
         if (cur1 != stop1)
         {  prev1 = cur1;
            cur1 += step;
         }
      }
      else
        if (key1 > key2)
        {
           if (prev1 != -1)
             entry(key_is_y, prev1, cur2, xmin, interval);
           if (cur2 != stop2)
           {  prev2 = cur2;
              cur2 += step;
           }
        }
        else  /* key1 == key2 */
        {
           entry(key_is_y, cur1, cur2, xmin, interval);
           if (cur1 != stop1)
           {  prev1 = cur1;
              cur1 += step;
           }
           if (cur2 != stop2)
           {  prev2 = cur2;
              cur2 += step;
           }
        }
   }   /* end of while(1)  */
}



int free_entry;  /* into global table */

void print_sizing_function(slice_tree t)
{  int i;
   
   printf("Sizing function for node %d, whose kids are ",t->traversal_id);
   if (!t->child1) printf("null and "); else
     printf("%d and ",t->child1->traversal_id);
   if (!t->child2) printf("null : "); else
     printf("%d : ",t->child2->traversal_id);

   for (i=t->shape_fcn_start; i <= t->shape_fcn_end; i++)
     printf(" (%d %d) ",(int)dims[i].x, (int)dims[i].y);
   printf("\n");
}

/* do the merge but leave the results in temporary private table T */

void merge_fcns(t)
slice_tree t;
{ int i;

  for (i = 0; i < B; i++)
    T[i].x = EMPTY;

  merge(t->direction,t->child1,t->child2);
  return ;
}



void update_dims_from_T(slice_tree t)
{
  int i;

  if (free_entry + B >= TBLMAX)
    fail("Out of shape table memory; increase TBLMAX");

  t->shape_fcn_start = free_entry;

  for (i = 0; i < B; i++)
    if (T[i].x != EMPTY)
    { 
      dims[free_entry++] = T[i];
    }  
      
  t->shape_fcn_end = free_entry-1;
}      
    



#define TMPMAX    200
int size_comp_x( const void *pp1, const void *pp2)  /* for use by qsort */
{
  const shapeentry *p1 = pp1, *p2= pp2;
   if (p1->x > p2->x) return 1;
   if (p1->x < p2->x) return -1;
   return 0;
}


void load_dimensions(graph g, slice_tree t)
{  int i,j,k;
   shapeentry temp[TMPMAX];
   vertex vv = t->name;

   j = 0;
   for (i = g->vertices[vv].size_start_index; i <=
                               g->vertices[vv].size_end_index; i++,j++)
   {  temp[j] = basic_shapes[i];
      if (j >= TMPMAX-2) {
         fail("Increase TMPMAX");
      }  
   }
   qsort( temp, j, sizeof( shapeentry), size_comp_x);

   /* now put the desirable entries into T */
   for (i = 0; i < B; i++)
    T[i].x = EMPTY;

   k = 0;  /* k is index into B, as free_entry is into dims */ 

   for (i=0; i < j; i++)
   {   
      if (i==0 || temp[i].x != T[k-1].x)  /* discard dups */
      {

        /* don't enter this one if it is worse y-wise than its predecessor */
        if (i != 0 && temp[i].y >= T[k-1].y) break;
         
        T[k] = temp[i];
        T[k].src1 = -1;  T[k].src2 = -1; 

        if (i != 0)
        {
          if (T[k].x <= T[k-1].x)
          {  /* now that we sort, should always be true */
            fprintf(stderr,"Graph entry %d\n",(int)vv);
            fail("x shape dimensions should increase");
          }
          /* can still be messed up by bad shape functions */
          if (T[k].y >= T[k-1].y)
          {
            fprintf(stderr,"Graph entry %d\n",(int)vv);
            fail("y shape dims should decrease");
          }
        }
        if (++k >= B)
        {
         fprintf(stderr,"Shape fcn with more than %d entries; extras ignored",
            B);
         goto doublebreak;  /* break out of 2 levels */
        }
      }
   }
doublebreak:  ;
}

/* probably should be reinitialized somewhere. */
long xy_pad_total = 0;

void pad_T_entries(long xpad, long ypad)
{
  int i;

  if ( xpad==0 && ypad==0) return;

  xy_pad_total += (xpad+ypad);

  for (i=0; i < B; i++)
    if (T[i].x != EMPTY)
    {
       T[i].x += xpad;
       T[i].y += ypad;
    }
}


int min_area(void)
{
  int i,min_i= -10000;  /*  force a coredump if not set*/
  long minarea = HUGEL;

  for (i= 0; i < B; i++)
    if (T[i].x != EMPTY && T[i].x * T[i].y < minarea)
    {  minarea = T[i].x * T[i].y;
       min_i = i;
    }
  return min_i;
}



void size1(graph g, slice_tree t, int extL, int extR, int extT, int extB)
     /* int extL, extR, extT, extB are  flags for "on the exterior" */
{
  int newL1, newL2, newR1, newR2, newT1, newT2, newB1, newB2;
   if (t == NULL) return;

   if (t->type_is_cut)
   {
     /* cut, paste and rename from mcplace.  Externality of subrooms */
     /* This externality code added 30 Jan 2007 for tag clouds */
     if (t->direction) { /* vertical cut */
       newL1 = extL;
       newL2 = newR1 = 0;
       newR2 = extR;
       newT1 = newT2 = extT;
       newB1 = newB2 = extB;
     }
     else {
       newT1 = extT;
       newT2 = newB1 = 0;
       newB2 = extB;
       newL1 = newL2 = extL;
       newR1 = newR2 = extB;
     }

     /* internal cut; compute children and then this one */
     size1(g,t->child1, newL1, newR1, newT1, newB1);
     size1(g,t->child2, newL2, newR2, newT2, newB2);
     merge_fcns(t);  /* results in temporary (and private) table */
     t->xpad = t->ypad = 0;
   }
   else
   {  /* it is a leaf */
      /* load dimensions into temporary table */
      load_dimensions(g,t);
      t->xpad = t->ypad = 0;

      /* DL's CSS current uses a 2-pixel *left* margin (perhaps even on the external guy) */

      if (! extL) t->xpad = 2;  /* 2 matches DL's CSS */
   }

   pad_T_entries(t->xpad,t->ypad);  /* ensure padding info initialized */
   update_dims_from_T(t);  /* make the computed values official and shared */
#ifdef BLAB
   print_sizing_function(t);
#endif


}


void size(graph g, slice_tree t)
{ size1(g,t,1,1,1,1); /* all initially external */
}



void get_best(slice_tree t, long xmx, long ymx)
{
  int i;
  int best_index;
  float minarea;

 best_index = -1;
 minarea = 1e20;

  printf("Searching for solution meeting constraints (%ld %ld)\n",
     xmx, ymx);
       
 /* select the minimum area solution that meets xmx, ymx constraints */
 for (i = t->shape_fcn_start; i <= t->shape_fcn_end; i++)
 {
   printf("Examining (%ld %ld) area = %e\n",dims[i].x, dims[i].y,
           (float)dims[i].x * (float)dims[i].y);
   if (dims[i].x <= xmx && dims[i].y <= ymx)
     if (dims[i].x * dims[i].y <= minarea)
     { minarea = dims[i].x * (float)dims[i].y;
       best_index = i;
     }  
 }

 if (best_index == -1)
   {printf("Size limitations too tight; no solutions");
    best_index = t->shape_fcn_start;  /* choose first [skinniest] arbitrarily */
   }

 printf("Found best solution meeting constraints is (%ld %ld)\n",
   dims[best_index].x, dims[best_index].y);
   
 t->shape_fcn_chosen = best_index;
}

void top_down_size(slice_tree t)
{
   if (t->type_is_cut)
   {  /* set the choice functions for children */
      t->child1->shape_fcn_chosen = dims[ t->shape_fcn_chosen].src1;
      t->child2->shape_fcn_chosen = dims[ t->shape_fcn_chosen].src2;
      top_down_size(t->child1);
      top_down_size(t->child2);
   }   
}

void init_before_sizing(void)
{ free_entry = 0;
}

/* returns the max dimension */
long size_slicing_structure(graph g, slice_tree t, long max_x_ok, long max_y_ok)
{  int chosen_index;
   long maxd;

   size(g,t);
   get_best(t,max_x_ok, max_y_ok);
   top_down_size(t);
   chosen_index = t->shape_fcn_chosen;
   maxd = MAX( dims[chosen_index].x, dims[chosen_index].y);
   return maxd;
}


/* Another Jan 2007 attempt; this one tries to centre leaves   */
/* in their owned room. Similar to how CSS now renders things  */

void place(slice_tree t, long xstart, long ystart, long xlimit, long ylimit)
{
   if (t == NULL) return;

   if (!t->type_is_cut) { /* centre this leaf */
     t->xposition = xstart + (xlimit - xstart - dims[t->shape_fcn_chosen].x)/2;
     t->yposition = ystart + (ylimit - ystart - dims[t->shape_fcn_chosen].y)/2;
   }
   else {
     /* I think that padding may have been added? */
     int x1 = dims[t->child1->shape_fcn_chosen].x;
     int y1 = dims[t->child1->shape_fcn_chosen].y;
     int x2 = dims[t->child2->shape_fcn_chosen].x;
     int y2 = dims[t->child2->shape_fcn_chosen].y;
     
     if (t->direction) { /* vertical cut */
       int surplus = xlimit - (xstart+x1+x2);
       int xstart2 =  xstart + x1 + surplus/2;
       place(t->child1,xstart,ystart,xstart2,ylimit);
       place(t->child2,xstart2,ystart,xlimit,ylimit);
     }
     else {
       int surplus = ylimit - (ystart+y1+y2);
       int ystart2 =  ystart + y1 + surplus/2;
       place(t->child1,xstart,ystart,xlimit,ystart2);
       place(t->child2,xstart,ystart2,xlimit,ylimit);
     }
   }
}


void say_leaf( graph g, float sc, int v, 
               long xpos, long ypos, long w, long h, FILE *stream) {
  char *pc;
  fprintf(stream,"\\put (%ld,%ld) {\\framebox (%ld,%ld) {",
          (long) (xpos * sc),
          (long) (ypos * sc), 
          (long) (w * sc),
          (long) (h * sc));
  
  for (pc = g->vertices[v].original_name_string; *pc != '\0'; pc++)
    {
      if (*pc == '_')
        putc('\\',stream);
      putc(*pc,stream);
    }
  fprintf(stream,"}}\n");
}




/* width of paper */
#define MAXPOINTS  400.0   

void latex_say(FILE *stream, slice_tree t,graph g, float sc)
{ 

  if (t->type_is_cut)
  { /* fprintf(stream,"\\put (%ld,%ld) {\\dashbox (%ld,%ld) {}}\n",
        (long)( t->xposition * sc),
        (long) (t->yposition * sc), 
        (long) (dims[t->shape_fcn_chosen].x * sc),
        (long) (dims[t->shape_fcn_chosen].y * sc)); */
     latex_say(stream,t->child1,g,sc);
     latex_say(stream,t->child2,g,sc);
  }
  else
  {
    say_leaf( g, sc, t->name, t->xposition, t->yposition,
              dims[t->shape_fcn_chosen].x,
              dims[t->shape_fcn_chosen].y, stream);
 }
}

/* this alternative cannot show slicing structure but */
/* can handle general layouts                         */

void print_latex_picture(graph g, float x[], float y[],
                         int xmax, int ymax, FILE *stream){
  int i;
  float sc = MAXPOINTS / (xmax > ymax ? xmax : ymax);

  fprintf(stream,"\\setlength{\\unitlength}{%10.5fpt}\n",sc); 

  fprintf(stream,"\\begin{picture}(%ld,%ld)\n",(long)xmax,(long)ymax);

  for (i = 0; i < g->n; ++i) {
    say_leaf( g, /* sc */ 1.0, i, x[i], y[i], 
              basic_shapes[ g->vertices[i].size_start_index + 1].x,
              basic_shapes[ g->vertices[i].size_start_index + 1].y,
              stream);
  }
  fprintf(stream,"\\end{picture}\n");
}

/* assume that placements have already been done, and we know */

void output_latex_file(FILE *stream, slice_tree t,long maxdimension,graph g)
{ float scalefactor = MAXPOINTS/maxdimension;

  fprintf(stream,"\\documentclass{article}\n");
  fprintf(stream,"\\begin{document}\n");
  fprintf(stream,"\\setlength{\\oddsidemargin}{-0.8in}\n");
  fprintf(stream,"\\setlength{\\unitlength}{%10.5fpt}\n",scalefactor); 
  fprintf(stream,"\\footnotesize\n");
  fprintf(stream,"\\begin{picture}(%ld,%ld)\n",(long)maxdimension,(long)maxdimension);
  /* fprintf(stream,"\\thicklines\n"); */
  latex_say(stream,t,g,/* scalefactor*/ 1.0);
  fprintf(stream,"\\end{picture}\\newpage\n");
}

void finish_latex_file(FILE *stream) {
  fprintf(stream,"\\end{document}\n");
}

/* Jan 07: eval quality of placement wrt "wirelength" */

void unpack_locations(slice_tree t, float *x, float *y) 
{
  if (!t) return;
  if (t->type_is_cut) {
    unpack_locations(t->child1, x, y);
    unpack_locations(t->child2, x, y);
  } else {
    x[t->name] =   t->xposition;
    y[t->name] =   t->yposition;
  }
}

float evaluate_placed_solution( graph g, float x[], float y[], double k)
{
  float badsum = 0.0;
  int i;
  adjlist a;

  // k=infinity would require special case processing....


  for (i = 0; i < g->n; ++i) {
    float x1 = x[i];
    float y1 = y[i];
#ifdef BLAB
    printf("From %f %f\n",x1,y1);
#endif
    for (a = g->adjacencies[i]; a != NULL; a = a->next) {
      float xdelta = fabs(x1 - x[a->nbr]); 
      float ydelta = fabs(y1 - y[a->nbr]);
#ifdef BLAB
      printf("edge %d %d has deltas %g %g and weight %d ", i, (int) a->nbr, xdelta, ydelta, (int) a->w); 
#endif
      
      float dist = pow(  pow(xdelta, k) + pow(ydelta, k), 1.0/k);
      
      // Equation 1 in paper says to add twice...
        badsum += ( a->w * dist);  /* weighted distance */
#ifdef BLAB
      printf(" dist is %g\n",dist);
#endif
    }
  }
  return badsum;
}         



/* discover the weighted wirelength, under the L_k distance, k >= 1. You must fake infinity */
float evaluate_sized_solution(graph g, slice_tree t, double k)
{
  float badsum, *x, *y;

  /* unpack coordinates */
  x = (float *) calloc(g->n, sizeof(*x));
  y = (float *) calloc(g->n, sizeof(*y));
  
  unpack_locations(t,x,y);

  badsum = evaluate_placed_solution(g,x,y,k);

  free(x);
  free(y);
  return badsum;
}

