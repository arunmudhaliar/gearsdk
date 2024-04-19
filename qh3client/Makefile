BUILDDIR = build

NETWORK_COMMON_SRCDIR = ../networkcommon/source
COMMON_SRCDIR = ../common
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
$(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
$(NETWORK_COMMON_SRCDIR)/message.cpp \
$(COMMON_SRCDIR)/timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/gxcrc32.cpp \
$(COMMON_SRCDIR)/nvwa/debug_new.cpp \

INCLUDES = -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" -I "../networkcommon/source/ev" -I "../networkcommon/source/quiche" -I "$(COMMON_SRCDIR)/libbson-1.0" -I "$(COMMON_SRCDIR)/zlib"
SRCDIR = ./qh3client
SRCS = $(SRCDIR)/main.cpp \
$(SRCDIR)/qh3client.cpp \
$(SRCDIR)/qh3client_helper.cpp \
$(SRCDIR)/http3_sample_client.cpp \
$(NCOMMONSRCS) \

OBJS = ${SRCS:.cpp=.o}

USER_DEFINES = -DLOG_LEVEL=1
# CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -frtti -std=c++17 -fsigned-char -fexceptions -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden
# CXFLAGS = -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBS = ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a ../common/libs/linux/libz.a ../common/libs/linux/libbson-static-1.0.a
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
	LIBS = ../networkcommon/libs/$(MACOS_LIB_DIR)/libquiche.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libev.a ../common/libs/$(MACOS_LIB_DIR)/libz.a ../common/libs/$(MACOS_LIB_DIR)/libbson-static-1.0.a -framework CoreFoundation -framework Security
endif

EXE = qh3client-app
CXX ?= clang

DEBUG_LDFLAGS = -W -Wall -g -O0 -DDEBUG
RELEASE_LDFLAGS = -W -Wall

#.cpp.o:
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

objects/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c
    	
all: debug

clean:
	rm -f $(OBJS) $(EXE)

debug: $(OBJS)
	$(CXX) $(DEBUG_LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

release: $(OBJS)
	$(CXX) $(RELEASE_LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)