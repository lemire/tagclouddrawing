/******************************************************************************
Copyright (c) 2006-2007, Owen Kaser and Daniel Lemire

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

/* note: this is not part of the after-Tagging-Workshop distribution */

// For an HTML table

import java.util.*;
import java.io.*;

public class Grid {

    boolean drawOutline;
    int W; // max width

    private int pad;
    private Vector<Integer> colWidth;  // not including padding
    private Vector<Integer> rowHeight;  // not including padding
    // private Vector<Tag> tags;
    private static final int heuristicHackPrune = 3;

    public static final int PAD_NO_LINES = 5;  // pixels from line and space (guess)
    public static final int PAD_WITH_LINES = 10;

    private static Tag fillerTag = new Tag(0,0,0,"");  // all fillers are THIS OBJECT 


    public Grid( boolean outline, int maxWidth ) {
        pad = outline ? PAD_WITH_LINES : PAD_NO_LINES;
        drawOutline = outline;
        W = maxWidth;
    }

    // width and height only make sense after a "pour" etc has sized
    public int width() {
        assert colWidth != null &&  colWidth.size() > 0;
        int actualW = 0;
        for (int c : colWidth) actualW += c;  // col contents
        actualW += (colWidth.size()*pad); // grid etc
        return actualW;
    }

    public int height() {
        assert rowHeight != null && rowHeight.size() > 0;
        int actualH = 0;
        for (int h : rowHeight) actualH += h;
        actualH += (rowHeight.size()*pad);
        return actualH;
    }

    // pour tags into a grid that may not legally fit...
    // this resizes columns, rows
    public void pour( int numCols, Vector<Tag> tags ) {
        colWidth = new Vector<Integer>();
        rowHeight = new Vector<Integer>();

        for (int i=0; i < numCols; ++i) colWidth.add(0);

        int colCtr=0; // counts mod numCols;
        int rowCtr=-1; // immediately incremented

        for (Tag t : tags) {
            if (colCtr==0) { // new row begins
                rowCtr++;
                rowHeight.add(0);
            }
            
            // must we widen this column?
            if (t.getBoxWidth() > colWidth.get(colCtr))  colWidth.set(colCtr, t.getBoxWidth());

            // or increase row height?
            if (t.getBoxHeight() > rowHeight.get(rowCtr)) rowHeight.set(rowCtr, t.getBoxHeight());
            
            colCtr = (++colCtr) % numCols;
        }
    }


    public String getTableHTML(int numCols, Vector<Tag> tags) {
        
        StringBuffer sb = new StringBuffer();

        sb.append("<table"); 
        if (drawOutline) sb.append(" border");
        sb.append(">");

        int ctr=0;
        for (Tag t : tags) {

            if (ctr % numCols == 0) {
                if (ctr != 0) sb.append("</tr>");
                sb.append("<tr>");
            }

            sb.append("<td>");
            //            for (int i=0; i < t.getEmph(); ++i) sb.append("<font size=\"+\">");
            sb.append(t.getHTML());


            sb.append("</td>\n");

            ++ctr;
        }

        sb.append("</tr></table>");

        return sb.toString();
    }


    int optimalNumColumns(Vector<Tag> ts) {

        int columnUpperBound;

        // even if all C columns contained only the smallest C boxes,
        // that would limit C.

        int [] widths = new int[ ts.size()];
        for (int i=0; i < ts.size(); ++i) widths[i] = ts.get(i).getBoxWidth();
        Arrays.sort(widths);
        
        // how big can prefix get before > W?
        int ctr = 0;
        int totalWidth=0;
        for (int w : widths) {
            if (totalWidth + w > W) break;
            totalWidth += w;
            ++ctr;
        }
        int colBound = ctr;

        int bestW = -1;
        int bestH = Integer.MAX_VALUE;

        for (int w = 1; w <= colBound; ++w) {
            // try it in w columns
            pour(w,ts);
            if (width() < W) { // it's feasible
                int h = height();
                if (h < bestH) {
                    bestW = w; bestH = h;
                }
            }
        }
        assert bestW >= 1;
        return bestW;
    }

    int optimalHeight(Vector<Tag> ts) {
        pour(optimalNumColumns(ts),ts);
        return height();
    }

    // if we can reorder tags, how to reorder?
    // I think it is truly optimal if there is only one box height
    // otherwise, it's an (analyzable?) heuristic 

    // if we can reorder, is there any benefit to leaving any
    // blank cells?  I don't think so.

    public Vector<Tag> optimalReordering( Vector<Tag> ts) {
        // sort tags by descending width
        Tag [] tags = new Tag[ ts.size()];
        int tctr=0;
        for (Tag t : ts) tags[tctr++] = t;

        Arrays.sort(tags, new Comparator<Tag>(){
            public int compare(Tag a, Tag b) { 
                return b.getBoxWidth() - a.getBoxWidth();}
        });

        // temp
        System.out.println("by descending width :");
        for (Tag t : tags) System.out.println(""+t);


        // essentially, tags are laid out in a column-major grid

        // determine max number of columns.  Rely on sortedness. Could use
        // binary search.

        int w = -1;
        int fch;  // FirstColumnHeight
        WIDTHFIND:
        for (fch = 1 ; fch <= tags.length; ++fch) {
            int widthSum = 0;
            // now, account for grid and padding around it
            // ww = number of columns
            int ww = (int) Math.ceil(tags.length / (double) fch);
            int maxW_adjusted = W - ww*pad;
            for (int colHeadItem = 0; colHeadItem < tags.length; 
                 colHeadItem += fch) {
                widthSum += tags[colHeadItem].getBoxWidth();
                if (widthSum > maxW_adjusted) continue WIDTHFIND;
            }
            // get here: ww is best column width value.
            w = ww;
            break;
        }
                
        assert w != -1;
        System.out.println("width " + w + " optimal");

        // within each column, sort by height
        // This is essentially what Shellsort does, can be done nicely
        // however, do this more naively (and _asymptotically_ faster)

        // at this point, we are storing the grid column-major


        int firstColumnContains = (int) Math.ceil(tags.length / (double) w);
        int numShortColumns =  w * firstColumnContains - tags.length;


        System.out.println("with " + tags.length + " items in " + w +
                           "cols, we have " + numShortColumns+" short, pad!");


        // pad so we can assume a full grid, for sanity
        Tag [] temp = new Tag[w * firstColumnContains];
        for (int i=0; i < tags.length; ++i) temp[i]=tags[i];
        
        for (int i=1; i <= numShortColumns; ++i) temp[ temp.length - i] = fillerTag;
        tags = temp;  // having padded it...

        
        
        for (int c = 0; c < w; ++c) {
            int colContains =  firstColumnContains;
            /*
            Tag [] temp = new Tag[colContains];
            int j=0;
            for (int i = c; i < tags.length; i += w)
                temp[j++] = tags[i];

            // sort by descending height
            Arrays.sort(temp, new Comparator<Tag>() {
                public int compare(Tag a, Tag b) { 
                    return b.getBoxHeight() - a.getBoxHeight();}
            });

            // replace into tags
            j=0;
            for (int i = c; i < tags.length; i += w)
                tags[i] = temp[j++];

            */
            Arrays.sort(temp, c*colContains, (c+1)*colContains,
                        new Comparator<Tag>() {
                public int compare(Tag a, Tag b) { 
                    return b.getBoxHeight() - a.getBoxHeight();}
            });
        }
        //all  columns processed


        // array back to vector, we need a transposition
        // since the grid will be row major and we have things
        // column major.
        Vector<Tag> answer = new Vector<Tag>();

        
        for (int r = 0; r < firstColumnContains; ++r)
            for (int c = 0; c < w; ++c) {
                int transposedIdx = c*firstColumnContains + r;
                if (transposedIdx < tags.length)
                    answer.add(tags[transposedIdx]);
            }
                
        return answer;
    }

    
    public Vector<Tag> optimalPaddingNoReorder( Vector<Tag> ts) {
        bestAnswerYet = ts;
        masterTs = ts;
        callCtr = 0;
        
        bestHeightYet = optimalHeight(ts);
        
        backtrack(0,new Vector<Tag>());

        return bestAnswerYet;
    }

    private Vector<Tag> masterTs;
    private Vector<Tag> bestAnswerYet;
    private int bestHeightYet;
    private int callCtr;

    private void backtrack(int numFixed, Vector<Tag> paddedPrefix) {


        // heuristic prune, if unreasonably many blanks used.
        if (paddedPrefix.size() - numFixed > heuristicHackPrune) return;


        if (++callCtr % 100000 == 0) 
            System.out.println(""+callCtr + " "+numFixed + " paddedPrefix len=" +paddedPrefix.size()); // feedback!

        // compute a lower bound on the cost of the current solution
        int cols = optimalNumColumns(paddedPrefix);
        // the final row may have extra room, so let's consider only the
        // completely filled in rows
        // which means perhaps that some entries at the end of paddedPrefix
        // should not be considered: how many?
        int partialRowSize = paddedPrefix.size() % cols;

        int originalPPSize = paddedPrefix.size();  // for sanity check
        // copy them off, temporarily.  Also count number that are not blanks
        int removedNotBlank = 0;
        Vector<Tag> surplus = new Vector<Tag>();
        for (int i = partialRowSize-1; i >= 0; --i) {  // don't reverse their order
            Tag t = paddedPrefix.get(  paddedPrefix.size()-1-i);
            if (t != fillerTag) ++removedNotBlank; // object identity
            surplus.add(t);
        }
        // remove all popped items from paddedPrefix
        //paddedPrefix.removeRange(paddedPrefix.size()-partialRowSize, paddedPrefix.size());
        for (int ctr = 0; ctr < surplus.size(); ++ctr)
            paddedPrefix.remove(paddedPrefix.size()-1);  // pop, should be cheap?
        
        // solve for height of this, given number of columns we know
        pour( cols, paddedPrefix);
        int bestCommittedHeight = height();

        // put back the items stolen from paddedPrefix
        paddedPrefix.addAll(surplus);
        
        // now, a lower bound on the height required for the remaining tags
        // is their total area / W.

        // we count the area of the tags only, and not any contribution from the
        // grid etc, because we cannot be assured how many columns would be used.
        // the best we could do is count 1 column's worth

        int totalArea = 0; 
        for (int remainingtag = numFixed - removedNotBlank;
             remainingtag < masterTs.size(); ++remainingtag) {
            totalArea += ( masterTs.get(remainingtag).getBoxHeight() *
                           masterTs.get(remainingtag).getBoxWidth());
        }

        // actually, _could_ take ceiling
        int bestUncommittedHeight = totalArea / W;

        // our lower bound is bestCommittedHeight + bestUncommittedHeight.

        if (bestCommittedHeight + bestUncommittedHeight > bestHeightYet) {
            // System.out.print("Prune");
            return; // prune branch, it's a lost cause
        }

        // We could also try pruning by solving the reordering-allowed problem on the
        // uncommmitted tags.  However, this could be pretty expensive.


        // option 1: we go with only the blanks so far, add rest of the tags and solve
        for (int j = numFixed; j < masterTs.size(); ++j) 
            paddedPrefix.add( masterTs.get(j));
        int h1 = optimalHeight( paddedPrefix); // nb, this is NOT necessarily optimal unless all tags same height

        if (h1 < bestHeightYet) {
            bestAnswerYet = (Vector<Tag>) paddedPrefix.clone();
            System.out.println("new best seen, it is " + h1);
            bestHeightYet = h1;
        }

        // the "next padding cell" is tried at each spot from here the (end-1) 

        // gradually put padded Prefix back
        int numAdded = masterTs.size() - numFixed;

        // include tempNumFixed == numFixed because you can have multiple skips in a row
        for (int tempNumFixed = masterTs.size() -1; tempNumFixed >= numFixed; tempNumFixed--) {
            // replace last guy by a skip
            paddedPrefix.set( paddedPrefix.size() -1, fillerTag);
            backtrack(tempNumFixed,paddedPrefix);
            paddedPrefix.remove( paddedPrefix.size()-1);
        }
        
        // at this point, we hopefully have just put paddedPrefix back to 
        // its original form
        assert originalPPSize == paddedPrefix.size();

        //        System.out.print("return");

        return;  // to caller, having explored all state-space options 
        // with the given prefix
    }


    public static void main( String [] argv) {

        Vector<Tag> ts = Tag.readTags(new File( argv[0]));

        Grid g = new Grid(true, 1000);

        Vector<Tag> ts2 = g.optimalPaddingNoReorder(ts);

        // slap html header junk
        System.out.println("<html><head>"
       + "<link rel=\"stylesheet\" type=\"text/css\" href=\"cloud.css\"/>"
       + "<title>Tag Cloud</title>"
       +" </head>"
        +"<body><div id=\"maincontent\">");
          
          


        System.out.println("solved for height " + g.bestHeightYet + " with " + g.callCtr + " calls." +
                           " max allowed blanks " + heuristicHackPrune );
        System.out.println("<p>");

        int optcols = g.optimalNumColumns(ts2);
        System.out.println( g.getTableHTML(optcols,ts2)+"<p>");        

         
        int o = g.optimalNumColumns(ts);

        for (int co = 2; co <= o; ++co) {
            g.pour(co, ts);
            System.out.println("<h3>No spaces, reordering " + co + " cols, est height " + g.height() + " width= " + g.width()+"</h3>");
            System.out.println( g.getTableHTML(co,ts));
        }

        System.out.println("<h1> Allowing Reordering </h1>");

                // and the optimal ones
        Vector<Tag> ts1 = g.optimalReordering(ts);
        o = g.optimalNumColumns(ts1);
        System.out.println( g.getTableHTML(o,ts1));
        System.out.println("</div><</body></html>");

    }

}
