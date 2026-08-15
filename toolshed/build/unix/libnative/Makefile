# Makefile for libnative.a
include ../rules.mak

CFLAGS += -I../../../include -Wall -MMD -MP

SRCS = libnativegs.c libnativedelete.c libnativeopen.c libnativeread.c libnativerename.c \
	libnativeseek.c libnativess.c libnativeread.c libnativewrite.c libnativereadln.c \
	libnativemakdir.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

vpath %.c ../../../libnative

libnative.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -f *.o *.a *.d

-include $(DEPS)
