# Makefile for libmisc.a
include ../rules.mak

CFLAGS += -I../../../include -Wall -MMD -MP

SRCS = libmiscendian.c libmisccococonv.c libmiscqueue.c libmiscutil.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

vpath %.c ../../../libmisc

libmisc.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -f *.o *.a *.d

-include $(DEPS)
