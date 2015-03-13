/* file: mincut.c                                              */

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



/* This file contains code for the modified Fiduccia-Mattheyses */
/* bipartitioning algorithm.  Begun with algorithm description  */
/* in Lengauer 1990, simplified version that operates on graphs */
/* and not hypergraphs.                                         */
/* Extra heuristic: among min cost sol'ns, prefer more balanced */
/* I also modified it so that it cannot put all nodes in one    */
/* partition.  This could happen before, if there were one      */
/* very weighty node, relative to total weight.                 */
/* This file contains bucket compression.  D-values in a range  */
/* are put into the same bucket.  Currently we have a constant  */
/* amount of compression (up to 20 or so OK on random graphs)   */
/* but the conjecture is that a small fixed number (maybe 50)   */
/* would be plenty.  Issue then is how to hash values to bucket */
/* Intuitively, there should be many buckets for the smaller    */
/* D-values, and only a few for the biggies.                    */

/* Written by Owen Kaser, July 30, 1991                         */
/* Exhaust     Oct.  8, 1991 by OFK                             */


/* Note: there are presumably good open-source implementations of */
/* graph bipartitioning that could be dropped in, to replace this.*/
/* Much work in the VLSI community, for instance, see "ratio cut" */
/* OFK, 11 May 2007                                               */


#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

struct timeval tp;


#define EXHAUST
#define EXH_TOOBIG 13 

#include "floorp.h"
#include "graph.h"  /* also gets slice.h */
#include "mincut.h"

#define BCR      1  /* Bucket Compression Ratio */

#define FMTRIALS 10

lognode logg[N];
vertex log_end; 
vertex log_best;

partition_cost_type initial_cost;

partition_cost_type mincost;
vertex_weight_type best_balance_at_mincost,initial_balance;
maxgain_type max_weighted_degree;
vertex_weight_type max_vtx_weight;
maxgain_type maxgain_a, maxgain_b ,gain_offset;
bucket_list *bucket_a, *bucket_b;   /* dynamically allocated arrays */

/* a few "forward decls" */
void print_Ds(void);
void remove_bucket(vertex, maxgain_type, bucket_list *, maxgain_type *);
void add_bucket(vertex,  maxgain_type, bucket_list *, maxgain_type *);
void print_partition(graph);

void get_W(void)
{

  /* static int warnedem = 0; / * Jan 07 hackery warning */
   vertex vv;
   vertex_weight_type weightsum = 0;

   max_vtx_weight = 0;

   for (vv = 0; vv < globgr->first_dummy; vv++)
   {
      if (vi[vv].w > max_vtx_weight)   max_vtx_weight = vi[vv].w;
      weightsum += vi[vv].w;
   }

   
   W = weightsum/2 + max_vtx_weight;  

   /* Jan 2007: loosening the constraint might lead to better clustering, but
    * a few examples show a massive increase in white space
    *
    * probably not worthwhile 
    *

   W = 3*weightsum/4 + max_vtx_weight;  // resetting
   if (!warnedem) {
     printf("WARNING hacking at get_W, fixme\n");
     warnedem=1;
   }
   */

   /* modified Aug 3, 1991 to forbid empty partitions */
   if (W >= weightsum)
     W = weightsum - 1;
}


void init(void )
{

   /* dynamically allocate and initialize the buckets */
   bucket_a =  BLL my_malloc( sizeof(bucket_list) *
                                           (2*max_weighted_degree/BCR + 1)
                                         );
   bucket_b = BLL  my_malloc( sizeof(bucket_list) *
                                           (2*max_weighted_degree/BCR + 1)
                                         );
   gain_offset = max_weighted_degree;  /* not div by BCR */
   get_W();

}


/* dispose of just the entries of the queue, not the array of ptrs */


void dispose_priority_q(void)
{
   bucket_list bl,blnext;
   int i;

   for (i = 0; i <= 2*max_weighted_degree/BCR; i++)
   {
      for (bl = bucket_a[i]; bl != NULL; bl = blnext)
      {
         blnext = bl->flink;
         my_free(bl);
      }

      for (bl = bucket_b[i]; bl != NULL; bl = blnext)
      {
         blnext = bl->flink;
         my_free(bl);
      }
   }
}


void print_sides(void)
{
  int i;

  for (i=0;  i < globgr->n; i++)
    printf("%s%d\n",vi[i].side?"    ":" ",i);

}

void  print_priority_q(void)
{
   bucket_list bl,blnext;
   int i;

   for (i = 0; i <= 2*max_weighted_degree/BCR; i++)
   {
      for (bl = bucket_a[i]; bl != NULL; bl = blnext)
      {
         blnext = bl->flink;
         printf("A %d have %d\n",i,bl->v);
      }

      for (bl = bucket_b[i]; bl != NULL; bl = blnext)
      {
         blnext = bl->flink;
         printf("B %d have %d\n",i,bl->v);
      }
   }
}

void set_temp_side(void)
{ 
  vertex vv;

  for (vv = 0; vv < globgr->n; vv++)  /* the real vertices, dums dont matter */
    vi[vv].temp_side = vi[vv].side;
}


void compute_D_values(void)
{
   vertex vv;
   adjlist aa;

   WA = WB = 0;
   /* set all Di values to 0 */
   for (vv = 0; vv < globgr->first_dummy; vv++)  /* the real vertices, dums dont matter */
   {
     vi[vv].D = 0 + gain_offset;
     if (vi[vv].temp_side)
       WB += vi[vv].w;
     else
       WA += vi[vv].w;
   }

   /* vertices are in their partitions as indicated by 'side' field */
   /* and during processing, we use 'temp_side'.                    */

   /* scan all edges, and update D's at each edge (only count edges once) */
   for (vv = 0; vv < globgr->n; vv++)
     for (aa = a[vv]; aa != NULL; aa = aa->next)
       if (aa->nbr < vv)
       {
         if (vi[vv].temp_side == vi[aa->nbr].temp_side)
         {  /* same partition now, discourage their swap */
            vi[vv].D -= aa->w;
            vi[ aa->nbr].D -= aa->w;
         }
         else
         {  /* diff partition now, encourage their swap */
            vi[vv].D += aa->w;
            vi[ aa->nbr].D += aa->w;
            /* this adds to partition cost */
            mincost += aa->w;
         }
       }
}




void unlock_em(void)
{  int i;

  for (i=0; i < globgr->first_dummy; i++)
  {  vi[i].locked = 0;}
}


void init_D_values_and_buckets(void)
{
   bucket_list temp, *bl1 = bucket_a,*bl2 = bucket_b;
   maxgain_type i,this;
   vertex vv;

   set_temp_side();
   unlock_em();
   compute_D_values();

   /* make all lists null */
   for (i=0; i <= 2*max_weighted_degree/BCR; i++,bl1++,bl2++)
     *bl1 = *bl2 = NULL;

   maxgain_a = maxgain_b = 0;

   /* now put elements in the appropriate lists */
   for (vv = 0; vv < globgr->first_dummy; vv++)  /* the real vertices, dums dont matter */
   {
      this = vi[vv].D / BCR;
      temp = BL my_malloc( sizeof( *temp));
      vi[vv].bucket_entry = temp;
      temp->v = vv;
      if (vi[vv].temp_side)
      {
         /* bucket B */
         if (maxgain_b < this)  {
            maxgain_b = this;
                                }
#ifdef BLAB
  printf("adding %d to bucket_b %d\n",vv, (int) this);
#endif
         temp->flink = bucket_b[this];
         if (bucket_b[this] != NULL)
           bucket_b[this]->blink = temp;
         bucket_b[this] = temp;
         temp->blink = NULL;
      }
      else
      {
         /* bucket a */
         if (maxgain_a < this){
            maxgain_a = this;
                              }
#ifdef BLAB
  printf("adding %d to bucket_a%d\n",vv,(int)this);
#endif
         temp->flink = bucket_a[this];
         if (bucket_a[this] != NULL)
           bucket_a[this]->blink = temp;
         bucket_a[this] = temp;
         temp->blink = NULL;
      }
   }
   best_balance_at_mincost = MAX(WA,WB);
}


void remove_bucket(vertex v,maxgain_type indx, bucket_list *strct, maxgain_type *one_maxgain)
{

   bucket_list my_bucket = vi[v].bucket_entry;

   if ( strct[indx] == my_bucket)
   {
      strct[indx] = my_bucket->flink;  /* might be null */
      if (strct[indx] != NULL)
        strct[indx]->blink = NULL;

      /* if this bucket has become empty, scan for next */
      /* if this was the maxgain bucket                 */
      if (indx == *one_maxgain && strct[indx] == NULL)
      {
        while (indx > 0 && strct[--indx] == NULL)
          continue;  /* will always stop at 0, even if 0 empty */
        *one_maxgain = indx;
      }
   }
   else  /* it has a predecessor */
   {  my_bucket->blink->flink = my_bucket->flink;  /* may be NULL */
      if (my_bucket->flink != NULL)  {
        my_bucket->flink->blink = my_bucket->blink;  /* not null */ }
   }
}


/* does not fiddle with v at all */

void add_bucket(vertex v, maxgain_type indx, bucket_list *strct, maxgain_type *one_maxgain)  /* bucket already exists */
{
   bucket_list my_bucket = vi[v].bucket_entry;

   if (*one_maxgain < indx) *one_maxgain = indx;

   my_bucket->blink = NULL;
   my_bucket->flink = strct[indx];
   if (strct[indx] != NULL)
     strct[indx]->blink = my_bucket;
   strct[indx] = my_bucket;
}



/* destroys its bucket, so make sure we're done with it */

void lock(vertex v)
{
   /* now, its side may, or may not, have been swapped.  Thus */
   /* temp_side may not reflect the structure it was in. But  */
   /* the side field is accurate.                             */

   vi[v].locked = 1;

   remove_bucket(v, vi[v].D/BCR,(vi[v].side) ? bucket_b : bucket_a,
                            (vi[v].side) ? &maxgain_b : &maxgain_a
                );
   my_free( vi[v].bucket_entry);
   vi[v].bucket_entry = NULL;
}

int end_of_pass;

vertex select_unlocked(void)
{
   vertex va=0,vb=0;
   char no_va, no_vb;
   vertex_weight_type t1,tt1,t2,tt2;
   vertex choice;

   while (1)
   {
     no_va = bucket_a[maxgain_a] == NULL;
     no_vb = bucket_b[maxgain_b] == NULL;

     if ( no_va && no_vb)
     {
        end_of_pass = 1;  return(0);
     }

     if (!no_va) va = bucket_a[ maxgain_a]->v;
     if (!no_vb) vb = bucket_b[ maxgain_b]->v;

     if (!no_va && !no_vb)  /* both */
     {
        /* can just one of them be moved? */
        if (WA + vi[vb].w > W)
          choice = va;  /* since we can't choose vb */
        else
          if (WB + vi[va].w > W)
            choice = vb;  /* similar */
          else  /* could choose either */
          {
             if (maxgain_a > maxgain_b)
               choice = va;
             else
               if (maxgain_b > maxgain_a)
                 choice = vb;
               else  /* equal gains, prefer more balanced partitions */
               {  t1 = WA + vi[vb].w; tt1 = WB - vi[vb].w;
                  t2 = WB + vi[va].w; tt2 = WA - vi[va].w;
                  t1 = MAX(t1,tt1);
                  t2 = MAX(t2,tt2);
                  if (t1 > t2) /* bad idea to move vb */
                    choice = vb;
                  else
                    choice = va;
               }
          }
          return(choice);
     }

     /* only one of these things exists */

     if (no_va) {
       if (WA + vi[vb].w < W)
         return vb;
       else
         lock(vb);  /* on its own side? */
		}

     if (no_vb) {
       if (WB + vi[va].w < W)
         return va;
       else
         lock(va);  /* on its own side? */
		}
   }  /* end of while */
}




void swap_and_update(vertex vv)
{
  int v;
  adjlist aa;
  bucket_list *p_bk;
  maxgain_type *p_mg;

   /* update all except for the log, when swap is done */

#ifdef BLAB
  printf("Swap and update vertex %d\n",vv);
#endif

   if ((vi[vv].temp_side = (1-vi[vv].temp_side)))
   {  /* it moves to side B */
      WB += vi[vv].w;  WA -= vi[vv].w;  /* update weights */
   }
   else /* it moves to side A */
   {
      WA += vi[vv].w;  WB -= vi[vv].w;  /* update weights */
   }

   /* for all unlocked vertices adjacent to vv, update their D values */
   for (aa = a[vv]; aa != NULL; aa = aa->next)
     if (!vi[aa->nbr].locked)
     {
        v = aa->nbr;

        if (v == vv)
        { printf("oops1, self loop seen vtx %d, graph is\n",v);
          print_graph1(stderr,globgr); exit(1);
         }

        if (vi[v].temp_side) {
          p_bk = bucket_b;
          p_mg = &maxgain_b; }
        else
        { p_bk = bucket_a;
          p_mg = &maxgain_a;
        }
        remove_bucket(v, vi[v].D/BCR,p_bk,p_mg);

       if (vi[v].temp_side == vi[vv].temp_side)
          vi[v].D -= (2*aa->w);  /* now on same side, happier here */
       else
          vi[v].D += (2*aa->w);  /* now on diff sides, unhappier */

        add_bucket(v, vi[v].D/BCR,p_bk,p_mg);
     }  /* end "for all unlocked nbrs */
   lock(vv);  /* also removes its entry from buckets */

}


void init_log(partition_cost_type startcost, vertex_weight_type startbal)
{
   initial_cost = startcost;
   initial_balance = startbal;
   log_best = 0; log_end = 0;
}

void enter_log(vertex v, partition_cost_type cost, vertex_weight_type bal)
{  lognode *temp;

   assert( log_end < N-1);

   temp = logg + log_end++;
   temp->vtx = v;
   temp->cost = cost;
   temp->balance = bal;
   
   /* for enter_log_multiply it's important that for all of the */
   /* values entered multiply, with the best cost/balance, that */
   /* we make log_best point to the LAST of them                */
   
   if ( cost < logg[log_best].cost ||
        (cost==logg[log_best].cost  &&  bal <= logg[log_best].balance)
      ) 
   {
      log_best = log_end-1;  
   }
}



void enter_log_multiply(vbase,start,end,cost,bal)
vertex vbase[];
vertex start,end;
partition_cost_type cost;
vertex_weight_type bal;

{  vertex i;

   if (end == (vertex) -1) return;

   for (i=start; i <= end; i++)
     enter_log(vbase[i],cost,bal);
}


int use_min(void)  /* do what the log says, until mincost reached */
{
   lognode *temp;
   vertex i;

   if (initial_cost == mincost && initial_balance == best_balance_at_mincost )
      return 0;  /* no improvement */

   for (i = 0,temp = logg; i < log_end; temp++,i++)
   {
      /* swap specified vertex */
      vi[temp->vtx].side = 1 - vi[temp->vtx].side;

      if (temp->cost == mincost && temp->balance == best_balance_at_mincost)
        return 1;
   }
   fprintf(stderr,"BugReport: final swap condition %ld, %ld not met\n",
       (long)mincost, (long)best_balance_at_mincost);
   exit(1);
   return 1;
}




int FM_pass(void)  /* return 1 if it made an improvement, 0 else */
{
   vertex vv;
   partition_cost_type cost;
   int result;
   vertex_weight_type current_balance; /* use max size of parts for this */

   mincost = 0;
   init_D_values_and_buckets();
#ifdef BLAB
   printf("Start of FM pass, cut cost is %d\n",(int)mincost); 
#endif

   init_log(mincost,best_balance_at_mincost);
   cost = mincost;

   end_of_pass = 0;
   do
   {
#ifdef BLAB
   print_Ds();
#endif

      vv = select_unlocked();
      if (end_of_pass) break;

      cost -= (vi[vv].D - gain_offset);
#ifdef BLAB
printf("***** cost -> %d after following swap\n",(int) cost);
#endif
      swap_and_update(vv);


      current_balance = MAX(WA,WB);
      if (cost < mincost)
      { mincost = cost;
        best_balance_at_mincost = current_balance;
      }
      else
        if (cost == mincost && best_balance_at_mincost > current_balance)
          best_balance_at_mincost = current_balance;  /* minimize the max */

      enter_log(vv,cost,current_balance);
   }
   while (1);

   result = use_min();  /* 1 unless no improvement */

#ifdef BLAB
   printf("end of FM_pass, next will begin with %d\n",(int)mincost);
#endif

   dispose_priority_q();
   return result;
}




void FM(void)
{
  init();

  while (FM_pass())
    continue;

  my_free(bucket_a);
  my_free(bucket_b);

  return ;
}

/* divide up initial partition, based on approx equal weights */

/* this forms an initial partition with based on equal num of swappables */
/* now: dumb quick hack.  */

vertex degree_calc(int i, maxgain_type *weighted_degree)
{
vertex degree;
adjlist t;

/* count degree and weighted degree*/
degree = 0;
*weighted_degree = 0;
for (t = a[i]; t != NULL; t = t->next)
{
  (*weighted_degree) += t->w;
  degree++;
}
if (*weighted_degree > max_weighted_degree)
  max_weighted_degree = *weighted_degree;

return degree;
}

struct permutation_struct {
  vertex v;
  int key;                };

int ps_compare( const void* ppps1, const void *ppps2)
{ 
  const struct permutation_struct *pps1 = ppps1;
  const struct permutation_struct *pps2 = ppps2;
 
  if (pps1->key < pps2->key) return -1;
  return pps1->key != pps2->key;
}


void initial_p4artition(void)
{
   vertex_weight_type totalsize,tsize2;
   vertex i,degree;
   maxgain_type weighted_degree;
   
   struct permutation_struct sigma[N];

   /* first, construct the permutation sigma */

   for (i=0; i < globgr->first_dummy; i++)
   {
      sigma[i].v = i;
      sigma[i].key = rand();
   }

  /* now get the permutation by sorting on key, from 0 .. first_dummy-1 */

  qsort(sigma,globgr->first_dummy,sizeof(struct permutation_struct),
        ps_compare);


   max_weighted_degree = 0;

   /* first pass, compute total size */
   totalsize = 0;  tsize2 = 0;

   for (i = 0; i < globgr->n; i++)
   {
      /* count degree and weighted degree*/

      degree = degree_calc(i,&weighted_degree);
      totalsize += vi[i].w;
   }

#ifdef BLAB
   printf("initial partition: totalsize is %d\n",(int) totalsize);
#endif

   /* second pass: assign first half to partition a, rest to b */
   /* use the random permutation here.                         */

   vi[sigma[0].v].side = 0;
   tsize2 = vi[sigma[0].v].w;

   for (i = 1; i < globgr->first_dummy; i++)
   {
      vertex_weight_type tempsize = tsize2 + vi[sigma[i].v].w;

      if (tempsize <= totalsize/2) {
        vi[sigma[i].v].side = 0;
        tsize2 = tempsize;
      } else vi[sigma[i].v].side = 1;

#ifdef BLAB
      printf("since tempsize is %d I set node %d to side %d\n",
             (int) tempsize, (int) sigma[i].v, (int) vi[sigma[i].v].side);
#endif
   }

}




/* put the info in this graph structure into the global graph vars */
/* (very much pre-object-oriented cruft (May 2007))  */


void unpack(graph g, char cutdir)
/* cutdir: 1 = vertical; 0 = horizontal */
{
   int i;
   long pull;
   vertex n1;


   n1 = globgr->n = g->n; globgr->first_dummy = g->first_dummy;

   init_a();  /* also NULL-ifies entries beyond n */

   for (i = 0; i < globgr->n; i++)
   { 
     {
       a[i] = g->adjacencies[i];  
       vi[i] = g->vertices[i];

       /* make extra dummies for nodes with nonzero pulls in the right dir */
       if (cutdir)
         pull = vi[i].horiz_pull;
       else
         pull = vi[i].vert_pull;

       if (pull) 
       {  /* make a dummy vertex for this */

          vi[n1].temp_side = vi[n1].side = (pull < 0);
          vi[n1].locked = 1;
          vi[n1].vert_pull = vi[n1].horiz_pull = vi[n1].D = 0;
          vi[n1].bucket_entry = NULL;
          vi[n1].w = 0;
          vi[n1].original_name = 0;

          /* and make adjacency entries pulling the vertex to its dummy */
          add_adjacency( a+i, n1, (edge_weight_type) labs(pull));
          add_adjacency( a+n1, (vertex)i, (edge_weight_type) labs(pull));

          n1++;
          if (n1 >= N)
            fail("Too many vertices\n");
       }
     } /* new else blob */
   }
   globgr->n = n1;

}


/* construct two graph structures by scanning the current partition */
/* information.  Also, make sure that the external pull info is     */
/* updated.   Note: we do not copy dummies.                         */

/* also makes sure that WA and WB are correct                       */

maxgain_type construct(graph g1, graph g2, char dir)  /* returns the cut size */
{  int i;
   vertex newname,cur1,cur2, num_in_1= 0, num_in_2 = 0;
   vertex old2new[N];  /* translation table for vertx names */
   adjlist al,*alp;
   graph whichgraph;
   maxgain_type *whichpull;
   maxgain_type cutsz;

   WA = WB = 0;

   for (i = 0; i < globgr->first_dummy; i++)
     if (vi[i].side)
     {
       old2new[i] = num_in_2;
       num_in_2++;
       WB += vi[i].w;
     }
     else
     { old2new[i] = num_in_1;
       num_in_1++;
       WA += vi[i].w;
     }


   g1->n = g1->first_dummy = num_in_1;
   g2->n = g2->first_dummy = num_in_2;

   /* now, create the structures for g1 and g2 internals */
   g1->vertices = VTXP my_malloc( sizeof(*(g1->vertices)) * (g1->n));
   g2->vertices = VTXP my_malloc( sizeof(*(g2->vertices)) * (g2->n));

   g1->adjacencies = AJLL my_malloc( sizeof(*(g1->adjacencies)) * (g1->n));
   g2->adjacencies = AJLL my_malloc( sizeof(*(g2->adjacencies)) * (g2->n));

   cur1 = cur2 = 0;

   for (i = 0; i < globgr->first_dummy; i++)
   {
      if (vi[i].side)
      {
         g2->vertices[cur2] = vi[i];
         g2->vertices[cur2].locked = 0;
         cur2++;
      }
      else
      {
         g1->vertices[cur1] = vi[i];
         g1->vertices[cur1].locked = 0;
         cur1++;
      }
   }


   /* now, adjust the adjacency lists and external pulls.    */
   /* edges within an subpartition remain; others contribute */
   /* to the modification to the external pulls              */

   cutsz = 0;

   for (i=0; i < globgr->first_dummy; i++)
   {
      newname = old2new[i];
      whichgraph = (vi[i].side) ? g2 : g1;
      whichpull = (dir) ? & whichgraph->vertices[newname].horiz_pull
                        : & whichgraph->vertices[newname].vert_pull;

      alp = &(whichgraph->adjacencies[ newname]);
      *alp = NULL;

      for (al = a[i]; al != NULL; al = al->next)   /* all adj to i */
      {

         if (al->nbr < globgr->first_dummy)
         {
           if (vi[al->nbr].side == vi[i].side)
           {   /* this edge remains in the partitioned problem, if not */
               /* to a dummy.                                          */
                add_adjacency( alp, old2new[al->nbr], al->w);
           }
           else
           {  /* this edge crosses between parts- update external pulls */
              /* only do this once per edge, which appears twice        */
              if (i > al->nbr)  /* only do once */
	      {
                cutsz += al->w;

                if (vi[i].side)
                {  /* in partition B, being pulled toward A = a left/up pull */
                   *whichpull += al->w;
		   /* no problem converting al->w to long, since it is */
		   /* always positive                                  */
                }
                else
                {  /* in A, negative pull (toward B= left or down) */
                   *whichpull -= al->w;
                }
	      }
           }
         }
      } /* end "for all entries adjacent to i " */
   }
   return cutsz;
}


/* remember and recall routines for the best partitions from FM */

void save_best(void)
{
  vertex i;

  for (i=0; i < globgr->first_dummy; i++)
    vi[i].best_side = vi[i].side;
}


void restore_best(void)
{ vertex i;

  for (i=0; i < globgr->first_dummy; i++)
    vi[i].side = vi[i].best_side;
}


/* print D values */
void print_Ds(void)
   {  int i;

      for (i=0; i < globgr->n; i++)
        printf("%d %d\n",i , (int) vi[i].D);

   }




/* exhaustive uses a gray-code (or is it gray) scheme */

void exhaust_get_W(void)
{
   vertex vv;
   vertex_weight_type weightsum = 0;

   max_vtx_weight = 0;

   for (vv = 0; vv < globgr->first_dummy; vv++)
   {
      if (vi[vv].w > max_vtx_weight)   max_vtx_weight = vi[vv].w;
      weightsum += vi[vv].w;
   }

   W = MAX((weightsum*2)/3, max_vtx_weight+1);  /* arbit 2/3 */
   /* modified Aug 3, 1991 to forbid empty partitions */
   if (W >= weightsum)
     W = weightsum - 1;
/*  printf("Returning W as %ld\n",(long)W); */
}



/* routines: given integer, compute which gray-code bit */
/* has changed from the previous gray codeword          */


/* don't use exhaustively partitioning w. more than 32 bits */
/* even if box is REALLY fast and you're very patient       */

static unsigned long binary_bits = 0;

void gray_code_init(void)
{
     binary_bits = 0;
}


vertex gray_code_flips_next(void)
{
  static unsigned long bit_masks[32] = {
    0x1,       0x2,       0x4,       0x8,
    0x10,      0x20,      0x40,      0x80,
    0x100,     0x200,     0x400,     0x800,
    0x1000,    0x2000,    0x4000,    0x8000,
    0x10000,   0x20000,   0x40000,   0x80000,
    0x100000,  0x200000,  0x400000,  0x800000,
    0x1000000, 0x2000000, 0x4000000, 0x80000000,
    0x10000000,0x20000000,0x40000000,0x80000000};
   int bit;

   binary_bits++;
  
  for (bit=0; bit < 32; bit++)
    if (binary_bits & bit_masks[bit]) return bit;

  fail("Error in gray code flips next");
  return 0;  /* keep LINT happy */
}


void exhaust_swap(vertex vv)
{
   adjlist aa;
   vertex v;

   /* update all except for the log, when swap is done */

   if ( (vi[vv].side = vi[vv].temp_side = (1-vi[vv].side)))
   {  /* it moves to side B */
      WB += vi[vv].w;  WA -= vi[vv].w;  /* update weights */
   }
   else /* it moves to side A */
   {
      WA += vi[vv].w;  WB -= vi[vv].w;  /* update weights */
   }

   /* for all unlocked vertices adjacent to vv, update their D values */
   for (aa = a[vv]; aa != NULL; aa = aa->next)
   {
        v = aa->nbr;

        if (v == vv)
        { printf("oops2, self loop seen on %d, graph is\n",v);
        print_graph1(stderr,globgr); // exit(1);
         }

       if (vi[v].side == vi[vv].side) {
          vi[v].D -= (2*aa->w);  /* now on same side, happier here */
          vi[vv].D -= (2*aa->w);      }
       else {
          vi[v].D += (2*aa->w);  /* now on diff sides, unhappier */
          vi[vv].D += (2*aa->w);
            }

   }  /* end "for all nbrs */
}


void exhaust_init(void)
{
  gain_offset = 0;
  exhaust_get_W();
}


	
void exhaust(void)
{
   maxgain_type cost;
   long num_iterations = pow((double)2,(double)globgr->first_dummy) - 1;
   long it;
   vertex v;
   exhaust_init();

   mincost = 0;
   set_temp_side();
   compute_D_values();
#ifdef BLAB
   printf("initial mincost is %d\n",(int) mincost);
#endif

#ifdef BLAB
printf("At start, Ds are:\n"); print_Ds();
#endif

   unlock_em();
   
   cost = mincost;
   best_balance_at_mincost = MAX(WA,WB);
   
   gray_code_init();
   save_best();

   for (it = 0; it < num_iterations; it++)
   {

     v = gray_code_flips_next();

#ifdef BLAB
printf("flip %d\n",v);
#endif

     cost -= vi[v].D;

     exhaust_swap(v);

#ifdef BLAB
  if (!cost) {
    printf("Cost is 0; WA is %d WB is %d W is %d\n", (int) WA, (int) WB, (int) W);
  }
#endif

#ifdef BLAB
  if ( cost < mincost) printf("cost %d better than mincost\n",(int) cost);
  printf("MAX of WA and WB is %d\n", (int) (MAX(WA,WB)));
#endif

     if ( (cost < mincost && MAX(WA,WB) <= W) ||
          (cost == mincost && 
           MAX(WA,WB) < best_balance_at_mincost
          )
        )
     {  
       mincost = cost;
#ifdef BLAB
       printf("mincost now %d\n",(int)mincost); 
#endif
       best_balance_at_mincost = MAX(WA,WB);
       save_best();
     }
   }
   restore_best();   

#ifdef BLAB
printf("---------------------------------\n");
#endif

}




maxgain_type best_FM_cut;
vertex_weight_type best_FM_weight;

maxgain_type mincost_partition(graph g, char dir, graph gA, graph gB)  /* returns cutsize */
     /* dir: 1 = vertical cut; 0 = horizontal */
{

   int junk;
   unpack(g,dir);  /* input data */

   if (g->first_dummy < EXH_TOOBIG)
   {
      initial_p4artition();  /* random partition */
      exhaust();
   }
   else
   {
     best_FM_cut = 1000000;  /* huge number */

     for (junk=0; junk < FMTRIALS; junk++)
     {
         initial_p4artition();
         FM();
         if (mincost < best_FM_cut || (mincost == best_FM_cut &&
                              best_balance_at_mincost < best_FM_weight))
         {
           best_FM_cut = mincost;
           best_FM_weight = best_balance_at_mincost;
           save_best();
         }
     }

     restore_best();
   }

#if BLAB
    print_partition(g);
#endif

    return construct(gA,gB,dir);
}

void print_partition(graph g)
{
  int i;

  printf("[");
  for (i=0; i < g->n; i++)
    printf("%d , ",g->vertices[i].original_name);
  printf("]\n --> [");
  for (i=0; i < g->n; i++)
    if (!vi[i].side)
     printf("%d, ",g->vertices[i].original_name);
  printf("]\n + [");
  for (i=0; i < g->n; i++)
    if (vi[i].side)
     printf("%d, ",g->vertices[i].original_name);
  printf("]\n\n");

}

