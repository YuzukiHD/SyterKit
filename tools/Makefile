
BUILD_DIR=$(CURDIR)/build

MKSUNXI  = mksunxi
BINTOARR = bin2array 

MKSUNXI_CSRC    = mksunxi.c
MKSUNXI_COBJS   = $(addprefix $(BUILD_DIR)/,$(MKSUNXI_CSRC:.c=.o))

BINTOARR_CSRC   = bin2array.c
BINTOARR_COBJS   = $(addprefix $(BUILD_DIR)/,$(BINTOARR_CSRC:.c=.o))

INCLUDES = -I includes
CFLAGS   = -O2 -std=gnu99 $(INCLUDES)
CXXFLAGS = -O2 -std=gnu++11 $(INCLUDES)

CC  = gcc
CXX = g++

all: tools
tools: $(BUILD_DIR) $(MKSUNXI) $(BINTOARR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	rm -rf build
	rm -f $(MKSUNXI)
	rm -f $(BINTOARR)

$(BUILD_DIR)/%.o : %.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.opp : %.cpp
	@echo "  CXX   $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(MKSUNXI): $(MKSUNXI_COBJS)
	@$(CC) $(CFLAGS) $(BUILD_DIR)/mksunxi.o -o $(MKSUNXI)

$(BINTOARR): $(BINTOARR_COBJS)
	@$(CC) $(CFLAGS) $(BUILD_DIR)/bin2array.o -o $(BINTOARR)