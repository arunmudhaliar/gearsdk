NETWORK_COMMON_SRCDIR = ../networkcommon/source
QHIREDIS_SRCDIR = ../qhiredis/source
SERVER_COMMON_SRCDIR = ../servercommon/source
COMMON_SRCDIR = ../common
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
$(NETWORK_COMMON_SRCDIR)/qbuffer.cpp \
$(NETWORK_COMMON_SRCDIR)/qtextfilelogger.cpp \
$(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \

SRCDIR = .
SRCS = $(SRCDIR)/qstats-crawler/main.cpp \
$(SRCDIR)/qstats-crawler/qstats-crawler.cpp \
$(SRCDIR)/qpgsql/qpgsql.cpp \
$(NCOMMONSRCS) \

INCLUDES = -I "$(NETWORK_COMMON_SRCDIR)/ev" -I "$(COMMON_SRCDIR)/zlib" -I "$(SRCDIR)/libpgsql/include" -I "$(SRCDIR)/qpgsql"

# C_SRCS =$(COMMON_SRCDIR)/crypto/sha256.c
C_SRCS =

OBJS = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}

USER_DEFINES = -DLOG_LEVEL=2
# CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -frtti -std=c++17 -fsigned-char -fexceptions -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden
# CXFLAGS = -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden

# LDFLAGS = -shared -rdynamic
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBS = ../networkcommon/libs/linux/libev.a ../common/libs/linux/libz.a -L./libs/linux -lpq -lpgport -lpgcommon -Wl,--as-needed -Wl,-rpath,'/usr/local/pgsql/lib', -lpgcommon -lpgport -lz -lreadline -lpthread -lrt -ldl -lm
endif
ifeq ($(UNAME),Darwin)
	LIBS = ../networkcommon/libs/macos/libev.a ../common/libs/macos/libz.a ./libs/macos/libpgcommon.a ./libs/macos/libpgport.a ./libs/macos/libpq.a -lresolv
endif

EXE = qstats-crawler-app
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