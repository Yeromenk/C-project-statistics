TARGET=lol
SOURCES=$(wildcard *.c)

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) -g -Wall -o $(TARGET) $(SOURCES)

clean:
	rm $(TARGET)
