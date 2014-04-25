CC=gcc
CFLAGS=-Wall -O2 
LDFALGS= -L/usr/local/lib -lconfig
LIBS= -lconfig
TARGET=carserver

SOURCES=src/carserver.o src/car_handler.o

OBJECTS=$(SOURCES:.c=.o)


all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFALGS) $(OBJECTS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(SOURCES)
	rm -rf $(TARGET)
