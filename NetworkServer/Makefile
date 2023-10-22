BUILDDIR = build

NETWORK_COMMON_SRCDIR = ../NetworkCommon/Source
COMMON_SRCDIR = ../Common
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/QNetworkInterface.cpp \
$(COMMON_SRCDIR)/Timer.cpp \
$(COMMON_SRCDIR)/SDKTypes.cpp \
$(COMMON_SRCDIR)/gxCrc32.cpp \

INCLUDES = -I "../NetworkCommon/Source/ev" -I "../NetworkCommon/Source/quiche"
SRCDIR = ./NetworkServer
SRCS = $(SRCDIR)/NetworkServer.cpp \
$(SRCDIR)/GameServer.cpp \
$(SRCDIR)/QNetworkServer.cpp \
$(NCOMMONSRCS) \

OBJS = ${SRCS:.cpp=.o}

USER_DEFINES = -DLOG_LEVEL=LOG_LEVEL_2
# CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -frtti -std=c++17 -fsigned-char -fexceptions -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden
# CXFLAGS = -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden

LDFLAGS = -g
# LDFLAGS = -shared -rdynamic
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
LIBS = ../NetworkCommon/libs/linux/libquiche.a ../NetworkCommon/libs/linux/libev.a
else
    ifeq ($(UNAME),Darwin)
        LIBS = ../NetworkCommon/libs/libquiche.a ../NetworkCommon/libs/libev.a -framework CoreFoundation -framework Security
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
