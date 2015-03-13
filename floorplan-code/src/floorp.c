/* file: floorp.c */

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


#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "floorp.h"

#include "graph.h"  /* and slice.h */
#include "mcplace.h"
#include "size.h"
#include "html.h"

#define PAPERSIZE 80

//extern float total_cut_size;

struct timeval tp;

void print_elapsed_time(FILE *stream, char *msg, struct timeval *init_time)
{
 long l1,l2;
 struct timeval tp1;

  gettimeofday(&tp1,(struct timezone *)NULL);


  if (tp1.tv_sec < init_time->tv_sec)
  {   /* wrap around 0 has happened */
     l1 = init_time->tv_sec - tp1.tv_sec;
  }
  else
    l1 = tp1.tv_sec - init_time->tv_sec;

  l2 = tp1.tv_usec - init_time->tv_usec;
  if (l2 < 0) {
     l2 += 1000000;
     l1--;     }
	
  fprintf(stream,"Elapsed time to %s is %ld sec %ld microsec \n",msg,l1,l2);
}


extern long sum_of_penalties;
extern long xy_pad_total;

float width_scalefactor;  /* used in mcplace.c */

#define FNLEN 100   /* max permitted filenames */

int main(int argc, char **argv)
{
  graph g;
  slice_tree st;
  char filename[FNLEN];
  char htmlfn[FNLEN];
  FILE *latexfile;
  FILE *htmlfile;
  char latexfilename[FNLEN];
  long maxdimension;
  long scalefactor;
  int xsize, ysize;
  int soln_area;

  srand(10);  /* make this program deterministic? */

  if (argc != 2)
    fail("usage: floorp <filename-without-extension>");

  /* we tack at most 5 extra chars on */
  if (strlen(argv[1]) >=  FNLEN - strlen("..dbg"))
      fail("command line argument is too long");

  /* Bad design alert: this code maintains a "global graph" */
  make_globgr();  

  (void) sprintf(filename,"%s.pl",argv[1]);
  (void) sprintf(latexfilename,"%s.tex",argv[1]);
  (void) sprintf(htmlfn,"%s.html",argv[1]);
  latexfile = fopen(latexfilename,"w");
  htmlfile = fopen(htmlfn,"w");

  if (!latexfile || !htmlfile)
    fail("problem in creating one of the output files");

  g = get_graph(filename);
#ifdef BLAB
  print_graph1(stdout,g);
#endif

  gettimeofday(&tp,(struct timezone *)NULL);

  /* Jan 30, 2007: hunt for best scalefactor */
  width_scalefactor = 1.4; /* seemed to be pretty good as first guess */

  while (1) {
    /* we start out with 100% of the horizontal and vertical space */
    st = rec_placement(g,1.0,1.0,0,STT NULL,0,1,1,1,1);
    print_elapsed_time(stdout,"completion of rec_placement",&tp);
    
    number_slice_tree(st);
    init_before_sizing();
    size(g,st);
    print_elapsed_time(stdout,"completion of size",&tp);
    
    /* compute best solution , over all*/
    get_best(st,(long) 10000000, (long) 10000000);

#define WIDTH_PARAM_DELTA 0.1
#define MIN_WIDTH_PARAM  1.2

    /* if we missed the mark (too wide), then increase width_scalefactor and retry */
    if (dims[st->shape_fcn_chosen].x > FIXEDWIDTH) {
      if (width_scalefactor >= 2.0) {
        fail("*****fixed width setting: you must have a single tag longer than max width");
      }
      width_scalefactor += WIDTH_PARAM_DELTA;
    }
    else { /* if our widest possible choice is not nearly wide enough (within 10% of goal)*/
      if ( /* st->shape_fcn_chosen == st->shape_fcn_end && */ 
          dims[st->shape_fcn_end].x < FIXEDWIDTH * 0.9) {
        if (width_scalefactor <= MIN_WIDTH_PARAM) {
          fprintf(stderr,"fixed width setting: maybe your entire data can be placed square within the limit.\n");
          break;
        }
        else width_scalefactor -= WIDTH_PARAM_DELTA;
      }
      else break;  /* happy with what we got */
    }
  }    

  xsize = dims[st->shape_fcn_chosen].x;
  ysize = dims[st->shape_fcn_chosen].y;

  maxdimension = MAX( xsize , ysize);
  soln_area = xsize * ysize;
  scalefactor = maxdimension/(PAPERSIZE-1) + 1;

  top_down_size(st);
  print_elapsed_time(stdout,"completion of top_down_size",&tp);
  place(st,(long)0, (long)0, (long) xsize, (long) ysize); 
  output_latex_file(latexfile,st,maxdimension,g);

  print_html(htmlfile,st,g);
  fclose(htmlfile);

#if 0
    printf("Total cut size is %e\n",total_cut_size);
    printf("Sum of penalties is %ld\n",sum_of_penalties);
#endif
  printf("X + Y padding total is %ld\n",xy_pad_total);

  printf("Euclidean sum distance of this solution is %f\n",
         evaluate_sized_solution(g,st,2.0));

  printf("The area is about %d\n", soln_area);
  finish_latex_file(latexfile);
  fclose(latexfile); 
  return 0;
}
