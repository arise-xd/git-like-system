CC = gcc
CFLAGS = -Wall -Wextra

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
    OPENSSL_PREFIX := $(shell brew --prefix openssl)
    CFLAGS += -I$(OPENSSL_PREFIX)/include
    LDFLAGS += -L$(OPENSSL_PREFIX)/lib -lcrypto
else
    LDFLAGS += -lcrypto
endif

TARGET = mygit

OBJS = main.o repository.o commit.o log.o utils.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean