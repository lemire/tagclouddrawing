/* file: html.c                                                    */


/******************************************************************************
Copyright (c) 2006-2007, Owen Kaser

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


/* from a slicing tree, generate nested html tables                */
/* Written by Owen Kaser, September 5, 2006                        */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "floorp.h"
#include "graph.h"
#include "html.h"


void print_html_header(FILE *of) {
 fprintf(of,"<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"cloud.css\"/><title>Some Nested Tables</head><body>\n");
}

void print_html_trailer(FILE *of) {
  fprintf(of,"</body></html>\n");
}

/* changed Jan 24, 2007 so that leaves are not tables */
/* changed Jan 29, 2007 to operate after sizing       */

void print_table(FILE *of, slice_tree t, graph g)
{
  char *pc;

  // Assumption of 3 shapes per tag should be generalized


  /* using null is stupid, as the null is actually embedded.  Fixing it to
   * use a real character will require making the CSS match
   * and also may need to check the javascript magic
   *
   * since it had been working, I have deferred fixing it
   * -ofk, 29 Feb 2008.
   */

  // array of 1-char prefixes for span classes, next
  static char *alternatives = "n\0e"; /* Assume all 3 shapes present for each tag */


  if (t->type_is_cut) {

  fprintf(of,"<table><tr>");  // ordinary
  //fprintf(of,"<table rules='all'><tr>"); // see structure
  //fprintf(of,"<table border> <tr>");  // see structure
    
    fprintf(of, "\n<td>");
    print_table(of,t->child1,g);
    fprintf(of,"</td>");
    
    if (t->direction) {
      // vertical cut, so we have a 2 column, 1 row table (s is)
    }
    else {  // horizontal, so 1 column, 2 row table
      fprintf(of,"</tr><tr>");
    }

    fprintf(of,"<td>");
    print_table(of,t->child2,g);
    fprintf(of,"</td>");
    fprintf(of,"</tr></table>\n");
  }
  else { /* leaf */
    
    char *tag = g->vertices[t->name].original_name_string;
    /* these would fire if this routine called prior to sizing */
    assert(t->shape_fcn_chosen >= t->shape_fcn_start);
    assert(t->shape_fcn_chosen <= t->shape_fcn_end);

    char alt_aspect_indicator = alternatives[t->shape_fcn_chosen - t->shape_fcn_start];

    // rather fragile and limited, demo-class code.

    // hackery has maintained the size of each tag as one or two digits prepended to
    // the name of the tag, since we drafted the VLSI floorplanner's data structure
    // which only had a string to carry around.  Ain't pretty, but...

    // pull the size info apart from the REAL tag string.
    // also, we convert +s into nbspaces.  
    // ie, right now, you cannot have a tag with a real + in it.
    // no attempt to see whether the tag has illegal HTML embedded in it, etc. 

    if (isdigit(*tag) && !isdigit(*(tag+1))) {
      fprintf(of,"<span class=\"tag%c%c\" >",*tag, alt_aspect_indicator);
      tag++;  /* skip 1*/
    }
    else
      if (isdigit(*tag) && isdigit(*(tag+1))) {
        fprintf(of,"<span class=\"tag%c%c%c\" >",*tag,*(tag+1), alt_aspect_indicator);
        tag += 2;  /* skip 2 */
      }
      else
        fprintf(of,"<span class=\"tag0%c\" >", alt_aspect_indicator);

    for (pc = tag;  *pc != '\0'; pc++)
    {
      if (*pc == '+')   /* verbatim except + to space conversion */
        fprintf(of,"&nbsp;");   /* should use a fixed-width space */
      else
        putc(*pc,of);
    }
    fprintf(of,"</span>"); 
  }    
}


void print_html(FILE *of, slice_tree t, graph g) {
  print_html_header(of);
  print_table(of,t,g);
  print_html_trailer(of);
}
