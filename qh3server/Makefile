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
$(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
$(NETWORK_COMMON_SRCDIR)/qtextfile.cpp \
$(QHIREDIS_SRCDIR)/qhiredis.cpp \
$(SERVER_COMMON_SRCDIR)/qmongo/qmongo.cpp \
$(QUTILS_SRCDIR)/discord_util.cpp \
$(COMMON_SRCDIR)/timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/gxcrc32.cpp \
$(COMMON_SRCDIR)/crypto_helper.cpp \
$(QH3_CLIENT_SRCDIR)/qh3client_helper.cpp \
$(QH3_CLIENT_SRCDIR)/qh3client.cpp \
$(ZK_SRCDIR)/source/qzookeeper.cpp \

INCLUDES = -I "$(ZK_SRCDIR)/zookeeper" -I "$(QUTILS_SRCDIR)" -I "$(QH3_CLIENT_SRCDIR)" -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" -I "$(NETWORK_COMMON_SRCDIR)/ev" -I "$(NETWORK_COMMON_SRCDIR)/quiche" -I "$(COMMON_SRCDIR)/libbson-1.0" -I "$(COMMON_SRCDIR)/zlib" -I "../qhiredis/hiredis" -I "$(SERVER_COMMON_SRCDIR)/libmongoc-1.0" -I "$(SERVER_COMMON_SRCDIR)"
SRCDIR = ./qh3server
SRCS = $(SRCDIR)/main.cpp \
$(SRCDIR)/qh3server.cpp \
$(SRCDIR)/qh3simple_router.cpp \
$(SRCDIR)/http3_command_server.cpp \
$(SRCDIR)/http3_sample_server.cpp \
$(SRCDIR)/qh3simple_router_structs.cpp \
$(NCOMMONSRCS) \

C_SRCS =$(COMMON_SRCDIR)/crypto/sha256.c \

OBJS = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}

USER_DEFINES = -DLOG_LEVEL=2 -DFORK_QH3_SERVER=1 -DTHREADED
# CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -frtti -std=c++17 -fsigned-char -fexceptions -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden
# CXFLAGS = -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden

# LDFLAGS = -shared -rdynamic
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBS = $(ZK_SRCDIR)/libs/linux/libzookeeper_mt.a ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a ../servercommon/libs/linux/libmongoc-static-1.0.a ../qhiredis/libs/linux/libhiredis.a ../common/libs/linux/libbson-static-1.0.a ../common/libs/linux/libz.a -lssl -lcrypto -lsasl2 -pthread -lrt -lzookeeper_mt -Wl,--no-as-needed -ldl -lsnappy -lzstd -ldpp -lresolv
	LDFLAGS = -L$(ZK_SRCDIR)/libs/linux -L$(SERVER_COMMON_SRCDIR)/../libs/linux -L$(QUTILS_SRCDIR)/libs/linux
	RPATH_FLAGS = -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/linux -Wl,-rpath,$(QUTILS_SRCDIR)/libs/linux
endif
ifeq ($(UNAME),Darwin)
	LIBS = ../networkcommon/libs/macos/libquiche.a ../networkcommon/libs/macos/libev.a ../servercommon/libs/macos/libmongoc-static-1.0.a ../qhiredis/libs/macos/libhiredis.a ../common/libs/macos/libbson-static-1.0.a ../common/libs/macos/libz.a -framework CoreFoundation -framework Security -lzookeeper_mt -ldpp -lresolv -lsasl2
	LDFLAGS = -L$(ZK_SRCDIR)/libs/macos -L$(SERVER_COMMON_SRCDIR)/../libs/macos -L$(QUTILS_SRCDIR)/libs/macos
	RPATH_FLAGS = -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/macos -Wl,-rpath,$(QUTILS_SRCDIR)/libs/macos
endif

EXE = qh3server-app
CXX ?= clang

DEBUG_LDFLAGS = -W -Wall -g -O0 -DDEBUG $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS = -W -Wall $(LDFLAGS) $(RPATH_FLAGS)

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