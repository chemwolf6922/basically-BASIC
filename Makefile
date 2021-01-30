CC=gcc

CFLAGS=-o3 -std=c99
# CFLAGS+=-g

LDFLAGS=
LDFLAGS+=-s -static

all:app

%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $<

app:main.o basic.o
	$(CC) $(LDFLAGS) -o $@ $^

test:test.o basic.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f app test *.o