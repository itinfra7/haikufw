CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
LDFLAGS ?=
LDLIBS ?=

UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Haiku)
LDLIBS += -lnetwork
endif

SOURCES = \
	config_parser.cpp \
	compiler.cpp \
	control.cpp \
	main.cpp

OBJECTS = $(SOURCES:.cpp=.o)

all: haikufwctl

haikufwctl: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

clean:
	rm -f $(OBJECTS) haikufwctl
