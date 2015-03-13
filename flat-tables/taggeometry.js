
/******************************************************************************
Copyright (c) 2006-2007, Daniel Lemire

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


/**
* Its purpose is to do browser-specific things, eg getting bounding boxes
*
* good document for local file IO at
* http://developer.mozilla.org/en/docs/Code_snippets:File_I/O
*/

// To perform local file IO from JavaScript in Mozilla, use the XPCOM 
//nsIFile file IO library.



/***
* generic functions
*/
isDigit=function(text){return (text >= '0' && text <= '9');}

function trim(stringToTrim) {
        return stringToTrim.replace(/^\s+|\s+$/g,"");
}


/**
* functions specific to openinfirefox.html
*/

function processInputTagCloudFile(filePath) {
//   alert("reading    file"+filePath); 
  filecontent = xpcomFileRead(filePath);
  lines=filecontent.split("\n");

  z = document.getElementById("savedwarning");
  while(z.firstChild) z.removeChild(z.firstChild);
  z=document.getElementById('testingarea');  
  while(z.firstChild) z.removeChild(z.firstChild);
  arr = new Array();
  alternatives = new Array();
  alternatives.push(""); // standard font setting
  alternatives.push("e"); // wider but lower setting
  alternatives.push("n"); // skinnier but higher setting

  for (var alt=0; alt<alternatives.length; alt++) {
    for(var i=0; i<lines.length; i++) {
  
     tagtext = parseText(lines[i]);
     if(tagtext.length == 0) continue;
     tagweight = parseFontSize(lines[i]);
     x=document.createElement('span');
     x.className="tag"+tagweight+alternatives[alt];
     x.appendChild(document.createTextNode(tagtext));
     z.appendChild(x);
     gamma = document.createElement('span');
     gamma.appendChild(document.createTextNode(" "));
     z.appendChild(gamma);
     a=x.offsetWidth;b=x.offsetHeight;
     
     t = new Array(4);
     t[0] = tagtext;
     t[1] =tagweight;
     t[2] = a;
     t[3] = b;
     arr.push(t);
    }
    firstspace = document.getElementById('testingarea').getElementsByTagName('span')[1];
    w=firstspace.offsetWidth;h=firstspace.offsetHeight;
    t = new Array(4);
    t[0] = " ";
    t[1] = -1;
    t[2] = w;
    t[3] = h;
    arr.push(t);
  }
  z=document.getElementById('dataarea');
  while(z.firstChild) z.removeChild(z.firstChild);
  mycontent = "# tag, weight, width (pixels), height(pixels)\n# last tag is white space (fake)\n#after sentinel the data may be repeated for alt fonts\n"
  for(var i=0; i<arr.length; i++) {
    t= arr[i];
    //alert(t);
    mycontent= mycontent+ t[0]+", "+ t[1] + ", "+ t[2]+", "+ t[3]+ " "+"\n";
  }
  //alert(z.value);
  z.appendChild(document.createTextNode(mycontent));
  return xpcomFileWrite(filePath+".csv", mycontent);
}

/**
* this is Owen's little format 
*/
function parseFontSize(line) {
  //alert(line+ " "+line.charAt(0)+ " "+ isDigit(line.charAt(0))+" "+ parseInt(line.charAt(0)));
  if(!isDigit(line.charAt(0))) return 0;
  if(!isDigit(line.charAt(1))) 
    return parseInt(line.charAt(0));
  return parseInt(line.substring(0,2));
  
}
var re = new RegExp("\\s+", 'g');

function parseText(line) {
  line = trim(line);
  line = line.replace(re,"\u00A0");
  if(!isDigit(line.charAt(0))) return line;
  if(!isDigit(line.charAt(1))) 
    return line.substring(1);
  return line.substring(2);
}

/**
* follows some IO specific functions
*/

function xpcomFileWrite(filePath, content) {
  z = document.getElementById("savedwarning");
  while(z.firstChild) z.removeChild(z.firstChild);
 //alert("writing to file"+filePath);
        if (Components)        // XPConnect 'Components' object check.
                try {
                
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

                            // Get file component.

                        var file = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
                        file.initWithPath(filePath);
                        if (!file.exists()) {
                                ////alert('Creating new file ' + filePath);
                                file.create(0x00, 0644);
                        }

                        // Write with nsIFileOutputStream.

                        var outputStream = Components.classes["@mozilla.org/network/file-output-stream;1"].createInstance(Components.interfaces.nsIFileOutputStream);
                        outputStream.init(file, 0x20 | 0x02, 00004,null);
                        outputStream.write(content, content.length);
                        outputStream.flush();
                        outputStream.close();
                        z.appendChild(document.createTextNode("Saved to : "+filePath));
                        return true;
                }
                catch(e) {
                        alert(e);
                        return false;
                }
        return false;
}


function xpcomFileRead(filePath) {
//alert("about to read "+filePath);
        if (Components)
                try {
                
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
                        var file = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
                        file.initWithPath(filePath);
                        if (!file.exists()) {
                                alert('File '+filePath+' does not exist');
                                return false;
                        }
                        var inputStream = Components.classes["@mozilla.org/network/file-input-stream;1"].createInstance(Components.interfaces.nsIFileInputStream);
                        inputStream.init(file, 0x01, 00004, null);
                        var sInputStream = Components.classes["@mozilla.org/scriptableinputstream;1"].createInstance(Components.interfaces.nsIScriptableInputStream);
                        sInputStream.init(inputStream);
     // alert("Ok, I managed to read the file!!!");
                        return sInputStream.read(sInputStream.available());
                }
                catch(e) {
                        alert(e);
                        return false;
                }
        return false;
}




function listDirEntries(filePath) {
	if( Components) {
		try{
		 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
        var file = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath(filePath);
        if (!file.exists()) {
          alert('File '+filePath+' does not exist');
          return false;
        }
	// file is the given directory (nsIFile)
        var entries = file.directoryEntries;
        var array = [];
        while(entries.hasMoreElements())
        {
          var entry = entries.getNext();
          entry.QueryInterface(Components.interfaces.nsIFile);
          array.push(entry.path);
        }
	return array;
		}
                catch(e) {
			alert("try specifying a directory by manually erasing the filename");
                        alert(e);
                        return false;
                }
	} else {
		alert("are you using firefox?");
		return false;
	}

}
function processAllInput(filePath) {
	filePath = filePath.match(".*/")
	alert("I'm going to process all input files in "+filePath+".\n It may look like your browser freezes, but just wait.");
	var arr = listDirEntries(filePath);
	var counter = 0;
        for(var i=0; i<arr.length; i++) {
          filename= arr[i];
	  if(filename.match(".*\.input$")) {
		  if (processInputTagCloudFile(filename)) ++counter;
		  //alert(filename);
          }
	}
	alert(" I converted "+counter+" XML files to CSV.");

}


/**
* next, we learn to load zoomcloud XML files
*/

function processAllZoomCloudXML(filePath) {
	filePath = filePath.match(".*/")
	alert("I'm going to process all XML files in "+filePath+".\n It may look like your browser freezes, but just wait.");
	var arr = listDirEntries(filePath);
	var counter = 0;
        for(var i=0; i<arr.length; i++) {
          filename= arr[i];
	  if(filename.match(".*\.xml$")) {
		  if (processZoomCloudXML(filename)) ++counter;
		  //alert(filename);
          }
	}
	alert(" I converted "+counter+" XML files to CSV.");
}

/**
* arbitrarily, we require the weight to be between 0 and 9 (inclusively)
*/
function normalizeWeight(weight, minweight,maxweight) {
  var band = (maxweight-minweight+1.0)/10.0;
  return Math.floor((weight - minweight)/band);

}



function processZoomCloudXML(filePath,MinNumberofTags) {
   MinNumberofTags = typeof(MinNumberofTags) != 'undefined' ? MinNumberofTags : 30; 
//xmlhttp = new XMLHttpRequest();
//alert(filePath);
   var s = xpcomFileRead(filePath);
  // parse the string to a new doc
   var doc = (new DOMParser()).parseFromString(s, "text/xml");
   var maxweight = doc.evaluate("/cloud/tags/@maxweight", doc, null, XPathResult.NUMBER_TYPE, null).numberValue;
   var minweight = doc.evaluate("/cloud/tags/@minweight", doc, null, XPathResult.NUMBER_TYPE, null).numberValue;
   //alert("maxweight is "+maxweight+  " "+minweight);
   arr= new Array();

  alternatives = new Array();
  alternatives.push(""); // standard font setting
  alternatives.push("e"); // wider but lower setting
  alternatives.push("n"); // skinnier but higher setting

  for (var alt=0; alt<alternatives.length; alt++) {

   var tags = doc.evaluate("/cloud/tags/tag", doc, null, XPathResult.ANY_TYPE, null);
   var thisTag = tags.iterateNext();
   //var alertText = "My tags are:\n"
   z=document.getElementById('testingarea');  
   while(z.firstChild) z.removeChild(z.firstChild);

   while (thisTag) {
	   var tagweight = doc.evaluate("weight", thisTag, null, XPathResult.NUMBER_TYPE, null).numberValue;
	   tagweight = normalizeWeight(tagweight,minweight,maxweight);
	   var tagtext = doc.evaluate("name", thisTag, null, XPathResult.STRING_TYPE, null).stringValue;
           //alertText += tagtext+ weight + "\n"
	   tagtext = trim(tagtext);
           tagtext = tagtext.replace(re,"\u00A0");
           x=document.createElement('span');
           x.className="tag"+tagweight+alternatives[alt];
           x.appendChild(document.createTextNode(tagtext));
           z.appendChild(x);
           gamma = document.createElement('span');
           gamma.appendChild(document.createTextNode(" "));
           z.appendChild(gamma);
           mywidth=x.offsetWidth;myheight=x.offsetHeight;
           t = new Array(4);
           t[0] = tagtext;
           t[1] =tagweight;
           t[2] = mywidth;
           t[3] = myheight;
	   arr.push(t);

           thisTag = tags.iterateNext();
   }
   /* this test will automatically pass if it passes the first iteration */
   if(arr.length < MinNumberofTags) {
	   alert("File "+filePath+" only has "+arr.length+" tags and I require at least "+MinNumberofTags)
	   return false;
   }
   if( ! arr.length > 1) alert("this script will die");
   var firstspace = document.getElementById('testingarea').getElementsByTagName('span')[1];
   w=firstspace.offsetWidth;h=firstspace.offsetHeight;
   t = new Array(4);
   t[0] = " ";
   t[1] = -1;
   t[2] = w;
   t[3] = h;
   arr.push(t);
   }
  
   z=document.getElementById('dataarea');
   while(z.firstChild) z.removeChild(z.firstChild);
   //alert(z.value);
   mycontent = fromArrayToCSV(arr);
   z.appendChild(document.createTextNode(mycontent));
   xpcomFileWrite(filePath+".csv", mycontent);
   return true;
}

function fromArrayToCSV(arr) {
  mycontent = "# tag, weight, width (pixels), height(pixels)\n# last tag is white space (fake)\n#repeated for various fontsizes\n"
  for(var i=0; i<arr.length; i++) {
    t= arr[i];
    //alert(t);
    mycontent= mycontent+ t[0]+", "+ t[1] + ", "+ t[2]+", "+ t[3]+ " "+"\n";
  }
  return mycontent;
}
