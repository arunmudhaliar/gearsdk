BUILDDIR = build

# INCLUDES = -I "../Common/Source/ev" -I "../NetworkCommon/Source/quiche"
SRCDIR = ./Source
COMMON_SRCDIR = ../Common
SRCS = $(SRCDIR)/QNetworkInterface.cpp \
$(COMMON_SRCDIR)/Timer.cpp \
$(COMMON_SRCDIR)/SDKTypes.cpp \
$(COMMON_SRCDIR)/gxCrc32.cpp \

OBJS = ${SRCS:.cpp=.o}

USER_DEFINES = -DLOG_LEVEL=LOG_LEVEL_1
CXXFLAGS = $(USER_DEFINES) -std=c++11 -fsigned-char -fexceptions -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden

LDFLAGS = -static
# LIBS = ../NetworkCommon/libs/libquiche.a ../NetworkCommon/libs/libev.a
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
