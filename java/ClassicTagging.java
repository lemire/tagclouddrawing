/******************************************************************************
Copyright (c) 2007, Daniel Lemire and Owen Kaser

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



/*** Tag Cloud optmizations, in Java

* The white space model is kind of odd but goes like this:
* if we have n tags on a line, we allow at least n-1 times the
* width of a space to white space. However, we do not necessarily
* render tag cloud this way.
*
*
* 
 * @author Owen Kaser
 * @date   21,22 January 2007
 * 
 * After the core dynamic programming algorithm, Daniel Lemire added
 * most of the rest of the code (esp. heuristics for Strip Packing)
 */

import java.io.*;
import java.util.*;

public class ClassicTagging {

    static public final boolean verbose = false;
    static public final String mouseclick = "onMouseOver='javascript:"
    + "this.style.backgroundColor=\"#fcc\";"
    +"x=\"(w,h)=\"+this.offsetWidth+\" \"+this.offsetHeight+ "
    +"\" (pos y, pos x)=  \"+this.offsetTop+ \" \"+this.offsetLeft;"
    +"z=document.getElementById(\"status\");"
    +"while(z.firstChild) z.removeChild(z.firstChild);"
    +"z.appendChild(document.createTextNode(x));'"
    +" onMouseOut='javascript:"
    + "this.style.backgroundColor=\"\";"
    +"z=document.getElementById(\"status\");"
    +"while(z.firstChild) z.removeChild(z.firstChild);' ";
    
    // this next flag process "edge" white space" differently
    // it is somewhat of a problem because our cost model assumes
    // uniform gaps!!! -DL
    static public final boolean treatEdgeWhiteSpaceDifferently = false;

    static public final String ALPHA_SWITCH = "--alpha=";
    static public final String GAMMA_SWITCH = "--gamma=";
    static public final String WIDTH_SWITCH = "--width=";
    static public final String MAXONLY_SWITCH = "--maxonly";
    static public final String INCLUDE5000_SWITCH = "--include5000";
    static public final String TIMINGS_SWITCH = "--timings";
    
    static public final String SWITCH_START = "--";
    static public final String OUTPUT_SWITCH = "-O";
    
    static public final double UNUSED = -1.0;
    public static double WIDTH = 550.0;
    //static public final double gamma = 2.0;
    static public final int COSTMODEL_NORMAL = 1;  // if this gets more complex, costmodel could become a class
    static public final int COSTMODEL_MAX = 2;
    
    // how do we use enums in Java?
    //enum CostModel {COSTMODEL_NORMAL,COSTMODEL_MAX,COSTMODEL_DANIEL}

    

    List<Tag> data;
   
    final double alpha; // alpha and beta control degree of importance, overall whitespace vs proxy "max  whitespace"
    final double beta;  // beta is for maxwhitespace
    final double gamma;
    final int n;
    final int costModel; 
    int [] choice;  // for solution recovery


    public ClassicTagging( double a, double b, double g,  int cstmodel, String inFn) {
        alpha = a; beta = b; gamma = g; costModel = cstmodel;
        data = Tag.readTags( new File(inFn));
        n = data.size();
        //t = new double[n];
        choice = new int[n];
    }
    
    
    private void alphaSort(Vector<Vector<Tag>> cloud) {
      for(Vector<Tag> line: cloud) {
        Collections.sort(line,new AlphaTagComparator());
      }
      Collections.sort(cloud,new Comparator<Vector<Tag>> (){
        AlphaTagComparator atc =  new AlphaTagComparator();
        public int compare(Vector<Tag> o1, Vector<Tag> o2) {
          return atc.compare(o1.get(0),o2.get(0));
        }
        public boolean equals(Object o) {
         return false;
        }
      });
    }
    private void markchoices(Vector<Vector<Tag>> cloud) {
      //System.out.println("markchoices begin data.size() = "+data.size()+" "+size(cloud));
      data = new Vector<Tag>();
      int counter = 0;
      int startcounter = 0;
      for(Vector<Tag> line: cloud) {
        //System.out.println("line.size() = "+line.size());
        counter += line.size();
        for (Tag t: line)
          data.add(t);
        choice[startcounter] = counter - 1;
        //System.out.println("starting from "+startcounter+" you should go to "+choice[startcounter]);
        startcounter = counter;
      }
      //System.out.println("data.size() = "+data.size()+ " " +counter);
      if(data.size() != counter) System.err.println("not going well!!!");
      //System.out.println("markchoices end data.size() = "+data.size());
    }
    
    private int size(Vector<Vector<Tag>> cloud) {
      int counter = 0;
      for(Vector<Tag> line: cloud) {
        counter += line.size();
      }
      return counter;
    }
    private int lineSize(Vector<Tag> line) {
      int occupied = 0;
      for(Tag t: line) {
        occupied += t.getBoxWidth();
      }
      return occupied + (line.size()-1)*getSpaceWidth();
    }    
    private boolean isTooLarge(Vector<Vector<Tag>> cloud) {
      for(Vector<Tag> line: cloud) {
        if(lineSize(line)>WIDTH) return true;
      }
      return false;
    }
    
    /**
    * algorithm attributed to Coffman et al.
    * is 2-optimal (?)  for the strip packing problem
    */
    public double computeNextFitDecreasingHeight() {
     Collections.sort(data,new HeightTagComparator());
     Vector<Vector<Tag>> cloud = new Vector<Vector<Tag>>();
     Vector<Tag> lastline = new Vector<Tag>();
     cloud.add(lastline);
     int currentwidth = 0;
     for(Tag t: data) {
       int tobeadded = t.getBoxWidth() + (lastline.size() > 0 ? getSpaceWidth() : 0);
       //System.out.println("tobeadded "+tobeadded);
       if(currentwidth + tobeadded <= WIDTH) {
         currentwidth += tobeadded;
         //System.out.println("t.getBoxWidth() "+t.getBoxWidth());
         //System.out.println("lineSize(lastline) "+lineSize(lastline));
         //System.out.println("currentwidth "+currentwidth);
         lastline.add(t);
         //System.out.println("lineSize(lastline) "+lineSize(lastline));
         assert (lineSize(lastline)==currentwidth);
         assert (lineSize(lastline)<=WIDTH);
       } else {
         //if(lineSize(lastline)>WIDTH)
         //  System.out.println("***overfull!!!***"+lineSize(lastline));
         lastline = new Vector<Tag>();
         //System.out.println("lines was filled up to "+currentwidth+ " of "+WIDTH);
         cloud.add(lastline);
         lastline.add(t);// even if overfull
         currentwidth = t.getBoxWidth();
       }
       //System.out.println(size(cloud));
     }
     //if(lastline.size() > 0) cloud.add(lastline);
     alphaSort(cloud);
     markchoices(cloud);
     return badness(cloud);
    }
    private double linebadness(Vector<Tag> line) {
      if(alpha != 0.0) 
        throw new RuntimeException("Choose  alpha  = 0.0 or go to heck");
      int nonWhiteArea = 0;
      int lineHeight = 0;
      for (Tag t : line ) {
        nonWhiteArea += t.getBoxArea();
        lineHeight = lineHeight > t.getBoxHeight() ? lineHeight : t.getBoxHeight();
      }
      return Math.abs( lineHeight * WIDTH - nonWhiteArea 
      - (line.size() - 1) * getSpaceWidth() * lineHeight );

    }
    
    private double badness(Vector<Vector<Tag>> cloud) {
      double totalbadness = 0.0;
      for(Vector<Tag> line: cloud) {
        if (costModel == COSTMODEL_NORMAL) {
          totalbadness += Math.pow(linebadness(line),gamma);
        }
        else if (costModel == COSTMODEL_MAX) {
          totalbadness = Math.max(totalbadness, linebadness(line) );
        }
        else throw new RuntimeException("costmodel "+costModel+" unknown");
      }
      return totalbadness;
    }
    
    /**
    * algorithm attributed to Coffman et al.
    * is 17/10-optimal (?) for the strip packing problem
    */
    public double  computeFirstFitDecreasingHeight(){
     Collections.sort(data,new HeightTagComparator());
     Vector<Vector<Tag>> cloud = new Vector<Vector<Tag>>();
     Vector<Integer> linesize = new Vector<Integer>();
     mainloop:
     for(Tag t: data) {
       final int boxwidth = t.getBoxWidth();
       // next loop should be implemented differently if cloud.size() is large 
       for(int k = 0; k < cloud.size() ; ++k) {
         final int tobeadded = boxwidth + getSpaceWidth();
         final int prospectivewidth = linesize.get(k).intValue() + tobeadded;
         if( prospectivewidth <= WIDTH) {
           cloud.get(k).add(t);
           linesize.set(k, prospectivewidth);
           continue  mainloop;
         } 
       }
       Vector<Tag> newline = new Vector<Tag>();
       newline.add(t);
       cloud.add(newline);
       linesize.add(new Integer(boxwidth));
       //System.out.println(size(cloud));
     }
     alphaSort(cloud);
     markchoices(cloud);
     return badness(cloud);    
    }    
    
    
    /**
    * my variant of an algorithm attributed to Coffman et al.
    * is 17/10-optimal (?) for the strip packing problem
    */
    public double computeFirstFitDecreasingHeightWidth() {
     Collections.sort(data,new HeightWidthTagComparator());
     Vector<Vector<Tag>> cloud = new Vector<Vector<Tag>>();
     Vector<Integer> linesize = new Vector<Integer>();
     mainloop:
     for(Tag t: data) {
       final int boxwidth = t.getBoxWidth();
       // next loop should be implemented differently if cloud.size() is large 
       for(int k = 0; k < cloud.size() ; ++k) {
         final int tobeadded = boxwidth + getSpaceWidth();
         final int prospectivewidth = linesize.get(k).intValue() + tobeadded;
         if( prospectivewidth <= WIDTH) {
           cloud.get(k).add(t);
           linesize.set(k, prospectivewidth);
           continue  mainloop;
         } 
       }
       Vector<Tag> newline = new Vector<Tag>();
       newline.add(t);
       cloud.add(newline);
       linesize.add(new Integer(boxwidth));
       //System.out.println(size(cloud));
     }
     alphaSort(cloud);
     markchoices(cloud);
     return badness(cloud);    
    }    

    // greedy solution
    // should be close to what the browser does.
    public double greedy_solve() {
     int begin = 0;
     double badnesssofar = 0.0;
     while(begin < data.size()) {
      //if(begin >=  data.size()) return badnesssofar;
      int currentwidth = 0;
      int nonWhiteArea = 0;
      int lineHeight = 0;
      double thisLineBadness = 0.0;

      int i = begin;
      choice[begin] =  data.size() - 1;
      double badness = 0.0;
       

      for(; i < data.size();++i) {
        //System.out.println("i = "+i+ " begin = "+begin);
        Tag t = data.get(i);
        int numberOfTagsOnLine = i - begin + 1;
        int compulsoryWhiteSpaceWidth = numberOfTagsOnLine > 1 ? getSpaceWidth() : 0;
        currentwidth += t.getBoxWidth() + compulsoryWhiteSpaceWidth;
        if(currentwidth > WIDTH) {
          if(i == begin) {
           System.err.println("WARNING: I can't fit "+ t.getBoxWidth()+ " in " + WIDTH);
          } else {
            choice[begin] =  i-1;
            break;
          }
        }
        nonWhiteArea += t.getBoxHeight() * t.getBoxWidth();
        if( t.getBoxHeight() > lineHeight) lineHeight = t.getBoxHeight();
        int gapCount = 1 + numberOfTagsOnLine;
        double eachGapArea = lineHeight*(WIDTH - currentwidth) / gapCount;
        double whiteArea = (WIDTH * lineHeight) - nonWhiteArea;
        double forcedWhiteSpace = (numberOfTagsOnLine - 1) * lineHeight *  getSpaceWidth();
        if( whiteArea - forcedWhiteSpace < 0 )
          System.out.println("you have negative badness"); 

        if (costModel == COSTMODEL_NORMAL) {
            thisLineBadness = alpha * Math.pow(eachGapArea,gamma) + beta *  Math.pow( Math.abs(whiteArea - forcedWhiteSpace),gamma);
            badness = thisLineBadness + badnesssofar;
        }
        else if (costModel == COSTMODEL_MAX) {
            if(alpha==1.0) 
              thisLineBadness = eachGapArea;
            else if (beta == 1.0)
              thisLineBadness = Math.abs(whiteArea - forcedWhiteSpace);
            else throw new RuntimeException("Choose integer values for alpha");
            badness = Math.max(thisLineBadness, badnesssofar);
        }
        else throw new RuntimeException("costmodel "+costModel+" unknown");
      }
      badnesssofar = badness;
      begin = i;
      //return  greedy_solve(i, badness);
     }
     return badnesssofar; 
    }
    
    private double memo_solve( int i) {
      double [] t = new double[n];
      for (int k=0; k < n; ++k)
            t[k] = UNUSED;   // memo-table entry
      return memo_solve(i,t);
    }
    
    private int computeWidth(int begin, int end) {
      int total = 0;
      for(int k = begin ; k <= end; ++k) {
        total += data.get(k).getBoxWidth();
      }
      return total + (end-begin) * getSpaceWidth();
    }
    // core of dyn programming soln
    private double memo_solve( int i, double[] t) {
        if ( i == -1) return 0.0;  // nothing has 0 badness
        
        if (t[i] != UNUSED) return t[i];
        // retVal will be used to store the best badness 
        double retVal = Double.MAX_VALUE; 
        double lineHeight = 0.0;
        double nonWhiteArea = 0.0;
        double widthSoFar = 0.0;
        
        // lastLineStart is actually just the possible line starting points
        // if lastLineStart = i, then you have have tag i on the line
        for (int lastLineStart = i; lastLineStart >= 0; --lastLineStart) {
            Tag newtag = data.get(lastLineStart);
            int numberOfTagsOnLine = i - lastLineStart + 1; 
            // gapCount is the number of "gaps" between tags
            // naturally, as long as you have at least one item on the 
            // the line, then you have two gaps
            int gapCount = 1 + numberOfTagsOnLine;
            // boxHgt is the height of the last tag we included
            double boxHgt = (double) (newtag.getBoxHeight());

            // naturally, the area covered is width x height
            // (this is a visual model that could be discussed further)
            nonWhiteArea += (newtag.getBoxHeight() * newtag.getBoxWidth());
            // lineHeight is just the max of all boxHgt
            if (boxHgt > lineHeight) lineHeight = boxHgt;
            // naturally, white area is just total area - nonwhite
            double whiteArea = (WIDTH * lineHeight) - nonWhiteArea;
           
            // if we already have one tag in place, we first have to insert a space
            // not all white space is bad! some white space is required!!!
            // presumably, only the "extra white space is bad!"
            int compulsoryWhiteSpaceWidth = numberOfTagsOnLine > 1 ? getSpaceWidth() : 0;
            // the width so far must take into account the compulsory white space
            widthSoFar +=  compulsoryWhiteSpaceWidth + newtag.getBoxWidth();
            assert (widthSoFar == computeWidth(lastLineStart,i));
            //assert(
            //if ( (lastLineStart==i) && (widthSoFar > WIDTH) )
             //    System.out.println("WARNING: I will allow "+widthSoFar+" to fit in "+WIDTH+" in order for a solution to exist");
            if ( (lastLineStart <i) && (widthSoFar > WIDTH) )break;
            // this "eachGapArea" should exclude the compulsory white space
            double eachGapArea = lineHeight*(WIDTH - widthSoFar) / gapCount;
            // compute badness of this proposed "last line"
            // note that "whiteArea" includes the compulsory white space
            double forcedWhiteSpace = (numberOfTagsOnLine - 1) * lineHeight *  getSpaceWidth();

            double solnBadness;
            // in the spirit of dynamic programming we compute the best badness
            // when we start the line at lastLineStart and go to i inclusively
            // memo_solve(lastLineStart-1) returns the best solution for
            // previous lines
            double badnesssofar = memo_solve(lastLineStart-1,t);
            double thisLineBadness = 0.0;

            if (costModel == COSTMODEL_NORMAL) {
                thisLineBadness = alpha * Math.pow(eachGapArea,gamma) + beta * Math.pow(Math.abs
                  (whiteArea - forcedWhiteSpace) 
                  ,gamma);
                solnBadness = thisLineBadness + badnesssofar;
            }
            else if (costModel == COSTMODEL_MAX) {
                if(alpha==1.0) 
                  thisLineBadness = eachGapArea;
                 else if (beta == 1.0)
                   thisLineBadness =Math.abs
                 ( whiteArea - forcedWhiteSpace);
                else throw new RuntimeException("Choose integer values for alpha");
                //thisLineBadness = eachGapArea;
                solnBadness = Math.max(thisLineBadness, badnesssofar);
            }
            else throw new RuntimeException("costmodel "+costModel+" unknown");
            
            if (verbose) {
                System.out.print(" from " + lastLineStart + " to " + i + "inclusive: ");
                System.out.print(" wsf = " + widthSoFar);
                System.out.print(" lHgt = " + lineHeight);
                System.out.print(" whA = " + whiteArea);
                System.out.println(" tlB = " + thisLineBadness);
            }

            // we only keep it if it is the best solution
            if (solnBadness < retVal) {
                retVal = solnBadness;
                choice[i] = lastLineStart;
            }
        } 
        assert retVal != Double.MAX_VALUE;
        return (t[i] = retVal);
    }
    
    // return the width of a single space
    private static int getSpaceWidth() {
      int w = Tag.spaceWidth;
      if(w == 0) {
          // falling back to Owen's scenario
          Tag minimumTag = new Tag("0 ");
          w = minimumTag.getBoxWidth();
      }
      return w;
    }
    
    private void cleanbuffers() {
        choice = new int[n];
    }
    
    private int forwardSolve(int begin, StringBuffer acc) {
      return forwardSolve(begin,acc,true);
    }
    
    // used by greedy solution
    // returns the height in pixel
    private int forwardSolve(int begin, StringBuffer acc, final boolean justify) {
      if(begin >= choice.length) return 0;
      //System.out.println(" begin = "+begin+ " out of "+choice.length);
      int end = choice[begin];
      //System.out.println(" end = "+end);
      //if(end > data.len
      int nGaps = 2 + (end - begin);
      if(justify) {
        nGaps = end-begin;
        if(nGaps == 0) nGaps =1; // so it does not crash
      }
      int occupied=0;
      int currentheight = 0;
      for (int j = begin; j <= end; ++j) {
        //System.out.print(data.get(j).getText()+ " ("+data.get(j).getBoxWidth()+") ");
        occupied += data.get(j).getBoxWidth();
        if(data.get(j).getBoxHeight() > currentheight)
           currentheight = data.get(j).getBoxHeight(); 
      }
      //System.out.println("\n\n");
      //if(occupied+ (end - begin) * getSpaceWidth() > WIDTH) 
      //  System.out.println("overfull line! "+(occupied+ (end - begin) * getSpaceWidth()-WIDTH));
      int eachGapWidth = (int) ((WIDTH - occupied) / nGaps);
      //if(eachGapWidth < getSpaceWidth()) 
          // if this happens, it isn't actually a bug!!!
        //  System.err.println("you have a problem: too dense!!! "+ eachGapWidth+ " "+ getSpaceWidth());
      double unaccountedwhitespace =  (WIDTH - (occupied+nGaps*eachGapWidth))/nGaps;
      double fractpixel = 0.0;
      int j = begin;
      int debugusedwidth = 0;
      if(justify) {
        if(end == begin)// we center
                    acc.append("<span style='padding-left:"+(int) Math.round((WIDTH - data.get(j).getBoxWidth())/2.0)+
            "px;padding-right:0px;margin:0px;' "+mouseclick+" ></span>");
        acc.append(data.get(j).getHTML());
        debugusedwidth += data.get(j).getBoxWidth();
        j++;
      }
      for (; j <= end; ++j) {          
          int complement = 0;
          if(justify) {
            fractpixel += unaccountedwhitespace;
            if(fractpixel > 0.99999) {
              complement = (int) Math.floor(fractpixel+0.001);
              fractpixel -= complement;
            }
          }
          acc.append("<span style='padding-left:"+(eachGapWidth+complement)+
            "px;padding-right:0px;margin:0px;' "+mouseclick+" ></span>");
          debugusedwidth += (eachGapWidth+complement);
          acc.append(data.get(j).getHTML());
          debugusedwidth += data.get(j).getBoxWidth();
       }
       if( (nGaps > 1) && (debugusedwidth != WIDTH) && (eachGapWidth > 0 ) )
         System.err.println("WARNING BUGGY CODE You tried to go to "+WIDTH+" but got to "+debugusedwidth+" with "+nGaps+" gap using "+unaccountedwhitespace+" + ws="+eachGapWidth + ", occupied by tags: "+ occupied);
       acc.append("<br />\n");  // unixism
       return currentheight + forwardSolve(end+1,acc);
    }

    private int backSolve(int end, StringBuffer acc) {
      return backSolve(end, acc,true);
    }
    
    // back-trace through the choices, assemble optimal soln.
    // return the height in pixels
    private int backSolve(int end, StringBuffer acc, final boolean justify) {
    
        if(justify == false) System.err.println("You may hit some bugs.");
        if (end == -1) return 0;

        int begin = choice[end];
        System.out.println("line goes from "+begin+" to "+end);
        int height = backSolve(begin-1, acc);
        int nGaps = 2 + (end - begin);
        if(justify) {
          nGaps = end - begin;
          if(nGaps == 0) nGaps = 1;// so it won't crash
        }
          

        // count length of line elements
        double occupied=0;
        int currentheight = 0;
        final boolean outputlines = false;
        if(outputlines) System.out.print("line:");
        for (int j = begin; j <= end; ++j) {
            if(outputlines) System.out.print(data.get(j).getText()+" ("+data.get(j).getBoxWidth()+") ");
            occupied += data.get(j).getBoxWidth();
            if(data.get(j).getBoxHeight() > currentheight)
              currentheight = data.get(j).getBoxHeight(); 
        }
        if(outputlines) System.out.println("\n");
        // determine desired gap size
        int eachGapWidth = (int) ((WIDTH - occupied) / nGaps);
        //System.out.println(((WIDTH - occupied) / nGaps));

        // minimum space exists AMIDST line, but not before beginning/ after end
        int numEnforcedMinGaps = (end-begin);
        int endGaps = 2;

        int endGapWidth=0;
        int midGapWidth=0;

        /*if ( treatEdgeWhiteSpaceDifferently && (eachGapWidth < getSpaceWidth())) {

            System.err.println("Warning, the dynamic programming cost model should not allow so little space - bug?\n");
            midGapWidth = getSpaceWidth();
            endGapWidth = (int) ((WIDTH - occupied - midGapWidth*numEnforcedMinGaps)/2);
            if (endGapWidth < 0) {
                System.err.println("you have a problem: too dense!!!");  
                endGapWidth = 0; // and results messed up
            }
        }
        else*/ 
        endGapWidth = midGapWidth = eachGapWidth;
        if(justify) endGapWidth = 0;
        if(end == begin) 
           endGapWidth = (int) Math.floor((WIDTH - data.get(begin).getBoxWidth())/2.0);
        double unaccountedwhitespace =  (WIDTH - (occupied+nGaps*eachGapWidth))/nGaps;
        double fractpixel = 0.0;        
        
        
        // output initial gap
        int debugusedwidth = 0;
        if(endGapWidth>0)  {
               debugusedwidth += endGapWidth;
               acc.append("<span style='padding-left:"+endGapWidth+
            "px;padding-right:0px;margin:0px;' "+mouseclick+" ></span>");
        }
        

        // output tag-gap pairs (omitting last tag)

        for (int j = begin; j < end; ++j) {
            acc.append(data.get(j).getHTML());
            debugusedwidth += data.get(j).getBoxWidth();
            int complement = 0;
            if(justify) {
              fractpixel += unaccountedwhitespace;
              if(fractpixel >= 0.9999) {
                complement = (int) Math.floor(fractpixel+0.001);
                fractpixel -= complement;
              }
            }
            debugusedwidth += midGapWidth+complement;
            acc.append("<span style='padding-left:"+(midGapWidth+complement)+
            "px;padding-right:0px;margin:0px;' "+mouseclick+" ></span>");
        }
        // output final tag and "br",
        acc.append(data.get(end).getHTML());
        debugusedwidth += data.get(end).getBoxWidth();
        // daniel prefers to still output final gap so we can debug this
        if(endGapWidth>0) {
            acc.append("<span style='padding-left:"+endGapWidth+
            "px;padding-right:0px;margin:0px;' "+mouseclick+" ></span>");
           debugusedwidth += endGapWidth;
        }
        acc.append("<br />\n");  // unixism
        if( (nGaps > 1) && (debugusedwidth != WIDTH) && (eachGapWidth > 0 ))
          System.err.println("WARNING BUGGY CODE You tried to go to "+WIDTH+" but got to "+debugusedwidth+" with "+nGaps+" gap using "+unaccountedwhitespace+" + ws="+eachGapWidth + ", occupied by tags: "+ occupied);

        return height + currentheight;
    }


    private static String HTMLoverhead( String bodystuff) {
        return "<html><head>"
       + "<link rel=\"stylesheet\" type=\"text/css\" href=\"cloud.css\"/>"
       + "<script language='JavaScript' src='badnesscalculator.js' />"
       + "<title>Tag Cloud</title>"
       +" </head>"
       +"<body><p id=\"status\" style=\"position: fixed; top: 20px; right: 20px; background:#cfc; \"/><div id=\"maincontent\" style=\"width:"+((int) Math.round(WIDTH))+"\" >"+bodystuff
        +"</div></body></html>";
    }
    
    public String letBrowserDecide() {return letBrowserDecide("justified");}
    // unoptimized output, just raw HTML
    public String letBrowserDecide(String textalign) {
      String s = "<div style='text-align:"+textalign+"' id='browser"+textalign+"' >";
      for( Tag t : data) {
        s += t.getHTML();
        if(t != data.get(data.size()-1)) s+= "<span "+mouseclick+"> </span>";//"<span style='margin-left:"+getSpaceWidth()+            "px;padding:0px;margin-right:0px;' /><wbr />"; 
          //s+= " ";'
      }
      s+="</div>";
      return s;
    }
    
    /** 
    * this is the stupidest algorithm on the planet, but it should deliver a 
    * good solution. Use carefully on large cases. We simply shuffle the
    * tags several times and pick a good case.
    */
    public double random_reorder_solve(int times) {
      //System.out.println("random reordering...");
      double smallestcost = Double.MAX_VALUE;
      //Collections.sort(data,new AlphaTagComparator());
      List<Tag> toshuffle = (List<Tag>) ((Vector<Tag>)data).clone();
      List<Tag> besttoshuffle = toshuffle;
      //Collections.sort(toshuffle,new AlphaTagComparator());
      Collections.shuffle(toshuffle);
      for(int i =0; i < times;++i) {
        cleanbuffers();
        data = toshuffle;
        double currentcost = memo_solve(toshuffle.size()-1);
        if(currentcost < smallestcost) {
          smallestcost = currentcost;
          besttoshuffle = (List<Tag>) ((Vector<Tag>)toshuffle).clone();
        }        
        Collections.shuffle(toshuffle);
      }
      cleanbuffers();
      data = besttoshuffle;
      if(memo_solve(data.size()-1)!= smallestcost) 
        System.err.println("Random reorder is buggy!");
      //System.out.println("random reordering...ok");
      //System.err.println("best is "+smallestcost);
      return smallestcost;
    }

    public void timings(int repeats) {
        Collections.sort(data,new AlphaTagComparator());
		    String ans = "Timings: "+data.size()+ " "+repeats+ " ";
        long start, end;
        System.gc();
        //
        start = System.currentTimeMillis();
        for(int k = 0; k < repeats; ++k){
          cleanbuffers();
          double cost = computeFirstFitDecreasingHeight();
        }
        end = System.currentTimeMillis();
        ans+=(end-start)+ " ";
        //
        System.gc();
        start = System.currentTimeMillis();
        for(int k = 0; k < repeats; ++k){
          cleanbuffers();
          double cost = computeNextFitDecreasingHeight();
        }
        end = System.currentTimeMillis();
        ans+=(end-start)+ " ";        
        //
        //
        System.gc();
        start = System.currentTimeMillis();
        for(int k = 0; k < repeats; ++k){
          cleanbuffers();
          double cost = computeFirstFitDecreasingHeightWidth();
        }
        end = System.currentTimeMillis();
        ans+=(end-start)+ " ";  
        //        
        Collections.sort(data,new AlphaTagComparator());
        System.gc();
        start = System.currentTimeMillis();
        for(int k = 0; k < repeats; ++k){
          cleanbuffers();
          double bestCostw = memo_solve(data.size()-1);
        }
        end = System.currentTimeMillis();
        ans+=(end-start)+ " ";
        
        //
            Collections.sort(data,new WeightTagComparator());
        //
        System.gc();
        start = System.currentTimeMillis();
        for(int k = 0; k < repeats; ++k){
          cleanbuffers();
          double bestCostw = memo_solve(data.size()-1);
        }
        end = System.currentTimeMillis();
        ans+=(end-start)+ " ";
        //
	
        Collections.sort(data,new AlphaTagComparator());
        
        //
        System.gc();
        start = System.currentTimeMillis();
        for(int k = 0; k < repeats; ++k){
          cleanbuffers();
          double greedyCost = greedy_solve();
        }
        end = System.currentTimeMillis();
        ans+=(end-start)+ " ";
        //
        Collections.sort(data,new WeightTagComparator());

        //
        System.gc();
        start = System.currentTimeMillis();
        for(int k = 0; k < repeats; ++k){
          cleanbuffers();
          double greedyCostw = greedy_solve();
        }
        end = System.currentTimeMillis();
        ans+=(end-start)+ " ";
        //
        System.out.println(ans); 
    }
    public String solve(boolean include5000) {
        StringBuffer ans = new StringBuffer();
        //System.out.println("data.size() = "+data.size());
        double FFDHCost = computeFirstFitDecreasingHeight();
        ans.append("<h2>FirstFitDecreasingHeight</h2>"
          + "<p>best cost is " + FFDHCost + "</p>"
          +"<div >");
        System.out.println("FFDHCost = "+FFDHCost);
        //System.out.println("data.size() = "+data.size());
        int FFDHHeight = forwardSolve(0, ans );
        ans.append("</div>");
        ans.append("<p style='font-size:0.8em;'> height = "+FFDHHeight+"</p>");
        //System.out.println("FFDHHeight = "+FFDHHeight);
        //
        double NFDHCost = computeNextFitDecreasingHeight();
        ans.append("<h2>NextFitDecreasingHeight</h2>"
          + "<p>best cost is " + NFDHCost + "</p>"
          +"<div >");
        System.out.println("NFDHCost = "+NFDHCost);
        int NFDHHeight = forwardSolve(0, ans );
        ans.append("</div>");
        ans.append("<p style='font-size:0.8em;'> height = "+NFDHHeight+"</p>");
        //System.out.println("NFDHHeight = "+NFDHHeight);
        //
        double FFDHWCost = computeFirstFitDecreasingHeightWidth();
        ans.append("<h2>FirstFitDecreasingHeightWidth</h2>"
          + "<p>best cost is " + FFDHWCost + "</p>"
          +"<div >");
        System.out.println("FFDHWCost = "+FFDHWCost);
        int FFDHWHeight = forwardSolve(0, ans );
        ans.append("</div>");
        ans.append("<p style='font-size:0.8em;'> height = "+FFDHWHeight+"</p>");
        
        
        double randbestCost10 = random_reorder_solve(10);
        ans.append("<h2>Dynamic Programming Display (random,10)</h2>"
          + "<p>best cost is " + randbestCost10 + "</p>"
          +"<div id='randomopti' >");
        int randbestHeight10 = backSolve(data.size()-1, ans );
        //System.out.println("randbestHeight10 = "+randbestHeight10);
        ans.append("</div>");
        ans.append("<p style='font-size:0.8em;'> height = "+randbestHeight10+"</p>");
        //System.out.println("randbestHeight10 = "+randbestHeight10);
       double randbestCost5000 = 0.0; 
        if(include5000) {
          randbestCost5000 = random_reorder_solve(5000);
          ans.append("<h2>Dynamic Programming Display (random,5000)</h2>"
          + "<p>best cost is " + randbestCost5000 + "</p>"
          +"<div id='randomopti' >");
          int randbestHeight5000 = backSolve(data.size()-1, ans );
          ans.append("</div>");
          System.out.println("randbestHeight5000 = "+randbestHeight5000);
        }
        
        Collections.sort(data,new AlphaTagComparator());
        double bestCost = memo_solve(data.size()-1);
        //System.out.println("randbestHeight10 = "+randbestHeight10);
        System.out.println("dynprog (alpha) = "+bestCost);
        //System.out.println("TODO: allow dynamic programming to use negative spaces?");
        ans.append("<h2>Dynamic Programming Display (alpha)</h2>"
          + "<p>best cost is " + bestCost + "</p>"
          +"<div id='opti' >");
        int bestHeight = backSolve(data.size()-1, ans );
        ans.append("</div>");
        ans.append("<p style='font-size:0.8em;'> height = "+bestHeight+"</p>");
        System.out.println("bestCost = "+bestCost);

        
        Collections.sort(data,new WeightTagComparator());
        cleanbuffers();
        double bestCostw = memo_solve(data.size()-1);
        System.out.println("dynprog (weight) = "+bestCost);
        ans.append("<h2>Dynamic Programming Display (weighted)</h2>"
          + "<p>best cost is " + bestCostw + "</p>"
          +"<div id='optiw' >");
        int bestHeightw = backSolve(data.size()-1, ans );
        ans.append("</div>");
        ans.append("<p style='font-size:0.8em;'> height = "+bestHeightw+"</p>");
        System.out.println("bestCostw = "+bestCostw);
        
        System.out.println("starting greedy...");
        ans.append("<h2>Greedy Display (alpha)</h2>");
        Collections.sort(data,new AlphaTagComparator());
        cleanbuffers();
        double greedyCost = greedy_solve();
        System.out.println("greedyCost = "+greedyCost);
        ans.append("<p>greedy cost is " + greedyCost + "</p>");
        ans.append("<div id='greedya'>");
        int greedyHeight = forwardSolve(0, ans );
        ans.append("</div>");
        ans.append("<p style='font-size:0.8em;'> height = "+greedyHeight+"</p>");
        System.out.println("greedyHeight = "+greedyHeight);
        
        
        ans.append("<h2>Greedy Display (weight)</h2>");
        Collections.sort(data,new WeightTagComparator());
        cleanbuffers();
        double greedyCostw = greedy_solve();
        ans.append("<p>greedy cost is " + greedyCostw + "</p>");
        ans.append("<div id='greedyw'>");
        int greedyHeightw = forwardSolve(0, ans );
        ans.append("</div>");
        ans.append("<p style='font-size:0.8em;'> height = "+greedyHeightw+"</p>");
        System.out.println("greedyCostw = "+greedyCostw);

        
        Collections.sort(data,new AlphaTagComparator());
        ans.append("<h2>Raw justified HTML (using the browser's simple greedy algo.)</h2>"+letBrowserDecide("justify"));
        
        
        Collections.sort(data,new WeightTagComparator());
        ans.append("<h2>Raw justified HTML (using the browser's simple greedy algo.)</h2>"+letBrowserDecide("justify"));
        String inc5000 = "";
        if(include5000) inc5000 += " " + randbestCost5000 ;
        System.out.println("Results: "+data.size()+ " "+FFDHCost 
          +" "+FFDHWCost+" "+NFDHCost
          + " "+randbestCost10+inc5000+ " "
          + bestCost+ " "+ bestCostw+ " "+greedyCost+ " "+ greedyCostw);
        System.out.println("Heights: "+data.size()+ " "+NFDHHeight 
          +" "+FFDHHeight+" "+FFDHWHeight
          + " "+randbestHeight10  + " "+ bestHeight+ " "+ bestHeightw+ " "+greedyHeight+ " "+ greedyHeightw);
        return ans.toString();
    }

    public static void main( String [] argv) throws Exception {
        System.out.println("Sample usage: java ClassicTagging " +"delicious.input.csv --gamma=2 --alpha=0 -Otest.html");
        String inFileName = null;
        String outFileName = null;
        boolean timings = false;
        double alpha = 0.0;// this is the recommended default
        double gamma = 2.0;
        int width = 550;
        int costModel = COSTMODEL_NORMAL;
        boolean include5000 = false;

        for (String option : argv) {
           if (option.startsWith(TIMINGS_SWITCH)) {
                  timings = true;
            }
            else
            if (option.startsWith(GAMMA_SWITCH)) {
              if(option.substring(GAMMA_SWITCH.length()).equals("max")) {
                  costModel = COSTMODEL_MAX;
                  gamma = 0.0;
              } else gamma = Double.parseDouble
                    (option.substring(GAMMA_SWITCH.length()));
            }
            else if (option.startsWith(INCLUDE5000_SWITCH)) {
              include5000 = true;
            }
             else
            if (option.startsWith(ALPHA_SWITCH)) {
                alpha = Double.parseDouble
                    (option.substring(ALPHA_SWITCH.length()));
                
                if (alpha < 0.0 || alpha > 1.0)
                    throw new RuntimeException("alpha " + alpha + " out of range 0-1");
            } else if (option.startsWith(WIDTH_SWITCH)) {
                 width = Integer.parseInt(option.substring(WIDTH_SWITCH.length()));
            }   else if (option.startsWith(OUTPUT_SWITCH)) {
                outFileName = option.substring(OUTPUT_SWITCH.length());
            }
            else if (option.startsWith(MAXONLY_SWITCH)) {
                costModel = COSTMODEL_MAX;
            }
            else if (option.startsWith(SWITCH_START)) {
                throw new RuntimeException("bad switch " + option);
            }
            else if (inFileName != null) {
                throw new RuntimeException("need exactly one non-switch name");
            }
            else inFileName = option;
        }

        if (inFileName == null) {
            throw new RuntimeException("need exactly one non-switch name");
        }     

        
        ClassicTagging ct = new ClassicTagging(alpha, 1.0 - alpha, gamma, costModel, inFileName);
        ct.WIDTH=width;
        if(outFileName != null) {
          String s = HTMLoverhead(ct.solve(include5000));
          FileWriter bw = null;
          try {
           bw = new FileWriter(outFileName);
           bw.write(s);
          } finally {
           if(bw != null) bw.close();
          }
        } else 
          System.out.println(HTMLoverhead(ct.solve(include5000)));
          if(timings) { 
            System.gc();
            ct.timings(5000);
          }
    }
    
}
