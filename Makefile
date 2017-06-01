
CC = gcc
CFLAGS = -fPIC -Wall -O2 -g
LDFLAGS = -shared
RM = rm -f
TARGET_LIB = libcgate.so

SRCS = libcgate.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.c=.d)

.PHONY: clean
clean:
	$(RM) $(OBJS) $(TARGET_LIB) *.d

