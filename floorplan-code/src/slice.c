/* file: slice.c                                              */

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


#include "floorp.h"
#include "graph.h"  /* includes slice.h */


slice_tree make_slice_leaf( vertex name)
{  slice_tree temp = STT my_malloc( sizeof(*temp));

   temp->name = name;
   temp->type_is_cut = temp->cutsize = 0;
   temp->child1 = temp->child2 = NULL;
   return temp;
}


slice_tree make_slice_node( char cut_is_vertical, slice_tree child1, slice_tree child2, maxgain_type cutsz)
{  slice_tree temp = STT my_malloc( sizeof(*temp));

   temp->direction = cut_is_vertical;
   temp->type_is_cut = 1;
   temp->name = 0;
   temp->child1 = child1;
   temp->child2 = child2;
   temp->cutsize = cutsz;
   return temp;
}

void print_slice_tree(FILE *stream, slice_tree t)
{
   if (t->child1 == NULL)
   {
      fprintf(stream,"Leaf %d is vertex %d\n", (int) t->traversal_id,
                (int)t->name);
   }
   else
   {  fprintf(stream,"%s cut (cost %d) %d with children %d %d\n",
             (t->direction) ? "vertical" : "horizontal",
             (int) t->cutsize,
             (int) t->traversal_id, (int)(t->child1->traversal_id),
             (int)(t->child2->traversal_id));
      fprintf(stream,"  xpad is %ld and ypad is %ld\n",
           (long)t->xpad, (long)t->ypad);

      print_slice_tree(stream, t->child1);
      print_slice_tree(stream, t->child2);
   }
}
 
vertex serial_number;

void number_slice_tre(slice_tree t)  /* traversal type really doesnt matter */
{
   if (t==NULL) return;

   t->traversal_id = ++serial_number;
   number_slice_tre(t->child1);
   number_slice_tre(t->child2);
}


void number_slice_tree(slice_tree t)
{
  serial_number = 0;
  number_slice_tre(t);
}

void slice_tree_print(slice_tree t)
{
   serial_number = 0;
   number_slice_tree(t);
   print_slice_tree(stdout,t);
}

void output_slice_tree(slice_tree t, FILE *stream)
{  
   if (t== NULL) return;
      
   fprintf(stream,"%c %c %d %d %d\n",
      (t->type_is_cut) ? 'C' : 'L',
      (t->direction) ? 'V' : 'H',
      (int) t->name,
      (int) t->traversal_id,
      (int) ((t->type_is_cut) ? t->cutsize : 0));
       

   if (t->type_is_cut)
   {
      output_slice_tree(t->child1,stream);
      output_slice_tree(t->child2,stream);
   }
}


slice_tree input_slice_tree(FILE *stream)
{
   char leaf_or_cut, cut_dir;
   int cell_name, nm;
   long cz;
   
   slice_tree t = STT my_malloc( sizeof(*t));

   fscanf(stream,"%c %c %d %d %ld\n",&leaf_or_cut,&cut_dir,&cell_name,&nm,&cz);
   t->traversal_id = nm;

   if (leaf_or_cut == 'L')  /* leaf */
   {  t->name = cell_name;
      t->type_is_cut = 0;
      t->direction = 0;
      t->cutsize = 0;
   }
   else
     if (leaf_or_cut == 'C') /* cut */
     {
       t->type_is_cut = 1;
       t->cutsize = cz;
       t->name = 0;
       t->direction = (cut_dir == 'V');  /* else 'H' gives dir=0 */
       t->child1 = input_slice_tree(stream);
       t->child2 = input_slice_tree(stream);
     }
     else
       fail("Slice tree data file is messed up");

   return t;
}

