# Paths
NETWORK_COMMON_SRCDIR = ../networkcommon/source
SERVER_COMMON_SRCDIR = ../servercommon/source
COMMON_SRCDIR = ../common
QH3_CLIENT_SRCDIR = ../qh3client/qh3client
Q_CLIENT_SRCDIR = ../qclient/source
SRCDIR = ./source

# Source files
SRCS = $(SRCDIR)/qunitysdk.cpp \
       $(COMMON_SRCDIR)/nvwa/debug_new.cpp \
       $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
       $(Q_CLIENT_SRCDIR)/qnetworkclient.cpp \
       $(QH3_CLIENT_SRCDIR)/qh3client.cpp \
       $(QH3_CLIENT_SRCDIR)/qh3client_helper.cpp \
       $(COMMON_SRCDIR)/sdktypes.cpp
C_SRCS =

OBJS = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}
DEPS = ${OBJS:.o=.d}

# Includes and flags
INCLUDES = -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" -I "$(NETWORK_COMMON_SRCDIR)/ev" -I "$(NETWORK_COMMON_SRCDIR)/quiche"
USER_DEFINES = -DLOG_LEVEL=2 -DTHREADED -DUSE_PTHREAD=1
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden -W -Wall -MMD -MP -fPIC
CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden -fPIC

DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1
RELEASE_CXXFLAGS = -O2 -Os -DPROD_BUILD=1 -ffunction-sections -fdata-sections
# Platform-specific settings
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
    LINUX_LIB_DIR = linux
	LIBS = -L../networkcommon/libs/$(LINUX_LIB_DIR) -lev -L../networkcommon/libs/$(LINUX_LIB_DIR) -lquiche -lz
    LDFLAGS = -shared
	RPATH_FLAGS = -Wl,-rpath,'$$ORIGIN/../networkcommon/libs/$(LINUX_LIB_DIR)'
    LIB_EXT = so
endif
ifeq ($(UNAME), Darwin)
    ARCH := $(shell uname -m)
    MACOS_LIB_DIR = macos_$(ARCH)
    LIBS = -L../networkcommon/libs/$(MACOS_LIB_DIR) -lev -L../networkcommon/libs/$(MACOS_LIB_DIR) -lquiche -lz -framework Security
    LDFLAGS = -dynamiclib -install_name @rpath/libqunityplugin.dylib
    RPATH_FLAGS = -Wl,-rpath,@loader_path/
    LIB_EXT = dylib

    DEBUG_CXXFLAGS += -mmacosx-version-min=12.3
    RELEASE_CXXFLAGS += -mmacosx-version-min=12.3
    LDFLAGS += -mmacosx-version-min=12.3
endif

DEBUG_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS) -Wl,-dead_strip

# Targets
LIB = libqunityplugin.$(LIB_EXT)
CXX ?= clang++
CC ?= clang

# Compile C++ source files to object files and generate dependency files
%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -fvisibility=default -c $< -o $@

# Compile C source files to object files and generate dependency files
%.o: %.c Makefile
	$(CC) $(CXFLAGS) -c $< -o $@

# Default target
all: debug

# Clean target
clean:
	rm -f $(OBJS) $(DEPS) $(LIB)

# Include dependency files
-include $(DEPS)

# Target dependencies
debug: CXXFLAGS += $(DEBUG_CXXFLAGS)
debug: $(LIB)

release: CXXFLAGS += $(RELEASE_CXXFLAGS)
release: $(LIB)
	strip -x $(LIB)  # Strip local symbols to reduce file size

$(LIB): $(OBJS)
	$(CXX) $(DEBUG_LDFLAGS) -o $(LIB) $(OBJS) $(LIBS)

# Phony targets
.PHONY: all clean debug release
