EXEFILE=hall
ZIPFILE=hall.zip
DBGEXEFILE=halld

SRCDIR=src
TMPDIR=.tmp

CXX=g++
CXXFLAGS=-Wall -Wextra --std=c++17 -lGL -lGLEW -lSDL2 -I/home/swan/code/ctti/include -I/usr/include/python3.6m -lpython3.6m -lboost_program_options -ffast-math -msse2 -march=native -ftree-vectorize -funsafe-math-optimizations -flto -fno-signed-zeros -fno-trapping-math -frename-registers -funroll-loops -lCGAL
RLSFLAGS=-Ofast -g
DBGFLAGS=-Og -ggdb -g

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
	$(CXX) $(CXXFLAGS) $(RLSFLAGS) $^ -o $@

$(OBJFILES) : $(OBJ_PAT) : $(CPP_PAT)
	$(CXX) $(CXXFLAGS) $(RLSFLAGS) -c -MMD $< -o $@

$(DBGEXEFILE) : $(DBGFILES)
	$(CXX) $(CXXFLAGS) $(DBGFLAGS) $^ -o $@

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
