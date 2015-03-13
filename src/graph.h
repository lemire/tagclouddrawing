/* file: graph.h                                                 */

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


#include <stdio.h>

#include "slice.h"

#ifdef GRAPH 
#define EXTERN
#else
#define EXTERN extern
#endif

#define N        2500 /* 2500 */        /* Maximum graph nodes  */

typedef struct adjlist_node {
   vertex nbr;
   edge_weight_type w;
   float fw;  /* used only before rounding */
   struct adjlist_node *next; } *adjlist;

typedef struct vtxlist_node {
    vertex v;
    struct vtxlist_node *next;} *vtxlist;

EXTERN adjlist *a;


#define BASESHAPES     N*5

typedef struct shapenode
{ long x,y;
  int src1,src2;
} shapeentry;


EXTERN shapeentry basic_shapes[ BASESHAPES];
EXTERN int bs_index;  


typedef struct bucket_node {
   vertex v;
   struct bucket_node *flink, *blink;  /* doubly linked */
                           } *bucket_list;

struct vertex_info {
   char temp_side;  /* 0=A or 1=B : A is left or top */
   char side;       /* 0 or 1 */
   char best_side;
   char locked;     /* 0 or 1 */
   maxgain_type vert_pull, horiz_pull;  /* positive is up or left */
   maxgain_type D;
   bucket_list bucket_entry;
   vertex_weight_type w;
   vertex original_name;
   char *original_name_string;
   int size_start_index, size_end_index;
                    };

EXTERN struct vertex_info *vi;  /* a direct ptr to the global graph */

typedef struct Graph {
   vertex n,first_dummy;
   struct vertex_info *vertices;  /* array I guess */
   adjlist *adjacencies;  /* also an array of lists */
                     } *graph;

EXTERN graph globgr;

void *my_malloc();
void  *my_shmalloc();
#define AJL (adjlist)
#define AJLL (adjlist *)
#define BL (bucket_list)
#define BLL (bucket_list *)
#define GPH (graph)
#define STT (slice_tree)
#define VTL (vtxlist)
#define VTLP (vtxlist *)
#define VTXP (struct vertex_info *)
#define NTP (net *)
#define VNI (vni)
#define VNIP (vni *)

void fail(char *);
void init_a(void);
void init_a1(vertex, graph);
void my_free(void *);
void dispose_adjlist(adjlist);
void add_adjacency(adjlist *, vertex, edge_weight_type);
void increase_adjacency(adjlist *, vertex, float);
void add_vertex(graph, vertex, vertex_weight_type, long, long, char *);
graph get_graph(char *);
graph random_graph(int );
void print_graph1(FILE *, graph);
void print_side(FILE *, graph);
void dispose_graph(graph);
void make_globgr(void);

#undef EXTERN
