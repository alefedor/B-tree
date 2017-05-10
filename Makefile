main: ./bin/main.o ./bin/cacher.o ./bin/logger.o | btree.main btree.vals btree.log
	g++ ./bin/main.o ./bin/cacher.o ./bin/logger.o -o main

./bin/main.o: bin ./src/main.cpp ./include/b-tree.h
	g++ -c -o ./bin/main.o ./src/main.cpp -Iinclude -Wall -Wextra -std=c++11 -O3

./bin/cacher.o: bin ./src/cacher.cpp ./include/cacher.h
	g++ -c -o ./bin/cacher.o ./src/cacher.cpp -Iinclude -Wall -Wextra -std=c++11 -O3

./bin/logger.o: bin ./src/logger.cpp ./include/logger.h
	g++ -c -o ./bin/logger.o ./src/logger.cpp -Iinclude -Wall -Wextra -std=c++11 -O3


clean: 
	rm -rf ./bin
	rm -rf main
	rm -rf btree.main
	rm -rf btree.log
	rm -rf btree.vals
	
	
bin:
	mkdir bin -p

btree.main: 
	touch "btree.main"

btree.vals: 
	touch "btree.vals"

btree.log: 
	touch "btree.log"
