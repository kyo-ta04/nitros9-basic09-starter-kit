# Makefile for libsys.a
include ../rules.mak

CFLAGS += -I../../../include -Wall -MMD -MP

SRCS = crc.c prsnam.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

vpath %.c ../../../libsys

libsys.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -f *.o *.a *.d

-include $(DEPS)
