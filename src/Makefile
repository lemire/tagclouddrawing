# #############################################################################
#	Make File for:	Tag cloud layout via mincut floorplanning
# #############################################################################
#
#<- ----------------------------------------------------------------------- ->
##
#	MACROS used:
#		PREFIX  Absolute path name prefix of all files
#		OBJDIR: Destination object directory
#               CFLAGS: flags for C compiler
#
#<- ----------------------------------------------------------------------- ->
#
# EDIT HERE ------------ EDIT HERE ---------- EDIT HERE ---------- EDIT HERE -
#
#	 vvvvvvvvvvvvvvvvvvvvvvvvvvvvv
PREFIX=$(PWD)/..
#	 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
# $(PWD)/.. is a reasonable default for PREFIX for many people
#<= ======================================================================= =>
#
OBJDIR = $(PREFIX)/bin
CFLAGS = -g -Wall -O2 #-DBLAB
CFLAGS1 = $(CFLAGS) -c

TAGCLOUD_FLAGS =  -DFIXEDWIDTH=550

PROJFLAGS = $(TAGCLOUD_FLAGS)
CCOMP = gcc   # although previous incarnations of this code have been built
	      # with several compilers

#list of files that are part of the "release"
MANIFEST = floorplan-code/src/*.[ch] floorplan-code/src/Makefile \
			floorplan-code/README floorplan-code/bin floorplan-code/data/owen.pl \
			floorplan-code/data/gut.sh floorplan-code/data/owen1.pl \
			floorplan-code/data/collocation-tags.pl floorplan-code/data/make_datafile \
			floorplan-code/data/cloud.css floorplan-code/data/cloudtable.css \
			flat-tables/cloud.css flat-tables/cloudtable.css flat-tables/converttag.sh \
			flat-tables/alsoopeninfirefox.template flat-tables/taggeometry.js \
			flat-tables/Tag.java flat-tables/WeightTagComparator.java flat-tables/HeightTagComparator.java \
			flat-tables/ClassicTagging.java flat-tables/HeightWidthTagComparator.java \
			flat-tables/data/Knowledge.xml.csv README-TAGPLACEMENT \
			flat-tables/firefoxlauncher.sh floorplan-code/data/firefoxlauncher.sh

#
#<- ----------------------------------------------------------------------- ->
#
all:	$(OBJDIR)/floorp
	@echo all done

$(OBJDIR)/floorp:	floorp.c mcplace.h $(OBJDIR)/mcplace.o size.h $(OBJDIR)/size.o graph.h floorp.h slice.h html.h  $(OBJDIR)/graph.o $(OBJDIR)/slice.o $(OBJDIR)/mincut.o $(OBJDIR)/html.o  Makefile
			$(CCOMP) $(CFLAGS) $(PROJFLAGS) -o $(OBJDIR)/floorp\
			floorp.c $(OBJDIR)/size.o $(OBJDIR)/slice.o $(OBJDIR)/mincut.o\
			$(OBJDIR)/mcplace.o $(OBJDIR)/graph.o \
			$(OBJDIR)/html.o  -lm
$(OBJDIR)/slice.o:	slice.c slice.h graph.h floorp.h Makefile
		$(CCOMP) $(CFLAGS1) $(PROJFLAGS)  slice.c
		mv slice.o $(OBJDIR)
$(OBJDIR)/html.o:	html.c html.h graph.h floorp.h Makefile
		$(CCOMP) $(CFLAGS1) $(PROJFLAGS)  html.c
		mv html.o $(OBJDIR)
$(OBJDIR)/graph.o:	graph.c slice.h graph.h floorp.h Makefile
		$(CCOMP)  $(CFLAGS1) $(PROJFLAGS)   -DGRAPH graph.c
		mv graph.o $(OBJDIR)
$(OBJDIR)/size.o:		size.c size.h graph.h floorp.h slice.h Makefile
		$(CCOMP)  $(CFLAGS1) $(PROJFLAGS)  size.c
		mv size.o $(OBJDIR)
$(OBJDIR)/mcplace.o:	mcplace.c mcplace.h graph.h floorp.h slice.h mincut.h Makefile
		$(CCOMP)  $(CFLAGS1) $(PROJFLAGS)  mcplace.c
		mv mcplace.o $(OBJDIR)
$(OBJDIR)/mincut.o:	mincut.c mincut.h graph.h floorp.h slice.h  Makefile
		$(CCOMP)  $(CFLAGS1) $(PROJFLAGS)  -DMINCUT mincut.c
		mv mincut.o $(OBJDIR)
placetest:	$(OBJDIR)/floorp Makefile
		cd ../data ; $(OBJDIR)/floorp owen
		@echo
		@echo ought to say area is about 75, I think. 
classictest: 	Makefile
		cd ../../flat-tables ; javac ClassicTagging.java; \
			java ClassicTagging data/Knowledge.xml.csv -OKnowledge.html; \
			echo "now run browser to see Knowledge.html in ../../flat-tables"
guttest:	$(OBJDIR)/floorp Makefile
		cd ../data; ./gut.sh ../src/mincut.c 20
clean:	
		@touch $(OBJDIR)/shutup.o $(OBJDIR)/floorp $(OBJDIR)/shutup~
		rm $(OBJDIR)/*.o $(OBJDIR)/floorp $(OBJDIR)/*~

tag-release:	clean
		@echo go to parent of floorplan-code and flat-tables	
		cd  ../.. ;\
		tar cvfz lemur-tagging-0.0.tar.gz $(MANIFEST);\
		zip lemur-tagging-0.0.zip $(MANIFEST)
