# ==============================================================================
# Project Configuration
# ==============================================================================

PROJECT  := corgi
VERSION  := 0.0.1

SRCDIR   := src
TESTDIR  := tests
LIBDIR   := lib
INCDIR   := include

BUILDDIR   := build
BUILD_OBJ  := $(BUILDDIR)/obj
BUILD_BIN  := $(BUILDDIR)/bin
BUILD_MAKE := $(BUILDDIR)/make

DISTDIR := $(PROJECT)-$(VERSION)
DISTTAR := $(DISTDIR).tar.gz

DOCDIR   := doc

SRCEXT   := c
HEADEXT  := h
OBJEXT   := o
DEPEXT   := d

SOURCES := $(wildcard $(SRCDIR)/*.$(SRCEXT)) \
					 $(wildcard $(SRCDIR)/Common/*.$(SRCEXT)) \
					 $(wildcard $(SRCDIR)/Files/*.$(SRCEXT))

HEADERS := $(wildcard $(SRCDIR)/*.$(HEADEXT)) \
					 $(wildcard $(SRCDIR)/*/*.$(HEADEXT))

OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILD_OBJ)/%, $(SOURCES:.$(SRCEXT)=.$(OBJEXT)))
DEPS    := $(patsubst $(SRCDIR)/%,$(BUILD_MAKE)/%,$(SOURCES:.$(SRCEXT)=.$(DEPEXT)))

# ==============================================================================
# Terminal Colors
# ==============================================================================

ifneq (,$(filter xterm-%color,$(TERM)))
	color = $(value $1)$2"\033[0m"
else
	color = $2
endif

GREEN := "\033[32m"
BROWN := "\033[33m"
RED   := "\033[91m"
BLUE  := "\033[34m"

define require-tool
$(if $(shell which $(1) 2>/dev/null),,\
	$(error $(2) requires $(1) but it was not found. \
					Please install $(1) and try again))
endef

# ==============================================================================
# Compiler configuration
# ==============================================================================

CC ?= $(shell which clang 2>/dev/null || \
							 which gcc 2>/dev/null || \
							 which cc 2>/dev/null || \
							 echo "cc")
CC := $(shell which $(CC))

# Detect compiler type by checking version string
CC_VERSION := $(shell $(CC) --version 2>/dev/null)

IS_CLANG := $(if $(findstring clang,$(CC_VERSION)),1,0)
IS_GCC   := $(if $(findstring gcc,$(CC_VERSION))\
								 $(findstring GCC,$(CC_VERSION))\
								 $(findstring Free Software Foundation,$(CC_VERSION)),1,0)

export CC
$(info $(shell echo $(call color,BROWN,Selected compiler) $(CC)))

UNAME_S := $(shell uname -s)

# Common warnings
CWARN_COMMON := -Wall -Wextra -Wcast-align -Wcast-qual -Wchar-subscripts \
	-Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security \
	-Wformat=2 -Winline -Wpacked -Wpointer-arith -Wredundant-decls \
	-Wstrict-overflow=2 -Wswitch-default -Wswitch-enum -Wundef \
	-Wunreachable-code -Wunused -Wvariadic-macros \
	-Wno-missing-field-initializers -Wno-narrowing -Wpedantic

# Clang-specific warnings
CWARN_CLANG := -Wno-unused-command-line-argument

# GCC-specific warnings
CWARN_GCC :=

ifeq ($(IS_CLANG),1)
	CWARN := $(CWARN_COMMON) $(CWARN_CLANG)
else ifeq ($(IS_GCC),1)
	CWARN := $(CWARN_COMMON) $(CWARN_GCC)
else
$(warning "Unknown compiler, build potentially not supported")
	CWARN := $(CWARN_COMMON)
endif

CMACHINE :=

CFLAGS   := -std=c99 -fPIE $(CMACHINE) $(CWARN)
INCFLAGS := -I$(SRCDIR) -I$(INCDIR)
LDFLAGS  :=

MUSL ?= 0
ifeq ($(MUSL),1)
	ifneq ($(strip $(SANITIZERS)),"")
$(warning "Sanitizers are not supported in MUSL builds")
	endif

	override CC      := $(shell which musl-gcc)
$(info $(shell echo $(call color,BROWN,Overriding compiler) $(CC)))
	SANITIZERS       := ""
	LDFLAGS          := -static $(LDFLAGS)
endif

ifndef SANITIZERS
	SANITIZERS := address,alignment,bool,bounds,enum,float-cast-overflow,${strip \
		}float-divide-by-zero,integer-divide-by-zero,nonnull-attribute,${strip \
		}return,returns-nonnull-attribute,shift,signed-integer-overflow,${strip \
		}undefined,unreachable,vla-bound,vptr,null

# Add leak sanitizer only on non-macOS platforms
# (on macOS, leak detection is included in AddressSanitizer)
	ifneq ($(UNAME_S),Darwin)
		SANITIZERS := $(SANITIZERS),leak
	endif
endif

CDEBUG := -D_DEBUG -ggdb -fstack-protector -fstrict-overflow \
	-fno-omit-frame-pointer

ifneq ($(strip $(SANITIZERS)),"")
	CDEBUG := CDEBUG -fsanitize=$(SANITIZERS)
endif

TARGET ?= Debug
ifeq ($(TARGET), Release)
	CFLAGS := -O3 $(CFLAGS)
else
	CFLAGS := -O0 $(CDEBUG) $(CFLAGS)
endif

GDB        ?= gdb
DOXYGEN    ?= doxygen
CLANG_TIDY ?= clang-tidy

# ==============================================================================
# Install Configuration
# ==============================================================================

prefix  ?= /usr/local
bindir  ?= $(DESTDIR)$(prefix)/bin
INSTALL ?= install

# ==============================================================================
# Meta targets
# ==============================================================================

all: $(BUILD_BIN)/$(PROJECT)

doc: $(DOCDIR)/html/index.html

$(DOCDIR)/html/index.html: Doxyfile CHANGELOG.md $(SOURCES) $(HEADERS)
	@$(call require-tool,$(DOXYGEN),Documentation generation)
	@$(DOXYGEN)

view-doc: $(DOCDIR)/html/index.html
	@xdg-open $<

init:
	@mkdir -p $(SRCDIR)
	@mkdir -p $(INCDIR)
	@mkdir -p $(LIBDIR)

compiler-info:
	@echo "Detected compiler: $(CC)"
	@echo "Compiler type: $(if $(filter 1,$(IS_CLANG)),Clang,\
														$(if $(filter 1,$(IS_GCC)),GCC,Unknown))"
	@echo "Version info:"
	@$(CC) --version

# ==============================================================================
# Build Rules
# ==============================================================================

# Build source objects
$(BUILD_OBJ)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@) $(dir $(BUILD_MAKE)/$*.$(DEPEXT))
	@echo CC $@
	@$(CC) $(CFLAGS) $(INCFLAGS) \
		-MMD -MP -MF $(BUILD_MAKE)/$*.$(DEPEXT) -c $< -o $@ \
		|| (echo $(call color,RED,\>! Failed to build $@ from $< !\<); exit 1)

# Build project binary
$(BUILD_BIN)/$(PROJECT): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo LD $@
	@$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $(BUILD_BIN)/$(PROJECT) \
		|| (echo $(call color,RED,=== Failed to build project $(PROJECT) ===); \
				exit 1)
	@echo $(call color,GREEN,=== Build done! ===)

# ==============================================================================
# Utility Targets
# ==============================================================================

# Remove objects
clean:
	@echo $(call color,BLUE,\> Removing object files)
	@rm -rf $(BUILD_OBJ)

# Remove objects, dependency makefiles, and binaries
cleaner: clean
	@echo $(call color,BLUE,\> Removing all build files)
	@rm -rf $(BUILD_BIN)
	@rm -rf $(BUILD_MAKE)

# Run project
run: $(BUILD_BIN)/$(PROJECT)
	@$< $(ARGS)

debug: $(BUILD_BIN)/$(PROJECT)
	@$(call require-tool,$(GDB),Debug target)
	@$(GDB) --args $< $(ARGS)

check: test

tidy:
	@$(call require-tool,$(CLANG_TIDY),Code checking)
	@$(CLANG_TIDY) -p $(BUILDDIR) $(SOURCES) -header-filter=.* -- \
	 $(CFLAGS) $(INCFLAGS)

install: $(BUILD_BIN)/$(PROJECT)
	@mkdir -p $(bindir)
	@$(INSTALL) -m 755 $< $(bindir)/$(PROJECT)
	@echo "Installed $(PROJECT) to $(bindir)/$(PROJECT)"

uninstall:
	@rm -f $(bindir)/$(PROJECT)
	@echo "Uninstalled $(PROJECT) from $(bindir)/$(PROJECT)"

# ==============================================================================
# Distribution Targets
# ==============================================================================

dist: $(DISTTAR)

$(DISTTAR):
	@git archive --format=tar.gz --prefix=$(DISTDIR)/ HEAD > $(DISTTAR)
	@echo "Created $(DISTTAR)"

distclean: cleaner
	@rm -rf $(DISTTAR) $(DISTDIR) .tmp/distcheck

distcheck: $(DISTTAR)
	@rm -rf .tmp/distcheck
	@mkdir -p .tmp/distcheck
	@tar -xzf $(DISTTAR) -C .tmp/distcheck
	@cd .tmp/distcheck/$(DISTDIR) && \
			$(MAKE) all TARGET=Release CC=$(CC) && \
			$(MAKE) test TARGET=Release CC=$(CC) && \
			$(MAKE) install DESTDIR=$$(pwd)/install-check prefix=/usr && \
			test -f $$(pwd)/install-check/usr/bin/$(PROJECT)
	@rm -rf .tmp/distcheck
	@echo $(call color,GREEN,=== distcheck passed! ===)

-include $(DEPS)

# ==============================================================================
# Testing Targets
# ==============================================================================

TEST_INTEGRATION_DIR := $(TESTDIR)/integration
TEST_DIR ?= .tmp/tests

test: test-integration

test-setup:
	@mkdir -p $(TEST_DIR)

test-clean:
	@echo $(call color,BLUE,\> Cleaning test artifacts)
	@rm -rf $(TEST_DIR)

test-integration: $(BUILD_BIN)/$(PROJECT) test-setup
	@echo $(call color,BROWN,Running integration tests...)
	@TEST_DIR=$(TEST_DIR) \
	 CORGI_BINARY=$(BUILD_BIN)/$(PROJECT) \
	 MAKE=$(MAKE) \
	 /bin/sh $(TEST_INTEGRATION_DIR)/runner.sh $(TESTS)

.PHONY: all remake clean cleaner run init debug doc view-doc check tidy \
				compiler-info install uninstall dist distclean distcheck \
				test test-integration test-clean test-setup
