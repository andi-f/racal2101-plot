# Makefile
VERSION = 3.02
CC      = /usr/bin/gcc
OBJ	 = racal2101-plot.o racal2101-cairo.o gpib-functions.c
CFLAGS  = -Wall -ggdb
LDFLAGS = -lgpib -lm `pkg-config --cflags --libs gtk+-2.0`
PRG 	= racal2101-plot
all: $(OBJ)
	$(CC) $(CFLAGS) -o $(PRG) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< $(LDFLAGS)

clean:
	rm -f $(PRG) *~ *.o a.out 
