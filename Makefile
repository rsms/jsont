c_sources :=	jsont.c

all: example1 example2

object_dir = .objs
objects = $(patsubst %,$(object_dir)/%,${c_sources:.c=.o})
object_dirs = $(sort $(foreach fn,$(objects),$(dir $(fn))))
-include ${objects:.o=.d}

CC = clang
LD = clang

CFLAGS 	+= -Wall -g -MMD -std=c99 -I.
#LDFLAGS +=
ifneq ($(DEBUG),)
	CFLAGS += -O0 -DDEBUG=1
else
	CFLAGS += -O3 -DNDEBUG
endif

clean:
	@rm -f jsont
	@rm -rf $(object_dir)

example1: $(objects) example1.o
	$(LD) $(LDFLAGS) -o $@ $^

example2: $(objects) example2.o
	$(LD) $(LDFLAGS) -o $@ $^

#test: test_jsont
#	test_jsont

#test_jsont: $(objects)
#test_jsont: $(object_dir)/test_jsont.o
#	$(LD) $(LDFLAGS) -o $@ $^

$(object_dir)/%.o: %.c
	@mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean all test
