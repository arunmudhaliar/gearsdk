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
    $(COMMON_SRCDIR)/serverinforeader.cpp \
    $(QH3_CLIENT_SRCDIR)/qh3client_helper.cpp \
    $(QH3_CLIENT_SRCDIR)/qh3client.cpp \
    $(ZK_SRCDIR)/source/qzookeeper.cpp \

# Add debug_new.cpp conditionally for the debug build
debug:
    NCOMMONSRCS += $(COMMON_SRCDIR)/nvwa/debug_new.cpp

INCLUDES = -I "$(ZK_SRCDIR)/zookeeper" \
    -I "$(QUTILS_SRCDIR)" \
    -I "$(QH3_CLIENT_SRCDIR)" \
    -I "$(COMMON_SRCDIR)" \
    -I "$(COMMON_SRCDIR)/uthash" \
    -I "$(NETWORK_COMMON_SRCDIR)/ev" \
    -I "$(NETWORK_COMMON_SRCDIR)/uv" \
    -I "$(NETWORK_COMMON_SRCDIR)/quiche" \
    -I "$(COMMON_SRCDIR)/zlib" \
    -I "../qhiredis/hiredis" \
    -I "$(SERVER_COMMON_SRCDIR)"

SRCDIR = .
QSRCDIR = ./../qserver/source

SRCS = $(SRCDIR)/serverplugin.cpp \
    $(SRCDIR)/plugin_gameserver.cpp \
    $(SRCDIR)/plugin_qhiredis.cpp \
    $(SRCDIR)/plugin_qzookeeper.cpp \
    $(SRCDIR)/../qh3server/qh3server/qh3server.cpp \
    $(SRCDIR)/../qh3server/qh3server/qh3router.cpp \
    $(SRCDIR)/../qh3server/qh3server/http3_command_server.cpp \
    $(SRCDIR)/../qh3server/qh3server/qh3router_structs.cpp \
    $(QSRCDIR)/room.cpp \
    $(QSRCDIR)/roomserver.cpp \
    $(QSRCDIR)/gameserver.cpp \
    $(QSRCDIR)/qnetworkserver.cpp \
    $(NCOMMONSRCS) \

# Default value for USE_NAPI
USE_NAPI ?= 0
# Conditional NAPI include directory
ifeq ($(USE_NAPI), 1)
    INCLUDES += -I $(shell node -p "require('../serverplugin_ts_proj/node_modules/node-addon-api').include")
	INCLUDES += -I $(shell . ~/.nvm/nvm.sh && nvm which current | sed 's|bin/node|include/node|')
	SRCS += $(SRCDIR)/serverplugin_napi_addon.cpp \
        $(SRCDIR)/gserverplugin_napi_addon.cpp \
        $(SRCDIR)/napi_helper.cpp

	NAPI_DEFINES := \
	'-DNODE_GYP_MODULE_NAME=serverplugin_addon' \
	'-DUSING_UV_SHARED=1' \
	'-DUSING_V8_SHARED=1' \
	'-DV8_DEPRECATION_WARNINGS=1' \
	'-DV8_DEPRECATION_WARNINGS' \
	'-DV8_IMMINENT_DEPRECATION_WARNINGS' \
	'-D_GLIBCXX_USE_CXX11_ABI=1' \
	'-D_DARWIN_USE_64_BIT_INODE=1' \
	'-D_LARGEFILE_SOURCE' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DOPENSSL_NO_PINSHARED' \
	'-DOPENSSL_THREADS' \
	'-DNAPI_DISABLE_CPP_EXCEPTIONS' \
	'-DBUILDING_NODE_EXTENSION'
endif

C_SRCS = $(COMMON_SRCDIR)/crypto/sha256.c \

OBJS = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}

LOG_LEVEL ?= 2
USER_DEFINES = -DLOG_LEVEL=$(LOG_LEVEL)

FORK_QH3_SERVER ?= 0
USER_DEFINES += -DFORK_QH3_SERVER=$(FORK_QH3_SERVER)
USER_DEFINES += -DTHREADED -DENABLE_ZK=1 -DUSE_LIBUV=1
ifeq ($(USE_NAPI), 1)
USER_DEFINES += $(NAPI_DEFINES)
USER_DEFINES += -Dnode_engine=v8
endif

CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden -W -Wall -fPIC
CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -ffunction-sections -fdata-sections -fvisibility=hidden -fPIC

DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1
RELEASE_CXXFLAGS = -O2 -DPROD_BUILD=1 -DNDEBUG

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
    ARCH := $(shell uname -m)
    LINUX_LIB_DIR = linux
    LIBS = $(ZK_SRCDIR)/libs/$(LINUX_LIB_DIR)/libzookeeper_mt.a \
        ../networkcommon/libs/$(LINUX_LIB_DIR)/libquiche.a \
        ../networkcommon/libs/$(LINUX_LIB_DIR)/libev.a \
        ../networkcommon/libs/$(LINUX_LIB_DIR)/libuv.a \
        ../qhiredis/libs/$(LINUX_LIB_DIR)/libhiredis.a \
        ../common/libs/$(LINUX_LIB_DIR)/libz.a \
        -lssl -lcrypto -lsasl2 -pthread -lrt -lzookeeper_mt \
        -Wl,--no-as-needed -ldl -lsnappy -lzstd -lresolv -lcurl -lunwind
    LDFLAGS = -L$(ZK_SRCDIR)/libs/$(LINUX_LIB_DIR) \
        -L$(SERVER_COMMON_SRCDIR)/../libs/$(LINUX_LIB_DIR) \
        -L$(QUTILS_SRCDIR)/libs/$(LINUX_LIB_DIR)
    LDFLAGS += -shared
    RPATH_FLAGS = -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/$(LINUX_LIB_DIR) \
        -Wl,-rpath,$(QUTILS_SRCDIR)/libs/$(LINUX_LIB_DIR)
    RPATH_FLAGS += -Wl,-rpath,'$$ORIGIN/../networkcommon/libs/$(LINUX_LIB_DIR)'
    LIB_EXT = so
    ifeq ($(USE_NAPI), 1)
	    LIB_EXT = node
	    DEBUG_LIB = libserverplugin-linux-$(ARCH)-debug.$(LIB_EXT)
	    RELEASE_LIB = libserverplugin-linux-$(ARCH).$(LIB_EXT)
    else
	    DEBUG_LIB = libserverplugin-debug.$(LIB_EXT)
	    RELEASE_LIB = libserverplugin.$(LIB_EXT)
    endif
endif

ifeq ($(UNAME), Darwin)
    ARCH := $(shell uname -m)
    MACOS_LIB_DIR = macos_$(ARCH)
    LIBS = $(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR)/libzookeeper_mt.a \
        ../networkcommon/libs/$(MACOS_LIB_DIR)/libquiche.a \
        ../networkcommon/libs/$(MACOS_LIB_DIR)/libev.a \
        ../networkcommon/libs/$(MACOS_LIB_DIR)/libuv.a \
        ../qhiredis/libs/$(MACOS_LIB_DIR)/libhiredis.a \
        ../common/libs/$(MACOS_LIB_DIR)/libz.a \
        -framework CoreFoundation -framework Security \
        -lresolv -lsasl2 -lcurl
    LDFLAGS = -L$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) \
        -L$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) \
        -L$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR) 
    LDFLAGS += -dynamiclib
    RPATH_FLAGS = -Wl,-rpath,../networkcommon/libs/$(MACOS_LIB_DIR) \
        -Wl,-rpath,$(ZK_SRCDIR)/libs/$(MACOS_LIB_DIR) \
        -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/$(MACOS_LIB_DIR) \
        -Wl,-rpath,$(QUTILS_SRCDIR)/libs/$(MACOS_LIB_DIR)
    LIB_EXT = dylib
    ifeq ($(USE_NAPI), 1)
	    LIB_EXT = node
	    MACOS_LIB_DIR = macos_$(ARCH)
	    DEBUG_LIB = libserverplugin-darwin-$(ARCH)-debug.$(LIB_EXT)
	    RELEASE_LIB = libserverplugin-darwin-$(ARCH).$(LIB_EXT)
    else
	    DEBUG_LIB = libserverplugin-debug.$(LIB_EXT)
	    RELEASE_LIB = libserverplugin.$(LIB_EXT)
    endif

    DEBUG_LDFLAGS = -install_name @rpath/$(DEBUG_LIB)
    RELEASE_LDFLAGS = -install_name @rpath/$(RELEASE_LIB)
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

ifeq ($(USE_NAPI), 1)
    ifeq ($(shell uname -s), Darwin)
        DEBUG_LDFLAGS += \
            -undefined dynamic_lookup \
            -Wl,-search_paths_first
        RELEASE_LDFLAGS += \
            -undefined dynamic_lookup \
            -Wl,-search_paths_first
    else
        DEBUG_LDFLAGS += \
            -Wl,--allow-shlib-undefined
        RELEASE_LDFLAGS += \
            -Wl,--allow-shlib-undefined
    endif
endif

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CXFLAGS) -c $< -o $@

all: debug

clean:
	rm -f $(OBJS) $(DEBUG_LIB) $(RELEASE_LIB) *.node *.o

$(OBJS): Makefile

debug: CXXFLAGS += $(DEBUG_CXXFLAGS)
debug: $(OBJS)
	$(CXX) $(DEBUG_LDFLAGS) -o $(DEBUG_LIB) $(OBJS) $(LIBS)

release: CXXFLAGS += $(RELEASE_CXXFLAGS)
release: $(OBJS)
	$(CXX) $(RELEASE_LDFLAGS) -o $(RELEASE_LIB) $(OBJS) $(LIBS)
    ifeq ($(UNAME), Linux)
		strip --strip-unneeded --keep-section=.text $(RELEASE_LIB)
    endif
