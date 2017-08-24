#
# Makefile
#
#

FLEX     := /usr/bin/flex
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

PROGRAMS := votecount

RESULTS  := ks_2016.csv ny_2016.csv 

all: $(PROGRAMS) $(RESULTS)

clean:
	-rm -f *.o $(PROGRAMS)

VOTECOUNT_OBJS := VoteCountMain.o ErrStatus.o Table.o

votecount: $(VOTECOUNT_OBJS) Makefile
	$(CXX) -o $@ $(CXXFLAGS) $(VOTECOUNT_OBJS)

DATADIR_NY := /localssd/rcownie/openelections-data-ny/2016

ny_2016.csv: votecount
	echo "votecount -a Candidate -s ny $(DATADIR_NY)/*_general_*precinct.csv" >a.gdb
	bash a.gdb
	#gdb votecount -x a.gdb
	

ks_2016.csv: votecount
	echo "votecount -a Candidate -s ks data/ks/2016_general_precinct.html" >a.gdb
	bash a.gdb
	#gdb votecount -x a.gdb


