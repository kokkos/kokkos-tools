default: all

explorer: liblabrador-explorer.so
retriever: liblabrador-retriever.so	

all: liblabrador-explorer.so liblabrador-retriever.so
 
liblabrador-explorer.so: labrador-explorer.cpp ./sqlite3/sqlite3.o
	g++ -shared -std=c++11 -pthread -g -fPIC -O2 -I../../common/include/ -I./sqlite3 -o liblabrador-explorer.so labrador-explorer.cpp ./sqlite3/sqlite3.o -ldl

liblabrador-retriever.so: labrador-retriever.cpp
	g++ -shared -std=c++11 -pthread -g -fPIC -O2 -I../../common/include/ -I./sqlite3 -o liblabrador-retriever.so labrador-retriever.cpp

./sqlite3/sqlite3.o: ./sqlite3/Makefile
	cd sqlite3 && make

./sqlite3/Makefile:
	cd sqlite3 && autoreconf -f -i && CFLAGS="-g -O2 -fPIC" ./configure

