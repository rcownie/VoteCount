#
# Makefile
#
#

FLEX     := /usr/bin/flex
CXX      := /usr/bin/g++
CXXFLAGS := -std=c++11 -ggdb -O2

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

all: $(PROGRAMS)

clean:
	-rm -f *.o $(PROGRAMS)


