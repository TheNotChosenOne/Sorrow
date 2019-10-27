SHELL := /bin/bash
EXEFILE=hall
ZIPFILE=hall.zip
DBGEXEFILE=halld
EAZEXEFILE=halle

SRCDIR=src
TMPDIR=.tmp
PDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
PCH_RLS=$(TMPDIR)/precompiledrls.h
PCH_DBG=$(TMPDIR)/precompileddbg.h
PCH_EAZ=$(TMPDIR)/precompiledeaz.h
PCH_INCLUDE=-include
PCH_SUFFIX=

CXX=g++
INCLUDES=-I$(PDIR).tmp/ -I$(PDIR)../ctti/include -I$(PDIR)src -I/usr/include/Box2D
CXXFLAGS=-Wall -Wextra -m64 --std=c++2a -DCGAL_DISABLE_ROUNDING_MATH_CHECK=ON -DBOOST_ALLOW_DEPRECATED_HEADERS $(INCLUDES)
ifneq ($(CXX),clang++)
	CXXFLAGS += -frename-registers
else
	PCH_INCLUDE=-include-pch
	PCH_SUFFIX=.pch
endif

HARDFLAGS=-march=native -ftree-vectorize -funroll-loops
EAZFLAGS=-O0 -g -ggdb
RLSFLAGS=-Ofast -g -funsafe-math-optimizations -flto -fno-signed-zeros -fno-trapping-math -ffast-math -msse2
DBGFLAGS=-Og -ggdb -g
LINKFLAGS=-lGL -lGLEW -lSDL2 -lboost_program_options -lCGAL -lBox2D -lpthread

CPP_PAT ::= $(SRCDIR)/%.cpp
OBJ_PAT ::= $(TMPDIR)/%-rls.o
DBG_PAT ::= $(TMPDIR)/%-dbg.o
EAZ_PAT ::= $(TMPDIR)/%-eaz.o

CPPFILES ::= $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*/*.cpp)
SRCFILES ::= $(CPPFILES) $(wildcard $(SRCDIR)/*.h $(SRCDIR)/*/*.h)
OBJFILES ::= $(CPPFILES:$(CPP_PAT)=$(OBJ_PAT))
DBGFILES ::= $(CPPFILES:$(CPP_PAT)=$(DBG_PAT))
EAZFILES ::= $(CPPFILES:$(CPP_PAT)=$(EAZ_PAT))

.PHONY: all clean test fast dbg rls

rls: $(PCH_RLS).pch $(EXEFILE)

$(EXEFILE) : $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(HARDFLAGS) $(RLSFLAGS) $(LINKFLAGS) $^ -o $@

$(OBJFILES) : $(OBJ_PAT) : $(CPP_PAT) $(PCH_RLS).pch
	$(CXX) $(CXXFLAGS) $(HARDFLAGS) $(RLSFLAGS) $(PCH_INCLUDE) $(PCH_RLS)$(PCH_SUFFIX) -c -MMD $< -o $@

dbg: $(PCH_DBG).pch $(DBGEXEFILE)

$(DBGEXEFILE) : $(DBGFILES)
	$(CXX) $(CXXFLAGS) $(HARDFLAGS) $(DBGFLAGS) $(LINKFLAGS) $^ -o $@

$(DBGFILES) : $(DBG_PAT) : $(CPP_PAT) $(PCH_DBG).pch
	$(CXX) $(CXXFLAGS) $(HARDFLAGS) $(DBGFLAGS) $(PCH_INCLUDE) $(PCH_DBG)$(PCH_SUFFIX) -c -MMD $< -o $@

fast : $(PCH_EAZ).pch $(EAZEXEFILE)

$(EAZEXEFILE): $(EAZFILES)
	$(CXX) $(CXXFLAGS) $(EAZFLAGS) $(LINKFLAGS) $^ -o $@

$(EAZFILES) : $(EAZ_PAT) : $(CPP_PAT) $(PCH_EAZ).pch
	$(CXX) $(CXXFLAGS) $(EAZFLAGS) $(PCH_INCLUDE) $(PCH_EAZ)$(PCH_SUFFIX) -c -MMD $< -o $@

HEADER_RGX="^\#include <"
FRESH := $(shell grep -h $(HEADER_RGX) $(SRCFILES) | sort | uniq)

CURRENT_RLS := $(shell cat $(PCH_RLS) 2>/dev/null)
$(PCH_RLS) : $(SRCFILES) $(TMPDIR)/
ifneq ($(FRESH),$(CURRENT_RLS))
	grep -h $(HEADER_RGX) $(SRCFILES) | sort | uniq >$(PCH_RLS)
endif

$(PCH_RLS).pch : $(PCH_RLS)
	$(CXX) $(CXXFLAGS) $(HARDFLAGS) $(RLSFLAGS) -x c++-header -c $(PCH_RLS) -o $(PCH_RLS).pch

CURRENT_DBG := $(shell cat $(PCH_DBG) 2>/dev/null)
$(PCH_DBG) : $(SRCFILES) $(TMPDIR)/
ifneq ($(FRESH),$(CURRENT_DBG))
	grep -h $(HEADER_RGX) $(SRCFILES) | sort | uniq >$(PCH_DBG)
endif

$(PCH_DBG).pch : $(PCH_DBG)
	$(CXX) $(CXXFLAGS) $(HARDFLAGS) $(DBGFLAGS) -x c++-header -c $(PCH_DBG) -o $(PCH_DBG).pch

CURRENT_EAZ := $(shell cat $(PCH_EAZ) 2>/dev/null)
$(PCH_EAZ) : $(SRCFILES) $(TMPDIR)/
ifneq ($(FRESH),$(CURRENT_EAZ))
	grep -h $(HEADER_RGX) $(SRCFILES) | sort | uniq >$(PCH_EAZ)
endif

$(PCH_EAZ).pch : $(PCH_EAZ)
	$(CXX) $(CXXFLAGS) $(EAZFLAGS) -x c++-header -c $(PCH_EAZ) -o $(PCH_EAZ).pch

$(foreach obj,$(OBJFILES),$(eval $(obj) : | $(dir $(obj))))
$(foreach obj,$(EAZFILES),$(eval $(obj) : | $(dir $(obj))))
$(foreach obj,$(DBGFILES),$(eval $(obj) : | $(dir $(obj))))

$(ZIPFILE) : $(SRCFILES) Makefile
	zip $@ $^

$(TMPDIR)/ :
	mkdir $@

$(TMPDIR)/%/ : | $(TMPDIR)/
	mkdir $@

# Phony Rules
all : $(EXEFILE) $(DBGEXEFILE) $(EAZFILES) $(ZIPFILE)

clean :
	rm -rf .tmp $(EXEFILE) $(DBGEXEFILE) $(EAZEXEFILE)

test :
	make -C $(TSTDIR)

DEP_PAT ::= $(TMPDIR)/%.d
DEPFILES ::= $(patsubst $(OBJ_PAT),$(TMPDIR)/%-rls.d,$(OBJFILES)) $(patsubst $(DBG_PAT),$(TMPDIR)/%-dbg.d,$(DBGFILES)) $(patsubst $(EAZ_PAT),$(TMPDIR)/%-eaz.d,$(EAZFILES))
-include $(DEPFILES)
