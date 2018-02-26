UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	SYSTEM ?= linux
else
	SYSTEM ?= macos
endif
SYSTEM.SUPPORTED = $(shell test -f makefiles/makefile.$(SYSTEM) && echo 1)

ifeq ($(SYSTEM.SUPPORTED), 1)
include makefiles/makefile.$(SYSTEM)
else
$(error "Platform '$(SYSTEM)' not supported")
endif

OPTFLAGS ?= -O3 -msse3 -march=native
#OPTFLAGS ?= -g3 -O0 #-fsanitize=address,undefined
DEFINES = -DDEBUG
INCLUDE_DIR += -I. -isystem vendor

INCLUDE_DIR += $(shell pkg-config --cflags $(LIBS))
LDFLAGS += $(shell pkg-config $(LIBS) --libs) -ljpeg -pthread

CXXFLAGS += -fPIC -Wall -Wextra -Wno-address
CXXFLAGS += $(INCLUDE_DIR)
CXXFLAGS += $(DEFINES) -std=c++11 $(OPTFLAGS)

LDFLAGS += $(OPTFLAGS)

SHELL = bash
OBJ_DIR = build
SRCDIRS = lib gl model rectangle vendor suncg
ccSRCS = $(shell find $(SRCDIRS) -name "*.cc" | sed 's/^\.\///g')
MAIN_SRCS := $(shell find -L -maxdepth 2 -name "*.cpp" | cut -c 3- | grep -v '^_')

OBJS = $(addprefix $(OBJ_DIR)/,$(ccSRCS:.cc=.o))
MAIN_OBJS  := $(addprefix $(OBJ_DIR)/, $(MAIN_SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d)
MAIN_DEPS = $(MAIN_OBJS:.o=.d)
BINS = $(MAIN_SRCS:.cpp=.bin)
SO = ../House3D/objrender.so

.PHONY: all clean run

all: $(BINS) $(SO)

ifneq ($(MAKECMDGOALS), clean)
sinclude $(DEPS)
sinclude $(MAIN_DEPS)
endif

$(BINS): %.bin: $(OBJS) $(OBJ_DIR)/%.o
	@echo "[bin] $@ ..."
	@$(CXX) $^ -o $@ $(CXXFLAGS) $(LDFLAGS)

$(SO): $(OBJS) python/pybind.cc python/house.cc
	@echo "[so] $@ ..."
	@$(CXX) $^ -fPIC -shared -o $@ $(CXXFLAGS) $(LDFLAGS) $(SOFLAGS)
	@echo "done."

$(OBJS): $(OBJ_DIR)/%.o: %.cc
	@echo "[cc] $< ..."
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

$(MAIN_OBJS): $(OBJ_DIR)/%.o: %.cpp
	@echo "[cpp] $< ..."
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

$(DEPS): $(OBJ_DIR)/%.d: %.cc Makefile
	@mkdir -p $(dir $@)
	@echo "[dep] $< ..."
	@$(CXX) $(CXXFLAGS) $(DEFINES) -MM -MT "$(OBJ_DIR)/$(<:.cc=.o) $(OBJ_DIR)/$(<:.cc=.d)" "$<"  > "$@"

$(MAIN_DEPS): $(OBJ_DIR)/%.d: %.cpp Makefile
	@mkdir -p $(dir $@)
	@echo "[dep] $< ..."
	@$(CXX) $(CXXFLAGS) $(DEFINES) -MM -MT "$(OBJ_DIR)/$(<:.cpp=.o) $(OBJ_DIR)/$(<:.cpp=.d)" "$<"  > "$@"

clean:
	@rm -rvf $(OBJ_DIR) $(BINS) $(SO)
