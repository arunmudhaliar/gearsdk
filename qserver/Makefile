NETWORK_COMMON_SRCDIR = ../networkcommon/source
QHIREDIS_SRCDIR = ../qhiredis/source
COMMON_SRCDIR = ../common
QUTILS_SRCDIR = ../qutils
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
$(NETWORK_COMMON_SRCDIR)/qbuffer.cpp \
$(NETWORK_COMMON_SRCDIR)/qtextfilelogger.cpp \
$(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
$(NETWORK_COMMON_SRCDIR)/roommessage.cpp \
$(QUTILS_SRCDIR)/discord_util.cpp \
$(COMMON_SRCDIR)/timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/gxcrc32.cpp \
$(COMMON_SRCDIR)/nvwa/debug_new.cpp \
$(QHIREDIS_SRCDIR)/qhiredis.cpp \
$(QHIREDIS_SRCDIR)/qhiredis_async.cpp \

INCLUDES = -I "$(QUTILS_SRCDIR)" -I "$(COMMON_SRCDIR)" -I "$(COMMON_SRCDIR)/uthash" -I "../networkcommon/source/ev" -I "../networkcommon/source/quiche" -I "../qhiredis/hiredis"
SRCDIR = ./source
SRCS = $(SRCDIR)/main.cpp \
$(SRCDIR)/room.cpp \
$(SRCDIR)/roomserver.cpp \
$(SRCDIR)/gameserver.cpp \
$(SRCDIR)/qnetworkserver.cpp \
$(NCOMMONSRCS) \

OBJS = ${SRCS:.cpp=.o}

USER_DEFINES = -DLOG_LEVEL=3
# CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -frtti -std=c++17 -fsigned-char -fexceptions -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden -W -Wall
CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden

# Debug and Release specific flags are better placed in their respective build rules
DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1
RELEASE_CXXFLAGS = -O2 -DPROD_BUILD=1

# LDFLAGS = -shared -rdynamic
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBS = ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a ../qhiredis/libs/linux/libhiredis.a ../common/libs/linux/libz.a  -lssl -lcrypto -lsasl2 -pthread -lrt -Wl,--no-as-needed -ldl -lsnappy -lzstd -ldpp -lresolv
	LDFLAGS = -L$(SERVER_COMMON_SRCDIR)/../libs/linux -L$(QUTILS_SRCDIR)/libs/linux
	RPATH_FLAGS = -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/linux -Wl,-rpath,$(QUTILS_SRCDIR)/libs/linux
endif
ifeq ($(UNAME),Darwin)
	LIBS = ../networkcommon/libs/macos/libquiche.a ../networkcommon/libs/macos/libev.a ../qhiredis/libs/macos/libhiredis.a ../common/libs/macos/libz.a -ldpp -framework CoreFoundation -framework Security
	LDFLAGS = -L$(SERVER_COMMON_SRCDIR)/../libs/macos -L$(QUTILS_SRCDIR)/libs/macos
	RPATH_FLAGS = -Wl,-rpath,$(SERVER_COMMON_SRCDIR)/../libs/macos -Wl,-rpath,$(QUTILS_SRCDIR)/libs/macos
endif

EXE = qserver-app
CXX ?= clang
CC ?= clang

DEBUG_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)
RELEASE_LDFLAGS = $(LDFLAGS) $(RPATH_FLAGS)

# DEBUG_LDFLAGS = -W -Wall -g -O0 -DDEBUG $(LDFLAGS) $(RPATH_FLAGS)
# RELEASE_LDFLAGS = -W -Wall $(LDFLAGS) $(RPATH_FLAGS)


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