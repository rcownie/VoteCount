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

RESULTS  := ks_2016.csv ny_2016.csv 

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

ny_2016.csv: votecount
	echo "votecount -a Candidate -s ny $(DATADIR_NY)/*_general_*precinct.csv" >a.gdb
	bash a.gdb
	#gdb votecount -x a.gdb
	

ks_2016.csv: votecount
	echo "votecount -a Candidate -s ks data/ks/2016_general_precinct.html" >a.gdb
	bash a.gdb
	#gdb votecount -x a.gdb


