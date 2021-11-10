CC=gcc
LDIR=/usr/local/lib

FLAGS=
# MAKEFLAGS += --silent
INC=-I. -I/usr/local/include/hiredis 
# -I../. 
LIBS=-L/usr/local/lib -lhiredis -lpthread
# -levent -lev -lpthread -luv
S_LIB=libhiredis.a

CFLAGS=-O3 -D_ALIGNED -D_GNU_SOURCE -fPIC -Wall -W -Wstrict-prototypes \
       -Wimplicit-function-declaration \
        -Wwrite-strings -Wno-missing-field-initializers \
        -g -ggdb $(INC) $(FLAGS)
SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%, $(SOURCES))
all: $(OBJECTS)
$(OBJECTS): %: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS) 
#$(S_LIB)
clean:
	rm $(OBJECTS)
