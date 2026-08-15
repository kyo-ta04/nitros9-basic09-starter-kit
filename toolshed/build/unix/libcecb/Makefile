# Makefile for libcecb.a
include ../rules.mak

CFLAGS += -I../../../include -Wall -MMD -MP

SRCS = libcecbbulkerase.c libcecbopen.c libcecbgs.c libcecbwav.c \
	libcecbcas.c libcecbread.c libcecbwrite.c libcecbtokenize.c \
	libcecbseek.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

vpath %.c ../../../libcecb

libcecb.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -f *.o *.a *.d

-include $(DEPS)
