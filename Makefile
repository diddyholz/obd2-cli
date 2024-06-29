CXX=g++
CXX_FLAGS=-g -Wall

LD=g++
LD_FLAGS=-g

SRC_DIR=src
LIB_DIR=lib

BUILD_DIR=obj
OUT_DIR=dist
OUT_NAME=obd2-cli

LIB_INCLUDES=$(foreach include,$(shell find $(LIB_DIR) -type d -name 'include'),-I$(include) )

CXX_SOURCES:=$(shell find $(SRC_DIR) -name '*.cpp') $(shell find $(LIB_DIR) -name '*.cpp')
OBJECTS:=$(addprefix $(BUILD_DIR)/,$(CXX_SOURCES:.cpp=.o))

$(OUT_DIR)/$(OUT_NAME): $(OBJECTS)
	mkdir -p $(dir $@)
	$(LD) -o $@ $(LD_FLAGS) $(OBJECTS)

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(LIB_INCLUDES) -c $< -o $@ $(CXX_FLAGS)

clean:
	rm -rf $(BUILD_DIR) $(OUT_DIR)
