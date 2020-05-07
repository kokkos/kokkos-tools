default: all

all: libapollo-tuner.so tuning_example

libapollo-tuner.so: apollo-tuner.cpp
	OMPI_CXX=g++ mpic++ -L/usr/lib64/openmpi/lib/ -lmpi -lmpi_cxx apollo-tuner.cpp -shared -fPIC -Wl,-rpath /ascldap/users/dzpolia/src/kokkos-tools/src/tools/apollo-tool/apollo/lib -Wl,-rpath /ascldap/users/dzpolia/src/kokkos-tools/src/tools/apollo-tool/apollo/lib64 -std=c++11 -I./apollo/include -I../../../include -fopenmp -I/usr/include/openmpi-x86_64 -L./apollo/lib -L./apollo/lib64 -o libapollo-tuner.so -lapollo -lmpi 

tuning_example: tuning_example.cpp
	OMPI_CXX=g++ mpic++ -std=c++11 tuning_example.cpp -L/ascldap/users/dzpolia/apollo/kokkos/lib64 -fopenmp -I/ascldap/users/dzpolia/apollo/kokkos/include -o tuning_example -lkokkoscore -lkokkoscontainers -ldl
