MAYA_LOCATION := /usr/autodesk/maya
MAYA_PLUGIN_PATH ?= $(HOME)/maya/plugins/plug-ins
NODE_NAME_PREFIX := sik_

MAKEFLAGS += -j
CXX ?= g++
SRC_PATHS := src 
BUILD_PATH := build
OBJ_PATH := $(BUILD_PATH)/obj
#BIN_PATH := $(BUILD_PATH)/bin
BIN_PATH := $(MAYA_PLUGIN_PATH)
BUILD_NAME ?= $(BIN_PATH)/simpleik.so

SRC_EXT := cpp

SOURCES := $(foreach srcpath, $(SRC_PATHS), $(wildcard $(srcpath)/*.$(SRC_EXT)))
OBJECTS := $(addprefix $(OBJ_PATH)/,$(notdir $(SOURCES:.$(SRC_EXT)=.o)))
DEPS := $(OBJECTS:.o=.d)

CXXFLAGS := $(CXXFLAGS) -g -fPIC -std=c++11 -O3 -DUSE_SSE -mssse3 -ffast-math -freciprocal-math -fno-finite-math-only -fvect-cost-model -DNODE_NAME_PREFIX=\"$(NODE_NAME_PREFIX)\"
LDFLAGS = -g -shared -L$(MAYA_LOCATION)/lib -lOpenMaya -pthread -DLINUX -ffast-math -lFoundation -lImage -Wall

INCLUDES := \
	-I. \
	-Iinclude \
	-I/usr/local/include \
	-I$(MAYA_LOCATION)/include \
	-I/usr/X11R6/include 

.PHONY: default
default : release

.PHONY: release
release: dirs
	@$(MAKE) all

.PHONY: dirs
dirs:
	@echo "Creating directories"
	@mkdir -p $(OBJ_PATH)
	@mkdir -p $(BIN_PATH)

.PHONY: clean
clean:
	@$(foreach s, $(OBJECTS), echo $(s);)
	@echo $(BUILD_NAME)
	@echo "Removing all build files"
	@rm  $(BUILD_NAME)
	@rm -r $(BUILD_PATH)

.PHONY: all
all: $(BUILD_NAME)

$(BUILD_NAME): $(OBJECTS)
	@$(CXX) -o $@ $^ $(LDFLAGS)

-include $(DEPS)

define MY_RULE
%.d: $(1)/%.$(SRC_EXT)
	@$(CXX) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) >$@

$(OBJ_PATH)/%.o: $(1)/%.$(SRC_EXT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<
endef

$(foreach srcpath, $(SRC_PATHS), $(eval $(call MY_RULE, $(srcpath))))

define MAKE_DEP_OBJ
%.d: $(1)/%.$(SRC_EXT)
	@$(CXX) $(CXXFLAGS) $$< -MM -MT $$(@:.d=.o) >$$@

$(OBJ_PATH)/%.o: $(1)/%.$(SRC_EXT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $$@ $$<
endef

$(foreach srcpath, $(SRC_PATHS), $(eval $(call MAKE_DEP_OBJ, $(srcpath))))

