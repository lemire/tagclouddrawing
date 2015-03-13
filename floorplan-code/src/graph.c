/* file: graph.c                                */

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


#include <string.h>
#include "floorp.h"
#include "graph.h"

int permutation[N];

/* utility fcn that doesn't really belong here */

void fail(char *msg)
{
   fprintf(stderr,"Failure: %s\n",msg);
   exit(1);
}


void init_a1(vertex nn,graph g)
{
   vertex i;
   for (i = 0; i < nn; i++)
   {
     g->adjacencies[i] = NULL;
   }
}


void init_a(void)
{
  init_a1(N,globgr);
}

void my_free(void *ptr)
{
  free(ptr);
}

void *my_malloc(int numbytes)
{ void *retval = malloc(numbytes);

  if (retval == NULL)
    fail("Out of memory");
  return retval;
}

int vtxlist_len(vtxlist vl)
{
  if (vl == NULL) return 0;
  else return 1 + vtxlist_len(vl->next);
}


void dispose_adjlist(adjlist al)
{  adjlist temp;

   for (; al; al = temp)
   {  temp = al->next;
      my_free(al);
   }   
}

void dispose_vtxlist(vtxlist vl)
{
  vtxlist temp,temp1;

  for (temp = vl; temp; temp = temp1)
  {  temp1 = temp->next;
     my_free(temp);
  }
}


void add_adjacency( adjlist *alptr, vertex nbrname, edge_weight_type edgeweight)
{ adjlist temp = AJL my_malloc(sizeof(*temp));
  temp->nbr = nbrname;
  temp->w = edgeweight;
  temp->next = *alptr;
  *alptr = temp;
}


void increase_adjacency( adjlist *alptr, vertex nbrname, float increment)
{ adjlist temp,temp1;
   
  /* first, scan to see if this adjacency already exists */
  
  for (temp1 = *alptr; temp1; temp1=temp1->next)
    if (temp1->nbr == nbrname) {
      break;                   }
  
  if (temp1==NULL)
  {  /* not adjacent yet */
     /* make adjacent */    
     temp = AJL my_malloc(sizeof(*temp));
     temp->nbr = nbrname;
     temp->w = 0;
     temp->fw = 0.0;
     temp->next = *alptr;
     *alptr = temp;
  }
  else
    temp = temp1;
      
  /* now increase it */
  temp->fw += increment;
  temp->w = temp->fw+1.0;  /* round up */    
}



void add_vertex(graph g, vertex i, vertex_weight_type vw, long lk, long ll, char *nm)
{
   g->vertices[i].original_name = i;
   g->vertices[i].vert_pull =  lk;
   g->vertices[i].horiz_pull = ll;
   g->vertices[i].w = vw;
   g->vertices[i].original_name_string = (char *) my_malloc( strlen(nm)+1);
   strcpy(g->vertices[i].original_name_string,nm);
}      



// nsl removed as penultimate item on lines

graph get_graph(char *fname)
{
   int i,e, h,nshapes; 
   vertex j,k;
   long lk,ll;
   vertex_weight_type vw;
   edge_weight_type ww;
   FILE *infile;

   char orig_name[100];
   graph g = GPH my_malloc( sizeof(*g));
#define MAX_XLTAB 5000
   int xlation_table[MAX_XLTAB];

   infile = fopen(fname,"r");
   if (infile == NULL)
      fail("Input file opening problem");

   /* 2nd item on line is unused, was "number of selfloops" */
   /* retained for compatibility with older data sets        */
   fscanf(infile,"%hd %*d\n",&g->n);
   g->first_dummy = g->n;
   bs_index =0;

   g->vertices = VTXP my_malloc( sizeof(*g->vertices) * g->n);
   g->adjacencies = AJLL my_malloc( sizeof(*g->adjacencies) * g->n);

   /* now read data for every vertex */
   for (i = 0; i < g->n; i++)
   {  fscanf(infile,"%hd %ld %ld %ld %d %*d %s\n",
         &j,&vw,&lk,&ll,&nshapes,orig_name);
   /* penultimate format item unused; was "number of selfloops for this vtx" */
      
      assert(j < MAX_XLTAB);

      xlation_table[j] = i;
      add_vertex(g,(vertex)i, vw, lk, ll, orig_name);

      g->vertices[i].size_start_index = bs_index;
      g->vertices[i].size_end_index = bs_index + nshapes - 1;

      for (h=0; h < nshapes; h++)
      {  fscanf(infile,"%ld %ld\n",&(basic_shapes[bs_index].x),
                                   &(basic_shapes[bs_index].y));
         if (++bs_index >= BASESHAPES)
           fail("Too many basic shapes specified, increase BASESHAPES");
      }
   }

   init_a1(g->n,g);

   /* now, read the number of edges */
   fscanf(infile,"%d\n",&e);

   for (i = 1; i <= e; i++)
   {  /* read all edges */
      fscanf(infile,"%hd %hd %hd\n",&j,&k,&ww); /* end1,2, and weight */
      j = xlation_table[j];
      k = xlation_table[k];

      add_adjacency( g->adjacencies+k, j, ww);
      add_adjacency( g->adjacencies+j, k, ww);
   }

   /* older data files now have a hyperedge count and     */
   /* then hyperedge info.  Was unused.  Now we just stop */
   /* reading...                                          */
   fclose(infile);
   return g;
}


graph random_graph(int nnodes)
{  int i,h,e;
   vertex j,k;
   edge_weight_type ww;
   vertex_weight_type vw;
   graph g = GPH my_malloc( sizeof(*g));
   int nshapes;
   long sqrshp;

   g->n = nnodes;
   g->first_dummy = g->n;

   g->vertices = VTXP my_malloc( sizeof(*g->vertices) * g->n);
   g->adjacencies = AJLL my_malloc( sizeof(*g->adjacencies) * g->n);

   bs_index =0;

   /* now make data for every vertex */
   for (i = 0; i < nnodes; i++)
   {
      vw = rand()%100+2;
      add_vertex(g,(vertex) i, vw, (long)0, (long)0, (char *) NULL);

      if (4+bs_index >= BASESHAPES)
         fail("Too many basic shapes specified, increase BASESHAPES");

      nshapes = 4;
      sqrshp = sqrt(g->vertices[i].w);

      g->vertices[i].size_start_index = bs_index;
      g->vertices[i].size_end_index = bs_index + nshapes - 1;

      basic_shapes[bs_index].x = (sqrshp/5)+1;
      basic_shapes[bs_index].y =
         1+(1.5*g->vertices[i].w)/basic_shapes[bs_index].x;
      bs_index++;
      basic_shapes[bs_index].x = (sqrshp == 1) ? sqrshp : sqrshp-1;
      basic_shapes[bs_index].y =
         1+(g->vertices[i].w)/basic_shapes[bs_index].x;
      bs_index++;
      basic_shapes[bs_index].x = basic_shapes[bs_index-1].y;
      basic_shapes[bs_index].y = basic_shapes[bs_index-1].x;
      bs_index++;
      basic_shapes[bs_index].x = basic_shapes[bs_index-3].y;
      basic_shapes[bs_index].y = basic_shapes[bs_index-3].x;
      bs_index++;

      /* make sure these x and y values are monotonically increasing/decr */
      for (h = -3 ; h != 0 ; h++)
      {  /* x values must monotonically increase */
         basic_shapes[bs_index+h].x =
           MAX( basic_shapes[bs_index+h-1].x + 1, basic_shapes[bs_index+h].x);
      }

      for (h = -2 ; h!= -5 ; h--)
      {  /* y values must monotonically decrease = increase going bkwds */
         basic_shapes[bs_index+h].y =
           MAX( basic_shapes[bs_index+h+1].y + 1, basic_shapes[bs_index+h].y);
      }

      if (i >= g->first_dummy) {
        g->vertices[i].w = 0;
        g->vertices[i].side = i % 2;
        g->vertices[i].locked = 1;
                        }
   }

   init_a1((vertex)nnodes,g);

   e = nnodes * 5;

   for (i = 1; i <= e; i++)
   {
      do {
        j = rand() % g->n; k = rand() % g->n;  /* Multiedges will be OK */
          }
      while (j ==k);

      ww =  rand() % 20;  /* weight */

      add_adjacency( g->adjacencies+k, j, ww);
      add_adjacency( g->adjacencies+j, k, ww);
   }
   return g;
}


void print_side(FILE *stream,graph g)
{  int i;
   
  fprintf(stream,"A: ");
  for (i=0; i < g->first_dummy; i++)
    if (!g->vertices[i].temp_side)
      fprintf(stream,"%d ",i);
  fprintf(stream,"\nB: ");
  for (i=0; i < g->first_dummy; i++)
    if (g->vertices[i].temp_side)
      fprintf(stream,"%d ",i);
  fprintf(stream,"\n");
}



void print_graph1(FILE *stream,graph g)
{
   int h,i,e; vertex j,k;
   edge_weight_type ww;
   adjlist temp;

   /* 0 to match the unused item mentioned in read_graph */
   fprintf(stream,"%hd 0\n",g->n);

   e = 0;
   /* now say data for every vertex */
   for (i = 0; i < g->n; i++)
   {  fprintf(stream,"%d %ld %d %d %d 0 %s\n",(int) i,g->vertices[i].w,
             (int)g->vertices[i].vert_pull,
             (int)g->vertices[i].horiz_pull,
             (int) (g->vertices[i].size_end_index -
                    g->vertices[i].size_start_index + 1
                    ),
                   g->vertices[i].original_name_string 
             );

      for (h = g->vertices[i].size_start_index;
           h <= g->vertices[i].size_end_index; h++)
      {  fprintf(stream,"%ld %ld\n",basic_shapes[h].x,basic_shapes[h].y);
      }                          

      for (temp = g->adjacencies[i]; temp; temp = temp->next)
        e++;
   }

   /* now, say the number of edges */

   fprintf(stream,"%d\n",e/2);

   for (i = 0; i < g->n; i++)
   {
      for (temp = g->adjacencies[i]; temp; temp = temp->next)
      {
         j = i; k = temp->nbr; ww = temp->w;
         if (j > k)
            fprintf(stream,"%hd %hd %hd\n",j,k,ww); /* end1,2, and weight */
      }
   }

   fprintf(stream,"0\n"); /* normally unused, but is a "hyperedge count"
                             for older versions of this software */
}


void dispose_graph(graph g)
{ int i;

  my_free(g->vertices);
  for (i = 0; i < g->n; i++)
     dispose_adjlist(g->adjacencies[i]);

  my_free(g->adjacencies);
  my_free(g);
}



/* crufty */
void make_globgr(void)
{
   globgr = GPH my_malloc( sizeof(*globgr));
   a = globgr->adjacencies = 
            AJLL my_malloc( N * sizeof(struct adjlist_node));
   vi = globgr->vertices = 
            VTXP my_malloc(N * sizeof(struct vertex_info));

   /* a and vi are convenient ways to access these parts of the glbl graph */
   globgr->n = N;
   globgr->first_dummy = N;
}
