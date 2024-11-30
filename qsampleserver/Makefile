# Directories
NETWORK_COMMON_SRCDIR = ../networkcommon/source
QHIREDIS_SRCDIR = ../qhiredis/source
SERVER_COMMON_SRCDIR = ../servercommon/source
COMMON_SRCDIR = ../common
QUTILS_SRCDIR = ../qutils
SRCDIR = ../qserver/source
MAIN_SRCDIR = ./qsampleserver
ZK_SRCDIR = ../qzookeeper

# Source files for the static library
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
              $(NETWORK_COMMON_SRCDIR)/qbuffer.cpp \
              $(NETWORK_COMMON_SRCDIR)/qtextfilelogger.cpp \
			  $(NETWORK_COMMON_SRCDIR)/qstatslogger.cpp \
			  $(NETWORK_COMMON_SRCDIR)/qcustomlogger.cpp \
              $(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
			  $(NETWORK_COMMON_SRCDIR)/qtimer_uv.cpp \
              $(NETWORK_COMMON_SRCDIR)/message.cpp \
			  $(NETWORK_COMMON_SRCDIR)/qtextfile.cpp \
			  $(NETWORK_COMMON_SRCDIR)/serverconfig.cpp \
              $(SERVER_COMMON_SRCDIR)/servercommon.cpp \
              $(QUTILS_SRCDIR)/discord_util.cpp \
              $(COMMON_SRCDIR)/timer.cpp \
              $(COMMON_SRCDIR)/sdktypes.cpp \
              $(COMMON_SRCDIR)/nvwa/debug_new.cpp \
			  $(COMMON_SRCDIR)/signal_handler/signal_handler.cpp \
              $(QHIREDIS_SRCDIR)/qhiredis.cpp \
              $(QHIREDIS_SRCDIR)/qhiredis_async.cpp \
			  $(ZK_SRCDIR)/source/qzookeeper.cpp \

# Source files for the executable
SRCS = $(SRCDIR)/room.cpp \
       $(SRCDIR)/roomserver.cpp \
       $(SRCDIR)/gameserver.cpp \
       $(SRCDIR)/qnetworkserver.cpp \
       $(NCOMMONSRCS)

MAIN_SRCS = $(MAIN_SRCDIR)/main.cpp

# Include paths
INCLUDES = -I "$(QUTILS_SRCDIR)" -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" \
           -I "../networkcommon/source/ev" -I "../networkcommon/source/quiche" \
           -I "../qhiredis/hiredis" -I "$(ZK_SRCDIR)/zookeeper"

# Object files
OBJS = $(SRCS:.cpp=.o)
MAIN_OBJS = ${MAIN_SRCS:.cpp=.o}

# Definitions and Flags
USER_DEFINES = -DTHREADED -DLOG_LEVEL=3 -DUSE_LIBUV=1
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden -W -Wall -fPIC
CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden -fPIC
DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1
RELEASE_CXXFLAGS = -O2 -DPROD_BUILD=1 -DNDEBUG

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBS = $(ZK_SRCDIR)/libs/linux/libzookeeper_mt.a ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a \
		   ../networkcommon/libs/linux/libuv.a \
	       ../qhiredis/libs/linux/libhiredis.a ../common/libs/linux/libz.a \
	       -pthread -Wl,--no-as-needed -ldl -ldpp -lssl -lcrypto -lcurl  -lsasl2
	LDFLAGS = -L$(ZK_SRCDIR)/libs/linux -L$(SERVER_COMMON_SRCDIR)/../libs/linux -L$(QUTILS_SRCDIR)/libs/linux
	RPATH_FLAGS = -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/linux -Wl,-rpath,$(QUTILS_SRCDIR)/libs/linux
endif
ifeq ($(UNAME),Darwin)
	ARCH := $(shell uname -m)
	MACOS_LIB_DIR = macos_$(ARCH)
	LIBS = ../networkcommon/libs/$(MACOS_LIB_DIR)/libquiche.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libev.a \
		   ../networkcommon/libs/$(MACOS_LIB_DIR)/libuv.a \
	       ../qhiredis/libs/$(MACOS_LIB_DIR)/libhiredis.a ../common/libs/$(MACOS_LIB_DIR)/libz.a \
	       -ldpp -framework CoreFoundation -framework Security -lcurl -lzookeeper_mt -lsasl2
	LDFLAGS = -L$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) -L$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) -L$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR)
	RPATH_FLAGS = -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR)
	DEBUG_CXXFLAGS += -mmacosx-version-min=12.4
	RELEASE_CXXFLAGS += -mmacosx-version-min=12.4
	LDFLAGS += -mmacosx-version-min=12.4
endif

EXE = qsampleserver-app
LIB = libqserver.a
CXX ?= clang
CC ?= clang

DEBUG_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CXFLAGS) -c $< -o $@

all: debug

clean:
	rm -f $(OBJS) $(MAIN_OBJS) $(LIB) $(EXE)

$(OBJS): Makefile
$(MAIN_OBJS): Makefile

debug: CXXFLAGS += $(DEBUG_CXXFLAGS)
debug: $(LIB) $(MAIN_OBJS)
	$(CXX) $(DEBUG_LDFLAGS) -o $(EXE) $(MAIN_OBJS) $(LIB) $(LIBS)

release: CXXFLAGS += $(RELEASE_CXXFLAGS)
release: $(LIB) $(MAIN_OBJS)
	$(CXX) $(RELEASE_LDFLAGS) -o $(EXE) $(MAIN_OBJS) $(LIB) $(LIBS)

$(LIB): $(OBJS)
ifeq ($(UNAME), Linux)
# libtool --mode=link $(CXX) -static -o $(LIB) $(OBJS)
	ar rcs $(LIB) $(OBJS)
else
	libtool -static -o $(LIB) $(OBJS)
endif
