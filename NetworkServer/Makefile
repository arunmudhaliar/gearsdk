BUILDDIR = build

NETWORK_COMMON_SRCDIR = ../networkcommon/Source
COMMON_SRCDIR = ../common
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
$(COMMON_SRCDIR)/Timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/gxCrc32.cpp \

INCLUDES = -I "../networkcommon/Source/ev" -I "../networkcommon/Source/quiche"
SRCDIR = ./NetworkServer
SRCS = $(SRCDIR)/main.cpp \
$(SRCDIR)/gameserver.cpp \
$(SRCDIR)/qnetworkserver.cpp \
$(NCOMMONSRCS) \

OBJS = ${SRCS:.cpp=.o}

USER_DEFINES = -DLOG_LEVEL=1
# CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -frtti -std=c++17 -fsigned-char -fexceptions -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden
# CXFLAGS = -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden

LDFLAGS = -g
# LDFLAGS = -shared -rdynamic
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
LIBS = ../networkcommon/libs/linux/libquiche.a ../networkcommon/libs/linux/libev.a
else
    ifeq ($(UNAME),Darwin)
        LIBS = ../networkcommon/libs/libquiche.a ../networkcommon/libs/libev.a -framework CoreFoundation -framework Security
    endif
endif

PLUGIN_SHARED = QServer
CXX ?= clang

#.cpp.o:
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

objects/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $(BUILDDIR)
    	
all: shared

clean:
	rm -f $(OBJS) $(PLUGIN_SHARED)

shared: $(OBJS)
	$(CXX) $(LDFLAGS) -o $(PLUGIN_SHARED) $(OBJS) $(LIBS)
