CC:=clang-18

CWARN:=-Wall -Wextra -Wcast-align -Wcast-qual -Wchar-subscripts\
-Wempty-body -Wfloat-equal -Wformat-nonliteral\
-Wformat-security -Wformat=2 -Winline\
-Wpacked -Wpointer-arith -Wredundant-decls -Wsign-promo -Wstrict-overflow=2\
-Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused\
-Wvariadic-macros -Wno-missing-field-initializers -Wno-narrowing\
 -Wno-unused-command-line-argument

CDEBUG:=-D _DEBUG -ggdb -fcheck-new -fsized-deallocation -fstack-protector\
-fstrict-overflow -fno-omit-frame-pointer\
-fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,${strip \
}float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,${strip \
}null,return,returns-nonnull-attribute,shift,${strip \
}signed-integer-overflow,undefined,unreachable,vla-bound,vptr

CMACHINE:=# -mavx512f -march=native -mtune=native

CFLAGS:=-std=c99 -fPIE $(CMACHINE) $(CWARN)
BUILDTYPE?=Debug

ifeq ($(BUILDTYPE), Release)
	CFLAGS:=-O3 $(CFLAGS)
else
	CFLAGS:=-O0 $(CDEBUG) $(CFLAGS)
endif

PROJECT	:= corgi
VERSION := 0.0.1

SRCDIR	:= src
TESTDIR := tests
LIBDIR	:= lib
INCDIR	:= include

BUILDDIR:= build
OBJDIR 	:= $(BUILDDIR)/obj
BINDIR	:= $(BUILDDIR)/bin
MAKEDIR := $(BUILDDIR)/make

DOCDIR := doc

SRCEXT	:= c
HEADEXT	:= h
OBJEXT	:= o
DEPEXT  := d

SOURCES := $(shell find $(SRCDIR) -type f -name "*.$(SRCEXT)")
HEADERS := $(shell find $(SRCDIR) -type f -name "*.$(HEADEXT)")
LIBS	  := $(patsubst lib%.a, %, $(shell find $(LIBDIR) -type f))
OBJECTS	:= $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))
DEPS    := $(patsubst $(SRCDIR)/%,$(MAKEDIR)/%,$(SOURCES:.$(SRCEXT)=.$(DEPEXT)))

ifneq (,$(filter xterm-%color,$(TERM)))
	color = $(value $1)$2"\033[0m"
else
	color = $2
endif

GREEN:="\033[32m"
BROWN:="\033[33m"
RED:="\033[91m"
BLUE:="\033[34m"

INCFLAGS:= -I$(SRCDIR) -I$(INCDIR)
LFLAGS  := -Llib/ $(addprefix -l, $(LIBS))

all: $(BINDIR)/$(PROJECT)

doc: $(DOCDIR)/html/index.html

$(DOCDIR)/html/index.html: Doxyfile CHANGELOG.md $(SOURCES) $(HEADERS)
	@doxygen

view-doc: $(DOCDIR)/html/index.html
	@xdg-open $<

init:
	@mkdir -p $(SRCDIR)
	@mkdir -p $(INCDIR)
	@mkdir -p $(LIBDIR)

# Build source objects
$(OBJDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	@echo $(call color,BROWN,\> Building object file) $@\
		$(call color,BROWN,from) $<
	@$(CC) $(CFLAGS) $(INCFLAGS) -c $< -o $@\
		|| (echo $(call color,RED,\>! Failed to build $@ from $< !\<); exit 1)

# Build project binary
$(BINDIR)/$(PROJECT): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo $(call color,BROWN,=== Building project $(PROJECT) ===)
	@$(CC) $(CFLAGS) $^ $(LFLAGS) -o $(BINDIR)/$(PROJECT)\
		|| (echo $(call color,RED,=== Failed to build project $(PROJECT) ===);\
		    exit 1)
	@echo $(call color,GREEN,=== Build done! ===)

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
	@gdb --args $< $(ARGS)


include $(DEPS)

# Build dependency makefiles
$(MAKEDIR)/%.$(DEPEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	@echo $(call color,BROWN,\> Updating dependencies for) $<...
	@$(CC) $(CFLAGS) $(INCFLAGS) -MM $< |\
		sed "s,\($*\.$(OBJEXT)\),$(OBJDIR)/\1 $@,g" > $@

.PHONY: all remake clean cleaner run init debug

