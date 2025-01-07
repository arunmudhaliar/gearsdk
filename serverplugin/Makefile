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
$(SERVER_COMMON_SRCDIR)/servercommon.cpp \
$(COMMON_SRCDIR)/timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/crypto_helper.cpp \
$(COMMON_SRCDIR)/signal_handler/signal_handler.cpp \
$(QH3_CLIENT_SRCDIR)/qh3client_helper.cpp \
$(QH3_CLIENT_SRCDIR)/qh3client.cpp \
$(ZK_SRCDIR)/source/qzookeeper.cpp \

# Add debug_new.cpp conditionally for the debug build
debug: NCOMMONSRCS += $(COMMON_SRCDIR)/nvwa/debug_new.cpp

INCLUDES = -I "$(ZK_SRCDIR)/zookeeper" -I "$(QUTILS_SRCDIR)" -I "$(QH3_CLIENT_SRCDIR)" -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" -I "$(NETWORK_COMMON_SRCDIR)/ev" -I "$(NETWORK_COMMON_SRCDIR)/uv" -I "$(NETWORK_COMMON_SRCDIR)/quiche" -I "$(COMMON_SRCDIR)/zlib" -I "../qhiredis/hiredis" -I "$(SERVER_COMMON_SRCDIR)"
SRCDIR = .
QSRCDIR = ./../qserver/source
SRCS = $(SRCDIR)/serverplugin.cpp \
$(SRCDIR)/plugin_gameserver.cpp \
$(SRCDIR)/../qh3server/qh3server/qh3server.cpp \
$(SRCDIR)/../qh3server/qh3server/qh3router.cpp \
$(SRCDIR)/../qh3server/qh3server/http3_command_server.cpp \
$(SRCDIR)/../qh3server/qh3server/qh3router_structs.cpp \
$(QSRCDIR)/room.cpp \
$(QSRCDIR)/roomserver.cpp \
$(QSRCDIR)/gameserver.cpp \
$(QSRCDIR)/qnetworkserver.cpp \
$(NCOMMONSRCS) \

C_SRCS =$(COMMON_SRCDIR)/crypto/sha256.c \

OBJS = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}

LOG_LEVEL ?= 2
USER_DEFINES = -DLOG_LEVEL=$(LOG_LEVEL)

FORK_QH3_SERVER ?= 0
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
	LINUX_LIB_DIR = linux
	LIBS = $(ZK_SRCDIR)/libs/$(LINUX_LIB_DIR)/libzookeeper_mt.a ../networkcommon/libs/$(LINUX_LIB_DIR)/libquiche.a ../networkcommon/libs/$(LINUX_LIB_DIR)/libev.a ../networkcommon/libs/$(LINUX_LIB_DIR)/libuv.a ../qhiredis/libs/$(LINUX_LIB_DIR)/libhiredis.a ../common/libs/$(LINUX_LIB_DIR)/libz.a -lssl -lcrypto -lsasl2 -pthread -lrt -lzookeeper_mt -Wl,--no-as-needed -ldl -lsnappy -lzstd -lresolv -lcurl -lunwind
	LDFLAGS = -L$(ZK_SRCDIR)/libs/$(LINUX_LIB_DIR) -L$(SERVER_COMMON_SRCDIR)/../libs/$(LINUX_LIB_DIR) -L$(QUTILS_SRCDIR)/libs/$(LINUX_LIB_DIR)
	LDFLAGS += -shared
	RPATH_FLAGS = -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/$(LINUX_LIB_DIR) -Wl,-rpath,$(QUTILS_SRCDIR)/libs/$(LINUX_LIB_DIR)
	RPATH_FLAGS += -Wl,-rpath,'$$ORIGIN/../networkcommon/libs/$(LINUX_LIB_DIR)'
	LIB_EXT = so
	DEBUG_LIB = libserverplugin-debug.$(LIB_EXT)
	RELEASE_LIB = libserverplugin.$(LIB_EXT)
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
	LIBS = ../networkcommon/libs/$(MACOS_LIB_DIR)/libquiche.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libev.a ../networkcommon/libs/$(MACOS_LIB_DIR)/libuv.a ../qhiredis/libs/$(MACOS_LIB_DIR)/libhiredis.a ../common/libs/$(MACOS_LIB_DIR)/libz.a -framework CoreFoundation -framework Security -lzookeeper_mt -lresolv -lsasl2 -lcurl
	LDFLAGS = -L$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) -L$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) -L$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR)
	LDFLAGS += -dynamiclib
	RPATH_FLAGS = -Wl,-rpath,../networkcommon/libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) -Wl,-rpath,$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR)
	LIB_EXT = dylib
	DEBUG_LIB = libserverplugin-debug.$(LIB_EXT)
	RELEASE_LIB = libserverplugin.$(LIB_EXT)
	DEBUG_LDFLAGS = -install_name @rpath/$(DEBUG_LIB)
	RELEASE_LDFLAGS = -install_name @rpath/$(RELEASE_LIB)
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

CXX ?= clang
CC ?= clang

DEBUG_LDFLAGS += $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS += $(LDFLAGS) $(RPATH_FLAGS)


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
	rm -f $(OBJS) $(DEBUG_LIB) $(RELEASE_LIB)

# You can explicitly tell make that all your targets depend on the Makefile. This way, any change to the Makefile will trigger a rebuild.
$(OBJS): Makefile

debug: CXXFLAGS += $(DEBUG_CXXFLAGS)
debug: $(OBJS)
	$(CXX) $(DEBUG_LDFLAGS) -o $(DEBUG_LIB) $(OBJS) $(LIBS)

release: CXXFLAGS += $(RELEASE_CXXFLAGS)
release: $(OBJS)
	$(CXX) $(RELEASE_LDFLAGS) -o $(RELEASE_LIB) $(OBJS) $(LIBS)