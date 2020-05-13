default: all

all: libapollo-tuner.so 

libapollo-tuner.so: apollo-tuner.cpp
	g++ -L/usr/lib64/openmpi/lib/ apollo-tuner.cpp -g -shared -fPIC -Wl,-rpath /ascldap/users/dzpolia/src/kokkos-tools/src/tools/apollo-tool/apollo/lib -Wl,-rpath /ascldap/users/dzpolia/src/kokkos-tools/src/tools/apollo-tool/apollo/lib -std=c++11 -I./apollo/include -I../../../include -fopenmp -I/usr/include/openmpi-x86_64 -L./apollo/lib -L./apollo/lib64 -o libapollo-tuner.so -lapollo 

tuning_example: tuning_example.cpp
	g++ -std=c++11 tuning_example.cpp -L/ascldap/users/dzpolia/apollo/kokkos/lib64 -fopenmp -I/ascldap/users/dzpolia/apollo/kokkos/include -o tuning_example -lkokkoscore -lkokkoscontainers -ldl
