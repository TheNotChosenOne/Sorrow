EXEFILE=hall
ZIPFILE=hall.zip
DBGEXEFILE=halld

SRCDIR=src
TMPDIR=.tmp
PDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

CXX=g++
CXXFLAGS=-Wall -Wextra --std=c++2a -I$(PDIR)../ctti/include -march=native -ftree-vectorize -funroll-loops -DCGAL_DISABLE_ROUNDING_MATH_CHECK=ON -I$(PDIR)src -I/usr/include/Box2D -DBOOST_ALLOW_DEPRECATED_HEADERS
RLSFLAGS=-Ofast -g -funsafe-math-optimizations -flto -fno-signed-zeros -fno-trapping-math -ffast-math -msse2 -frename-registers
DBGFLAGS=-Og -ggdb -g
LINKFLAGS=-lGL -lGLEW -lSDL2 -lboost_program_options -lCGAL -lBox2D -lpthread

CPP_PAT ::= $(SRCDIR)/%.cpp
OBJ_PAT ::= $(TMPDIR)/%.o
DBG_PAT ::= $(TMPDIR)/%-dbg.o
DEP_PAT ::= $(TMPDIR)/%.d

CPPFILES ::= $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*/*.cpp)
OBJFILES ::= $(CPPFILES:$(CPP_PAT)=$(OBJ_PAT))
DBGFILES ::= $(CPPFILES:$(CPP_PAT)=$(DBG_PAT))
DEPFILES ::= $(patsubst $(OBJ_PAT),$(DEP_PAT), $(OBJFILES) $(DBGFILES))

.PHONY: all clean test

$(EXEFILE) : $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(RLSFLAGS) $(LINKFLAGS) $^ -o $@

$(OBJFILES) : $(OBJ_PAT) : $(CPP_PAT)
	$(CXX) $(CXXFLAGS) $(RLSFLAGS) -c -MMD $< -o $@

$(DBGEXEFILE) : $(DBGFILES)
	$(CXX) $(CXXFLAGS) $(DBGFLAGS) $(LINKFLAGS) $^ -o $@

$(DBGFILES) : $(DBG_PAT) : $(CPP_PAT)
	$(CXX) $(CXXFLAGS) $(DBGFLAGS) -c -MMD $< -o $@

$(foreach obj,$(OBJFILES) $(DBGFILES),$(eval $(obj) : | $(dir $(obj))))

$(ZIPFILE) : $(CPPFILES) Makefile
	zip $@ $^

$(TMPDIR)/ :
	mkdir $@

$(TMPDIR)/%/ : | $(TMPDIR)/
	mkdir $@

# Phony Rules
all : $(EXEFILE) $(DBGEXEFILE) $(ZIPFILE)

clean :
	rm -rf .tmp hall halld

test :
	make -C $(TSTDIR)

-include $(DEPFILES)
