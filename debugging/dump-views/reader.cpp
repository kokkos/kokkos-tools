#include<fstream>
#include <iostream>
#include <string>
struct ptr_info {
  std::string who;
  void* what;
  size_t how_much;
  bool where;
  mutable void* canonical; // look, I'm in a hurry, I'm sorry
};
int main(int argc, char* argv[]){
  std::string filename = "checkpoint.kokkos";
  if(argc > 1){
    filename = argv[1];
  }
  std::ifstream input(filename);
  int num_allocations;
  std::cout << "00000000000000000\n";
  input >> num_allocations;
  std::cout << num_allocations << std::endl;
  std::cout << "11111111111111111\n";
  for(int x =0; x<num_allocations;++x){
    ptr_info info;
    input >> info.who >> info.how_much; 
    char* raw_data = (char*)malloc(info.how_much);
    int num_entries = info.how_much/sizeof(float);
    input.read(raw_data, info.how_much);
    std::cout<<info.who<<"\n,"<<info.how_much<<std::endl;
  std::cout << "22222222222222222\n";
    auto float_data = (float*)raw_data;
    for(int num = 0;num <std::min(num_entries,5);++num){
      std::cout << float_data[num] <<","<<std::endl; 
    }  
    std::cout << std::endl;
  std::cout << "33333333333333333\n";
  }
}
