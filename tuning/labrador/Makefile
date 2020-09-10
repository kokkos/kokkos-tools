default: all

explorer: liblabrador-explorer.so util
retriever: liblabrador-retriever.so util
util: liblabrador-utils.a


all: explorer retriever
 
liblabrador-explorer.so: labrador-explorer.cpp util
	g++ -shared -std=c++11 -pthread -g -fPIC -O2 -I../../common/include/ -I./sqlite3 -o liblabrador-explorer.so labrador-explorer.cpp ./liblabrador-utils.a -ldl

liblabrador-utils.a: labrador-exploration.o ./sqlite3/sqlite3.o
	ar crf liblabrador-utils.a labrador-exploration.o ./sqlite3/sqlite3.o

labrador-exploration.o: labrador-exploration.cpp labrador-exploration.hpp ./sqlite3/sqlite3.o
	g++ -std=c++11 -pthread -g -fPIC -O2 -I../../common/include/ -I./sqlite3 -c labrador-exploration.cpp ./sqlite3/sqlite3.o -ldl -o labrador-exploration.o


liblabrador-retriever.so: labrador-retriever.cpp util
	g++ -std=c++11 -shared -std=c++11 -pthread -g -fPIC -O2 -I../../common/include/ -I./sqlite3 labrador-retriever.cpp ./liblabrador-utils.a -o liblabrador-retriever.so

./sqlite3/sqlite3.o: ./sqlite3/Makefile
	cd sqlite3 && make

./sqlite3/Makefile:
	cd sqlite3 && autoreconf -f -i && CFLAGS="-g -O2 -fPIC" ./configure

