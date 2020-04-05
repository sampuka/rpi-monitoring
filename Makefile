all: slave master

clean:
	rm slave
	rm master

slave: slave.cpp
	g++ -std=c++17 -Wall -Wextra -Wno-psabi -pedantic -o slave slave.cpp -lpthread

master: master.cpp
	g++ -std=c++17 -Wall -Wextra -Wno-psabi -pedantic -o master master.cpp -lpthread

.PHONY: all clean
