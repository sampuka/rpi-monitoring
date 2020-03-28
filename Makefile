all: slave master

clean:
	rm slave
	rm master

slave: slave.cpp
	g++ -std=c++17 -Wall -Wextra -pedantic -o slave slave.cpp

master: master.cpp
	g++ -std=c++17 -Wall -Wextra -pedantic -o master master.cpp

.PHONY: all clean
