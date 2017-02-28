IDIR =../include
CC=g++
CFLAGS=-std=c++0x -Wall -I$(IDIR)
LFLAGS=-Wall
ODIR=obj

_DEPS = utility.cpp
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = myserver.o myclient.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: myserver myclient

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

myserver: obj/myserver.o
	$(CC) -o $@ $^ $(CFLAGS)

myclient: obj/myclient.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 