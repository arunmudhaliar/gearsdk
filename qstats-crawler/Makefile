NETWORK_COMMON_SRCDIR = ../networkcommon/source
QHIREDIS_SRCDIR = ../qhiredis/source
SERVER_COMMON_SRCDIR = ../servercommon/source
COMMON_SRCDIR = ../common
NCOMMONSRCS = $(NETWORK_COMMON_SRCDIR)/essentials.cpp \
$(NETWORK_COMMON_SRCDIR)/qbuffer.cpp \
$(NETWORK_COMMON_SRCDIR)/qtextfilelogger.cpp \
$(NETWORK_COMMON_SRCDIR)/qtimer.cpp \
$(COMMON_SRCDIR)/sdktypes.cpp \
$(COMMON_SRCDIR)/nvwa/debug_new.cpp \

SRCDIR = .
SRCS = $(SRCDIR)/qstats-crawler/main.cpp \
$(SRCDIR)/qstats-crawler/qstats-crawler.cpp \
$(SRCDIR)/qpgsql/qpgsql.cpp \
$(NCOMMONSRCS) \

INCLUDES = -I "$(COMMON_SRCDIR)" -I "$(NETWORK_COMMON_SRCDIR)/ev" -I "$(COMMON_SRCDIR)/zlib" -I "$(SRCDIR)/libpgsql/include" -I "$(SRCDIR)/qpgsql"

# C_SRCS =$(COMMON_SRCDIR)/crypto/sha256.c
C_SRCS =

OBJS = ${SRCS:.cpp=.o} ${C_SRCS:.c=.o}

USER_DEFINES = -DLOG_LEVEL=2
CXXFLAGS = $(INCLUDES) $(USER_DEFINES) -std=c++17 -fvisibility=hidden -W -Wall -fPIC
CXFLAGS = $(INCLUDES) $(USER_DEFINES) -Os -s -ffunction-sections -fdata-sections -fvisibility=hidden -fPIC

# Debug and Release specific flags are better placed in their respective build rules
DEBUG_CXXFLAGS = -g -O0 -DDEBUG -DDEV_BUILD=1
RELEASE_CXXFLAGS = -O2 -DPROD_BUILD=1

# LDFLAGS = -shared -rdynamic
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	LIBLDIR = ./libs/linux
	LIBS = ../networkcommon/libs/linux/libev.a ../common/libs/linux/libz.a -L$(LIBLDIR) -Wl,--as-needed -Wl,-rpath,'./libs/linux',--enable-new-dtags  -lpgcommon -lpgport -lpq -lz -lreadline -lpthread -lrt -ldl -lm -lresolv
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
	LIBS = ../networkcommon/libs/$(MACOS_LIB_DIR)/libev.a ../common/libs/$(MACOS_LIB_DIR)/libz.a ./libs/$(MACOS_LIB_DIR)/libpgcommon.a ./libs/$(MACOS_LIB_DIR)/libpgport.a ./libs/$(MACOS_LIB_DIR)/libpq.a -lresolv
endif

EXE = qstats-crawler-app
CXX ?= clang
CC ?= clang

DEBUG_LDFLAGS =
RELEASE_LDFLAGS =


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

# debug: $(OBJS)
# 	$(CXX) $(DEBUG_LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

# release: $(OBJS)
# 	$(CXX) $(RELEASE_LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)