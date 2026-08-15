# Makefile for libcoco.a
include ../rules.mak

CFLAGS += -I../../../include -Wall -MMD -MP

SRCS = libcocogs.c libcocodelete.c libcocoopen.c libcocoread.c libcocorename.c \
	libcocoseek.c libcocoss.c libcocoread.c libcocowrite.c libcocoreadln.c \
	libcocomakdir.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

vpath %.c ../../../libcoco

libcoco.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -f *.o *.a *.d

-include $(DEPS)
