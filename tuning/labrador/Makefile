default: all

all: liblabrador-explorer.so libexperiment.so
 
liblabrador-explorer.so: labrador-explorer.cpp ./sqlite3/sqlite3.o
	g++ -shared -pthread -g -fPIC -O2 -I../../common/include/ -I./sqlite3 -o liblabrador-explorer.so labrador-explorer.cpp ./sqlite3/sqlite3.o -ldl

libexperiment.so: experiment.cpp
	g++ -shared -pthread -g -fPIC -O2 -I../../common/include/ -I./sqlite3 -o libexperiment.so experiment.cpp
