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

RESULTS  := ks_2016.csv

all: $(PROGRAMS) $(RESULTS)

clean:
	-rm -f *.o $(PROGRAMS)

VOTECOUNT_OBJS := VoteCountMain.o ErrStatus.o Table.o

votecount: $(VOTECOUNT_OBJS) Makefile
	$(CXX) -o $@ $(CXXFLAGS) $(VOTECOUNT_OBJS)

ks_2016.csv: votecount
	echo "run -s ks data/ks/2016_general_precinct.html" >a.gdb
	gdb votecount -x a.gdb
	#votecount -o $@ -s ks data/ks/2016_general_precinct.html

