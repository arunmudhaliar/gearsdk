BUILDDIR = build

NETWORK_COMMON_SRCDIR = ../networkcommon/source
COMMON_SRCDIR = ../common
QCLIENT_SRCDIR = ../qclient/source
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
$(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
$(NETWORK_COMMON_SRCDIR)/message.cpp \
$(COMMON_SRCDIR)/timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/nvwa/debug_new.cpp \

INCLUDES = -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" -I "$(NETWORK_COMMON_SRCDIR)/ev" -I "$(NETWORK_COMMON_SRCDIR)/uv" -I "$(NETWORK_COMMON_SRCDIR)/quiche" -I "$(COMMON_SRCDIR)/zlib"

SRCDIR = .
SRCS = $(SRCDIR)/main.cpp \
$(SRCDIR)/gclient.cpp \
$(QCLIENT_SRCDIR)/qnetworkclient.cpp \
$(NCOMMONSRCS) \

OBJS = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}

USER_DEFINES = -DLOG_LEVEL=2 -DUSE_PTHREAD=1

CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden -W -Wall -fPIC
CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden -fPIC

# Debug and Release specific flags are better placed in their respective build rules
DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1 -D_DEBUG_NEW_FILENAME_LEN=128
RELEASE_CXXFLAGS = -O2 -DPROD_BUILD=1 -DNDEBUG

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBS = ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a ../networkcommon/libs/linux/libuv.a -lz
endif
ifeq ($(UNAME),Darwin)
	# Define a variable that gets the architecture
	ARCH := $(shell uname -m)
	# ifeq ($(ARCH),arm64)
	# 	@echo "ARM architecture"
	# else ifeq ($(ARCH),x86_64)
	# 	@echo "x86_64 architecture"
	# else
	# 	@echo "Unsupported architecture - $(ARCH). Exiting !!!"
	# 	exit 1
	# endif
	MACOS_LIB_DIR = macos_$(ARCH)
	LIBS = ../networkcommon/libs/$(MACOS_LIB_DIR)/libquiche.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libev.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libuv.a -lz -framework CoreFoundation -framework Security

	DEBUG_CXXFLAGS += -mmacosx-version-min=12.4
	RELEASE_CXXFLAGS += -mmacosx-version-min=12.4
	LDFLAGS += -mmacosx-version-min=12.4
	LDFLAGS += -mmacosx-version-min=12.4
endif

EXE = qgfist-app
CXX ?= clang
CC ?= clang

DEBUG_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)

# DEBUG_LDFLAGS = -W -Wall -g -O0 -DDEBUG
# RELEASE_LDFLAGS = -W -Wall

#.cpp.o:
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

# objects/%.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c

# Add this rule back and adjust for the `objects/` directory, if you decide to use it.
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: debug

clean:
	rm -f $(OBJS) $(EXE)

# You can explicitly tell make that all your targets depend on the Makefile. This way, any change to the Makefile will trigger a rebuild.
$(OBJS): Makefile

debug: CXXFLAGS += $(DEBUG_CXXFLAGS)
debug: $(OBJS)
	$(CXX) $(DEBUG_LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

release: CXXFLAGS += $(RELEASE_CXXFLAGS)
release: $(OBJS)
	$(CXX) $(RELEASE_LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)