default: all

all: liblabrador-explorer.so liblabrador-retriever.so
 
liblabrador-explorer.so: labrador-explorer.cpp ./sqlite3/sqlite3.o
	g++ -shared -pthread -g -fPIC -O2 -I../../common/include/ -I./sqlite3 -o liblabrador-explorer.so labrador-explorer.cpp ./sqlite3/sqlite3.o -ldl

liblabrador-retriever.so: labrador-retriever.cpp
	g++ -shared -pthread -g -fPIC -O2 -I../../common/include/ -I./sqlite3 -o liblabrador-retriever.so labrador-retriever.cpp
