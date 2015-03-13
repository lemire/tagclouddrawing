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


import java.util.*;
import java.io.*;
import java.util.regex.*;

/* Tags are immutable */

public class Tag {

    public static final boolean verbose = false;
    private static Pattern pattern = Pattern.compile("(.*?), (.*?), (.*?), (.*)");
    public static int spaceWidth = 0;

    private int fontHeight;
    private int fontWidth;
    private String text;
    private int boxHeight;
    private int boxWidth;
    private int emphs;


    /* This guessing about font height/width is now unused */
    // guess 1 pixels per point
    // This must match whatever goes on in the CSS

    // in 75dpi, 1 pt is about 1 pixel
    public static final int [] fontHeights = {8,10,12,14,16,18,20,22,24,32};
    public static final int [] fontWidths  = {8,10,12,14,16,18,20,22,24,32};
    public static final int MAX_FONTSPEC = fontHeights.length-1;
    
    private Tag() {}

    public Tag(int e, int fh, int fw, String t) {
        boxHeight = fontHeight = fontHeights[fh];
        fontWidth = fontWidths[fw];
        text = t.replaceAll("\\s+","&nbsp;");
        boxWidth = fontWidth*t.length();
        emphs = e;
    }

    private static int parseFontSize(String line) {
        assert line.length() > 0;
        char firstChar = line.charAt(0);
        
        if ( ! Character.isDigit(firstChar)) return 0;
        char secondChar = line.charAt(1);// may need more
        int ans = 0;
        if( ! Character.isDigit(secondChar))
          ans = firstChar - '0';
        else ans = Integer.parseInt(line.substring(0,2));
        if (ans > MAX_FONTSPEC) {
            System.out.println("largest font spec is " +
                               MAX_FONTSPEC + ":" +line);
            return MAX_FONTSPEC;
        }
        return ans;
    }

    private static String parseText(String line) {
        assert line.length() > 0;
        char firstChar = line.charAt(0);
        if ( ! Character.isDigit(firstChar)) return line;
        char secondChar = line.charAt(1);// may need more
        if( ! Character.isDigit(secondChar))
          return line.substring(1);
        return line.substring(2);
    }

    public Tag( String line) {
        this(parseFontSize(line), parseFontSize(line), parseFontSize(line), parseText(line));
    }

    public int getBoxHeight() { return boxHeight;}

    public int getBoxWidth() { return boxWidth;}
    
    public int getBoxArea() { return boxWidth * boxHeight;}

    public int getEmph() { return emphs;}

    public String getText() { return text;}


    public String toString() {
        return "fh="+fontHeight+" fw="+fontWidth+" text='"+text+"' bxhgt="+
            boxHeight+" bxwdth="+boxWidth;
    }
    
    /**
    * Checks to see if the input file is in Daniel's CSV format
    */
    public static  boolean CSVmode(File tagsFile) {
        try {
            BufferedReader r = new BufferedReader( new FileReader(tagsFile));
            String line;
            while ( (line = r.readLine()) != null) {
              if (line.matches(".*,.*,.*,.*")) {
                  r.close();
                  return true;
              }
            }
            r.close();
            return false;
        }
        catch (IOException e) {
            throw new RuntimeException("error reading tags file " + e);
        } 
    }
    public static Vector<Tag> readTags( File tagsFile) {
      return readTags(tagsFile,true);
    }

    public static Vector<Tag> readTags( File tagsFile, final boolean pruneduplicates) {
        // first, we check which type of file we have
        HashSet<String> keepingtrack = null;
        if(pruneduplicates) keepingtrack = new HashSet<String>();
        boolean csv = CSVmode(tagsFile);
        Vector<Tag> answer = new Vector<Tag>();
        boolean spaceWidthFound = false; // for sanity (see later)
        try {
            BufferedReader r = new BufferedReader( new FileReader(tagsFile));
            String line;
            while ( (line = r.readLine()) != null) {
                if (line.matches("\\s*")) continue;
                if(csv) {// entering csv mode
                  if (line.charAt(0)=='#') continue;   // comment
                  Matcher matcher = pattern.matcher(line);
                  if (!matcher.find()) {
                    System.err.println("BUG! "+line);
                    continue;
                  }
                  Tag t = new Tag();
                  t.text =   matcher.group(1);  
                  t.emphs = Integer.parseInt(matcher.group(2).trim());
                  t.boxWidth = Integer.parseInt(matcher.group(3).trim());
                  t.boxHeight = Integer.parseInt(matcher.group(4).trim());
                  if(t.text.trim().length() == 0) {
                    spaceWidth = t.boxWidth;
                    System.out.println("white space width:" + spaceWidth);
                    spaceWidthFound = true;
                    continue;
                  }
                  if(pruneduplicates) {
                    // we do not allow two tags with the same text
                    if(! keepingtrack.contains(t.text)) { 
                      answer.add(t);
                      keepingtrack.add(t.text);
                    }
                  } else 
                    // if you don't prune duplicates, then we don't care
                    answer.add(t);
                } else // we fall back to Owen's mode 
                answer.add( new Tag(line));
            }
            if(csv && !spaceWidthFound)
                System.err.println("You have a problem buddy, no space width data!!!");

            if (verbose) {
                System.out.println("Read tags:");
                for (Tag t : answer) System.out.println(t);
            }
            r.close();
            return answer;
        }
        catch (IOException e) {
            throw new RuntimeException("error reading tags file " + e);
        } 
    }

    public String getHTML() {
        StringBuffer sb = new StringBuffer();
        sb.append("<span class=\"tag"+ emphs +"\" "+ClassicTagging.mouseclick
        //+ " style='width:"+getBoxWidth()+"px' "
        +">"); 
        sb.append(text);
        sb.append("</span>");// the <wbr /> is a soft line break
        return sb.toString();
    }


    public static void main (String [] argv) {
        Vector<Tag> tv = Tag.readTags(new File( argv[0]));
    }
}
