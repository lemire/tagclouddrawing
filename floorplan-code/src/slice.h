/* file: slice.h                                              */

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



typedef unsigned short edge_weight_type;
typedef unsigned long  vertex_weight_type;
typedef /* signed */ long partition_cost_type;
typedef /* signed */ long maxgain_type;
typedef unsigned short vertex;


typedef struct slicetree_node {
    vertex name;  /* if a leaf */
    char type_is_cut; /* 0 = leaf */
    vertex traversal_id;
    char direction;  /* of cut, if an internal node */
    struct slicetree_node *child1, *child2;
    maxgain_type cutsize;
    int shape_fcn_start, shape_fcn_end;  /* indices into table */
    int shape_fcn_chosen;
    long xpad,ypad;
    long xposition,yposition;
} *slice_tree;


slice_tree make_slice_leaf(vertex);
slice_tree make_slice_node(char, slice_tree, slice_tree, maxgain_type);
void print_slice_tree(FILE *, slice_tree);
void number_slice_tree(slice_tree);
void slice_tree_print(slice_tree);
void output_slice_tree(slice_tree, FILE *);
slice_tree input_slice_tree(FILE *);
