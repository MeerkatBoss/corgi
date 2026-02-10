# ============================================================================
# Project Configuration
# ============================================================================

PROJECT  := corgi
VERSION  := 0.0.1

SRCDIR   := src
TESTDIR  := tests
LIBDIR   := lib
INCDIR   := include

BUILDDIR := build
OBJDIR   := $(BUILDDIR)/obj
BINDIR   := $(BUILDDIR)/bin
MAKEDIR  := $(BUILDDIR)/make

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

LIBS    := $(patsubst lib%.a, %, $(shell find $(LIBDIR) -type f 2>/dev/null))
OBJECTS := $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))
DEPS    := $(patsubst $(SRCDIR)/%,$(MAKEDIR)/%,$(SOURCES:.$(SRCEXT)=.$(DEPEXT)))

# ============================================================================
# Compiler configuration
# ============================================================================

CC ?= $(shell which clang 2>/dev/null || which gcc 2>/dev/null || which cc 2>/dev/null || echo "cc")
CC := $(shell which $(CC))

# Detect compiler type by checking version string
CC_VERSION := $(shell $(CC) --version 2>/dev/null)
IS_CLANG := $(if $(findstring clang,$(CC_VERSION)),1,0)
IS_GCC := $(if $(findstring gcc,$(CC_VERSION))$(findstring GCC,$(CC_VERSION))$(findstring Free Software Foundation,$(CC_VERSION)),1,0)

export CC
$(info Selected compiler $(CC))

# Common warnings
CWARN_COMMON := -Wall -Wextra -Wcast-align -Wcast-qual -Wchar-subscripts \
	-Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security \
	-Wformat=2 -Winline -Wpacked -Wpointer-arith -Wredundant-decls \
	-Wstrict-overflow=2 -Wswitch-default -Wswitch-enum -Wundef \
	-Wunreachable-code -Wunused -Wvariadic-macros \
	-Wno-missing-field-initializers -Wno-narrowing

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

CDEBUG := -D_DEBUG -ggdb -fstack-protector -fstrict-overflow \
	-fno-omit-frame-pointer \
	-fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,${strip \
	}float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,${strip \
	}null,return,returns-nonnull-attribute,shift,${strip \
	}signed-integer-overflow,undefined,unreachable,vla-bound,vptr

CMACHINE :=

CFLAGS   := -std=gnu99 -fPIE $(CMACHINE) $(CWARN)
INCFLAGS := -I$(SRCDIR) -I$(INCDIR)
LDFLAGS  :=

TARGET ?= Debug
ifeq ($(TARGET), Release)
	CFLAGS := -O3 $(CFLAGS)
else
	CFLAGS := -O0 $(CDEBUG) $(CFLAGS)
endif

# ============================================================================
# Terminal Colors
# ============================================================================

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
  $(error $(2) requires $(1) but it was not found. Please install $(1) and try again))
endef

GDB        ?= gdb
DOXYGEN    ?= doxygen
CLANG_TIDY ?= clang-tidy

# ============================================================================
# Meta targets
# ============================================================================

all: $(BINDIR)/$(PROJECT)

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
	@echo "Compiler type: $(if $(filter 1,$(IS_CLANG)),Clang,$(if $(filter 1,$(IS_GCC)),GCC,Unknown))"
	@echo "Version info:"
	@$(CC) --version

# ============================================================================
# Build Rules
# ============================================================================

# Build source objects
$(OBJDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@) $(dir $(MAKEDIR)/$*.$(DEPEXT))
	@echo CC $@
	@$(CC) $(CFLAGS) $(INCFLAGS) -MMD -MP -MF $(MAKEDIR)/$*.$(DEPEXT) -c $< -o $@ \
		|| (echo $(call color,RED,\>! Failed to build $@ from $< !\<); exit 1)

# Build project binary
$(BINDIR)/$(PROJECT): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo LD $@
	@$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $(BINDIR)/$(PROJECT) \
		|| (echo $(call color,RED,=== Failed to build project $(PROJECT) ===); \
		    exit 1)
	@echo $(call color,GREEN,=== Build done! ===)

# ============================================================================
# Utility Targets
# ============================================================================

# Remove objects
clean:
	@echo $(call color,BLUE,\> Removing object files)
	@rm -rf $(OBJDIR)

# Remove objects, dependency makefiles, and binaries
cleaner: clean
	@echo $(call color,BLUE,\> Removing all build files)
	@rm -rf $(BINDIR)
	@rm -rf $(MAKEDIR)

# Run project
run: $(BINDIR)/$(PROJECT)
	@$< $(ARGS)

debug: $(BINDIR)/$(PROJECT)
	@$(call require-tool,$(GDB),Debug target)
	@$(GDB) --args $< $(ARGS)

check:
	@$(call require-tool,$(CLANG_TIDY),Code checking)
	@$(CLANG_TIDY) -p $(BUILDDIR) $(SOURCES) -- $(CFLAGS) $(INCFLAGS)

-include $(DEPS)

.PHONY: all remake clean cleaner run init debug doc view-doc check compiler-info
