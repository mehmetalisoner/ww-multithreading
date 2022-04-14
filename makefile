CC = gcc
CFLAGS = -g -Wall -fsanitize=address,undefined

ww: ww.c
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $^

