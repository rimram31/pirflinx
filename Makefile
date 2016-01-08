CC=g++
CFLAGS=-w
LDFLAGS=-lpigpio -lpthread -lrt
EXEC=pirflinx

SOURCES = $(wildcard *.cpp) $(wildcard coding/*.cpp)
OBJECTS = $(SOURCES:%.cpp=%.o)

all: $(EXEC)

pirflinx: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(EXEC) $(OBJECTS)
