MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXX=nvcc
CXXFLAGS=-O0 -std=c++11 -g -I.
SHARED_CXXFLAGS=-shared -Xcompiler -fPIC
PROTOBUF_DIR=${MAKEFILE_PATH}/protobuf-install
all: kp_checkpoint.so reader

protobuf-build:
	mkdir ${MAKEFILE_PATH}/protobuf-build

protobuf-build/Makefile: protobuf-build
	cd protobuf-build && cmake -DCMAKE_INSTALL_PREFIX=${MAKEFILE_PATH}/protobuf-install -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF ${MAKEFILE_PATH}/protobuf/cmake/

protobuf-install/lib64/libprotobuf.a: protobuf-build/Makefile
	cd ${MAKEFILE_PATH}/protobuf-build && make -j8 install

kp_checkpoint.so: ${MAKEFILE_PATH}kp_kernel_logger.cpp protobuf-install/lib64/libprotobuf.a
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}kp_kernel_logger.cpp ${MAKEFILE_PATH}/protobuf/checkpointing.pb.cc -L$(PROTOBUF_DIR)/lib64 -I$(PRTOBUF_DIR)/include -lprotobuf

reader: ${MAKEFILE_PATH}reader.cpp protobuf-install/lib64/libprotobuf.a
	$(CXX) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}reader.cpp
clean:
	rm *.so
