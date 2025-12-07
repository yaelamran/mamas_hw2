
cacheSim: cacheSim.cpp
	g++ -o cacheSim cacheSim.cpp

.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

all: cacheSim
	g++ -o $@ $^
%.o: %.cpp
	g++ -c $< -o $@