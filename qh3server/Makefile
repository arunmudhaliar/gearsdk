NETWORK_COMMON_SRCDIR = ../networkcommon/source
COMMON_SRCDIR = ../common
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
$(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
$(COMMON_SRCDIR)/timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/gxcrc32.cpp \

INCLUDES = -I "../networkcommon/source/ev" -I "../networkcommon/source/quiche"
SRCDIR = ./qh3server
SRCS = $(SRCDIR)/main.cpp \
$(SRCDIR)/qh3server.cpp \
$(SRCDIR)/http3_sample_server.cpp \
$(NCOMMONSRCS) \

OBJS = ${SRCS:.cpp=.o}

USER_DEFINES = -DLOG_LEVEL=2
# CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -frtti -std=c++17 -fsigned-char -fexceptions -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden
# CXFLAGS = -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden

# LDFLAGS = -shared -rdynamic
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBS = ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a -pthread -Wl,--no-as-needed -ldl
endif
ifeq ($(UNAME),Darwin)
	LIBS = ../networkcommon/libs/macos/libquiche.a ../networkcommon/libs/macos/libev.a -framework CoreFoundation -framework Security
endif

EXE = qh3server-app
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