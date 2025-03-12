# Paths
NETWORK_COMMON_SRCDIR = ../networkcommon/source
QHIREDIS_SRCDIR = ../qhiredis/source
SERVER_COMMON_SRCDIR = ../servercommon/source
COMMON_SRCDIR = ../common
QUTILS_SRCDIR = ../qutils
ZK_SRCDIR = ../qzookeeper
QH3_CLIENT_SRCDIR = ../qh3client/qh3client
SRCDIR = ../qh3server/qh3server
MAIN_SRCDIR = ./qh3sampleserver

# Source files
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
$(NETWORK_COMMON_SRCDIR)/qbuffer.cpp \
$(NETWORK_COMMON_SRCDIR)/qtextfilelogger.cpp \
$(NETWORK_COMMON_SRCDIR)/qstatslogger.cpp \
$(NETWORK_COMMON_SRCDIR)/qcustomlogger.cpp \
$(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
$(NETWORK_COMMON_SRCDIR)/qtimer_uv.cpp \
$(NETWORK_COMMON_SRCDIR)/qtextfile.cpp \
$(NETWORK_COMMON_SRCDIR)/serverconfig.cpp \
$(NETWORK_COMMON_SRCDIR)/message.cpp \
$(QHIREDIS_SRCDIR)/qhiredis.cpp \
$(QHIREDIS_SRCDIR)/qhiredis_async.cpp \
$(SERVER_COMMON_SRCDIR)/qmongo/qmongo.cpp \
$(SERVER_COMMON_SRCDIR)/servercommon.cpp \
$(QUTILS_SRCDIR)/discord_util.cpp \
$(COMMON_SRCDIR)/timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/crypto_helper.cpp \
$(COMMON_SRCDIR)/nvwa/debug_new.cpp \
$(COMMON_SRCDIR)/signal_handler/signal_handler.cpp \
$(COMMON_SRCDIR)/serverinforeader.cpp \
$(QH3_CLIENT_SRCDIR)/qh3client_helper.cpp \
$(QH3_CLIENT_SRCDIR)/qh3client.cpp \
$(ZK_SRCDIR)/source/qzookeeper.cpp

SRCS = $(SRCDIR)/qh3server.cpp \
$(SRCDIR)/qh3router.cpp \
$(SRCDIR)/http3_command_server.cpp \
$(SRCDIR)/http3_sample_server.cpp \
$(SRCDIR)/qh3router_structs.cpp \
$(NCOMMONSRCS)

C_SRCS = $(COMMON_SRCDIR)/crypto/sha256.c

MAIN_SRCS = $(MAIN_SRCDIR)/main.cpp

OBJS = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}
MAIN_OBJS = ${MAIN_SRCS:.cpp=.o}

LOG_LEVEL ?= 2
USER_DEFINES = -DLOG_LEVEL=$(LOG_LEVEL)

FORK_QH3_SERVER ?= 1
USER_DEFINES += -DFORK_QH3_SERVER=$(FORK_QH3_SERVER)

# Includes and flags
INCLUDES = -I "$(ZK_SRCDIR)/zookeeper" -I "$(QUTILS_SRCDIR)" -I "$(QH3_CLIENT_SRCDIR)" -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" -I "$(NETWORK_COMMON_SRCDIR)/ev" -I "$(NETWORK_COMMON_SRCDIR)/uv" -I "$(NETWORK_COMMON_SRCDIR)/quiche" -I "$(COMMON_SRCDIR)/libbson-1.0" -I "$(COMMON_SRCDIR)/zlib" -I "../qhiredis/hiredis" -I "$(SERVER_COMMON_SRCDIR)/libmongoc-1.0" -I "$(SERVER_COMMON_SRCDIR)"

USER_DEFINES += -DTHREADED -DENABLE_ZK=1 -DUSE_LIBUV=1
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden -W -Wall -fPIC
CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden -fPIC

DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1
RELEASE_CXXFLAGS = -O2 -DPROD_BUILD=1 -DNDEBUG

# Platform-specific settings
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBS = $(ZK_SRCDIR)/libs/linux/libzookeeper_mt.a ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a ../networkcommon/libs/linux/libuv.a ../servercommon/libs/linux/libmongoc-static-1.0.a ../qhiredis/libs/linux/libhiredis.a ../common/libs/linux/libbson-static-1.0.a ../common/libs/linux/libz.a -lssl -lcrypto -lsasl2 -pthread -lrt -lzookeeper_mt -Wl,--no-as-needed -ldl -lsnappy -lzstd -ldpp -lresolv -lcurl -lunwind
	LDFLAGS = -L$(ZK_SRCDIR)/libs/linux -L$(SERVER_COMMON_SRCDIR)/../libs/linux -L$(QUTILS_SRCDIR)/libs/linux
	RPATH_FLAGS = -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/linux -Wl,-rpath,$(QUTILS_SRCDIR)/libs/linux
endif
ifeq ($(UNAME), Darwin)
	ARCH := $(shell uname -m)
	MACOS_LIB_DIR = macos_$(ARCH)
	LIBS = ../networkcommon/libs/$(MACOS_LIB_DIR)/libquiche.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libev.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libuv.a ../servercommon/libs/$(MACOS_LIB_DIR)/libmongoc-static-1.0.a ../qhiredis/libs/$(MACOS_LIB_DIR)/libhiredis.a ../common/libs/$(MACOS_LIB_DIR)/libbson-static-1.0.a ../common/libs/$(MACOS_LIB_DIR)/libz.a -framework CoreFoundation -framework Security -lzookeeper_mt -ldpp -lresolv -lsasl2 -lcurl
	LDFLAGS = -L$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) -L$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) -L$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR)
	RPATH_FLAGS = -Wl,-rpath,$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR)

	DEBUG_CXXFLAGS += -mmacosx-version-min=12.4
	RELEASE_CXXFLAGS += -mmacosx-version-min=12.4
	LDFLAGS += -mmacosx-version-min=12.4
endif

DEBUG_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)

# Targets
LIB = libqh3server.a
EXE = qh3sampleserver-app
CXX ?= clang
CC ?= clang

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