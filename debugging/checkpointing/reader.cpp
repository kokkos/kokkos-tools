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
std::string program_prefix = R"PREFIX(
#include <Kokkos_Core.hpp>
#include <fstream>
#include <iostream>
struct ptr_info {
  std::string who;
  void* what;
  size_t how_much;
  bool where;
  mutable void* canonical;
};

std::pair<void*,size_t> get_ptr_data(std::ifstream& input){
    ptr_info info;
    input >> info.who >> info.how_much; 
    char* raw_data = (char*)malloc(info.how_much);
    input.read(raw_data, info.how_much);
    info.what = raw_data;
    return std::make_pair(info.what, info.how_much);
}

int main(int argc, char* argv[]){
  Kokkos::initialize(argc, argv);
  {
    std::ifstream input("checkpoint.kokkos");
)PREFIX";
std::string program_suffix = R"SUFFIX(
  }
  Kokkos::finalize();
}
)SUFFIX";
std::string get_kokkos_view_string(std::string name){
  std::string scalar_type_name = name+"_scalar_type";
  std::string view_type_name  = name+"_view_type";
  std::string view_type = "Kokkos::View<"+scalar_type_name+", Kokkos::CudaSpace>";
  std::string mirror_view_type_name = view_type_name+"::HostMirror";
  std::string mirror_name = name + "_mirror";
  std::string data_name = name+"_data";
   
  std::string build = "    using "+scalar_type_name+ "= USER_FILL_THIS_IN;\n";  
  build += "    using "+view_type_name+ "= "+ view_type+"; /** you need to fill in the number of dimensions, sorry */\n";
  build += "    auto "+data_name+" = get_ptr_data(input);\n";
  build += "    "+mirror_view_type_name + " "+mirror_name +"( reinterpret_cast<"+scalar_type_name+"*>("+data_name +".first),"+data_name+".second / sizeof(" + scalar_type_name+")); /** you may need to adapt this for Views > 1D */\n";
  build += "    "+view_type_name+" "+name+"(\""+name+"\","+data_name+".second / sizeof(" + scalar_type_name+"));\n";
  build += "    Kokkos::deep_copy("+name+","+mirror_name+");\n";

  return build; 
}
int main(int argc, char* argv[]){
  std::string filename = "checkpoint.kokkos";
  if(argc > 1){
    filename = argv[1];
  }
  std::ifstream input(filename);
  int num_allocations;
  input >> num_allocations;
  std::string program = program_prefix;

  for(int x =0; x<num_allocations;++x){
    ptr_info info;
    input >> info.who >> info.how_much; 
    char* raw_data = (char*)malloc(info.how_much);
    int num_entries = info.how_much/sizeof(float);
    input.read(raw_data, info.how_much);
    std::string reader = get_kokkos_view_string(info.who);    
    program+=reader;
  }
  program += program_suffix;
  std::cout << program;
}
