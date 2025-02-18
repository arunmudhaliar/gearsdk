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

NETWORK_COMMON_SRCDIR = ../networkcommon/source
QHIREDIS_SRCDIR = ../qhiredis/source
SERVER_COMMON_SRCDIR = ../servercommon/source
COMMON_SRCDIR = ../common
QUTILS_SRCDIR = ../qutils
ZK_SRCDIR = ../qzookeeper
QH3_CLIENT_SRCDIR = ../qh3client/qh3client
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
$(COMMON_SRCDIR)/signal_handler/signal_handler.cpp \
$(COMMON_SRCDIR)/serverinforeader.cpp \
$(QH3_CLIENT_SRCDIR)/qh3client_helper.cpp \
$(QH3_CLIENT_SRCDIR)/qh3client.cpp \
$(ZK_SRCDIR)/source/qzookeeper.cpp \
$(COMMON_SRCDIR)/nvwa/debug_new.cpp \

INCLUDES = -I "$(ZK_SRCDIR)/zookeeper" -I "$(QUTILS_SRCDIR)" -I "$(QH3_CLIENT_SRCDIR)" -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" -I "$(NETWORK_COMMON_SRCDIR)/ev" -I "$(NETWORK_COMMON_SRCDIR)/uv" -I "$(NETWORK_COMMON_SRCDIR)/quiche" -I "$(COMMON_SRCDIR)/libbson-1.0" -I "$(COMMON_SRCDIR)/zlib" -I "../qhiredis/hiredis" -I "$(SERVER_COMMON_SRCDIR)/libmongoc-1.0" -I "$(SERVER_COMMON_SRCDIR)"
SRCDIR = ./qh3server
QSRCDIR = ./../qserver/source
SRCS = $(SRCDIR)/main.cpp \
$(SRCDIR)/qh3server.cpp \
$(SRCDIR)/qh3router.cpp \
$(SRCDIR)/http3_command_server.cpp \
$(SRCDIR)/http3_sample_server.cpp \
$(SRCDIR)/qh3router_structs.cpp \
$(SRCDIR)/../../serverplugin/plugin_qh3server.cpp \
$(SRCDIR)/../../serverplugin/plugin_gameserver.cpp \
$(QSRCDIR)/room.cpp \
$(QSRCDIR)/roomserver.cpp \
$(QSRCDIR)/gameserver.cpp \
$(QSRCDIR)/qnetworkserver.cpp \
$(NCOMMONSRCS) \

ifeq ($(BUILD_MODE), release)
    SRCS := $(filter-out $(COMMON_SRCDIR)/nvwa/debug_new.cpp, $(SRCS))
    $(info Excluding $(COMMON_SRCDIR)/nvwa/debug_new.cpp for release.)
endif

C_SRCS =$(COMMON_SRCDIR)/crypto/sha256.c \

OBJS = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}

LOG_LEVEL ?= 2
USER_DEFINES = -DLOG_LEVEL=$(LOG_LEVEL)

FORK_QH3_SERVER ?= 1
USER_DEFINES += -DFORK_QH3_SERVER=$(FORK_QH3_SERVER)

USER_DEFINES += -DTHREADED -DENABLE_ZK=1 -DUSE_LIBUV=1
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden -W -Wall -fPIC
CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden -fPIC

# Debug and Release specific flags are better placed in their respective build rules
DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1
RELEASE_CXXFLAGS = -O2 -DPROD_BUILD=1 -DNDEBUG

# LDFLAGS = -shared -rdynamic
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBS = $(ZK_SRCDIR)/libs/linux/libzookeeper_mt.a ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a ../networkcommon/libs/linux/libuv.a ../servercommon/libs/linux/libmongoc-static-1.0.a ../qhiredis/libs/linux/libhiredis.a ../common/libs/linux/libbson-static-1.0.a ../common/libs/linux/libz.a -lssl -lcrypto -lsasl2 -pthread -lrt -lzookeeper_mt -Wl,--no-as-needed -ldl -lsnappy -lzstd -ldpp -lresolv -lcurl -lunwind
	LDFLAGS = -L$(ZK_SRCDIR)/libs/linux -L$(SERVER_COMMON_SRCDIR)/../libs/linux -L$(QUTILS_SRCDIR)/libs/linux
	RPATH_FLAGS = -Wl,-rpath,../networkcommon/libs/linux -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/linux -Wl,-rpath,$(QUTILS_SRCDIR)/libs/linux
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
	LIBS = ../networkcommon/libs/$(MACOS_LIB_DIR)/libquiche.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libev.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libuv.a ../servercommon/libs/$(MACOS_LIB_DIR)/libmongoc-static-1.0.a ../qhiredis/libs/$(MACOS_LIB_DIR)/libhiredis.a ../common/libs/$(MACOS_LIB_DIR)/libbson-static-1.0.a ../common/libs/$(MACOS_LIB_DIR)/libz.a -framework CoreFoundation -framework Security -lzookeeper_mt -ldpp -lresolv -lsasl2 -lcurl
	LDFLAGS = -L$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) -L$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) -L$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR)
	RPATH_FLAGS = -Wl,-rpath,../networkcommon/libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR)

	DEBUG_CXXFLAGS += -mmacosx-version-min=12.4
	RELEASE_CXXFLAGS += -mmacosx-version-min=12.4
	LDFLAGS += -mmacosx-version-min=12.4
	LDFLAGS += -mmacosx-version-min=12.4
endif

EXE = qh3server-app
CXX ?= clang
CC ?= clang

DEBUG_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)


# Add this rule back and adjust for the `objects/` directory, if you decide to use it.
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Assuming you introduce CXFLAGS for C compilation
%.o: %.c
	$(CC) $(CXFLAGS) -c $< -o $@


#.cpp.o:
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

# objects/%.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c

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