NETWORK_COMMON_SRCDIR = ../networkcommon/source
COMMON_SRCDIR = ../common

# Common source files
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
$(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
$(NETWORK_COMMON_SRCDIR)/message.cpp \
$(COMMON_SRCDIR)/timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/gxcrc32.cpp \
$(COMMON_SRCDIR)/nvwa/debug_new.cpp \

# Include paths
INCLUDES = -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" \
-I "../networkcommon/source/ev" -I "../networkcommon/source/quiche" \
-I "$(COMMON_SRCDIR)/libbson-1.0" -I "$(COMMON_SRCDIR)/zlib"

# Main source directory
SRCDIR = ./qh3client

# Application source files (includes main.cpp for non-test builds)
APP_SRCS = $(SRCDIR)/main.cpp \
$(SRCDIR)/qh3client.cpp \
$(SRCDIR)/qh3client_helper.cpp \
$(SRCDIR)/http3_sample_client.cpp \
$(NCOMMONSRCS)

# Test-specific source files (excludes main.cpp and includes test_main.cpp)
TEST_SRCS = $(filter-out $(SRCDIR)/main.cpp, $(APP_SRCS)) \
$(SRCDIR)/test/test_main.cpp \
$(SRCDIR)/test/test_qh3client.cpp

# Object files (for non-test builds)
APP_OBJS = ${APP_SRCS:.cpp=.o}

# Object files (for test builds)
TEST_OBJS = ${TEST_SRCS:.cpp=.o}

LOG_LEVEL ?= 1 
USER_DEFINES = -DLOG_LEVEL=$(LOG_LEVEL)

CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden -W -Wall -fPIC
CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden -fPIC

# Debug and Release specific flags are better placed in their respective build rules
DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1
RELEASE_CXXFLAGS = -O2 -DPROD_BUILD=1

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
    LIBS = ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a ../common/libs/linux/libz.a ../common/libs/linux/libbson-static-1.0.a
endif
ifeq ($(UNAME), Darwin)
    # Define a variable that gets the architecture
    ARCH := $(shell uname -m)
    MACOS_LIB_DIR = macos_$(ARCH)
    LIBS = ../networkcommon/libs/$(MACOS_LIB_DIR)/libquiche.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libev.a \
    ../common/libs/$(MACOS_LIB_DIR)/libz.a ../common/libs/$(MACOS_LIB_DIR)/libbson-static-1.0.a -framework CoreFoundation -framework Security

    DEBUG_CXXFLAGS += -mmacosx-version-min=12.3
    RELEASE_CXXFLAGS += -mmacosx-version-min=12.3
    LDFLAGS += -mmacosx-version-min=12.3
endif

EXE = qh3client-app
TEST_EXE = qh3client-test-app
CXX ?= clang
CC ?= clang

DEBUG_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: debug

clean:
	rm -f $(APP_OBJS) $(TEST_OBJS) $(EXE) $(TEST_EXE)

# Rebuild objects when Makefile changes
$(APP_OBJS): Makefile

# Debug build for main application
debug: CXXFLAGS += $(DEBUG_CXXFLAGS)
debug: $(APP_OBJS)
	$(CXX) $(DEBUG_LDFLAGS) -o $(EXE) $(APP_OBJS) $(LIBS)

# Release build for main application
release: CXXFLAGS += $(RELEASE_CXXFLAGS)
release: $(APP_OBJS)
	$(CXX) $(RELEASE_LDFLAGS) -o $(EXE) $(APP_OBJS) $(LIBS)

# Test build - Exclude main.cpp and include test_main.cpp
test: CXXFLAGS += $(RELEASE_CXXFLAGS) $(shell pkg-config --cflags gtest)
test: LIBS += $(shell pkg-config --libs gtest)
test: $(TEST_OBJS)
	$(CXX) $(RELEASE_LDFLAGS) -o $(TEST_EXE) $(TEST_OBJS) $(LIBS)

test-debug: CXXFLAGS += $(DEBUG_CXXFLAGS) $(shell pkg-config --cflags gtest)
test-debug: LIBS += $(shell pkg-config --libs gtest)
test-debug: $(TEST_OBJS)
	$(CXX) $(DEBUG_LDFLAGS) -o $(TEST_EXE) $(TEST_OBJS) $(LIBS)