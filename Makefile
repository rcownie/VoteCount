#
# Makefile
#
#

FLEX     := /usr/bin/flex
CC       := /usr/bin/gcc
CXX      := /usr/bin/g++
CXXFLAGS := -std=c++11 -ggdb -O0

#
# General rule for C++ compilation
#
%.o: %.cpp
	$(CXX) -o $@ $(CXXFLAGS) -c $<

#
# Super-pessimistic rule for dependencies
#
%.o: Makefile *.h

PROGRAMS := votecount uncomment

RESULTS  := ks_2016.csv ks_cols.txt ks_vals.txt ny_2016.csv ny_cols.txt ny_vals.txt

all: $(PROGRAMS) votecount_2016.conf $(RESULTS)

clean:
	-rm -f *.o $(PROGRAMS)

uncomment: uncomment.lex
	$(FLEX) uncomment.lex
	$(CC) -o $@ lex.yy.c -lfl

VOTECOUNT_OBJS := VoteCountMain.o ErrStatus.o Table.o

votecount: $(VOTECOUNT_OBJS) Makefile
	$(CXX) -o $@ $(CXXFLAGS) $(VOTECOUNT_OBJS)

votecount_2016.conf: uncomment votecount_2016.conf+
	uncomment <votecount_2016.conf+ >$@

DATADIR_NY := /localssd/rcownie/openelections-data-ny/2016

ny_cols.txt: votecount Makefile
	votecount -a NoColumn -c empty.conf -s ny $(DATADIR_NY)/*_general_*precinct.csv >$@

ny_vals.txt: votecount Makefile
	votecount -a absentee_affidavit -s ny $(DATADIR_NY)/*_general_*precinct.csv >$@

ny_2016.csv: votecount
	echo "votecount -o $@ -s ny $(DATADIR_NY)/*_general_*precinct.csv" >a.gdb
	bash a.gdb
	#gdb votecount -x a.gdb

DATADIR_KS := /localssd/rcownie/openelections-data-ks/2016

ks_cols.txt: votecount Makefile
	votecount -a NoColumn -c empty.conf -s ks $(DATADIR_KS)/*_general_*precinct.csv >$@

ks_vals.txt: votecount Makefile
	votecount -a Candidate -s ks $(DATADIR_KS)/*_general_*precinct.csv >$@

ks_2016.csv: votecount
	echo "votecount -o $@ -s ks $(DATADIR_KS)/*_general_*precinct.csv" >a.gdb
	bash a.gdb
	#gdb votecount -x a.gdb


