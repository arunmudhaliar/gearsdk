# Default to debug mode if no target is specified
BUILD_MODE ?= debug

# Check the target specified
ifeq ($(MAKECMDGOALS), debug)
    BUILD_MODE = debug
endif

ifeq ($(MAKECMDGOALS), release)
    BUILD_MODE = release
endif
$(info Building $(BUILD_MODE) mode.)

# Paths
NETWORK_COMMON_SRCDIR = ../networkcommon/source
SERVER_COMMON_SRCDIR = ../servercommon/source
COMMON_SRCDIR = ../common
QH3_CLIENT_SRCDIR = ../qh3client/qh3client
Q_CLIENT_SRCDIR = ../qclient/source
SRCDIR = ./source

# Source files common to both debug and release builds
SDK_SRCS = $(COMMON_SRCDIR)/sdktypes.cpp \
    $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
    $(Q_CLIENT_SRCDIR)/qnetworkclient.cpp \
    $(QH3_CLIENT_SRCDIR)/qh3client.cpp \
    $(QH3_CLIENT_SRCDIR)/qh3client_helper.cpp \
    $(SRCDIR)/qunitysdk.cpp

# Source files specific to debug build
DEBUG_SRCS = $(COMMON_SRCDIR)/nvwa/debug_new.cpp

# Build directories
DEBUG_DIR = debug_build
RELEASE_DIR = release_build

# Includes and flags
INCLUDES = -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" -I "$(NETWORK_COMMON_SRCDIR)/ev" -I "$(NETWORK_COMMON_SRCDIR)/quiche"
USER_DEFINES = -DLOG_LEVEL=2 -DTHREADED -DUSE_PTHREAD=1
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=default -W -Wall -MMD -MP -fPIC
# CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -s -ffunction-sections -fdata-sections -fvisibility=default -fPIC

DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1
RELEASE_CXXFLAGS = -O2 -Os -DPROD_BUILD=1 -DNDEBUG -ffunction-sections -fdata-sections
RELEASE_CXXFLAGS += -fmessage-length=0  -std=gnu++1z
RELEASE_CXXFLAGS += -Wno-trigraphs -fno-common -Wno-missing-field-initializers -Werror=return-type -Wunreachable-code 
RELEASE_CXXFLAGS += -Wno-non-virtual-dtor -Wno-overloaded-virtual -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function 
RELEASE_CXXFLAGS += -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wuninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-conversion
RELEASE_CXXFLAGS += -Wenum-conversion -Wno-float-conversion -fstrict-aliasing -Wdeprecated-declarations -Winvalid-offsetof
RELEASE_CXXFLAGS += -g -fvisibility-inlines-hidden -Wno-sign-conversion

# Platform-specific settings
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
    LINUX_LIB_DIR = linux
	LIBS = -L../networkcommon/libs/$(LINUX_LIB_DIR) -lev -L../networkcommon/libs/$(LINUX_LIB_DIR) -lquiche -lz
    LDFLAGS = -shared -Wl,--gc-sections
	RPATH_FLAGS = -Wl,-rpath,'$$ORIGIN/../networkcommon/libs/$(LINUX_LIB_DIR)'
    LIB_EXT = so
endif
ifeq ($(UNAME), Darwin)
    RELEASE_CXXFLAGS += -Wstrict-prototypes -Wno-semicolon-before-method-body -Wno-c++11-extensions -Wno-newline-eof -Wno-four-char-constants -Wno-exit-time-destructors
    RELEASE_CXXFLAGS += -Werror=non-modular-include-in-framework-module -Wno-missing-prototypes -Werror=deprecated-objc-isa-usage -Werror=objc-root-class -Wint-conversion 
    RELEASE_CXXFLAGS += -fmodules -gmodules -fdiagnostics-show-note-include-stack -fmacro-backtrace-limit=0 -fmodules-prune-interval=86400 -fmodules-prune-after=345600
    RELEASE_CXXFLAGS += -Wnon-modular-include-in-framework-module -fpascal-strings -Wdocumentation -Wquoted-include-in-framework-header -Wconditional-uninitialized -Wconstant-conversion
    RELEASE_CXXFLAGS += -Wbool-conversion -Wnon-literal-null-conversion -Wobjc-literal-conversion -Wshorten-64-to-32 -fasm-blocks -Winfinite-recursion -Wmove -Wcomma -Wblock-capture-autoreleasing -Wrange-loop-analysis -Wunguarded-availability
    ARCH := $(shell uname -m)
    MACOS_LIB_DIR = macos_$(ARCH)
    LIBS = -L../networkcommon/libs/$(MACOS_LIB_DIR) -lev -L../networkcommon/libs/$(MACOS_LIB_DIR) -lquiche -lz -framework Security
    LDFLAGS = -dynamiclib -install_name @rpath/libqunityplugin.dylib
    RPATH_FLAGS = -Wl,-rpath,@loader_path/
    LIB_EXT = dylib

    # Set architecture flags based on the detected architecture
    ifeq ($(ARCH), arm64)
        ARCH_FLAGS = -arch arm64
    else
        ARCH_FLAGS = -arch x86_64
    endif

    DEBUG_CXXFLAGS += -mmacosx-version-min=12.4 $(ARCH_FLAGS)
    RELEASE_CXXFLAGS += -mmacosx-version-min=12.4 $(ARCH_FLAGS)
    LDFLAGS += -mmacosx-version-min=12.4 $(ARCH_FLAGS) -Wl,-dead_strip
endif

DEBUG_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)

# Targets
DEBUG_LIB = $(DEBUG_DIR)/libqunityplugin.$(LIB_EXT)
RELEASE_LIB = $(RELEASE_DIR)/libqunityplugin.$(LIB_EXT)
CXX ?= clang++
CC ?= clang

# Compile and link debug
debug: $(DEBUG_LIB)

$(DEBUG_LIB): $(SDK_SRCS) $(DEBUG_SRCS) | $(DEBUG_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUG_CXXFLAGS) -o $@ $^ $(DEBUG_LDFLAGS) $(LIBS)

# Compile and link release
release: $(RELEASE_LIB)

$(RELEASE_LIB): $(SDK_SRCS) | $(RELEASE_DIR)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -o $@ $^ $(RELEASE_LDFLAGS) $(LIBS)
# strip -x $@  # Strip local symbols to reduce file size

# Create directories if they do not exist
$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)

$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

# Clean target
clean:
	rm -rf $(DEBUG_DIR) $(RELEASE_DIR)

# Default target
all: debug

.PHONY: all clean debug release
