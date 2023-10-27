BUILDDIR = build

# INCLUDES = -I "../common/Source/ev" -I "../networkcommon/Source/quiche"
SRCDIR = ./Source
COMMON_SRCDIR = ../common
SRCS = $(SRCDIR)/QNetworkInterface.cpp \
$(COMMON_SRCDIR)/Timer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/gxCrc32.cpp \

OBJS = ${SRCS:.cpp=.o}

USER_DEFINES = -DLOG_LEVEL=LOG_LEVEL_1
CXXFLAGS = $(USER_DEFINES) -std=c++11 -fsigned-char -fexceptions -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden

LDFLAGS = -static
# LIBS = ../networkcommon/libs/libquiche.a ../networkcommon/libs/libev.a
PLUGIN_SHARED = libCommon.a
CXX ?= clang

#.cpp.o:
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

objects/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $(BUILDDIR)

all: static

clean:
	rm -f $(OBJS) $(PLUGIN_SHARED)

static: $(OBJS)
	$(CXX) $(LDFLAGS) -o $(PLUGIN_SHARED) $(OBJS)
