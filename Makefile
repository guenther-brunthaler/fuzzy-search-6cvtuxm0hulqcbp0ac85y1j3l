# The following line declares this Makefile as strictly POSIX-compliant.
.POSIX:

# Default build options. Override these by either passing the C compiler
# options via "CFLAGS=" directly as an argument on the "make" command line, or
# by "export MAKEFLAGS=e" first and then setting "export CFLAGS=". The same
# applies to CPPFLAGS (passed to the C preprocessor) and LDFLAGS (passed to
# the linker).
CPPFLAGS = -D NDEBUG
CFLAGS = -O
LDFLAGS = -s

TARGETS = similars $(similars_OBJECTS)

similars_OBJECTS = similars.o

# Augmented flags which are actually used for compiling.
AUG_CFLAGS= $(CPPFLAGS) $(CFLAGS)
AUG_LDFLAGS= $(LDFLAGS)

.PHONY: all clean

all: $(TARGETS)

clean:
	-rm $(TARGETS)

.c.o:
	$(CC) -o $@ $(AUG_CFLAGS) -c $<

similars: $(similars_OBJECTS)
	$(CC) -o $@ $(AUG_LDFLAGS) $(similars_OBJECTS)
