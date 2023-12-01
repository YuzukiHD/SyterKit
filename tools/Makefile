
BUILD_DIR=$(CURDIR)/build

MKSUNXI = mksunxi

CSRC    = mksunxi.c
CXXSRC  =

COBJS   = $(addprefix $(BUILD_DIR)/,$(CSRC:.c=.o))
CXXOBJS = $(addprefix $(BUILD_DIR)/,$(CXXSRC:.cpp=.opp))

INCLUDES = -I includes -I ../lib
CFLAGS   = -O2 -std=gnu99 $(INCLUDES)
CXXFLAGS = -O2 -std=gnu++11 $(INCLUDES)

CC  ?= gcc
CXX ?= g++

all: tools
tools: $(BUILD_DIR) $(MKSUNXI)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf build
	rm -f $(MKSUNXI)

$(BUILD_DIR)/%.o : %.c
	echo "  CC    $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.opp : %.cpp
	echo "  CXX   $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MKSUNXI): $(COBJS)
	$(CC) $(CFLAGS) $(BUILD_DIR)/mksunxi.o -o $(MKSUNXI)
